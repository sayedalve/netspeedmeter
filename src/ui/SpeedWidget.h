#pragma once

#include "core/SpeedSample.h"
#include "config/AppConfig.h"

#include <QWidget>
#include <QTimer>
#include <QPoint>
#include <QPropertyAnimation>

// Forward declarations
class QPainter;
class QMouseEvent;

namespace nsm {

class NetworkPoller;

/**
 * @brief Compact taskbar-integrated speed widget.
 *
 * Design:
 *  • 100% transparent background (no painting, only text/arrows).
 *  • Fixed compact size fitting inside Windows 11 taskbar.
 *  • X-axis-only drag constrained to taskbar row.
 *  • Hardcoded colors: upload arrow RED, download arrow GREEN, text WHITE.
 *  • No graph, no peak indicator, no color settings.
 *  • Stays on top of taskbar (ToolTip + StaysOnTopHint).
 */
class SpeedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SpeedWidget(NetworkPoller* poller, QWidget* parent = nullptr);
    ~SpeedWidget() override;

    /** @brief Position widget at stored location, or auto-detect taskbar left. */
    void restorePosition();

    /** @brief Force re-evaluation of taskbar geometry and snap if needed. */
    void snapToTaskbar();

signals:
    void settingsRequested();
    void exitRequested();

protected:
    void paintEvent(QPaintEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    void moveEvent(QMoveEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void onSpeedUpdated(SpeedSample sample);
    void onConfigChanged(AppConfig newConfig);
    void onSavePositionTimer();

private:
    // ── Rendering ─────────────────────────────────────────────────────────────
    void drawArrowsAndText(QPainter& p);
    void drawShadowedText(QPainter& p, int x, int y,
                          const QString& text, const QFont& font,
                          const QColor& color);

    // ── Taskbar geometry ────────────────────────────────────────────────────
    QRect taskbarRect() const;
    int   taskbarHeight() const;

    // ── Formatting ──────────────────────────────────────────────────────────
    QString formatSpeed(double bps) const;

    // ── Layout ────────────────────────────────────────────────────────────────
    void updateLayoutMetrics();

    // Core reference
    NetworkPoller* m_poller { nullptr };

    // Drag state (X-axis only)
    bool   m_dragging { false };
    QPoint m_dragOffset;
    int    m_taskbarY { 0 };     // cached Y to constrain drag

    // Current speeds
    double m_currentUploadBps   { 0.0 };
    double m_currentDownloadBps { 0.0 };

    // Layout
    struct LayoutMetrics {
        int arrowSize  { 7 };
        int textX      { 18 };
        int rowHeight  { 16 };
        int baselineY  { 0 };
        QFont font;
    } m_layout;

    // Config cache
    struct StyleCache {
        double opacity      { 0.92 };
        double fontScale    { 1.0 };
        QString fontFamily  { QStringLiteral("Segoe UI") };
        int    fontSize     { 11 };
        QString speedUnit   { QStringLiteral("auto") };
        int    decimalPlaces{ 1 };
        bool   positionLocked { false };
    } m_style;

    // Persistence timer
    QTimer* m_savePosTimer { nullptr };
};

} // namespace nsm
