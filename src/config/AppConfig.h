#pragma once

#include <QString>
#include <QStringList>
#include <QPoint>
#include <QSize>

#include "core/NetworkPoller.h"

namespace nsm {

struct AppConfig
{
    // General
    int updateIntervalMs { 1000 };
    bool startWithWindows { false };
    bool minimiseToTray { true };

    // Network
    NetworkPoller::AdapterMode adapterMode { NetworkPoller::AdapterMode::AutoPrimary };
    QStringList selectedAdapterGuids;

    // Appearance
    double opacity { 0.92 };
    double fontScale { 1.0 };
    QString fontFamily { QStringLiteral("Segoe UI") };
    int fontSize { 11 };
    bool fontBold { false }; // FIXED: Normal text by default
    bool showGraph { false };
    int graphHistorySize { 60 };

    // Speed Display
    QString speedUnit { QStringLiteral("auto") };
    int decimalPlaces { 1 };

    // Window / Positioning
    QPoint  widgetPos  { 0, 0 };
    QSize   widgetSize { 120, 36 };
    bool positionLocked { false };
};

} // namespace nsm