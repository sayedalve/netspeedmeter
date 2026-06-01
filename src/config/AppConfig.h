#pragma once

#include <QString>
#include <QStringList>
#include <QPoint>
#include <QSize>

#include "core/NetworkPoller.h"

namespace nsm {

/**
 * @brief Pure data structure that holds every user-configurable value.
 */
struct AppConfig
{
    // ── General ───────────────────────────────────────────────────────────────
    int updateIntervalMs { 1000 };
    bool startWithWindows { false };
    bool minimiseToTray { true };

    // ── Network ───────────────────────────────────────────────────────────────
    NetworkPoller::AdapterMode adapterMode { NetworkPoller::AdapterMode::AutoPrimary };
    QStringList selectedAdapterGuids;

    // ── Appearance ────────────────────────────────────────────────────────────
    double opacity { 0.92 };
    double fontScale { 1.0 };
    QString fontFamily { QStringLiteral("Segoe UI") };
    int fontSize { 11 };
    bool showGraph { false };   // permanently disabled
    int graphHistorySize { 60 };

    // ── Speed Display ─────────────────────────────────────────────────────────
    /**
     * @brief Speed unit display mode.
     * "auto"   – auto-select KB/MB/GB (never B/s)
     * "mbps"   – always MB/s
     * "kbps"   – always KB/s
     * "bits"   – bits/sec: Kbps, Mbps, Gbps
     */
    QString speedUnit { QStringLiteral("auto") };
    int decimalPlaces { 1 };

    // ── Window / Positioning ──────────────────────────────────────────────────
    QPoint  widgetPos  { 0, 0 };   // 0,0 = auto-detect taskbar left
    QSize   widgetSize { 130, 40 };
    bool positionLocked { false };
};

} // namespace nsm
