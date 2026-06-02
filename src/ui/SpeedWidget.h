#pragma once

#include "core/SpeedSample.h"
#include "config/AppConfig.h"

#include <QWidget>
#include <QTimer>
#include <QPoint>

// Forward declarations
class QPainter;
class QMouseEvent;

namespace nsm {

class NetworkPoller;

class SpeedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SpeedWidget(NetworkPoller* poller, QWidget* parent = nullptr);
    ~SpeedWidget() override;

    void restorePosition();
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
    void onVisibilityCheckTimer();

private:
    void drawArrowsAndText(QPainter& p);
    void drawShadowedText(QPainter& p, int x, int y,
                          const QString& text, const QFont& font,
                          const QColor& color);

    QRect taskbarRect() const;
    int   taskbarHeight() const;
    bool  isFullscreenOrStartMenuActive() const;

    QString formatSpeed(double bps) const;
    void updateLayoutMetrics();

    NetworkPoller* m_poller { nullptr };

    bool   m_dragging { false };
    QPoint m_dragOffset;
    int    m_taskbarY { 0 };

    double m_currentUploadBps   { 0.0 };
    double m_currentDownloadBps { 0.0 };

    struct LayoutMetrics {
        int arrowSize  { 6 };
        int textX      { 14 };
        int rowHeight  { 14 };
        int baselineY  { 0 };
        QFont font;
    } m_layout;

    struct StyleCache {
        double opacity      { 0.92 };
        double fontScale    { 1.0 };
        QString fontFamily  { QStringLiteral("Segoe UI") };
        int    fontSize     { 11 };
        bool   fontBold     { false }; // ADDED
        QString speedUnit   { QStringLiteral("auto") };
        int    decimalPlaces{ 1 };
        bool   positionLocked { false };
    } m_style;

    QTimer* m_savePosTimer { nullptr };
    QTimer* m_visibilityTimer { nullptr };
    bool    m_shouldBeVisible { true };
};

} // namespace nsm