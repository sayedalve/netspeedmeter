#pragma once

#include "AppConfig.h"

#include <QObject>
#include <QString>
#include <QMutex>

namespace nsm {

/**
 * @brief Singleton that owns the single AppConfig instance and persists it
 *        to a JSON file in the user's application data directory.
 *
 * File location (Windows):
 *   %APPDATA%/NetSpeedMeter/config.json
 *
 * Usage:
 * @code
 *   // Read
 *   AppConfig cfg = ConfigManager::instance().config();
 *
 *   // Modify and save
 *   cfg.updateIntervalMs = 500;
 *   ConfigManager::instance().setConfig(cfg);
 *   ConfigManager::instance().save();
 * @endcode
 *
 * Thread safety:
 *   config() and setConfig() are mutex-guarded.
 *   load() and save() should only be called from the GUI / main thread.
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    /** @brief Return the process-wide singleton. */
    static ConfigManager& instance();

    // ── Config access (thread-safe) ───────────────────────────────────────────

    /** @brief Return a copy of the current configuration. */
    AppConfig config() const;

    /**
     * @brief Replace the current configuration.
     *        Emits configChanged() but does NOT auto-save.
     *        Call save() explicitly after modifying.
     */
    void setConfig(const AppConfig& cfg);

    // ── Persistence ───────────────────────────────────────────────────────────

    /**
     * @brief Load configuration from disk.
     *        If the file does not exist, defaults are kept.
     * @return true on success (or first-run with no file).
     */
    bool load();

    /**
     * @brief Persist the current configuration to disk.
     * @return true on success.
     */
    bool save() const;

    /** @brief Full path to the config JSON file (for display in Settings). */
    QString configFilePath() const;

signals:
    /**
     * @brief Emitted whenever setConfig() is called.
     *        The NetworkPoller, widget, and other components listen to this
     *        to apply changes without polling ConfigManager on every tick.
     */
    void configChanged(nsm::AppConfig newConfig);

private:
    // Singleton — private ctor / no copy
    explicit ConfigManager(QObject* parent = nullptr);
    ConfigManager(const ConfigManager&)            = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    mutable QMutex m_mutex;
    AppConfig      m_config;
    QString        m_filePath;

    // ── JSON field names ──────────────────────────────────────────────────────
    // Declared as static constexpr so the compiler can deduplicate the strings.
    // (Avoids magic-string scatter across load/save implementations.)
    static constexpr const char* K_GENERAL          = "general";
    static constexpr const char* K_UPDATE_INTERVAL  = "updateIntervalMs";
    static constexpr const char* K_START_WITH_WIN   = "startWithWindows";
    static constexpr const char* K_MIN_TO_TRAY      = "minimiseToTray";

    static constexpr const char* K_NETWORK          = "network";
    static constexpr const char* K_ADAPTER_MODE     = "adapterMode";
    static constexpr const char* K_ADAPTER_GUIDS    = "selectedAdapterGuids";

    static constexpr const char* K_APPEARANCE       = "appearance";
    static constexpr const char* K_OPACITY          = "opacity";
    static constexpr const char* K_FONT_SCALE       = "fontScale";
    static constexpr const char* K_SHOW_GRAPH       = "showGraph";
    static constexpr const char* K_GRAPH_HISTORY    = "graphHistorySize";
    static constexpr const char* K_SPEED_UNIT       = "speedUnit";
    static constexpr const char* K_DECIMAL_PLACES   = "decimalPlaces";
    static constexpr const char* K_ACCENT_COLOR     = "accentColor";
    static constexpr const char* K_BG_COLOR         = "backgroundColor";
    static constexpr const char* K_TEXT_COLOR       = "textColor";

    static constexpr const char* K_WINDOW           = "window";
    static constexpr const char* K_POS_X            = "posX";
    static constexpr const char* K_POS_Y            = "posY";
    static constexpr const char* K_WIDTH            = "width";
    static constexpr const char* K_HEIGHT           = "height";
    static constexpr const char* K_POS_LOCKED       = "positionLocked";
};

} // namespace nsm
