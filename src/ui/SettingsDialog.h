#pragma once

#include "config/AppConfig.h"
#include "core/NetworkPoller.h"

#include <QDialog>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>

// Forward declarations
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QSlider;
class QListWidget;
class QRadioButton;
class QButtonGroup;
class QGroupBox;
class QLineEdit;
class QFontComboBox;

namespace nsm {

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(NetworkPoller* poller,
                            QWidget* parent = nullptr);

    void showTab(int index);

signals:
    void settingsApplied(nsm::AppConfig newConfig);

private slots:
    void onApply();
    void onOk();
    void onReset();
    void onAdapterModeChanged(int index);
    void onRefreshAdapters();
    void onOpacitySliderChanged(int value);
    void onFontScaleSliderChanged(int value);
    void onFontSizeChanged(int value);
    void onFontFamilyChanged(const QString& family);
    void onFontBoldToggled(bool checked); // NEW
    void onSpeedUnitChanged(int index);
    void onDecimalPlacesChanged(int index);
    void onShowGraphToggled(bool checked);
    void onIntervalChanged(int value);
    void onStartupToggled(bool checked);
    void onTrayToggled(bool checked);

private:
    QWidget* buildGeneralTab();
    QWidget* buildNetworkTab();
    QWidget* buildAppearanceTab();
    void applyDialogStyle();

    void loadFromConfig(const AppConfig& cfg);
    AppConfig collectToConfig() const;

    void populateAdapterList();
    static QString formatPercent(int value);

    void applyLiveUpdate();

    NetworkPoller* m_poller { nullptr };
    QTabWidget* m_tabs { nullptr };
    
    bool m_isLoading { false }; // FIXED: Loading lock for race condition

    // General tab
    QSpinBox* m_intervalSpin   { nullptr };
    QCheckBox* m_startupCheck   { nullptr };
    QCheckBox* m_trayCheck      { nullptr };

    // Network tab
    QComboBox* m_adapterModeCombo { nullptr };
    QListWidget* m_adapterList      { nullptr };
    QPushButton* m_refreshBtn       { nullptr };
    QLabel* m_adapterHint      { nullptr };

    // Appearance tab
    QSlider* m_opacitySlider    { nullptr };
    QLabel* m_opacityLabel     { nullptr };
    QSlider* m_fontScaleSlider  { nullptr };
    QLabel* m_fontScaleLabel   { nullptr };
    QFontComboBox* m_fontFamilyCombo  { nullptr };
    QSpinBox* m_fontSizeSpin     { nullptr };
    QCheckBox* m_fontBoldCheck    { nullptr }; // NEW
    QCheckBox* m_showGraphCheck   { nullptr };
    QSpinBox* m_graphHistorySpin { nullptr };
    QComboBox* m_speedUnitCombo   { nullptr };
    QComboBox* m_decimalCombo     { nullptr };

    // Button bar
    QPushButton* m_okBtn     { nullptr };
    QPushButton* m_applyBtn  { nullptr };
    QPushButton* m_cancelBtn { nullptr };
    QPushButton* m_resetBtn  { nullptr };
};

} // namespace nsm