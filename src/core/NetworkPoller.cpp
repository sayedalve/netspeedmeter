#include "NetworkPoller.h"

#include <QMutexLocker>
#include <QElapsedTimer>
#include <QThread>

// ── Windows headers ───────────────────────────────────────────────────────────
#ifdef Q_OS_WIN
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  include <winsock2.h>
#  include <iphlpapi.h>
#  include <ws2tcpip.h>
// Linked via CMakeLists: iphlpapi, ws2_32
#endif

#include <algorithm>
#include <unordered_map>

namespace nsm {

// ─────────────────────────────────────────────────────────────────────────────
//  Virtual adapter keyword blocklist
//  (Adapted from the Python reference's adapter filtering logic)
// ─────────────────────────────────────────────────────────────────────────────
static const QStringList kVirtualKeywords {
    QStringLiteral("loopback"),
    QStringLiteral("virtual"),
    QStringLiteral("hyper-v"),
    QStringLiteral("vmware"),
    QStringLiteral("virtualbox"),
    QStringLiteral("tap"),
    QStringLiteral("teredo"),
    QStringLiteral("isatap"),
    QStringLiteral("6to4"),
    QStringLiteral("pseudo"),
    QStringLiteral("miniport"),
    QStringLiteral("wan miniport"),
    QStringLiteral("ppoe"),
    QStringLiteral("bluetooth"),
};

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

NetworkPoller::NetworkPoller(QObject* parent)
    : QThread(parent)
{
    qRegisterMetaType<nsm::SpeedSample>("nsm::SpeedSample");
    qRegisterMetaType<nsm::AdapterInfo>("nsm::AdapterInfo");
}

NetworkPoller::~NetworkPoller()
{
    stop();
    wait();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Configuration setters (thread-safe)
// ─────────────────────────────────────────────────────────────────────────────

void NetworkPoller::setIntervalMs(int ms)
{
    QMutexLocker lock(&m_mutex);
    m_intervalMs = qBound(100, ms, 10000);
}

void NetworkPoller::setAdapterMode(AdapterMode mode)
{
    QMutexLocker lock(&m_mutex);
    m_mode = mode;
}

void NetworkPoller::setSelectedAdapters(const QStringList& guids)
{
    QMutexLocker lock(&m_mutex);
    m_selectedGuids = guids;
}

void NetworkPoller::stop()
{
    QMutexLocker lock(&m_mutex);
    m_stopRequested = true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Static adapter enumeration
// ─────────────────────────────────────────────────────────────────────────────

QVector<AdapterInfo> NetworkPoller::enumerateAdapters()
{
    QVector<AdapterInfo> result;

#ifdef Q_OS_WIN
    MIB_IF_TABLE2* pTable = nullptr;
    if (GetIfTable2(&pTable) != NO_ERROR || !pTable)
        return result;

    for (ULONG i = 0; i < pTable->NumEntries; ++i) {
        const MIB_IF_ROW2& row = pTable->Table[i];

        // Skip loopback
        if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK)
            continue;

        // Skip disconnected / not present
        if (row.MediaConnectState == MediaConnectStateDisconnected)
            continue;

        AdapterInfo info;
        info.ifIndex = static_cast<quint64>(row.InterfaceIndex);

        // Description (human-readable, wide string)
        info.name = QString::fromWCharArray(row.Description);

        // GUID  → "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}"
        WCHAR guidStr[64] = {};
        StringFromGUID2(row.InterfaceGuid, guidStr, 64);
        info.guid = QString::fromWCharArray(guidStr);

        result.append(info);
    }

    FreeMibTable(pTable);
#endif

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Physical adapter heuristic
// ─────────────────────────────────────────────────────────────────────────────

bool NetworkPoller::isPhysicalAdapter(const QString& description)
{
    const QString lower = description.toLower();
    for (const QString& kw : kVirtualKeywords) {
        if (lower.contains(kw))
            return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Low-level counter reads via GetIfEntry2
// ─────────────────────────────────────────────────────────────────────────────

bool NetworkPoller::readAdapterCounters(quint64 ifIndex,
                                        quint64& bytesIn,
                                        quint64& bytesOut) const
{
#ifdef Q_OS_WIN
    MIB_IF_ROW2 row = {};
    row.InterfaceIndex = static_cast<NET_IFINDEX>(ifIndex);

    if (GetIfEntry2(&row) != NO_ERROR)
        return false;

    bytesIn  = row.InOctets;
    bytesOut = row.OutOctets;
    return true;
#else
    Q_UNUSED(ifIndex)
    bytesIn = bytesOut = 0;
    return false;
#endif
}

bool NetworkPoller::readAdapterCounters(const QVector<quint64>& indices,
                                        quint64& bytesIn,
                                        quint64& bytesOut) const
{
    bytesIn = bytesOut = 0;
    bool ok = false;

    for (quint64 idx : indices) {
        quint64 bi = 0, bo = 0;
        if (readAdapterCounters(idx, bi, bo)) {
            bytesIn  += bi;
            bytesOut += bo;
            ok = true;
        }
    }
    return ok;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Auto-primary adapter selection
// ─────────────────────────────────────────────────────────────────────────────

quint64 NetworkPoller::selectPrimaryAdapter(
        const QVector<AdapterInfo>& candidates,
        const std::unordered_map<quint64, quint64>& prevBytesIn,
        const std::unordered_map<quint64, quint64>& prevBytesOut) const
{
    quint64 bestIdx   = 0;
    quint64 bestBytes = 0;

    for (const AdapterInfo& info : candidates) {
        if (!isPhysicalAdapter(info.name))
            continue;

        quint64 bi = 0, bo = 0;
        if (!readAdapterCounters(info.ifIndex, bi, bo))
            continue;

        // Delta since last poll
        quint64 dIn  = 0, dOut = 0;
        auto itIn  = prevBytesIn.find(info.ifIndex);
        auto itOut = prevBytesOut.find(info.ifIndex);

        if (itIn  != prevBytesIn.end())  dIn  = bi - itIn->second;
        if (itOut != prevBytesOut.end()) dOut = bo - itOut->second;

        const quint64 total = dIn + dOut;
        if (total > bestBytes) {
            bestBytes = total;
            bestIdx   = info.ifIndex;
        }
    }

    // Fallback: if nothing had traffic, pick the first physical adapter
    if (bestIdx == 0) {
        for (const AdapterInfo& info : candidates) {
            if (isPhysicalAdapter(info.name)) {
                bestIdx = info.ifIndex;
                break;
            }
        }
    }

    return bestIdx;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Main polling loop
// ─────────────────────────────────────────────────────────────────────────────

void NetworkPoller::run()
{
    // Previous sample counters (for computing deltas)
    std::unordered_map<quint64, quint64> prevBytesIn;
    std::unordered_map<quint64, quint64> prevBytesOut;

    quint64 prevAutoIdx = 0;   // last chosen auto-primary adapter index

    QElapsedTimer elapsed;
    elapsed.start();

    // ── Prime the counters so the first emitted sample is meaningful ──────────
    {
        const QVector<AdapterInfo> adapters = enumerateAdapters();
        for (const AdapterInfo& a : adapters) {
            quint64 bi = 0, bo = 0;
            if (readAdapterCounters(a.ifIndex, bi, bo)) {
                prevBytesIn[a.ifIndex]  = bi;
                prevBytesOut[a.ifIndex] = bo;
            }
        }
    }

    // ── Polling loop ──────────────────────────────────────────────────────────
    while (true) {

        // Read config snapshot (lock only for this read, not the whole sleep)
        int         intervalMs;
        AdapterMode mode;
        QStringList selectedGuids;
        bool        stopRequested;

        {
            QMutexLocker lock(&m_mutex);
            intervalMs    = m_intervalMs;
            mode          = m_mode;
            selectedGuids = m_selectedGuids;
            stopRequested = m_stopRequested;
        }

        if (stopRequested)
            break;

        // Sleep in small increments so stop() is responsive
        const qint64 startMs = elapsed.elapsed();
        while ((elapsed.elapsed() - startMs) < intervalMs) {
            {
                QMutexLocker lock(&m_mutex);
                if (m_stopRequested)
                    goto done;   // immediate exit from nested loop
            }
            QThread::msleep(50);
        }

        // ── How long did we actually sleep? ───────────────────────────────────
        const double dtMs = static_cast<double>(elapsed.elapsed() - startMs);
        const double dtSec = (dtMs > 0.0) ? (dtMs / 1000.0) : 1.0;

        // ── Enumerate current adapters ─────────────────────────────────────────
        const QVector<AdapterInfo> adapters = enumerateAdapters();

        // ── Collect targets according to the configured mode ──────────────────
        QVector<quint64> targetIndices;

        if (mode == AdapterMode::AllAdapters) {
            for (const AdapterInfo& a : adapters) {
                if (isPhysicalAdapter(a.name))
                    targetIndices.append(a.ifIndex);
            }
        }
        else if (mode == AdapterMode::Specific) {
            for (const AdapterInfo& a : adapters) {
                if (selectedGuids.contains(a.guid))
                    targetIndices.append(a.ifIndex);
            }
        }
        else { // AutoPrimary
            const quint64 primary = selectPrimaryAdapter(adapters, prevBytesIn, prevBytesOut);
            if (primary != 0) {
                targetIndices.append(primary);

                // Notify GUI if the selected adapter changed
                if (primary != prevAutoIdx) {
                    prevAutoIdx = primary;
                    for (const AdapterInfo& a : adapters) {
                        if (a.ifIndex == primary) {
                            emit activeAdapterChanged(a);
                            break;
                        }
                    }
                }
            }
        }

        // ── Read new counters ─────────────────────────────────────────────────
        quint64 newBytesIn = 0, newBytesOut = 0;
        bool readOk = readAdapterCounters(targetIndices, newBytesIn, newBytesOut);

        if (!readOk) {
            emit pollingError(QStringLiteral("Failed to read network adapter counters."));
            // Don't abort — adapters may reappear after a brief disconnect
        }
        else {
            // ── Compute combined previous totals ──────────────────────────────
            quint64 sumPrevIn = 0, sumPrevOut = 0;
            for (quint64 idx : targetIndices) {
                auto itIn  = prevBytesIn.find(idx);
                auto itOut = prevBytesOut.find(idx);
                if (itIn  != prevBytesIn.end())  sumPrevIn  += itIn->second;
                if (itOut != prevBytesOut.end())  sumPrevOut += itOut->second;
            }

            // Guard against counter wrap-around (64-bit counters rarely wrap,
            // but defend anyway)
            const quint64 deltaIn  = (newBytesIn  >= sumPrevIn)  ? (newBytesIn  - sumPrevIn)  : 0;
            const quint64 deltaOut = (newBytesOut >= sumPrevOut)  ? (newBytesOut - sumPrevOut) : 0;

            SpeedSample sample;
            sample.downloadBps = static_cast<double>(deltaIn)  / dtSec;
            sample.uploadBps   = static_cast<double>(deltaOut) / dtSec;

            emit speedUpdated(sample);

            // ── Update previous counters for all adapters ─────────────────────
            for (const AdapterInfo& a : adapters) {
                quint64 bi = 0, bo = 0;
                if (readAdapterCounters(a.ifIndex, bi, bo)) {
                    prevBytesIn[a.ifIndex]  = bi;
                    prevBytesOut[a.ifIndex] = bo;
                }
            }
        }
    }

done:
    ;  // label for goto from inner loop — clean exit
}

} // namespace nsm
