#pragma once

#include "AppConfig.h"

#include <QObject>
#include <QString>
#include <QMutex>

namespace nsm {

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    static ConfigManager& instance();

    AppConfig config() const;
    void setConfig(const AppConfig& cfg);

    bool load();
    bool save() const;

    QString configFilePath() const;

signals:
    void configChanged(nsm::AppConfig newConfig);

private:
    explicit ConfigManager(QObject* parent = nullptr);
    ConfigManager(const ConfigManager&)            = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    mutable QMutex m_mutex;
    AppConfig      m_config;
    QString        m_filePath;

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
    static constexpr const char* K_FONT_FAMILY      = "fontFamily";
    static constexpr const char* K_FONT_SIZE        = "fontSize";
    static constexpr const char* K_SHOW_GRAPH       = "showGraph";
    static constexpr const char* K_GRAPH_HISTORY    = "graphHistorySize";
    static constexpr const char* K_SPEED_UNIT       = "speedUnit";
    static constexpr const char* K_DECIMAL_PLACES   = "decimalPlaces";

    static constexpr const char* K_WINDOW           = "window";
    static constexpr const char* K_POS_X            = "posX";
    static constexpr const char* K_POS_Y            = "posY";
    static constexpr const char* K_WIDTH            = "width";
    static constexpr const char* K_HEIGHT           = "height";
    static constexpr const char* K_POS_LOCKED       = "positionLocked";
};

} // namespace nsm
