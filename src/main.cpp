#include <QApplication>
#include <QDebug>

#include "config/ConfigManager.h"
#include "core/NetworkPoller.h"
#include "ui/TrayManager.h"
#include "ui/SpeedWidget.h"

/**
 * @file main.cpp
 *
 * NetSpeedMeter — Batch 5+ refined.
 *
 * Behaviour:
 *  • Config loaded from JSON.
 *  • NetworkPoller starts in background.
 *  • SpeedWidget appears as compact taskbar overlay (transparent, X-drag only).
 *  • TrayManager provides system tray icon and Settings dialog.
 */

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("NetSpeedMeter"));
    QApplication::setOrganizationName(QStringLiteral("NetSpeedMeter"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication::setQuitOnLastWindowClosed(false);

    // ── Configuration ─────────────────────────────────────────────────────────
    nsm::ConfigManager& cfgMgr = nsm::ConfigManager::instance();
    if (!cfgMgr.load())
        qWarning() << "Failed to load config; using defaults.";

    const nsm::AppConfig cfg = cfgMgr.config();

    // ── Network Poller ────────────────────────────────────────────────────────
    nsm::NetworkPoller poller;
    poller.setIntervalMs(cfg.updateIntervalMs);
    poller.setAdapterMode(cfg.adapterMode);
    poller.setSelectedAdapters(cfg.selectedAdapterGuids);

    QObject::connect(&cfgMgr, &nsm::ConfigManager::configChanged,
                     &app, [&](const nsm::AppConfig& newCfg) {
        poller.setIntervalMs(newCfg.updateIntervalMs);
        poller.setAdapterMode(newCfg.adapterMode);
        poller.setSelectedAdapters(newCfg.selectedAdapterGuids);
        qDebug() << "Config updated — interval:" << newCfg.updateIntervalMs << "ms";
    });

    QObject::connect(&poller, &nsm::NetworkPoller::pollingError,
                     &app, [](const QString& msg) {
        qWarning() << "Poller error:" << msg;
    });

    poller.start();

    // ── Speed Widget (taskbar overlay) ────────────────────────────────────────
    nsm::SpeedWidget speedWidget(&poller);
    speedWidget.restorePosition();
    speedWidget.show();

    // ── Tray Manager ──────────────────────────────────────────────────────────
    nsm::TrayManager trayMgr(&poller, &app);
    trayMgr.install();

    QObject::connect(&speedWidget, &nsm::SpeedWidget::settingsRequested,
                     &trayMgr, &nsm::TrayManager::showSettings);
    QObject::connect(&speedWidget, &nsm::SpeedWidget::exitRequested,
                     &trayMgr, &nsm::TrayManager::quitApp);

    if (!cfg.minimiseToTray) {
        trayMgr.showSettings();
    }

    QObject::connect(&trayMgr, &nsm::TrayManager::exitRequested,
                     &app, &QApplication::quit);

    // ── Save config on clean exit ─────────────────────────────────────────────
    QObject::connect(&app, &QApplication::aboutToQuit, [&] {
        trayMgr.uninstall();
        speedWidget.hide();
        poller.stop();
        poller.wait(2000);
        cfgMgr.save();
        qDebug() << "NetSpeedMeter shut down cleanly.";
    });

    return app.exec();
}
