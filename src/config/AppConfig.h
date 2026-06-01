#pragma once

#include <QString>
#include <QStringList>
#include <QPoint>
#include <QSize>

#include "core/NetworkPoller.h"   // for AdapterMode enum

namespace nsm {

/**
 * @brief Pure data structure that holds every user-configurable value.
 *
 * Design rules:
 *  - No Qt object, no signals.  AppConfig is a plain value type.
 *  - All members have sensible defaults assigned inline.
 *  - ConfigManager is the only class that serialises / deserialises this.
 *  - Other components hold a copy retrieved via ConfigManager::config().
 *
 * Section groupings mirror the Settings dialog tabs:
 *   [general]    – behaviour, update rate, startup
 *   [network]    – adapter selection
 *   [appearance] – widget aesthetics, font scaling, opacity
 *   [window]     – position, lock state
 *   [graph]      – history buffer size
 */
struct AppConfig
{
    // ── General ───────────────────────────────────────────────────────────────

    /** Poll interval in milliseconds.  Range: [100, 10000]. */
    int updateIntervalMs { 1000 };

    /** Launch the application on Windows startup. */
    bool startWithWindows { false };

    /** Minimise to tray on close instead of quitting. */
    bool minimiseToTray { true };

    // ── Network ───────────────────────────────────────────────────────────────

    /** Adapter selection strategy. */
    NetworkPoller::AdapterMode adapterMode { NetworkPoller::AdapterMode::AutoPrimary };

    /**
     * @brief GUIDs of explicitly selected adapters.
     * Only used when adapterMode == Specific.
     */
    QStringList selectedAdapterGuids;

    // ── Appearance ────────────────────────────────────────────────────────────

    /**
     * @brief Widget opacity [0.0, 1.0].
     * 0.0 = fully transparent, 1.0 = fully opaque.
     */
    double opacity { 0.88 };

    /**
     * @brief Global font scale factor.
     * 1.0 = default, 1.5 = 50 % larger, etc.
     */
    double fontScale { 1.0 };

    /**
     * @brief Show the mini area-chart graph behind the speed text.
     */
    bool showGraph { true };

    /**
     * @brief Number of history samples kept for the in-widget mini-graph.
     * Each sample corresponds to one poll interval.
     */
    int graphHistorySize { 60 };

    /**
     * @brief Speed unit display mode.
     * "auto"  – auto-select B/KB/MB/GB
     * "mbps"  – always Mbps
     * "kbps"  – always KB/s
     */
    QString speedUnit { QStringLiteral("auto") };

    /**
     * @brief Number of decimal places for displayed speeds [0, 2].
     */
    int decimalPlaces { 1 };

    // ── Window / Positioning ──────────────────────────────────────────────────

    /** Widget position on screen (top-left corner). */
    QPoint  widgetPos  { 100, 100 };

    /** Widget size. */
    QSize   widgetSize { 160, 56  };

    /** When true, the widget cannot be dragged. */
    bool positionLocked { false };

    // ── Theme (Red & Black — kept here for future light/dark switching) ────────

    /**
     * @brief Primary accent colour as #RRGGBB hex string.
     * Default: vivid red.
     */
    QString accentColor { QStringLiteral("#E53935") };

    /**
     * @brief Background colour as #RRGGBBAA hex string (alpha supported).
     */
    QString backgroundColor { QStringLiteral("#CC000000") };

    /**
     * @brief Text colour for speed values.
     */
    QString textColor { QStringLiteral("#FFFFFF") };
};

} // namespace nsm
