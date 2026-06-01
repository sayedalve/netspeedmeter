#include "SettingsDialog.h"
#include "config/ConfigManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QFontComboBox>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QFrame>
#include <QScrollArea>
#include <QSizePolicy>
#include <QFont>
#include <QFontMetrics>
#include <QScreen>
#include <QApplication>
#include <QDebug>

namespace nsm {

// ═════════════════════════════════════════════════════════════════════════════
//  Design tokens
// ═════════════════════════════════════════════════════════════════════════════
namespace Theme {
constexpr const char* BG_WINDOW    = "#1A1A1A";
constexpr const char* BG_CARD      = "#242424";
constexpr const char* BG_INPUT     = "#2E2E2E";
constexpr const char* ACCENT       = "#E53935";
constexpr const char* ACCENT_HOVER = "#EF5350";
constexpr const char* TEXT_PRIMARY = "#F0F0F0";
constexpr const char* TEXT_SECONDARY = "#9E9E9E";
constexpr const char* BORDER       = "#3A3A3A";
constexpr int RADIUS        = 6;
constexpr int CARD_RADIUS   = 8;
constexpr int SPACING       = 10;
constexpr int LABEL_W       = 160;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Global dialog stylesheet
// ═════════════════════════════════════════════════════════════════════════════
static const QString kDialogStyle = QStringLiteral(R"(
QDialog {
    background-color: #1A1A1A;
    color: #F0F0F0;
}
QTabWidget::pane {
    border: 1px solid #3A3A3A;
    border-top: none;
    background-color: #1A1A1A;
    border-radius: 0px 6px 6px 6px;
}
QTabWidget::tab-bar {
    alignment: left;
}
QTabBar::tab {
    background-color: #242424;
    color: #9E9E9E;
    padding: 8px 22px;
    margin-right: 2px;
    border: 1px solid #3A3A3A;
    border-bottom: none;
    border-radius: 6px 6px 0 0;
    font-size: 13px;
}
QTabBar::tab:selected {
    background-color: #2E2E2E;
    color: #F0F0F0;
    border-bottom: 2px solid #E53935;
}
QTabBar::tab:hover:!selected {
    background-color: #2C2C2C;
    color: #D0D0D0;
}
QGroupBox {
    background-color: #242424;
    border: 1px solid #3A3A3A;
    border-radius: 8px;
    margin-top: 14px;
    padding: 8px 12px 12px 12px;
    font-size: 12px;
    font-weight: 600;
    color: #9E9E9E;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 12px;
    top: -1px;
    padding: 0 4px;
    color: #9E9E9E;
    letter-spacing: 0.5px;
    text-transform: uppercase;
}
QLabel {
    color: #F0F0F0;
    background: transparent;
    font-size: 13px;
}
QLabel[secondary="true"] {
    color: #9E9E9E;
    font-size: 11px;
}
QSpinBox, QDoubleSpinBox {
    background-color: #2E2E2E;
    color: #F0F0F0;
    border: 1px solid #3A3A3A;
    border-radius: 6px;
    padding: 4px 8px;
    font-size: 13px;
    min-width: 80px;
}
QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid #E53935;
}
QSpinBox::up-button, QSpinBox::down-button,
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
    background-color: #383838;
    border: none;
    border-radius: 3px;
    width: 16px;
}
QSpinBox::up-button:hover, QSpinBox::down-button:hover,
QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {
    background-color: #484848;
}
QComboBox {
    background-color: #2E2E2E;
    color: #F0F0F0;
    border: 1px solid #3A3A3A;
    border-radius: 6px;
    padding: 5px 10px;
    font-size: 13px;
    min-width: 160px;
}
QComboBox:focus {
    border: 1px solid #E53935;
}
QComboBox::drop-down {
    border: none;
    width: 22px;
}
QComboBox::down-arrow {
    image: none;
    width: 0;
    height: 0;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid #9E9E9E;
    margin-right: 6px;
}
QComboBox QAbstractItemView {
    background-color: #2E2E2E;
    color: #F0F0F0;
    border: 1px solid #3A3A3A;
    selection-background-color: #E53935;
    selection-color: #FFFFFF;
    outline: none;
}
QCheckBox {
    color: #F0F0F0;
    font-size: 13px;
    spacing: 8px;
}
QCheckBox::indicator {
    width: 16px;
    height: 16px;
    border: 2px solid #5A5A5A;
    border-radius: 4px;
    background-color: #2E2E2E;
}
QCheckBox::indicator:checked {
    background-color: #E53935;
    border-color: #E53935;
}
QCheckBox::indicator:hover {
    border-color: #E53935;
}
QSlider::groove:horizontal {
    background-color: #3A3A3A;
    height: 4px;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background-color: #E53935;
    width: 16px;
    height: 16px;
    margin: -6px 0;
    border-radius: 8px;
}
QSlider::handle:horizontal:hover {
    background-color: #EF5350;
}
QSlider::sub-page:horizontal {
    background-color: #E53935;
    border-radius: 2px;
}
QListWidget {
    background-color: #2E2E2E;
    border: 1px solid #3A3A3A;
    border-radius: 6px;
    color: #F0F0F0;
    font-size: 13px;
    outline: none;
}
QListWidget::item {
    padding: 6px 8px;
    border-bottom: 1px solid #383838;
}
QListWidget::item:selected {
    background-color: #3A3A3A;
    color: #F0F0F0;
}
QListWidget::item:hover {
    background-color: #343434;
}
QPushButton {
    background-color: #2E2E2E;
    color: #F0F0F0;
    border: 1px solid #3A3A3A;
    border-radius: 6px;
    padding: 6px 18px;
    font-size: 13px;
}
QPushButton:hover {
    background-color: #383838;
    border-color: #5A5A5A;
}
QPushButton:pressed {
    background-color: #252525;
}
QPushButton[primary="true"] {
    background-color: #E53935;
    color: #FFFFFF;
    border: none;
    font-weight: 600;
}
QPushButton[primary="true"]:hover {
    background-color: #EF5350;
}
QPushButton[primary="true"]:pressed {
    background-color: #C62828;
}
QPushButton[danger="true"] {
    color: #EF9A9A;
    border-color: #5A3A3A;
}
QPushButton[danger="true"]:hover {
    background-color: #3A2020;
    border-color: #E53935;
}
QFontComboBox {
    background-color: #2E2E2E;
    color: #F0F0F0;
    border: 1px solid #3A3A3A;
    border-radius: 6px;
    padding: 4px 8px;
    font-size: 13px;
    min-width: 200px;
}
QFontComboBox:focus {
    border: 1px solid #E53935;
}
QFontComboBox::drop-down {
    border: none;
    width: 22px;
}
QFontComboBox QAbstractItemView {
    background-color: #2E2E2E;
    color: #F0F0F0;
    border: 1px solid #3A3A3A;
    selection-background-color: #E53935;
    selection-color: #FFFFFF;
    outline: none;
}
QScrollBar:vertical {
    background: transparent;
    width: 6px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: #4A4A4A;
    border-radius: 3px;
    min-height: 20px;
}
QScrollBar::handle:vertical:hover {
    background: #606060;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
QFrame[frameShape="4"],
QFrame[frameShape="5"] {
    color: #3A3A3A;
}
)");

// ═════════════════════════════════════════════════════════════════════════════
//  SettingsDialog implementation
// ═════════════════════════════════════════════════════════════════════════════

SettingsDialog::SettingsDialog(NetworkPoller* poller, QWidget* parent)
    : QDialog(parent)
    , m_poller(poller)
{
    setWindowTitle(tr("NetSpeedMeter — Settings"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumSize(520, 480);
    resize(560, 520);
    setModal(true);

    applyDialogStyle();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 12);
    root->setSpacing(12);

    auto* header = new QLabel(tr("Settings"), this);
    QFont hf = header->font();
    hf.setPointSize(15);
    hf.setWeight(QFont::DemiBold);
    header->setFont(hf);
    header->setStyleSheet(QStringLiteral("color: #F0F0F0;"));
    root->addWidget(header);

    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    root->addWidget(sep);

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(buildGeneralTab(),    tr("General"));
    m_tabs->addTab(buildNetworkTab(),    tr("Network"));
    m_tabs->addTab(buildAppearanceTab(), tr("Appearance"));
    root->addWidget(m_tabs, 1);

    auto* btnSep = new QFrame(this);
    btnSep->setFrameShape(QFrame::HLine);
    root->addWidget(btnSep);

    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);

    m_resetBtn  = new QPushButton(tr("Reset Defaults"), this);
    m_resetBtn->setProperty("danger", true);
    m_resetBtn->setToolTip(tr("Restore all settings to their factory defaults"));

    btnRow->addWidget(m_resetBtn);
    btnRow->addStretch();

    m_okBtn     = new QPushButton(tr("OK"),     this);
    m_applyBtn  = new QPushButton(tr("Apply"),  this);
    m_cancelBtn = new QPushButton(tr("Cancel"), this);

    m_okBtn->setProperty("primary", true);
    m_okBtn->setDefault(true);
    m_okBtn->setMinimumWidth(80);
    m_applyBtn->setMinimumWidth(80);
    m_cancelBtn->setMinimumWidth(80);

    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_applyBtn);
    btnRow->addWidget(m_okBtn);
    root->addLayout(btnRow);

    connect(m_okBtn,     &QPushButton::clicked, this, &SettingsDialog::onOk);
    connect(m_applyBtn,  &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_resetBtn,  &QPushButton::clicked, this, &SettingsDialog::onReset);

    loadFromConfig(ConfigManager::instance().config());
}

void SettingsDialog::applyDialogStyle()
{
    setStyleSheet(kDialogStyle);
}

// ── Helper: labelled row ────────────────────────────────────────────────────
static QHBoxLayout* labeledRow(const QString& labelText,
                               QWidget* control,
                               const QString& hint = {})
{
    auto* row = new QHBoxLayout;
    row->setSpacing(12);

    auto* lbl = new QLabel(labelText);
    lbl->setFixedWidth(Theme::LABEL_W);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lbl->setStyleSheet(QStringLiteral("color: #C0C0C0; font-size: 13px;"));
    row->addWidget(lbl);
    row->addWidget(control, 1);

    if (!hint.isEmpty()) {
        auto* hintLbl = new QLabel(hint);
        hintLbl->setProperty("secondary", true);
        hintLbl->setStyleSheet(QStringLiteral("color: #6A6A6A; font-size: 11px;"));
        row->addWidget(hintLbl);
    }

    return row;
}

// ── General tab ─────────────────────────────────────────────────────────────
QWidget* SettingsDialog::buildGeneralTab()
{
    auto* page = new QWidget;
    auto* vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(16, 16, 16, 16);
    vbox->setSpacing(14);

    // Polling group
    auto* pollGroup = new QGroupBox(tr("Update Rate"), page);
    auto* pollVBox  = new QVBoxLayout(pollGroup);
    pollVBox->setSpacing(10);

    m_intervalSpin = new QSpinBox(pollGroup);
    m_intervalSpin->setRange(100, 10000);
    m_intervalSpin->setSingleStep(100);
    m_intervalSpin->setSuffix(tr(" ms"));
    m_intervalSpin->setToolTip(tr("How often the speed is sampled (100–10,000 ms)"));

    pollVBox->addLayout(labeledRow(tr("Poll interval"), m_intervalSpin));

    auto* pollHint = new QLabel(
        tr("Lower values = more responsive display, slightly higher CPU usage."), pollGroup);
    pollHint->setWordWrap(true);
    pollHint->setStyleSheet(QStringLiteral("color: #666666; font-size: 11px; padding-left: %1px;")
                            .arg(Theme::LABEL_W + 12));
    pollVBox->addWidget(pollHint);

    vbox->addWidget(pollGroup);

    // LIVE connection: interval changes apply immediately
    connect(m_intervalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SettingsDialog::onIntervalChanged);

    // Behaviour group
    auto* behavGroup = new QGroupBox(tr("Behaviour"), page);
    auto* behavVBox  = new QVBoxLayout(behavGroup);
    behavVBox->setSpacing(10);

    m_startupCheck = new QCheckBox(tr("Launch NetSpeedMeter when Windows starts"), behavGroup);
    m_trayCheck    = new QCheckBox(tr("Minimise to system tray instead of closing"), behavGroup);

    behavVBox->addWidget(m_startupCheck);
    behavVBox->addWidget(m_trayCheck);

    vbox->addWidget(behavGroup);

    // LIVE connections
    connect(m_startupCheck, &QCheckBox::toggled, this, &SettingsDialog::onStartupToggled);
    connect(m_trayCheck,    &QCheckBox::toggled, this, &SettingsDialog::onTrayToggled);

    // Storage info
    auto* infoGroup = new QGroupBox(tr("Storage"), page);
    auto* infoVBox  = new QVBoxLayout(infoGroup);

    auto* pathLabel = new QLabel(
        tr("Config file: <span style='color:#6A6A6A; font-family:Consolas,monospace;'>%1</span>")
        .arg(ConfigManager::instance().configFilePath()), infoGroup);
    pathLabel->setWordWrap(true);
    pathLabel->setTextFormat(Qt::RichText);
    infoVBox->addWidget(pathLabel);

    vbox->addWidget(infoGroup);
    vbox->addStretch();

    return page;
}

// ── Network tab ───────────────────────────────────────────────────────────────
QWidget* SettingsDialog::buildNetworkTab()
{
    auto* page = new QWidget;
    auto* vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(16, 16, 16, 16);
    vbox->setSpacing(14);

    auto* modeGroup = new QGroupBox(tr("Adapter Selection Mode"), page);
    auto* modeVBox  = new QVBoxLayout(modeGroup);
    modeVBox->setSpacing(10);

    m_adapterModeCombo = new QComboBox(modeGroup);
    m_adapterModeCombo->addItem(tr("Auto — Primary adapter (recommended)"),
                                 QVariant::fromValue(int(NetworkPoller::AdapterMode::AutoPrimary)));
    m_adapterModeCombo->addItem(tr("All — Sum all physical adapters"),
                                 QVariant::fromValue(int(NetworkPoller::AdapterMode::AllAdapters)));
    m_adapterModeCombo->addItem(tr("Specific — Choose below"),
                                 QVariant::fromValue(int(NetworkPoller::AdapterMode::Specific)));

    modeVBox->addLayout(labeledRow(tr("Mode"), m_adapterModeCombo));

    m_adapterHint = new QLabel(
        tr("Auto mode ignores virtual, VPN, and Bluetooth adapters automatically."),
        modeGroup);
    m_adapterHint->setWordWrap(true);
    m_adapterHint->setStyleSheet(QStringLiteral("color: #666666; font-size: 11px;"));
    modeVBox->addWidget(m_adapterHint);

    vbox->addWidget(modeGroup);

    auto* listGroup = new QGroupBox(tr("Available Adapters"), page);
    auto* listVBox  = new QVBoxLayout(listGroup);
    listVBox->setSpacing(8);

    auto* listTopRow = new QHBoxLayout;
    auto* listDesc = new QLabel(
        tr("Check adapters to monitor (only used in Specific mode):"), listGroup);
    listDesc->setStyleSheet(QStringLiteral("color: #9E9E9E; font-size: 11px;"));
    listTopRow->addWidget(listDesc, 1);

    m_refreshBtn = new QPushButton(tr("⟳  Refresh"), listGroup);
    m_refreshBtn->setFixedHeight(26);
    m_refreshBtn->setToolTip(tr("Re-scan for network adapters"));
    listTopRow->addWidget(m_refreshBtn);
    listVBox->addLayout(listTopRow);

    m_adapterList = new QListWidget(listGroup);
    m_adapterList->setMinimumHeight(160);
    m_adapterList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    listVBox->addWidget(m_adapterList, 1);

    vbox->addWidget(listGroup, 1);

    connect(m_adapterModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onAdapterModeChanged);
    connect(m_refreshBtn, &QPushButton::clicked,
            this, &SettingsDialog::onRefreshAdapters);

    populateAdapterList();

    return page;
}

// ── Appearance tab ────────────────────────────────────────────────────────────
QWidget* SettingsDialog::buildAppearanceTab()
{
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; }"));

    auto* inner = new QWidget;
    scroll->setWidget(inner);

    auto* vbox = new QVBoxLayout(inner);
    vbox->setContentsMargins(16, 16, 16, 16);
    vbox->setSpacing(14);

    // ── Opacity ───────────────────────────────────────────────────────────────
    auto* opacGroup = new QGroupBox(tr("Transparency"), inner);
    auto* opacVBox  = new QVBoxLayout(opacGroup);
    opacVBox->setSpacing(10);

    auto* opacRow = new QHBoxLayout;
    m_opacitySlider = new QSlider(Qt::Horizontal, opacGroup);
    m_opacitySlider->setRange(5, 100);
    m_opacitySlider->setSingleStep(1);
    m_opacitySlider->setTickInterval(10);
    m_opacitySlider->setMinimumWidth(200);

    m_opacityLabel = new QLabel(opacGroup);
    m_opacityLabel->setFixedWidth(44);
    m_opacityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto* opacLbl = new QLabel(tr("Widget opacity"), opacGroup);
    opacLbl->setFixedWidth(Theme::LABEL_W);
    opacLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    opacLbl->setStyleSheet(QStringLiteral("color: #C0C0C0; font-size: 13px;"));

    opacRow->addWidget(opacLbl);
    opacRow->addWidget(m_opacitySlider, 1);
    opacRow->addWidget(m_opacityLabel);
    opacVBox->addLayout(opacRow);

    vbox->addWidget(opacGroup);

    connect(m_opacitySlider, &QSlider::valueChanged,
            this, &SettingsDialog::onOpacitySliderChanged);

    // ── Font ──────────────────────────────────────────────────────────────────
    auto* fontGroup = new QGroupBox(tr("Typography"), inner);
    auto* fontVBox  = new QVBoxLayout(fontGroup);
    fontVBox->setSpacing(10);

    // Font family
    auto* familyRow = new QHBoxLayout;
    m_fontFamilyCombo = new QFontComboBox(fontGroup);
    m_fontFamilyCombo->setFontFilters(QFontComboBox::ScalableFonts);
    m_fontFamilyCombo->setMinimumWidth(200);

    auto* familyLbl = new QLabel(tr("Font family"), fontGroup);
    familyLbl->setFixedWidth(Theme::LABEL_W);
    familyLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    familyLbl->setStyleSheet(QStringLiteral("color: #C0C0C0; font-size: 13px;"));

    familyRow->addWidget(familyLbl);
    familyRow->addWidget(m_fontFamilyCombo, 1);
    fontVBox->addLayout(familyRow);

    connect(m_fontFamilyCombo, &QFontComboBox::currentTextChanged,
            this, &SettingsDialog::onFontFamilyChanged);

    // Font size
    auto* sizeRow = new QHBoxLayout;
    m_fontSizeSpin = new QSpinBox(fontGroup);
    m_fontSizeSpin->setRange(6, 24);
    m_fontSizeSpin->setSuffix(tr(" pt"));

    auto* sizeLbl = new QLabel(tr("Font size"), fontGroup);
    sizeLbl->setFixedWidth(Theme::LABEL_W);
    sizeLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sizeLbl->setStyleSheet(QStringLiteral("color: #C0C0C0; font-size: 13px;"));

    sizeRow->addWidget(sizeLbl);
    sizeRow->addWidget(m_fontSizeSpin, 1);
    sizeRow->addStretch();
    fontVBox->addLayout(sizeRow);

    connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SettingsDialog::onFontSizeChanged);

    // Font scale (legacy, kept for compatibility)
    auto* scaleRow = new QHBoxLayout;
    m_fontScaleSlider = new QSlider(Qt::Horizontal, fontGroup);
    m_fontScaleSlider->setRange(50, 300);
    m_fontScaleSlider->setSingleStep(5);
    m_fontScaleSlider->setMinimumWidth(200);

    m_fontScaleLabel = new QLabel(fontGroup);
    m_fontScaleLabel->setFixedWidth(44);
    m_fontScaleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto* scaleLbl = new QLabel(tr("Scale factor"), fontGroup);
    scaleLbl->setFixedWidth(Theme::LABEL_W);
    scaleLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    scaleLbl->setStyleSheet(QStringLiteral("color: #C0C0C0; font-size: 13px;"));

    scaleRow->addWidget(scaleLbl);
    scaleRow->addWidget(m_fontScaleSlider, 1);
    scaleRow->addWidget(m_fontScaleLabel);
    fontVBox->addLayout(scaleRow);

    vbox->addWidget(fontGroup);

    connect(m_fontScaleSlider, &QSlider::valueChanged,
            this, &SettingsDialog::onFontScaleSliderChanged);

    // ── Speed Display ─────────────────────────────────────────────────────────
    auto* displayGroup = new QGroupBox(tr("Speed Display"), inner);
    auto* displayVBox  = new QVBoxLayout(displayGroup);
    displayVBox->setSpacing(10);

    m_speedUnitCombo = new QComboBox(displayGroup);
    m_speedUnitCombo->addItem(tr("Auto  (KB/s, MB/s, GB/s)"),  QStringLiteral("auto"));
    m_speedUnitCombo->addItem(tr("Always KB/s"),               QStringLiteral("kbps"));
    m_speedUnitCombo->addItem(tr("Always MB/s"),               QStringLiteral("mbps"));
    m_speedUnitCombo->addItem(tr("Bits/sec  (Kbps, Mbps, Gbps)"), QStringLiteral("bits"));
    displayVBox->addLayout(labeledRow(tr("Unit mode"), m_speedUnitCombo));

    m_decimalCombo = new QComboBox(displayGroup);
    m_decimalCombo->addItem(tr("0 decimal places"),  0);
    m_decimalCombo->addItem(tr("1 decimal place"),   1);
    m_decimalCombo->addItem(tr("2 decimal places"),  2);
    displayVBox->addLayout(labeledRow(tr("Precision"), m_decimalCombo));

    vbox->addWidget(displayGroup);

    // LIVE connections
    connect(m_speedUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onSpeedUnitChanged);
    connect(m_decimalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onDecimalPlacesChanged);

    vbox->addStretch();

    return scroll;
}

// ── Adapter list population ─────────────────────────────────────────────────
void SettingsDialog::populateAdapterList()
{
    if (!m_adapterList)
        return;

    m_adapterList->clear();

    const QStringList currentGuids = ConfigManager::instance().config().selectedAdapterGuids;
    const QVector<AdapterInfo> adapters = NetworkPoller::enumerateAdapters();

    if (adapters.isEmpty()) {
        auto* dummy = new QListWidgetItem(tr("(No adapters found — check permissions)"),
                                          m_adapterList);
        dummy->setFlags(Qt::NoItemFlags);
        dummy->setForeground(QColor(QStringLiteral("#666666")));
        return;
    }

    for (const AdapterInfo& a : adapters) {
        auto* item = new QListWidgetItem(a.name, m_adapterList);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(currentGuids.contains(a.guid)
                            ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, a.guid);
        item->setToolTip(a.guid);
    }

    onAdapterModeChanged(m_adapterModeCombo ? m_adapterModeCombo->currentIndex() : 0);
}

// ── Slots ───────────────────────────────────────────────────────────────────

void SettingsDialog::onAdapterModeChanged(int /*index*/)
{
    if (!m_adapterModeCombo || !m_adapterList)
        return;

    const auto mode = static_cast<NetworkPoller::AdapterMode>(
        m_adapterModeCombo->currentData().toInt());

    const bool specific = (mode == NetworkPoller::AdapterMode::Specific);
    m_adapterList->setEnabled(specific);
    m_refreshBtn->setEnabled(true);

    if (m_adapterHint) {
        if (mode == NetworkPoller::AdapterMode::AutoPrimary)
            m_adapterHint->setText(tr(
                "Auto mode picks the most-active physical adapter, ignoring VPN "
                "and virtual interfaces."));
        else if (mode == NetworkPoller::AdapterMode::AllAdapters)
            m_adapterHint->setText(tr(
                "All-adapters mode sums traffic across every physical NIC."));
        else
            m_adapterHint->setText(tr(
                "Check the adapters you want to monitor. "
                "Traffic is summed across all checked adapters."));
    }

    applyLiveUpdate();
}

void SettingsDialog::onRefreshAdapters()
{
    populateAdapterList();
}

void SettingsDialog::onOpacitySliderChanged(int value)
{
    if (m_opacityLabel)
        m_opacityLabel->setText(QStringLiteral("%1%").arg(value));
    applyLiveUpdate();
}

void SettingsDialog::onFontScaleSliderChanged(int value)
{
    if (m_fontScaleLabel)
        m_fontScaleLabel->setText(QStringLiteral("%1×").arg(
            QString::number(value / 100.0, 'f', 2)));
    applyLiveUpdate();
}

void SettingsDialog::onFontSizeChanged(int /*value*/)
{
    applyLiveUpdate();
}

void SettingsDialog::onFontFamilyChanged(const QString& /*family*/)
{
    applyLiveUpdate();
}

void SettingsDialog::onSpeedUnitChanged(int /*index*/)
{
    applyLiveUpdate();
}

void SettingsDialog::onDecimalPlacesChanged(int /*index*/)
{
    applyLiveUpdate();
}

void SettingsDialog::onShowGraphToggled(bool /*checked*/)
{
    applyLiveUpdate();
}

void SettingsDialog::onIntervalChanged(int /*value*/)
{
    applyLiveUpdate();
}

void SettingsDialog::onStartupToggled(bool /*checked*/)
{
    applyLiveUpdate();
}

void SettingsDialog::onTrayToggled(bool /*checked*/)
{
    applyLiveUpdate();
}

// ── Live update helper ────────────────────────────────────────────────────────
void SettingsDialog::applyLiveUpdate()
{
    const AppConfig newCfg = collectToConfig();
    ConfigManager::instance().setConfig(newCfg);
    ConfigManager::instance().save();
    emit settingsApplied(newCfg);
}

// ── Load / Save config ──────────────────────────────────────────────────────
void SettingsDialog::loadFromConfig(const AppConfig& cfg)
{
    if (m_intervalSpin)  m_intervalSpin->setValue(cfg.updateIntervalMs);
    if (m_startupCheck)  m_startupCheck->setChecked(cfg.startWithWindows);
    if (m_trayCheck)     m_trayCheck->setChecked(cfg.minimiseToTray);

    if (m_adapterModeCombo) {
        const int targetData = static_cast<int>(cfg.adapterMode);
        for (int i = 0; i < m_adapterModeCombo->count(); ++i) {
            if (m_adapterModeCombo->itemData(i).toInt() == targetData) {
                m_adapterModeCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    if (m_opacitySlider) {
        const int pct = qRound(cfg.opacity * 100.0);
        m_opacitySlider->setValue(qBound(5, pct, 100));
        onOpacitySliderChanged(m_opacitySlider->value());
    }
    if (m_fontScaleSlider) {
        const int scaled = qRound(cfg.fontScale * 100.0);
        m_fontScaleSlider->setValue(qBound(50, scaled, 300));
        onFontScaleSliderChanged(m_fontScaleSlider->value());
    }
    if (m_fontFamilyCombo) {
        m_fontFamilyCombo->setCurrentFont(QFont(cfg.fontFamily));
    }
    if (m_fontSizeSpin) {
        m_fontSizeSpin->setValue(qBound(6, cfg.fontSize, 24));
    }
    if (m_showGraphCheck)     m_showGraphCheck->setChecked(cfg.showGraph);
    if (m_graphHistorySpin)   m_graphHistorySpin->setValue(cfg.graphHistorySize);

    if (m_speedUnitCombo) {
        const int idx = m_speedUnitCombo->findData(cfg.speedUnit);
        m_speedUnitCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    if (m_decimalCombo) {
        const int idx = m_decimalCombo->findData(cfg.decimalPlaces);
        m_decimalCombo->setCurrentIndex(idx >= 0 ? idx : 1);
    }
}

AppConfig SettingsDialog::collectToConfig() const
{
    AppConfig cfg = ConfigManager::instance().config();

    if (m_intervalSpin)  cfg.updateIntervalMs = m_intervalSpin->value();
    if (m_startupCheck)  cfg.startWithWindows = m_startupCheck->isChecked();
    if (m_trayCheck)     cfg.minimiseToTray   = m_trayCheck->isChecked();

    if (m_adapterModeCombo) {
        cfg.adapterMode = static_cast<NetworkPoller::AdapterMode>(
            m_adapterModeCombo->currentData().toInt());
    }
    if (m_adapterList) {
        cfg.selectedAdapterGuids.clear();
        for (int i = 0; i < m_adapterList->count(); ++i) {
            const QListWidgetItem* item = m_adapterList->item(i);
            if (item && item->checkState() == Qt::Checked) {
                const QString guid = item->data(Qt::UserRole).toString();
                if (!guid.isEmpty())
                    cfg.selectedAdapterGuids.append(guid);
            }
        }
    }

    if (m_opacitySlider)    cfg.opacity    = m_opacitySlider->value() / 100.0;
    if (m_fontScaleSlider)  cfg.fontScale  = m_fontScaleSlider->value() / 100.0;
    if (m_fontFamilyCombo)  cfg.fontFamily = m_fontFamilyCombo->currentFont().family();
    if (m_fontSizeSpin)     cfg.fontSize   = m_fontSizeSpin->value();
    if (m_showGraphCheck)   cfg.showGraph  = m_showGraphCheck->isChecked();
    if (m_graphHistorySpin) cfg.graphHistorySize = m_graphHistorySpin->value();
    if (m_speedUnitCombo)   cfg.speedUnit  = m_speedUnitCombo->currentData().toString();
    if (m_decimalCombo)     cfg.decimalPlaces = m_decimalCombo->currentData().toInt();

    return cfg;
}

// ── Button handlers ─────────────────────────────────────────────────────────
void SettingsDialog::onApply()
{
    applyLiveUpdate();
}

void SettingsDialog::onOk()
{
    applyLiveUpdate();
    accept();
}

void SettingsDialog::onReset()
{
    const auto answer = QMessageBox::question(
        this,
        tr("Reset to Defaults"),
        tr("This will restore all settings to their factory defaults.\n\n"
           "Your current settings will be lost. Continue?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (answer != QMessageBox::Yes)
        return;

    loadFromConfig(AppConfig{});
    applyLiveUpdate();
}

// ── showTab ─────────────────────────────────────────────────────────────────
void SettingsDialog::showTab(int index)
{
    if (m_tabs && index >= 0 && index < m_tabs->count())
        m_tabs->setCurrentIndex(index);
}

// ── Helper ───────────────────────────────────────────────────────────────────
QString SettingsDialog::formatPercent(int value)
{
    return QStringLiteral("%1%").arg(value);
}

} // namespace nsm
