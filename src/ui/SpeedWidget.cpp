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

#ifdef Q_OS_WIN
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
            | Qt::ToolTip          // hides from taskbar app list
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
    // Primary screen taskbar area (bottom by default on Win11)
    QScreen* scr = QGuiApplication::primaryScreen();
    if (!scr) return QRect(0, 0, 1920, 40);

    QRect avail = scr->availableGeometry();
    QRect full  = scr->geometry();

    // Taskbar is the difference between full and available
    if (avail.bottom() < full.bottom()) {
        // Bottom taskbar
        return QRect(full.left(), avail.bottom(),
                     full.width(), full.bottom() - avail.bottom());
    }
    else if (avail.top() > full.top()) {
        // Top taskbar
        return QRect(full.left(), full.top(),
                     full.width(), avail.top() - full.top());
    }
    else if (avail.left() > full.left()) {
        // Left taskbar
        return QRect(full.left(), full.top(),
                     avail.left() - full.left(), full.height());
    }
    else if (avail.right() < full.right()) {
        // Right taskbar
        return QRect(avail.right(), full.top(),
                     full.right() - avail.right(), full.height());
    }

    // Fallback: assume bottom taskbar ~48px
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

    // NO background painting — fully transparent
    drawArrowsAndText(p);
}

void SpeedWidget::drawArrowsAndText(QPainter& p)
{
    const int arrowSize = m_layout.arrowSize;
    const int xArrow = 6;

    // ── Upload arrow (RED) ──────────────────────────────────────────────────
    const int upY = 6 + arrowSize;
    QPolygonF upArrow;
    upArrow << QPointF(xArrow + arrowSize / 2.0, upY - arrowSize)
            << QPointF(xArrow, upY)
            << QPointF(xArrow + arrowSize, upY);
    p.setPen(Qt::NoPen);
    p.setBrush(kColorUploadArrow);
    p.drawPolygon(upArrow);

    // Upload text (WHITE)
    const QString upText = formatSpeed(m_currentUploadBps);
    drawShadowedText(p, m_layout.textX, upY - 1, upText,
                     m_layout.font, kColorText);

    // ── Download arrow (GREEN) ─────────────────────────────────────────────
    const int downY = height() - 6;
    QPolygonF downArrow;
    downArrow << QPointF(xArrow + arrowSize / 2.0, downY + arrowSize)
              << QPointF(xArrow, downY)
              << QPointF(xArrow + arrowSize, downY);
    p.setBrush(kColorDownloadArrow);
    p.drawPolygon(downArrow);

    // Download text (WHITE)
    const QString downText = formatSpeed(m_currentDownloadBps);
    drawShadowedText(p, m_layout.textX, downY + 1, downText,
                     m_layout.font, kColorText);
}

// ── Text shadow for readability on any wallpaper ────────────────────────────

void SpeedWidget::drawShadowedText(QPainter& p, int x, int y,
                                   const QString& text, const QFont& font,
                                   const QColor& color)
{
    p.setFont(font);

    // Shadow (1px offset, semi-transparent black)
    p.setPen(QColor(0, 0, 0, 180));
    p.drawText(x + 1, y + 1, text);

    // Main text
    p.setPen(color);
    p.drawText(x, y, text);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Mouse — X-axis only, constrained to taskbar row
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
    // Subtle opacity boost on hover for feedback
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

// ═════════════════════════════════════════════════════════════════════════════
//  Layout & Formatting
// ═════════════════════════════════════════════════════════════════════════════

void SpeedWidget::updateLayoutMetrics()
{
    const qreal scale = static_cast<qreal>(m_style.fontScale);

    int baseSize = m_style.fontSize;
    int fontSize = qMax(7, qRound(baseSize * scale));

    m_layout.arrowSize = qMax(5, qRound(7 * scale));
    m_layout.textX     = qMax(14, qRound(18 * scale));
    m_layout.rowHeight = qMax(12, qRound(16 * scale));

    QFont font(m_style.fontFamily);
    font.setPointSize(fontSize);
    font.setWeight(QFont::DemiBold);
    font.setStyleStrategy(QFont::PreferAntialias);
    m_layout.font = font;
}

QString SpeedWidget::formatSpeed(double bps) const
{
    const bool bitsMode = (m_style.speedUnit == QStringLiteral("bits"));

    // Convert to appropriate base unit
    double value;
    QString unit;

    if (bitsMode) {
        // bits/sec: Kbps, Mbps, Gbps (1 byte = 8 bits)
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
        // bytes/sec: KB/s, MB/s, GB/s (never B/s)
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
            // auto — never show B/s
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
