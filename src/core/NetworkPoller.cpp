#include "NetworkPoller.h"

// ── FIXED: Native Windows headers using the native C++ macro ──
#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <windows.h>
#  include <objbase.h>    // <--- THIS IS THE MISSING MAGIC LINE
#  include <iphlpapi.h>
#  include <netioapi.h>
#endif

// ── Qt and Standard Library Includes ──
#include <QMutexLocker>
#include <QElapsedTimer>
#include <QThread>
#include <QStringList>
#include <QDebug>
#include <algorithm>
#include <unordered_map>

namespace nsm {

static const QStringList kVirtualKeywords {
    QStringLiteral("loopback"), QStringLiteral("virtual"), QStringLiteral("hyper-v"),
    QStringLiteral("vmware"), QStringLiteral("virtualbox"), QStringLiteral("tap"),
    QStringLiteral("teredo"), QStringLiteral("isatap"), QStringLiteral("6to4"),
    QStringLiteral("pseudo"), QStringLiteral("miniport"), QStringLiteral("wan miniport"),
    QStringLiteral("ppoe"), QStringLiteral("bluetooth"),
};

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

QVector<AdapterInfo> NetworkPoller::enumerateAdapters()
{
    QVector<AdapterInfo> result;
#ifdef _WIN32
    MIB_IF_TABLE2* pTable = nullptr;
    if (GetIfTable2(&pTable) != NO_ERROR || !pTable)
        return result;
    for (ULONG i = 0; i < pTable->NumEntries; ++i) {
        const MIB_IF_ROW2& row = pTable->Table[i];
        if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK)
            continue;
        if (row.MediaConnectState == MediaConnectStateDisconnected)
            continue;
        AdapterInfo info;
        info.ifIndex = static_cast<quint64>(row.InterfaceIndex);
        info.name = QString::fromWCharArray(row.Description);
        WCHAR guidStr[64] = {};
        StringFromGUID2(row.InterfaceGuid, guidStr, 64);
        info.guid = QString::fromWCharArray(guidStr);
        result.append(info);
    }
    FreeMibTable(pTable);
#endif
    return result;
}

bool NetworkPoller::isPhysicalAdapter(const QString& description)
{
    const QString lower = description.toLower();
    for (const QString& kw : kVirtualKeywords) {
        if (lower.contains(kw))
            return false;
    }
    return true;
}

bool NetworkPoller::readAdapterCounters(quint64 ifIndex,
                                        quint64& bytesIn,
                                        quint64& bytesOut) const
{
#ifdef _WIN32
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

quint64 NetworkPoller::selectPrimaryAdapter(
        const QVector<AdapterInfo>& candidates,
        const std::unordered_map<quint64, quint64>& prevBytesIn,
        const std::unordered_map<quint64, quint64>& prevBytesOut) const
{
    quint64 bestIdx = 0, bestBytes = 0;
    for (const AdapterInfo& info : candidates) {
        if (!isPhysicalAdapter(info.name))
            continue;
        quint64 bi = 0, bo = 0;
        if (!readAdapterCounters(info.ifIndex, bi, bo))
            continue;
        quint64 dIn = 0, dOut = 0;
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

void NetworkPoller::run()
{
    std::unordered_map<quint64, quint64> prevBytesIn;
    std::unordered_map<quint64, quint64> prevBytesOut;
    quint64 prevAutoIdx = 0;
    QElapsedTimer elapsed;
    elapsed.start();
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
    while (true) {
        int intervalMs;
        AdapterMode mode;
        QStringList selectedGuids;
        bool stopRequested;
        {
            QMutexLocker lock(&m_mutex);
            intervalMs    = m_intervalMs;
            mode          = m_mode;
            selectedGuids = m_selectedGuids;
            stopRequested = m_stopRequested;
        }
        if (stopRequested)
            break;
        const qint64 startMs = elapsed.elapsed();
        while ((elapsed.elapsed() - startMs) < intervalMs) {
            {
                QMutexLocker lock(&m_mutex);
                if (m_stopRequested)
                    goto done;
            }
            QThread::msleep(50);
        }
        const double dtMs = static_cast<double>(elapsed.elapsed() - startMs);
        const double dtSec = (dtMs > 0.0) ? (dtMs / 1000.0) : 1.0;
        const QVector<AdapterInfo> adapters = enumerateAdapters();
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
        else {
            const quint64 primary = selectPrimaryAdapter(adapters, prevBytesIn, prevBytesOut);
            if (primary != 0) {
                targetIndices.append(primary);
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
        quint64 newBytesIn = 0, newBytesOut = 0;
        bool readOk = readAdapterCounters(targetIndices, newBytesIn, newBytesOut);
        if (!readOk) {
            emit pollingError(QStringLiteral("Failed to read network adapter counters."));
        }
        else {
            quint64 sumPrevIn = 0, sumPrevOut = 0;
            for (quint64 idx : targetIndices) {
                auto itIn  = prevBytesIn.find(idx);
                auto itOut = prevBytesOut.find(idx);
                if (itIn  != prevBytesIn.end())  sumPrevIn  += itIn->second;
                if (itOut != prevBytesOut.end()) sumPrevOut += itOut->second;
            }
            const quint64 deltaIn  = (newBytesIn  >= sumPrevIn)  ? (newBytesIn  - sumPrevIn)  : 0;
            const quint64 deltaOut = (newBytesOut >= sumPrevOut) ? (newBytesOut - sumPrevOut) : 0;
            SpeedSample sample;
            sample.downloadBps = static_cast<double>(deltaIn)  / dtSec;
            sample.uploadBps   = static_cast<double>(deltaOut) / dtSec;
            emit speedUpdated(sample);
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
    ;
}

} // namespace nsm