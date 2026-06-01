#pragma once

#include "SpeedSample.h"

#include <QThread>
#include <QMutex>
#include <QStringList>
#include <QString>

#include <cstdint>
#include <vector>
#include <unordered_map>

namespace nsm {

// ─────────────────────────────────────────────────────────────────────────────
//  AdapterInfo — lightweight descriptor for a physical/virtual NIC
// ─────────────────────────────────────────────────────────────────────────────
struct AdapterInfo
{
    quint64 ifIndex   { 0 };    ///< Windows interface index (unique per boot)
    QString name;               ///< Human-readable adapter description
    QString guid;               ///< GUID string from Windows

    bool operator==(const AdapterInfo& o) const noexcept { return ifIndex == o.ifIndex; }
};

// ─────────────────────────────────────────────────────────────────────────────
//  NetworkPoller
// ─────────────────────────────────────────────────────────────────────────────
/**
 * @brief Background QThread that polls Windows' GetIfEntry2() at a
 *        configurable interval and emits SpeedSample measurements.
 *
 * Architecture
 * ────────────
 *  • Lives in its own QThread (moveToThread pattern).
 *  • Emits speedUpdated() on the thread's event loop – Qt::QueuedConnection
 *    ensures safe delivery to the GUI thread.
 *  • Adapter selection:  "auto" (highest-traffic physical adapter) OR an
 *    explicit list of adapter GUIDs supplied by the settings dialog.
 *  • The delta between consecutive raw byte counters is divided by the
 *    elapsed milliseconds to yield bytes/second, which avoids drift caused
 *    by timer jitter.
 *
 * Lifetime
 * ────────
 *  Create → configure → start().  To stop:  call stop() then wait().
 *
 * Thread safety
 * ─────────────
 *  All public setters are mutex-guarded and safe to call from the GUI thread
 *  while the poller is running.
 */
class NetworkPoller : public QThread
{
    Q_OBJECT

public:
    /// Adapter selection mode
    enum class AdapterMode {
        AutoPrimary,    ///< Automatically pick the most-active physical adapter
        AllAdapters,    ///< Sum all non-virtual adapters
        Specific        ///< Only the GUIDs listed in setSelectedAdapters()
    };
    Q_ENUM(AdapterMode)

    explicit NetworkPoller(QObject* parent = nullptr);
    ~NetworkPoller() override;

    // ── Configuration (thread-safe) ──────────────────────────────────────────

    /**
     * @brief Set the polling interval in milliseconds.
     * @param ms  Must be in [100, 10000].  Default: 1000 ms.
     */
    void setIntervalMs(int ms);

    /**
     * @brief Choose how adapters are selected.
     */
    void setAdapterMode(AdapterMode mode);

    /**
     * @brief Supply explicit adapter GUIDs when mode == Specific.
     *        Safe to call while running; takes effect on the next tick.
     */
    void setSelectedAdapters(const QStringList& guids);

    // ── Enumeration (can be called from any thread, including GUI) ───────────

    /**
     * @brief Return a snapshot of all currently visible network adapters.
     *        This is a blocking, one-shot query — not the poll loop.
     */
    static QVector<AdapterInfo> enumerateAdapters();

    // ── Control ──────────────────────────────────────────────────────────────

    /** @brief Signal the run-loop to exit cleanly. */
    void stop();

signals:
    /**
     * @brief Emitted once per polling interval with the latest speeds.
     * @param sample  Bytes/second for upload and download.
     */
    void speedUpdated(nsm::SpeedSample sample);

    /**
     * @brief Emitted when the active adapter changes (auto mode).
     * @param info  The newly-selected adapter.
     */
    void activeAdapterChanged(nsm::AdapterInfo info);

    /**
     * @brief Emitted if a fatal polling error occurs (e.g. API failure).
     */
    void pollingError(const QString& message);

protected:
    /** @brief QThread entry point — contains the polling loop. */
    void run() override;

private:
    // ── Helpers ──────────────────────────────────────────────────────────────

    /**
     * @brief Read raw bytes-in and bytes-out for a single adapter index.
     * @param[in]  ifIndex   The Windows adapter index.
     * @param[out] bytesIn   Total cumulative bytes received.
     * @param[out] bytesOut  Total cumulative bytes sent.
     * @return true on success.
     */
    bool readAdapterCounters(quint64 ifIndex,
                             quint64& bytesIn,
                             quint64& bytesOut) const;

    /**
     * @brief Read summed counters for a list of adapter indices.
     */
    bool readAdapterCounters(const QVector<quint64>& indices,
                             quint64& bytesIn,
                             quint64& bytesOut) const;

    /**
     * @brief Choose the "primary" adapter automatically.
     *
     * Heuristic: physical Ethernet/Wi-Fi adapter with the highest combined
     * byte count in the current sample, excluding loopback and known-virtual
     * adapter descriptions (Hyper-V, TAP, etc.).
     */
    quint64 selectPrimaryAdapter(const QVector<AdapterInfo>& candidates,
                                 const std::unordered_map<quint64, quint64>& prevBytesIn,
                                 const std::unordered_map<quint64, quint64>& prevBytesOut) const;

    /**
     * @brief Return true if the adapter description looks like a physical NIC.
     */
    static bool isPhysicalAdapter(const QString& description);

    // ── State (all accessed under m_mutex) ───────────────────────────────────
    mutable QMutex m_mutex;

    int         m_intervalMs   { 1000 };
    AdapterMode m_mode         { AdapterMode::AutoPrimary };
    QStringList m_selectedGuids;
    bool        m_stopRequested{ false };
};

} // namespace nsm

// Make AdapterInfo usable in QVariant / signals
Q_DECLARE_METATYPE(nsm::AdapterInfo)
Q_DECLARE_METATYPE(nsm::SpeedSample)
