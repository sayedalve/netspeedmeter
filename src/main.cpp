#include <QApplication>
#include <QDebug>
#include <QTimer>

#include "config/ConfigManager.h"
#include "core/NetworkPoller.h"
#include "ui/SettingsDialog.h"

/**
 * @file main.cpp
 *
 * Batch 2 entry point — extends Batch 1 with the Settings Dialog.
 *
 * Smoke-test behaviour:
 *  • Config is loaded from JSON.
 *  • NetworkPoller starts in the background.
 *  • SettingsDialog is opened immediately so Batch 2 can be verified visually.
 *  • Closing the dialog exits the application (Batch 3 adds the tray which
 *    keeps the app alive instead).
 *
 * In Batch 3 the dialog will be triggered from the tray context menu, and
 * the keepAlive timer will be replaced by the QSystemTrayIcon.
 */

int main(int argc, char* argv[])
{
    // ── Qt Application ────────────────────────────────────────────────────────
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("NetSpeedMeter"));
    QApplication::setOrganizationName(QStringLiteral("NetSpeedMeter"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    // High-DPI is opt-in prior to Qt 6.0; Qt 6 enables it automatically.
    // Explicitly set round policy for clean pixel rendering.
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

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

    // Apply live config changes emitted by the Settings Dialog
    QObject::connect(&cfgMgr, &nsm::ConfigManager::configChanged,
                     &app, [&](const nsm::AppConfig& newCfg) {
        poller.setIntervalMs(newCfg.updateIntervalMs);
        poller.setAdapterMode(newCfg.adapterMode);
        poller.setSelectedAdapters(newCfg.selectedAdapterGuids);
        qDebug() << "Config updated — interval:" << newCfg.updateIntervalMs << "ms";
    });

    // Debug speed readout (Batch 1 smoke-test, kept for Batch 2 verification)
    QObject::connect(&poller, &nsm::NetworkPoller::speedUpdated,
                     &app, [](nsm::SpeedSample s) {
        qDebug().nospace()
            << "↑ " << QString::number(nsm::SpeedSample::toKBps(s.uploadBps),   'f', 1) << " KB/s  "
            << "↓ " << QString::number(nsm::SpeedSample::toKBps(s.downloadBps), 'f', 1) << " KB/s";
    });

    QObject::connect(&poller, &nsm::NetworkPoller::pollingError,
                     &app, [](const QString& msg) {
        qWarning() << "Poller error:" << msg;
    });

    poller.start();

    // ── Batch 2 smoke-test: open the Settings Dialog immediately ──────────────
    nsm::SettingsDialog dlg(&poller);

    // When the dialog is closed (OK/Cancel), exit the app.
    // In Batch 3 this becomes "hide dialog" and the tray keeps the app alive.
    QObject::connect(&dlg, &QDialog::finished, &app, &QApplication::quit);

    dlg.show();

    // ── Save config on clean exit ─────────────────────────────────────────────
    QObject::connect(&app, &QApplication::aboutToQuit, [&] {
        poller.stop();
        poller.wait(2000);
        cfgMgr.save();
        qDebug() << "NetSpeedMeter shut down cleanly.";
    });

    return app.exec();
}
