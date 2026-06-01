#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QPixmap>
#include <QPainter>

// Forward declarations
class QWidget;

namespace nsm {

class NetworkPoller;
class SettingsDialog;

/**
 * @brief Manages the system tray icon, its context menu, and the
 *        SettingsDialog lifecycle for Batch 3.
 */
class TrayManager : public QObject
{
    Q_OBJECT

public:
    explicit TrayManager(NetworkPoller* poller, QObject* parent = nullptr);
    ~TrayManager() override;

    void install();
    void uninstall();
    bool isInstalled() const;

public slots:
    void showSettings();
    void hideSettings();
    void toggleSettings();
    void quitApp();

signals:
    void exitRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onSettingsDialogFinished(int result);

private:
    static QIcon generateTrayIcon();
    void buildMenu();

    NetworkPoller* m_poller { nullptr };

    QSystemTrayIcon* m_trayIcon { nullptr };
    QMenu*           m_trayMenu { nullptr };
    QAction*         m_settingsAction { nullptr };
    QAction*         m_exitAction     { nullptr };

    SettingsDialog* m_settingsDlg { nullptr };
    bool m_installed { false };
};

} // namespace nsm
