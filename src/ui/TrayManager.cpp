#include "TrayManager.h"
#include "SettingsDialog.h"
#include "core/NetworkPoller.h"
#include "config/ConfigManager.h"
#include "config/AppConfig.h"

#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QDebug>

namespace nsm {

static constexpr int kIconSize   = 64;
static constexpr int kIconRadius = 10;

TrayManager::TrayManager(NetworkPoller* poller, QObject* parent)
    : QObject(parent)
    , m_poller(poller)
{
    Q_ASSERT(poller != nullptr);

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(generateTrayIcon());
    m_trayIcon->setToolTip(tr("NetSpeedMeter — right-click for menu"));

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &TrayManager::onTrayActivated);

    buildMenu();
}

TrayManager::~TrayManager()
{
    uninstall();
    delete m_settingsDlg;
    m_settingsDlg = nullptr;
}

void TrayManager::install()
{
    if (m_installed)
        return;

    m_trayIcon->show();
    m_installed = true;
    qDebug() << "TrayManager: tray icon installed.";
}

void TrayManager::uninstall()
{
    if (!m_installed)
        return;

    m_trayIcon->hide();
    m_installed = false;
    qDebug() << "TrayManager: tray icon uninstalled.";
}

bool TrayManager::isInstalled() const
{
    return m_installed;
}

void TrayManager::showSettings()
{
    if (!m_settingsDlg) {
        m_settingsDlg = new SettingsDialog(m_poller, nullptr);
        m_settingsDlg->installEventFilter(this);

        connect(m_settingsDlg, &QDialog::finished,
                this, &TrayManager::onSettingsDialogFinished);
    }

    m_settingsDlg->show();
    m_settingsDlg->raise();
    m_settingsDlg->activateWindow();
}

void TrayManager::hideSettings()
{
    if (m_settingsDlg)
        m_settingsDlg->hide();
}

void TrayManager::toggleSettings()
{
    if (m_settingsDlg && m_settingsDlg->isVisible())
        hideSettings();
    else
        showSettings();
}

void TrayManager::quitApp()
{
    emit exitRequested();
}

void TrayManager::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        toggleSettings();
        break;
    default:
        break;
    }
}

bool TrayManager::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_settingsDlg && event->type() == QEvent::Close) {
        // FIXED: Always just hide the dialog. Never kill the app here.
        event->ignore();
        m_settingsDlg->hide();
        return true;
    }
    return QObject::eventFilter(watched, event);
}

void TrayManager::onSettingsDialogFinished(int result)
{
    Q_UNUSED(result)
    
    // FIXED: Removed the logic that emitted exitRequested() when the window closed.
    if (m_settingsDlg && m_settingsDlg->isVisible())
        m_settingsDlg->hide();
}

void TrayManager::buildMenu()
{
    m_trayMenu = new QMenu();

    m_settingsAction = new QAction(tr("Settings…"), this);
    m_settingsAction->setIcon(generateTrayIcon().pixmap(16, 16));
    connect(m_settingsAction, &QAction::triggered,
            this, &TrayManager::showSettings);

    m_exitAction = new QAction(tr("Exit"), this);
    connect(m_exitAction, &QAction::triggered,
            this, &TrayManager::quitApp);

    m_trayMenu->addAction(m_settingsAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_exitAction);

    m_trayIcon->setContextMenu(m_trayMenu);
}

QIcon TrayManager::generateTrayIcon()
{
    QPixmap pixmap(kIconSize, kIconSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bgRect(0, 0, kIconSize, kIconSize);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#1A1A1A")));
    painter.drawRoundedRect(bgRect, kIconRadius, kIconRadius);

    QPen borderPen(QColor(QStringLiteral("#E53935")));
    borderPen.setWidthF(2.5);
    borderPen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(bgRect.adjusted(1.2, 1.2, -1.2, -1.2),
                            kIconRadius - 1, kIconRadius - 1);

    const QColor arrowColor(QStringLiteral("#E53935"));
    painter.setPen(Qt::NoPen);
    painter.setBrush(arrowColor);

    const qreal cx = kIconSize / 2.0;
    const qreal cy = kIconSize / 2.0 + 2.0;
    const qreal arrowW = 22.0;
    const qreal arrowH = 18.0;
    const qreal stemW  = 6.0;

    QPolygonF arrow;
    arrow << QPointF(cx, cy - arrowH)
          << QPointF(cx + arrowW, cy)
          << QPointF(cx + arrowW - stemW, cy)
          << QPointF(cx, cy - arrowH + stemW)
          << QPointF(cx - arrowW + stemW, cy)
          << QPointF(cx - arrowW, cy);
    painter.drawPolygon(arrow);

    painter.end();

    QIcon icon;
    icon.addPixmap(pixmap);
    return icon;
}

} // namespace nsm