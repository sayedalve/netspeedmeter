#include "ConfigManager.h"

#include <QMutexLocker>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

namespace nsm {

ConfigManager& ConfigManager::instance()
{
    static ConfigManager inst;
    return inst;
}

ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
{
    const QString appDataDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QDir dir(appDataDir);
    if (!dir.exists())
        dir.mkpath(appDataDir);

    m_filePath = appDataDir + QStringLiteral("/config.json");
}

AppConfig ConfigManager::config() const
{
    QMutexLocker lock(&m_mutex);
    return m_config;
}

void ConfigManager::setConfig(const AppConfig& cfg)
{
    {
        QMutexLocker lock(&m_mutex);
        m_config = cfg;
    }
    emit configChanged(cfg);
}

QString ConfigManager::configFilePath() const
{
    return m_filePath;
}

static QString adapterModeToString(NetworkPoller::AdapterMode mode)
{
    switch (mode) {
    case NetworkPoller::AdapterMode::AutoPrimary:  return QStringLiteral("auto");
    case NetworkPoller::AdapterMode::AllAdapters:  return QStringLiteral("all");
    case NetworkPoller::AdapterMode::Specific:     return QStringLiteral("specific");
    }
    return QStringLiteral("auto");
}

static NetworkPoller::AdapterMode adapterModeFromString(const QString& s)
{
    if (s == QStringLiteral("all"))      return NetworkPoller::AdapterMode::AllAdapters;
    if (s == QStringLiteral("specific")) return NetworkPoller::AdapterMode::Specific;
    return NetworkPoller::AdapterMode::AutoPrimary;
}

bool ConfigManager::load()
{
    QFile file(m_filePath);

    if (!file.exists()) {
        qDebug() << "ConfigManager: no config file found, using defaults.";
        QMutexLocker lock(&m_mutex);
        m_config = AppConfig{}; // Explicitly ensure defaults are mapped
        return true;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "ConfigManager: cannot open" << m_filePath;
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "ConfigManager: JSON parse error:" << parseError.errorString();
        return false;
    }

    const QJsonObject root = doc.object();
    AppConfig cfg;

    if (root.contains(K_GENERAL)) {
        const QJsonObject gen = root[K_GENERAL].toObject();
        cfg.updateIntervalMs  = gen[K_UPDATE_INTERVAL].toInt(cfg.updateIntervalMs);
        cfg.startWithWindows  = gen[K_START_WITH_WIN].toBool(cfg.startWithWindows);
        cfg.minimiseToTray    = gen[K_MIN_TO_TRAY].toBool(cfg.minimiseToTray);
        cfg.updateIntervalMs = qBound(100, cfg.updateIntervalMs, 10000);
    }

    if (root.contains(K_NETWORK)) {
        const QJsonObject net = root[K_NETWORK].toObject();
        cfg.adapterMode = adapterModeFromString(
            net[K_ADAPTER_MODE].toString(adapterModeToString(cfg.adapterMode)));
        const QJsonArray guidsArr = net[K_ADAPTER_GUIDS].toArray();
        cfg.selectedAdapterGuids.clear();
        for (const QJsonValue& v : guidsArr)
            cfg.selectedAdapterGuids.append(v.toString());
    }

    if (root.contains(K_APPEARANCE)) {
        const QJsonObject app = root[K_APPEARANCE].toObject();
        cfg.opacity       = app[K_OPACITY].toDouble(cfg.opacity);
        cfg.fontScale     = app[K_FONT_SCALE].toDouble(cfg.fontScale);
        cfg.fontFamily    = app[K_FONT_FAMILY].toString(cfg.fontFamily);
        cfg.fontSize      = app[K_FONT_SIZE].toInt(cfg.fontSize);
        cfg.fontBold      = app[K_FONT_BOLD].toBool(cfg.fontBold);
        cfg.showGraph     = app[K_SHOW_GRAPH].toBool(cfg.showGraph);
        cfg.graphHistorySize = app[K_GRAPH_HISTORY].toInt(cfg.graphHistorySize);
        cfg.speedUnit     = app[K_SPEED_UNIT].toString(cfg.speedUnit);
        cfg.decimalPlaces = app[K_DECIMAL_PLACES].toInt(cfg.decimalPlaces);

        cfg.opacity       = qBound(0.05, cfg.opacity, 1.0);
        cfg.fontScale     = qBound(0.5,  cfg.fontScale, 3.0);
        cfg.fontSize      = qBound(6, cfg.fontSize, 24);
        cfg.graphHistorySize = qBound(10, cfg.graphHistorySize, 300);
        cfg.decimalPlaces = qBound(0, cfg.decimalPlaces, 2);
    }

    if (root.contains(K_WINDOW)) {
        const QJsonObject win = root[K_WINDOW].toObject();
        cfg.widgetPos.setX(win[K_POS_X].toInt(cfg.widgetPos.x()));
        cfg.widgetPos.setY(win[K_POS_Y].toInt(cfg.widgetPos.y()));
        cfg.widgetSize.setWidth(win[K_WIDTH].toInt(cfg.widgetSize.width()));
        cfg.widgetSize.setHeight(win[K_HEIGHT].toInt(cfg.widgetSize.height()));
        cfg.positionLocked = win[K_POS_LOCKED].toBool(cfg.positionLocked);
    }

    QMutexLocker lock(&m_mutex);
    m_config = cfg;

    qDebug() << "ConfigManager: loaded from" << m_filePath;
    return true;
}

bool ConfigManager::save() const
{
    AppConfig cfg;
    {
        QMutexLocker lock(&m_mutex);
        cfg = m_config;
    }

    QJsonObject root;

    {
        QJsonObject gen;
        gen[K_UPDATE_INTERVAL] = cfg.updateIntervalMs;
        gen[K_START_WITH_WIN]  = cfg.startWithWindows;
        gen[K_MIN_TO_TRAY]     = cfg.minimiseToTray;
        root[K_GENERAL]        = gen;
    }

    {
        QJsonObject net;
        net[K_ADAPTER_MODE] = adapterModeToString(cfg.adapterMode);
        QJsonArray guidsArr;
        for (const QString& g : cfg.selectedAdapterGuids)
            guidsArr.append(g);
        net[K_ADAPTER_GUIDS] = guidsArr;
        root[K_NETWORK] = net;
    }

    {
        QJsonObject app;
        app[K_OPACITY]       = cfg.opacity;
        app[K_FONT_SCALE]    = cfg.fontScale;
        app[K_FONT_FAMILY]   = cfg.fontFamily;
        app[K_FONT_SIZE]     = cfg.fontSize;
        app[K_FONT_BOLD]     = cfg.fontBold;
        app[K_SHOW_GRAPH]    = cfg.showGraph;
        app[K_GRAPH_HISTORY] = cfg.graphHistorySize;
        app[K_SPEED_UNIT]    = cfg.speedUnit;
        app[K_DECIMAL_PLACES]= cfg.decimalPlaces;
        root[K_APPEARANCE]   = app;
    }

    {
        QJsonObject win;
        win[K_POS_X]      = cfg.widgetPos.x();
        win[K_POS_Y]      = cfg.widgetPos.y();
        win[K_WIDTH]      = cfg.widgetSize.width();
        win[K_HEIGHT]     = cfg.widgetSize.height();
        win[K_POS_LOCKED] = cfg.positionLocked;
        root[K_WINDOW]    = win;
    }

    const QJsonDocument doc(root);

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "ConfigManager: cannot write to" << m_filePath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "ConfigManager: saved to" << m_filePath;
    return true;
}

} // namespace nsm