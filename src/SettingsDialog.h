#pragma once

#include "config/AppConfig.h"
#include "core/NetworkPoller.h"

#include <QDialog>
#include <QTabWidget>
#include <QLabel>

// ── Forward declarations (avoid full includes in header) ──────────────────────
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QSlider;
class QListWidget;
class QPushButton;
class QRadioButton;
class QButtonGroup;
class QGroupBox;
class QLineEdit;
class QColor;

namespace nsm {

// ─────────────────────────────────────────────────────────────────────────────
//  ColorButton — a QPushButton that displays and lets the user pick a QColor
// ─────────────────────────────────────────────────────────────────────────────
/**
 * @brief A button whose face is painted with the chosen colour.
 *
 * Clicking it opens QColorDialog.  The chosen colour is stored internally
 * and can be read back as a "#RRGGBB" hex string.
 */
class ColorButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ColorButton(const QString& initialHex = QStringLiteral("#000000"),
                         QWidget* parent = nullptr);

    /** @brief Return the current colour as "#RRGGBB" hex string. */
    QString colorHex() const;

    /** @brief Set the button colour from a hex string without opening the picker. */
    void setColorHex(const QString& hex);

signals:
    void colorChanged(const QString& hex);

private slots:
    void onClicked();

private:
    void applyColor();
    QColor m_color;
};

// ─────────────────────────────────────────────────────────────────────────────
//  SettingsDialog — three-tab settings dialog
// ─────────────────────────────────────────────────────────────────────────────
/**
 * @brief Modal tabbed dialog for all user-configurable options.
 *
 * Tabs:
 *  • General    — update rate, startup, tray behaviour
 *  • Network    — adapter mode, adapter list with checkboxes
 *  • Appearance — opacity, font scale, graph toggle, colours, units
 *
 * Lifecycle:
 *  1. Caller creates the dialog passing a pointer to the live NetworkPoller
 *     (needed to enumerate adapters on the Network tab).
 *  2. Dialog calls loadFromConfig() on construction to populate all widgets.
 *  3. On "Apply" / "OK", saveToConfig() writes to ConfigManager and calls
 *     ConfigManager::save().
 *  4. "Cancel" discards all changes.
 *  5. "Reset to Defaults" restores the default AppConfig.
 *
 * No raw new:  All child widgets use Qt parent ownership.
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param poller  Live NetworkPoller — used to call enumerateAdapters().
     *                Must outlive the dialog.
     * @param parent  Qt parent widget.
     */
    explicit SettingsDialog(NetworkPoller* poller,
                            QWidget* parent = nullptr);

    /** @brief Open the dialog at a specific tab (0=General,1=Network,2=Appearance). */
    void showTab(int index);

signals:
    /**
     * @brief Emitted immediately when the user clicks Apply or OK, after
     *        ConfigManager has been updated.  Lets the overlay widget
     *        (Batch 4) live-update without restarting the app.
     */
    void settingsApplied(nsm::AppConfig newConfig);

private slots:
    void onApply();
    void onOk();
    void onReset();
    void onAdapterModeChanged(int index);
    void onRefreshAdapters();
    void onOpacitySliderChanged(int value);
    void onFontScaleSliderChanged(int value);

private:
    // ── Build helpers ─────────────────────────────────────────────────────────
    QWidget* buildGeneralTab();
    QWidget* buildNetworkTab();
    QWidget* buildAppearanceTab();

    void applyDialogStyle();

    // ── Data flow ─────────────────────────────────────────────────────────────
    void loadFromConfig(const AppConfig& cfg);
    AppConfig collectToConfig() const;

    // ── Utility ───────────────────────────────────────────────────────────────
    void populateAdapterList();
    static QString formatPercent(int value);  // slider → "88%"

    // ── Core references ───────────────────────────────────────────────────────
    NetworkPoller* m_poller { nullptr };

    // ── Tab container ─────────────────────────────────────────────────────────
    QTabWidget* m_tabs { nullptr };

    // ══════════════════════════════════════════════════════════════════════════
    //  General tab widgets
    // ══════════════════════════════════════════════════════════════════════════
    QSpinBox*   m_intervalSpin   { nullptr };   // ms
    QCheckBox*  m_startupCheck   { nullptr };
    QCheckBox*  m_trayCheck      { nullptr };

    // ══════════════════════════════════════════════════════════════════════════
    //  Network tab widgets
    // ══════════════════════════════════════════════════════════════════════════
    QComboBox*   m_adapterModeCombo { nullptr };
    QListWidget* m_adapterList      { nullptr };   // checkable items
    QPushButton* m_refreshBtn       { nullptr };
    QLabel*      m_adapterHint      { nullptr };

    // ══════════════════════════════════════════════════════════════════════════
    //  Appearance tab widgets
    // ══════════════════════════════════════════════════════════════════════════
    QSlider*       m_opacitySlider    { nullptr };
    QLabel*        m_opacityLabel     { nullptr };   // "88%"
    QSlider*       m_fontScaleSlider  { nullptr };
    QLabel*        m_fontScaleLabel   { nullptr };   // "1.00×"
    QCheckBox*     m_showGraphCheck   { nullptr };
    QSpinBox*      m_graphHistorySpin { nullptr };
    QComboBox*     m_speedUnitCombo   { nullptr };
    QComboBox*     m_decimalCombo     { nullptr };
    ColorButton*   m_accentColorBtn   { nullptr };
    ColorButton*   m_bgColorBtn       { nullptr };
    ColorButton*   m_textColorBtn     { nullptr };

    // ── Button bar ────────────────────────────────────────────────────────────
    QPushButton* m_okBtn     { nullptr };
    QPushButton* m_applyBtn  { nullptr };
    QPushButton* m_cancelBtn { nullptr };
    QPushButton* m_resetBtn  { nullptr };
};

} // namespace nsm
