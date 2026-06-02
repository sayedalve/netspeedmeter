#include "SpeedWidget.h"
#include "core/NetworkPoller.h"
#include "core/SpeedSample.h"
#include "config/ConfigManager.h"
#include "config/AppConfig.h"

#include <QPainter>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QScreen>
#include <QGuiApplication>
#include <QTimer>

#ifdef _WIN32
#  include <windows.h>
#endif

namespace nsm {

// ═════════════════════════════════════════════════════════════════════════════
//  SpeedWidget — Compact taskbar-integrated overlay
// ═════════════════════════════════════════════════════════════════════════════

static constexpr int kDefaultWidth  = 120;
static constexpr int kDefaultHeight = 36;

// Hardcoded colors — NEVER change these
static const QColor kColorUploadArrow(220, 53, 69);    // Red #DC3545
static const QColor kColorDownloadArrow(40, 167, 69);  // Green #28A745
static const QColor kColorText(255, 255, 255);         // White

SpeedWidget::SpeedWidget(NetworkPoller* poller, QWidget* parent)
    : QWidget(parent,
              Qt::FramelessWindowHint
            | Qt::WindowStaysOnTopHint
            | Qt::Tool             // Using Tool instead of ToolTip prevents disappearing on click
            | Qt::NoDropShadowWindowHint)
    , m_poller(poller)
{
    Q_ASSERT(poller != nullptr);

    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);

    setFixedSize(kDefaultWidth, kDefaultHeight);
    setWindowTitle(tr("NetSpeedMeter"));

    // Load config and connect
    onConfigChanged(ConfigManager::instance().config());

    connect(poller, &NetworkPoller::speedUpdated,
            this, &SpeedWidget::onSpeedUpdated, Qt::QueuedConnection);

    connect(&ConfigManager::instance(), &ConfigManager::configChanged,
            this, &SpeedWidget::onConfigChanged);

    // Debounced position save
    m_savePosTimer = new QTimer(this);
    m_savePosTimer->setSingleShot(true);
    m_savePosTimer->setInterval(500);
    connect(m_savePosTimer, &QTimer::timeout,
            this, &SpeedWidget::onSavePositionTimer);

    updateLayoutMetrics();

    // ── Z-Order Checker Timer ──
    QTimer* visTimer = new QTimer(this);
    connect(visTimer, &QTimer::timeout, this, &SpeedWidget::onVisibilityCheckTimer);
    visTimer->start(500);
}

SpeedWidget::~SpeedWidget() = default;

// ── Positioning ───────────────────────────────────────────────────────────────

void SpeedWidget::restorePosition()
{
    const AppConfig cfg = ConfigManager::instance().config();

    // Detect taskbar geometry
    QRect tbar = taskbarRect();
    int tbarH = taskbarHeight();

    // Default: left side of taskbar, vertically centered
    int x = cfg.widgetPos.x();
    int y = cfg.widgetPos.y();

    if (x == 0 && y == 0) {
        // First run — snap to taskbar left
        x = tbar.left() + 8;
        y = tbar.top() + (tbarH - height()) / 2;
    } else {
        // Validate stored Y is still on taskbar; if not, recenter
        if (qAbs(y - tbar.top()) > tbarH) {
            y = tbar.top() + (tbarH - height()) / 2;
        }
    }

    m_taskbarY = tbar.top() + (tbarH - height()) / 2;
    move(x, y);
}

void SpeedWidget::snapToTaskbar()
{
    QRect tbar = taskbarRect();
    int tbarH = taskbarHeight();
    m_taskbarY = tbar.top() + (tbarH - height()) / 2;

    int x = qBound(tbar.left(), pos().x(), tbar.right() - width());
    move(x, m_taskbarY);
}

QRect SpeedWidget::taskbarRect() const
{
    QScreen* scr = QGuiApplication::primaryScreen();
    if (!scr) return QRect(0, 0, 1920, 40);

    QRect avail = scr->availableGeometry();
    QRect full  = scr->geometry();

    if (avail.bottom() < full.bottom()) {
        return QRect(full.left(), avail.bottom(),
                     full.width(), full.bottom() - avail.bottom());
    }
    else if (avail.top() > full.top()) {
        return QRect(full.left(), full.top(),
                     full.width(), avail.top() - full.top());
    }
    else if (avail.left() > full.left()) {
        return QRect(full.left(), full.top(),
                     avail.left() - full.left(), full.height());
    }
    else if (avail.right() < full.right()) {
        return QRect(avail.right(), full.top(),
                     full.right() - avail.right(), full.height());
    }
    return QRect(full.left(), full.bottom() - 48, full.width(), 48);
}

int SpeedWidget::taskbarHeight() const
{
    return taskbarRect().height();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Painting — NO background, only arrows + text
// ═════════════════════════════════════════════════════════════════════════════

void SpeedWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    drawArrowsAndText(p);
}

void SpeedWidget::drawArrowsAndText(QPainter& p)
{
    const int arrowSize = m_layout.arrowSize;
    const int xArrow = 6;
    
    // Tightened Y coordinates
    const int upBaseline = 15;
    const int downBaseline = 29;

    // Upload arrow (RED)
    QPolygonF upArrow;
    upArrow << QPointF(xArrow + arrowSize / 2.0, upBaseline - arrowSize)
            << QPointF(xArrow, upBaseline)
            << QPointF(xArrow + arrowSize, upBaseline);
    p.setPen(Qt::NoPen);
    p.setBrush(kColorUploadArrow);
    p.drawPolygon(upArrow);

    // Upload text (WHITE)
    const QString upText = formatSpeed(m_currentUploadBps);
    drawShadowedText(p, m_layout.textX, upBaseline, upText,
                     m_layout.font, kColorText);

    // Download arrow (GREEN)
    QPolygonF downArrow;
    downArrow << QPointF(xArrow, downBaseline - arrowSize)
              << QPointF(xArrow + arrowSize, downBaseline - arrowSize)
              << QPointF(xArrow + arrowSize / 2.0, downBaseline);
    p.setBrush(kColorDownloadArrow);
    p.drawPolygon(downArrow);

    // Download text (WHITE)
    const QString downText = formatSpeed(m_currentDownloadBps);
    drawShadowedText(p, m_layout.textX, downBaseline, downText,
                     m_layout.font, kColorText);
}

void SpeedWidget::drawShadowedText(QPainter& p, int x, int y,
                                   const QString& text, const QFont& font,
                                   const QColor& color)
{
    p.setFont(font);
    p.setPen(QColor(0, 0, 0, 180));
    p.drawText(x + 1, y + 1, text);
    p.setPen(color);
    p.drawText(x, y, text);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Mouse Events
// ═════════════════════════════════════════════════════════════════════════════

void SpeedWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && !m_style.positionLocked) {
        m_dragging = true;
        m_dragOffset = event->pos();
    }
    QWidget::mousePressEvent(event);
}

void SpeedWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        int newX = event->globalPosition().toPoint().x() - m_dragOffset.x();
        QRect tbar = taskbarRect();
        newX = qBound(tbar.left(), newX, tbar.right() - width());
        move(newX, m_taskbarY);
    }
    QWidget::mouseMoveEvent(event);
}

void SpeedWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        m_savePosTimer->start();
    }
    QWidget::mouseReleaseEvent(event);
}

void SpeedWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit settingsRequested();
    }
    QWidget::mouseDoubleClickEvent(event);
}

void SpeedWidget::moveEvent(QMoveEvent* /*event*/)
{
    if (!m_dragging)
        return;
    m_savePosTimer->start();
}

void SpeedWidget::enterEvent(QEnterEvent* /*event*/)
{
    setWindowOpacity(qMin(1.0, m_style.opacity + 0.05));
}

void SpeedWidget::leaveEvent(QEvent* /*event*/)
{
    setWindowOpacity(m_style.opacity);
}

void SpeedWidget::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.setStyleSheet(QStringLiteral(R"(
        QMenu {
            background-color: #1A1A1A;
            color: #F0F0F0;
            border: 1px solid #3A3A3A;
            border-radius: 6px;
            padding: 4px;
        }
        QMenu::item {
            padding: 6px 24px;
            border-radius: 4px;
        }
        QMenu::item:selected {
            background-color: #E53935;
            color: #FFFFFF;
        }
        QMenu::separator {
            background-color: #3A3A3A;
            height: 1px;
            margin: 4px 8px;
        }
    )"));

    QAction* settingsAct = menu.addAction(tr("Settings…"));
    QAction* lockAct     = menu.addAction(
        m_style.positionLocked ? tr("Unlock Position") : tr("Lock Position"));
    menu.addSeparator();
    QAction* exitAct = menu.addAction(tr("Exit"));

    connect(settingsAct, &QAction::triggered, this, &SpeedWidget::settingsRequested);
    connect(lockAct, &QAction::triggered, [&]() {
        AppConfig cfg = ConfigManager::instance().config();
        cfg.positionLocked = !cfg.positionLocked;
        ConfigManager::instance().setConfig(cfg);
        ConfigManager::instance().save();
    });
    connect(exitAct, &QAction::triggered, this, &SpeedWidget::exitRequested);

    menu.exec(event->globalPos());
}

// ═════════════════════════════════════════════════════════════════════════════
//  Slots
// ═════════════════════════════════════════════════════════════════════════════

void SpeedWidget::onSpeedUpdated(SpeedSample sample)
{
    m_currentUploadBps   = sample.uploadBps;
    m_currentDownloadBps = sample.downloadBps;
    update();
}

void SpeedWidget::onConfigChanged(AppConfig newConfig)
{
    m_style.opacity       = newConfig.opacity;
    m_style.fontScale     = newConfig.fontScale;
    m_style.fontFamily    = newConfig.fontFamily;
    m_style.fontSize      = newConfig.fontSize;
    m_style.speedUnit     = newConfig.speedUnit;
    m_style.decimalPlaces = newConfig.decimalPlaces;
    m_style.positionLocked = newConfig.positionLocked;

    setWindowOpacity(m_style.opacity);
    updateLayoutMetrics();
    update();
}

void SpeedWidget::onSavePositionTimer()
{
    AppConfig cfg = ConfigManager::instance().config();
    cfg.widgetPos  = pos();
    cfg.widgetSize = size();
    ConfigManager::instance().setConfig(cfg);
    ConfigManager::instance().save();
    qDebug() << "SpeedWidget: position saved" << cfg.widgetPos;
}

void SpeedWidget::onVisibilityCheckTimer()
{
#ifdef _WIN32
    HWND fg = GetForegroundWindow();
    if (!fg) return;

    char className[256];
    GetClassNameA(fg, className, sizeof(className));
    QString cls = QString::fromLatin1(className);

    RECT rc;
    GetWindowRect(fg, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    bool isFullScreen = (w >= sw && h >= sh && cls != "Progman" && cls != "WorkerW");
    
    bool isStartMenu = (cls == "Windows.UI.Core.CoreWindow" ||
                        cls == "SearchUI" ||
                        cls == "StartMenuExperienceHost" ||
                        cls == "SearchHost");

    if (isFullScreen || isStartMenu) {
        if (isVisible()) hide();
    } else {
        if (!isVisible()) show();
        
        // FIXED: Force widget to absolute top of Z-Order so taskbar clicks can't hide it
        HWND hwnd = reinterpret_cast<HWND>(winId());
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
#endif
}

// ═════════════════════════════════════════════════════════════════════════════
//  Layout & Formatting
// ═════════════════════════════════════════════════════════════════════════════

void SpeedWidget::updateLayoutMetrics()
{
    const AppConfig cfg = ConfigManager::instance().config(); // Grabs fresh config
    const qreal scale = static_cast<qreal>(m_style.fontScale);

    int baseSize = m_style.fontSize;
    int fontSize = qMax(7, qRound(baseSize * scale));

    m_layout.arrowSize = qMax(5, qRound(7 * scale));
    m_layout.textX     = qMax(14, qRound(18 * scale));
    m_layout.rowHeight = qMax(12, qRound(16 * scale));

    QFont font(m_style.fontFamily);
    font.setPointSize(fontSize);
    
    // FIXED: Now respects the bold setting, defaults to Normal instead of DemiBold
    font.setWeight(cfg.fontBold ? QFont::Bold : QFont::Normal); 
    
    font.setStyleStrategy(QFont::PreferAntialias);
    m_layout.font = font;
}

QString SpeedWidget::formatSpeed(double bps) const
{
    const bool bitsMode = (m_style.speedUnit == QStringLiteral("bits"));

    double value;
    QString unit;

    if (bitsMode) {
        double bits = bps * 8.0;
        double kbps = bits / 1000.0;
        double mbps = kbps / 1000.0;
        double gbps = mbps / 1000.0;

        if (gbps >= 1.0) {
            value = gbps;
            unit  = QStringLiteral("Gbps");
        } else if (mbps >= 1.0) {
            value = mbps;
            unit  = QStringLiteral("Mbps");
        } else {
            value = kbps;
            unit  = QStringLiteral("Kbps");
        }
    }
    else {
        double kbps = bps / 1024.0;
        double mbps = kbps / 1024.0;
        double gbps = mbps / 1024.0;

        if (m_style.speedUnit == QStringLiteral("mbps")) {
            value = mbps;
            unit  = QStringLiteral("MB/s");
        }
        else if (m_style.speedUnit == QStringLiteral("kbps")) {
            value = kbps;
            unit  = QStringLiteral("KB/s");
        }
        else {
            if (gbps >= 1.0) {
                value = gbps;
                unit  = QStringLiteral("GB/s");
            } else if (mbps >= 1.0) {
                value = mbps;
                unit  = QStringLiteral("MB/s");
            } else {
                value = kbps;
                unit  = QStringLiteral("KB/s");
            }
        }
    }

    return QString::number(value, 'f', m_style.decimalPlaces) + QLatin1Char(' ') + unit;
}

} // namespace nsm