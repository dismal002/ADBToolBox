#include "mainwindow.h"
#include "appsettings.h"
#include "adbrunner.h"
#include "theme/materialtheme.h"
#include "theme/iconhelper.h"
#include "dialogs/touchsimudialog.h"
#include "dialogs/keysimudialog.h"
#include "dialogs/softsimudialog.h"
#include "dialogs/sysctldialog.h"
#include "dialogs/runcontroldialog.h"
#include "dialogs/newscriptdialog.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QScrollArea>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QTextStream>
#include <QListWidgetItem>
#include <QStyleFactory>
#include <QScreen>
#include <QCloseEvent>
#include <QEvent>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QMetaObject>
#include <QButtonGroup>
#include <QFrame>
#include <QFont>
#include <QFontMetrics>
#include <QDesktopServices>
#include <QUrl>

namespace {
QString shellExecutable()
{
#ifdef Q_OS_WIN
    return QStringLiteral("cmd.exe");
#else
    return QStringLiteral("bash");
#endif
}

QStringList shellCommandArgs(const QString &cmd)
{
#ifdef Q_OS_WIN
    return {QStringLiteral("/C"), cmd};
#else
    return {QStringLiteral("-c"), cmd};
#endif
}

QStringList shellScriptArgs(const QString &scriptPath)
{
#ifdef Q_OS_WIN
    return {QStringLiteral("/C"), QStringLiteral("\"") + scriptPath + QStringLiteral("\"")};
#else
    return {scriptPath};
#endif
}

QString scriptFileExtension()
{
#ifdef Q_OS_WIN
    return QStringLiteral(".cmd");
#else
    return QStringLiteral(".sh");
#endif
}

QString scriptDelayCommand()
{
#ifdef Q_OS_WIN
    return QStringLiteral("timeout /t 1 >nul\r\n\r\n");
#else
    return QStringLiteral("sleep 1\n\n");
#endif
}
}

// ════════════════════════════════════════════════════════════════════════════
//  Constructor / Destructor
// ════════════════════════════════════════════════════════════════════════════

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Check adb
    if (!AdbRunner::isAdbAvailable()) {
        QMessageBox::warning(this, tr("No ADB"),
            tr("ADB not found in PATH. Please install Android SDK platform tools."));
    }

    // Ensure Script directory exists
    QDir().mkpath(QCoreApplication::applicationDirPath() + "/Script");

    // Load language translator from settings
    QString lang = AppSettings::instance()->language();
    if (lang.isEmpty()) lang = QStringLiteral("en");
    if (lang != QLatin1String("en")) {
        QString qm = QStringLiteral("toolbox_") + lang + QStringLiteral(".qm");
        if (m_translator.load(QStringLiteral(":/translations/") + qm) ||
            m_translator.load(qm)) {
            qApp->installTranslator(&m_translator);
        }
    }

    setupUi();
    retranslateUi();

    // Restore start page
    setCurrentTab(AppSettings::instance()->startPage());

    // Start CPU timer
    m_cpuTimer = new QTimer(this);
    m_cpuTimer->setInterval(AppSettings::instance()->cpuRefreshMs());
    connect(m_cpuTimer, &QTimer::timeout, this, &MainWindow::onCpuTimerTick);
    m_cpuTimer->start();

    // Battery monitor timer
    m_batMonTimer = new QTimer(this);
    m_batMonTimer->setInterval(2000);
    connect(m_batMonTimer, &QTimer::timeout, this, &MainWindow::onBatteryMonitorTick);

    // Auto fill timer (polls foreground app every 1s when tab 1 active)
    m_autoFillTimer = new QTimer(this);
    m_autoFillTimer->setInterval(1000);
    connect(m_autoFillTimer, &QTimer::timeout, this, &MainWindow::onReadForegroundApp);

    // Initial device list
    onRefreshDevices();
}

MainWindow::~MainWindow()
{
    AppSettings::instance()->save();
}

// ════════════════════════════════════════════════════════════════════════════
//  UI Setup
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::setupUi()
{
    resize(1060, 680);
    setMinimumSize(880, 580);

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Top Header Bar
    setupHeaderBar();
    mainLayout->addWidget(m_headerBar);

    // Body with Left Sidebar + Stacked Widget
    auto *bodyWidget = new QWidget(central);
    auto *bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    setupSidebar();
    bodyLayout->addWidget(m_sidebarContainer);

    m_pageStack = new QStackedWidget(bodyWidget);
    bodyLayout->addWidget(m_pageStack, 1);

    mainLayout->addWidget(bodyWidget, 1);

    // Setup pages
    setupTab0_DeviceInfo();
    setupTab1_AppManagement();
    setupTab2_Display();
    setupTab3_BatteryControl();
    setupTab4_BatteryMonitor();
    setupTab5_UiTweaks();
    setupTab6_ScriptEditor();
    setupTab7_Terminal();
    setupTab8_Settings();
    setupTab9_About();

    // Stack switch handler
    connect(m_pageStack, &QStackedWidget::currentChanged, this, [this](int idx) {
        if (m_sidebarList && m_sidebarList->currentRow() != idx) {
            m_sidebarList->setCurrentRow(idx);
        }
        m_batMonTimer->stop();
        m_autoFillTimer->stop();
        switch (idx) {
        case 0: onRefreshDeviceInfo(); break;
        case 1:
            onRefreshPackages();
            if (m_autoFillChk->isChecked()) m_autoFillTimer->start();
            break;
        case 2: onRefreshDisplay(); break;
        case 4: m_batMonTimer->start(); break;
        case 5: onRefreshUiTweaks(); break;
        case 6: refreshScriptList(); break;
        default: break;
        }
    });

    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::setupHeaderBar()
{
    m_headerBar = new QWidget(this);
    m_headerBar->setObjectName(QStringLiteral("headerBar"));
    auto *lay = new QHBoxLayout(m_headerBar);
    lay->setContentsMargins(18, 8, 18, 8);
    lay->setSpacing(12);

    m_headerLogo = new QLabel(m_headerBar);
    m_headerLogo->setPixmap(IconHelper::createPixmap(IconHelper::Apps, Qt::white, 26));

    auto *titleLay = new QVBoxLayout;
    titleLay->setSpacing(1);

    m_headerTitle = new QLabel(tr("Android ADB Toolbox"), m_headerBar);
    m_headerTitle->setObjectName(QStringLiteral("headerTitle"));
    QFont tf = m_headerTitle->font();
    tf.setPointSize(11);
    tf.setBold(true);
    m_headerTitle->setFont(tf);

    m_headerSubtitle = new QLabel(tr("Universal Android Modding & ADB Toolkit"), m_headerBar);
    m_headerSubtitle->setObjectName(QStringLiteral("headerSubtitle"));
    QFont sf = m_headerSubtitle->font();
    sf.setPointSize(9);
    m_headerSubtitle->setFont(sf);

    titleLay->addWidget(m_headerTitle);
    titleLay->addWidget(m_headerSubtitle);

    lay->addWidget(m_headerLogo);
    lay->addLayout(titleLay);
    lay->addStretch();

    // Device connection info
    m_deviceStatusDot = new QLabel(m_headerBar);
    m_deviceStatusDot->setFixedSize(10, 10);
    m_deviceStatusDot->setStyleSheet(QStringLiteral("background-color: #9E9E9E; border-radius: 5px;"));

    m_deviceLabel = new QLabel(tr("Device:"), m_headerBar);
    QFont df = m_deviceLabel->font();
    df.setBold(true);
    m_deviceLabel->setFont(df);

    m_deviceCombo = new QComboBox(m_headerBar);
    m_deviceCombo->setMinimumWidth(220);

    m_refreshDevBtn = new QPushButton(tr("Refresh"), m_headerBar);
    m_refreshDevBtn->setIcon(IconHelper::createIcon(IconHelper::Refresh, Qt::white));

    m_lanDebugBtn = new QPushButton(tr("LAN Debug"), m_headerBar);
    m_lanDebugBtn->setIcon(IconHelper::createIcon(IconHelper::Network, Qt::white));

    lay->addWidget(m_deviceStatusDot);
    lay->addWidget(m_deviceLabel);
    lay->addWidget(m_deviceCombo);
    lay->addWidget(m_refreshDevBtn);
    lay->addWidget(m_lanDebugBtn);

    connect(m_refreshDevBtn, &QPushButton::clicked, this, &MainWindow::onRefreshDevices);
    connect(m_lanDebugBtn,   &QPushButton::clicked, this, &MainWindow::onLanDebugClicked);
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onDeviceChanged);
}

void MainWindow::setupSidebar()
{
    m_sidebarContainer = new QWidget(this);
    m_sidebarContainer->setObjectName(QStringLiteral("sidebarContainer"));
    m_sidebarContainer->setFixedWidth(215);

    auto *lay = new QVBoxLayout(m_sidebarContainer);
    lay->setContentsMargins(6, 12, 6, 12);
    lay->setSpacing(6);

    m_sidebarList = new QListWidget(m_sidebarContainer);
    m_sidebarList->setObjectName(QStringLiteral("sidebarList"));
    m_sidebarList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_sidebarList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    const QStringList titles = {
        tr("Device Info"),
        tr("App Management"),
        tr("Display"),
        tr("Battery Control"),
        tr("Battery Monitor"),
        tr("UI Tweaks"),
        tr("Script Editor"),
        tr("ADB Terminal"),
        tr("Settings"),
        tr("About")
    };

    for (const QString &title : titles) {
        auto *item = new QListWidgetItem(title, m_sidebarList);
        item->setSizeHint(QSize(175, 44));
        m_sidebarList->addItem(item);
    }

    lay->addWidget(m_sidebarList, 1);

    m_sidebarVersion = new QLabel(QStringLiteral("Android ADB Toolbox · v%1").arg(QCoreApplication::applicationVersion()), m_sidebarContainer);
    m_sidebarVersion->setAlignment(Qt::AlignCenter);
    m_sidebarVersion->setStyleSheet(QStringLiteral("color: #94A3B8; font-size: 11px; padding: 6px;"));
    lay->addWidget(m_sidebarVersion);

    connect(m_sidebarList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0 && row < m_pageStack->count()) {
            m_pageStack->setCurrentIndex(row);
        }
    });

    updateSidebarIcons();
}

// ── Tab 0: Device Info ───────────────────────────────────────────────────────

void MainWindow::setupTab0_DeviceInfo()
{
    m_tab0 = new QWidget;
    auto *tabLay = new QVBoxLayout(m_tab0);
    tabLay->setContentsMargins(18, 16, 18, 16);

    auto *scroll = new QScrollArea(m_tab0);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *inner = new QWidget(scroll);
    auto *lay = new QVBoxLayout(inner);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(16);

    // Top Banner Card
    m_topCard = new QGroupBox(tr("Device Overview"), inner);
    auto *topCardLay = new QHBoxLayout(m_topCard);
    topCardLay->setContentsMargins(16, 12, 16, 12);

    m_devStatusBadge = new QLabel(QStringLiteral("⚪ ") + tr("No device connected. Please connect via USB or LAN."), m_topCard);
    QFont bf = m_devStatusBadge->font();
    bf.setBold(true);
    bf.setPointSize(11);
    m_devStatusBadge->setFont(bf);

    m_refreshInfoBtn = new QPushButton(tr("Refresh Info"), m_topCard);
    m_refreshInfoBtn->setIcon(IconHelper::createIcon(IconHelper::Refresh, Qt::white));

    topCardLay->addWidget(m_devStatusBadge, 1);
    topCardLay->addWidget(m_refreshInfoBtn);
    lay->addWidget(m_topCard);

    // Grid of Information Cards
    auto *grid = new QGridLayout;
    grid->setSpacing(16);

    // Card 1: Hardware & System
    m_hwGrp = new QGroupBox(tr("Hardware and System"), inner);
    auto *hwForm = new QFormLayout(m_hwGrp);
    hwForm->setSpacing(10);
    m_brandKeyLabel = new QLabel(tr("Brand:"), m_hwGrp);
    m_brandLabel = new QLabel(tr("N/A"), m_hwGrp);
    m_brandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    hwForm->addRow(m_brandKeyLabel, m_brandLabel);

    m_modelKeyLabel = new QLabel(tr("Model:"), m_hwGrp);
    m_modelLabel = new QLabel(tr("N/A"), m_hwGrp);
    m_modelLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    hwForm->addRow(m_modelKeyLabel, m_modelLabel);

    m_codeKeyLabel = new QLabel(tr("Codename:"), m_hwGrp);
    m_codeLabel = new QLabel(tr("N/A"), m_hwGrp);
    m_codeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    hwForm->addRow(m_codeKeyLabel, m_codeLabel);

    m_sysKeyLabel = new QLabel(tr("System:"), m_hwGrp);
    m_sysLabel = new QLabel(tr("N/A"), m_hwGrp);
    m_sysLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    hwForm->addRow(m_sysKeyLabel, m_sysLabel);

    m_boardKeyLabel = new QLabel(tr("Board ID:"), m_hwGrp);
    m_boardLabel = new QLabel(tr("N/A"), m_hwGrp);
    m_boardLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    hwForm->addRow(m_boardKeyLabel, m_boardLabel);

    grid->addWidget(m_hwGrp, 0, 0);

    // Card 2: Processor & Platform
    m_procGrp = new QGroupBox(tr("Processor and Platform"), inner);
    auto *procForm = new QFormLayout(m_procGrp);
    procForm->setSpacing(10);
    m_cpuPlatKeyLabel = new QLabel(tr("CPU Platform:"), m_procGrp);
    m_cpuPlatLabel = new QLabel(tr("N/A"), m_procGrp);
    m_cpuPlatLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    procForm->addRow(m_cpuPlatKeyLabel, m_cpuPlatLabel);

    m_cpuArchKeyLabel = new QLabel(tr("CPU Arch:"), m_procGrp);
    m_cpuArchLabel = new QLabel(tr("N/A"), m_procGrp);
    m_cpuArchLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    procForm->addRow(m_cpuArchKeyLabel, m_cpuArchLabel);

    m_kernelKeyLabel = new QLabel(tr("Kernel:"), m_procGrp);
    m_kernelLabel = new QLabel(tr("N/A"), m_procGrp);
    m_kernelLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_kernelLabel->setWordWrap(true);
    procForm->addRow(m_kernelKeyLabel, m_kernelLabel);

    m_buildKeyLabel = new QLabel(tr("Build:"), m_procGrp);
    m_buildLabel = new QLabel(tr("N/A"), m_procGrp);
    m_buildLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_buildLabel->setWordWrap(true);
    procForm->addRow(m_buildKeyLabel, m_buildLabel);

    grid->addWidget(m_procGrp, 0, 1);

    // Card 3: Display & Battery & Storage
    m_dispGrp = new QGroupBox(tr("Display, Battery and Storage"), inner);
    auto *dispForm = new QFormLayout(m_dispGrp);
    dispForm->setSpacing(10);
    m_dispKeyLabel = new QLabel(tr("Display Info:"), m_dispGrp);
    m_dispLabel = new QLabel(tr("N/A"), m_dispGrp);
    m_dispLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    dispForm->addRow(m_dispKeyLabel, m_dispLabel);

    m_storKeyLabel = new QLabel(tr("Storage Type:"), m_dispGrp);
    m_storLabel = new QLabel(tr("N/A"), m_dispGrp);
    m_storLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    dispForm->addRow(m_storKeyLabel, m_storLabel);

    m_batKeyLabel = new QLabel(tr("Battery:"), m_dispGrp);
    m_batLabel = new QLabel(tr("N/A"), m_dispGrp);
    m_batLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    dispForm->addRow(m_batKeyLabel, m_batLabel);

    m_uptimeKeyLabel = new QLabel(tr("Uptime:"), m_dispGrp);
    m_uptimeLabel = new QLabel(tr("N/A"), m_dispGrp);
    m_uptimeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    dispForm->addRow(m_uptimeKeyLabel, m_uptimeLabel);

    grid->addWidget(m_dispGrp, 1, 0, 1, 2);

    lay->addLayout(grid);
    lay->addStretch();

    scroll->setWidget(inner);
    tabLay->addWidget(scroll);

    connect(m_refreshInfoBtn, &QPushButton::clicked, this, &MainWindow::onRefreshDeviceInfo);

    m_pageStack->addWidget(m_tab0);
}

// ── Tab 1: App Management ────────────────────────────────────────────────────

void MainWindow::setupTab1_AppManagement()
{
    m_tab1 = new QWidget;
    auto *lay = new QHBoxLayout(m_tab1);
    lay->setContentsMargins(18, 16, 18, 16);
    lay->setSpacing(16);

    // Left: package list card
    auto *leftGrp = new QGroupBox(tr("Installed Packages"), m_tab1);
    auto *leftLay = new QVBoxLayout(leftGrp);
    leftLay->setSpacing(10);

    auto *topRow = new QHBoxLayout;
    m_pkgInput = new QLineEdit(leftGrp);
    m_pkgInput->setPlaceholderText(tr("Package Name"));
    m_autoFillChk = new QCheckBox(tr("Auto-fill"), leftGrp);
    m_selectAllChk = new QCheckBox(tr("Select All"), leftGrp);
    topRow->addWidget(m_pkgInput, 1);
    topRow->addWidget(m_autoFillChk);
    topRow->addWidget(m_selectAllChk);
    leftLay->addLayout(topRow);

    m_pkgList = new QListWidget(leftGrp);
    m_pkgList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    leftLay->addWidget(m_pkgList, 1);

    m_refreshPkgBtn = new QPushButton(tr("Refresh"), leftGrp);
    m_refreshPkgBtn->setIcon(IconHelper::createIcon(IconHelper::Refresh, Qt::white));
    leftLay->addWidget(m_refreshPkgBtn);
    lay->addWidget(leftGrp, 1);

    // Right: action buttons card
    auto *rightGrp = new QGroupBox(tr("App Operations"), m_tab1);
    auto *rightLay = new QVBoxLayout(rightGrp);
    rightLay->setSpacing(8);

    m_installBtn   = new QPushButton(tr("Install APK"), rightGrp);
    m_uninstallBtn = new QPushButton(tr("Uninstall"), rightGrp);
    m_enableBtn    = new QPushButton(tr("Enable"), rightGrp);
    m_disableBtn   = new QPushButton(tr("Disable"), rightGrp);
    m_forceStopBtn = new QPushButton(tr("Force Stop"), rightGrp);
    m_getInfoBtn   = new QPushButton(tr("Get Info"), rightGrp);
    m_extractBtn   = new QPushButton(tr("Extract APK"), rightGrp);
    m_openAppBtn   = new QPushButton(tr("Open App"), rightGrp);
    m_wipeDataBtn  = new QPushButton(tr("Wipe Data"), rightGrp);
    m_enableAllBtn = new QPushButton(tr("Enable All"), rightGrp);
    m_disableAllBtn= new QPushButton(tr("Disable All"), rightGrp);
    m_allowDgChk   = new QCheckBox(tr("Allow Downgrade"), rightGrp);

    for (auto *btn : {m_installBtn, m_uninstallBtn, m_enableBtn, m_disableBtn,
                      m_forceStopBtn, m_getInfoBtn, m_extractBtn, m_openAppBtn,
                      m_wipeDataBtn, m_enableAllBtn, m_disableAllBtn}) {
        btn->setMinimumHeight(32);
        rightLay->addWidget(btn);
    }
    rightLay->addWidget(m_allowDgChk);
    rightLay->addStretch();
    lay->addWidget(rightGrp);

    connect(m_installBtn,    &QPushButton::clicked, this, &MainWindow::onInstallApk);
    connect(m_uninstallBtn,  &QPushButton::clicked, this, &MainWindow::onUninstallPkg);
    connect(m_enableBtn,     &QPushButton::clicked, this, &MainWindow::onEnablePkg);
    connect(m_disableBtn,    &QPushButton::clicked, this, &MainWindow::onDisablePkg);
    connect(m_forceStopBtn,  &QPushButton::clicked, this, &MainWindow::onForceStopPkg);
    connect(m_getInfoBtn,    &QPushButton::clicked, this, &MainWindow::onGetInfoPkg);
    connect(m_extractBtn,    &QPushButton::clicked, this, &MainWindow::onExtractApk);
    connect(m_openAppBtn,    &QPushButton::clicked, this, &MainWindow::onOpenApp);
    connect(m_wipeDataBtn,   &QPushButton::clicked, this, &MainWindow::onWipeData);
    connect(m_enableAllBtn,  &QPushButton::clicked, this, &MainWindow::onEnableAllPkg);
    connect(m_disableAllBtn, &QPushButton::clicked, this, &MainWindow::onDisableAllPkg);
    connect(m_selectAllChk,  &QCheckBox::toggled,   this, &MainWindow::onSelectAllPkg);
    connect(m_autoFillChk,   &QCheckBox::toggled,   this, &MainWindow::onAutoFillToggled);
    connect(m_pkgList, &QListWidget::currentTextChanged, m_pkgInput, &QLineEdit::setText);
    connect(m_refreshPkgBtn, &QPushButton::clicked, this, &MainWindow::onRefreshPackages);

    m_pageStack->addWidget(m_tab1);
}

// ── Tab 2: Display ────────────────────────────────────────────────────────────

void MainWindow::setupTab2_Display()
{
    m_tab2 = new QWidget;
    auto *tabLay = new QVBoxLayout(m_tab2);
    tabLay->setContentsMargins(18, 16, 18, 16);

    auto *scroll = new QScrollArea(m_tab2);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *inner = new QWidget(scroll);
    auto *lay = new QVBoxLayout(inner);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(14);

    // Row 1: Two cards side by side (Resolution on left, Density on right - Image 1!)
    auto *topRow = new QHBoxLayout;
    topRow->setSpacing(14);

    // Resolution card
    m_resGrp = new QGroupBox(tr("Screen Resolution"), inner);
    auto *resLay = new QVBoxLayout(m_resGrp);
    resLay->setSpacing(10);

    m_dispCurLabel = new QLabel(tr("Resolution: 1080x2400"), m_resGrp);
    m_dispCurLabel->setStyleSheet(QStringLiteral("color: #64748B; font-weight: 500; font-size: 13px;"));
    resLay->addWidget(m_dispCurLabel);

    auto *resInputRow = new QHBoxLayout;
    m_resWBox = new QSpinBox(m_resGrp);
    m_resWBox->setRange(100, 7680);
    m_resWBox->setValue(1080);
    m_resHBox = new QSpinBox(m_resGrp);
    m_resHBox->setRange(100, 7680);
    m_resHBox->setValue(2400);

    auto *wBoxLay = new QVBoxLayout;
    m_resWLabel = new QLabel(tr("Horizontal Pixels"), m_resGrp);
    wBoxLay->addWidget(m_resWLabel);
    wBoxLay->addWidget(m_resWBox);

    auto *hBoxLay = new QVBoxLayout;
    m_resHLabel = new QLabel(tr("Vertical Pixels"), m_resGrp);
    hBoxLay->addWidget(m_resHLabel);
    hBoxLay->addWidget(m_resHBox);

    resInputRow->addLayout(wBoxLay);
    resInputRow->addLayout(hBoxLay);
    resLay->addLayout(resInputRow);

    auto *resBtnRow = new QHBoxLayout;
    m_chgResBtn = new QPushButton(tr("Modify"), m_resGrp);
    m_chgResBtn->setIcon(IconHelper::createIcon(IconHelper::Check, Qt::white));
    m_rstResBtn = new QPushButton(tr("Restore"), m_resGrp);
    m_rstResBtn->setIcon(IconHelper::createIcon(IconHelper::Restore, Qt::white));
    resBtnRow->addWidget(m_chgResBtn);
    resBtnRow->addWidget(m_rstResBtn);
    resLay->addLayout(resBtnRow);

    topRow->addWidget(m_resGrp, 1);

    // Density (DPI) card
    m_dpiGrp = new QGroupBox(tr("Display Density"), inner);
    auto *dpiLay = new QVBoxLayout(m_dpiGrp);
    dpiLay->setSpacing(10);

    m_dpiCurLabel = new QLabel(tr("Density (DPI): 396  Min Width (dp): 436"), m_dpiGrp);
    m_dpiCurLabel->setStyleSheet(QStringLiteral("color: #64748B; font-weight: 500; font-size: 13px;"));
    dpiLay->addWidget(m_dpiCurLabel);

    auto *dpiInputRow = new QHBoxLayout;
    m_densTypeCombo = new QComboBox(m_dpiGrp);
    m_densTypeCombo->addItems({"DPI", "ldpi", "mdpi", "hdpi", "xhdpi", "xxhdpi", "xxxhdpi"});
    m_dpiBox = new QSpinBox(m_dpiGrp);
    m_dpiBox->setRange(60, 960);
    m_dpiBox->setValue(396);

    auto *typeLay = new QVBoxLayout;
    m_unitLabel = new QLabel(tr("Unit"), m_dpiGrp);
    typeLay->addWidget(m_unitLabel);
    typeLay->addWidget(m_densTypeCombo);

    auto *dpiValLay = new QVBoxLayout;
    m_dpiValLabel = new QLabel(tr("DPI"), m_dpiGrp);
    dpiValLay->addWidget(m_dpiValLabel);
    dpiValLay->addWidget(m_dpiBox);

    dpiInputRow->addLayout(typeLay);
    dpiInputRow->addLayout(dpiValLay);
    dpiLay->addLayout(dpiInputRow);

    auto *dpiBtnRow = new QHBoxLayout;
    m_chgDpiBtn = new QPushButton(tr("Modify"), m_dpiGrp);
    m_chgDpiBtn->setIcon(IconHelper::createIcon(IconHelper::Check, Qt::white));
    m_rstDpiBtn = new QPushButton(tr("Restore"), m_dpiGrp);
    m_rstDpiBtn->setIcon(IconHelper::createIcon(IconHelper::Restore, Qt::white));
    dpiBtnRow->addWidget(m_chgDpiBtn);
    dpiBtnRow->addWidget(m_rstDpiBtn);
    dpiLay->addLayout(dpiBtnRow);

    topRow->addWidget(m_dpiGrp, 1);
    lay->addLayout(topRow);

    // Row 2: Key Simulation Card (Exact layout from Image 1!)
    m_simGrp = new QGroupBox(tr("Key Simulation"), inner);
    auto *simLay = new QVBoxLayout(m_simGrp);
    simLay->setSpacing(10);

    auto *simGrid = new QGridLayout;
    simGrid->setSpacing(10);

    m_simBackBtn       = new QPushButton(tr("Back"), m_simGrp);
    m_simHomeBtn       = new QPushButton(tr("Home"), m_simGrp);
    m_simRecentsBtn    = new QPushButton(tr("Recents"), m_simGrp);
    m_simLockBtn       = new QPushButton(tr("Lock Screen"), m_simGrp);
    m_simVolUpBtn      = new QPushButton(tr("Volume +"), m_simGrp);
    m_simVolDownBtn    = new QPushButton(tr("Volume -"), m_simGrp);
    m_simMuteBtn       = new QPushButton(tr("Mute"), m_simGrp);
    m_simScreenshotBtn = new QPushButton(tr("Screenshot"), m_simGrp);

    m_simBackBtn->setIcon(IconHelper::createIcon(IconHelper::Back, Qt::white));
    m_simHomeBtn->setIcon(IconHelper::createIcon(IconHelper::NavHome, Qt::white));
    m_simRecentsBtn->setIcon(IconHelper::createIcon(IconHelper::Recents, Qt::white));
    m_simLockBtn->setIcon(IconHelper::createIcon(IconHelper::Lock, Qt::white));
    m_simVolUpBtn->setIcon(IconHelper::createIcon(IconHelper::VolumeUp, Qt::white));
    m_simVolDownBtn->setIcon(IconHelper::createIcon(IconHelper::VolumeDown, Qt::white));
    m_simMuteBtn->setIcon(IconHelper::createIcon(IconHelper::Mute, Qt::white));
    m_simScreenshotBtn->setIcon(IconHelper::createIcon(IconHelper::Screenshot, Qt::white));

    for (auto *b : {m_simBackBtn, m_simHomeBtn, m_simRecentsBtn, m_simLockBtn,
                    m_simVolUpBtn, m_simVolDownBtn, m_simMuteBtn, m_simScreenshotBtn}) {
        b->setMinimumHeight(40);
    }

    simGrid->addWidget(m_simBackBtn,       0, 0);
    simGrid->addWidget(m_simHomeBtn,       0, 1);
    simGrid->addWidget(m_simRecentsBtn,    0, 2);
    simGrid->addWidget(m_simLockBtn,       0, 3);
    simGrid->addWidget(m_simVolUpBtn,      1, 0);
    simGrid->addWidget(m_simVolDownBtn,    1, 1);
    simGrid->addWidget(m_simMuteBtn,       1, 2);
    simGrid->addWidget(m_simScreenshotBtn, 1, 3);
    simLay->addLayout(simGrid);

    m_simTipLabel1 = new QLabel(tr("* Right-click [Lock Screen] for Power Menu (on supported devices)"), m_simGrp);
    m_simTipLabel2 = new QLabel(tr("* Right-click [Screenshot] to open the screenshots folder"), m_simGrp);
    m_simTipLabel1->setStyleSheet(QStringLiteral("color: #64748B; font-size: 12px;"));
    m_simTipLabel2->setStyleSheet(QStringLiteral("color: #64748B; font-size: 12px;"));
    simLay->addWidget(m_simTipLabel1);
    simLay->addWidget(m_simTipLabel2);

    lay->addWidget(m_simGrp);

    // Row 3: Navigation & Status Bar Tweaks Card
    m_barGrp = new QGroupBox(tr("Navigation and Status Bar"), inner);
    auto *barLay = new QHBoxLayout(m_barGrp);
    barLay->setSpacing(8);
    m_hideBothBtn   = new QPushButton(tr("Hide Both"), m_barGrp);
    m_showBothBtn   = new QPushButton(tr("Show Both"), m_barGrp);
    m_hideNavBtn    = new QPushButton(tr("Hide Navigation"), m_barGrp);
    m_hideStaBtn    = new QPushButton(tr("Hide Status Bar"), m_barGrp);
    m_hideStaIcoBtn = new QPushButton(tr("Hide Status Icons"), m_barGrp);
    m_killFgBtn     = new QPushButton(tr("Kill Foreground App"), m_barGrp);
    m_iconMarginCombo = new QComboBox(m_barGrp);
    m_iconMarginCombo->addItems({"0","2","4","8","12","16"});

    for (auto *btn : {m_hideBothBtn, m_showBothBtn, m_hideNavBtn,
                      m_hideStaBtn, m_hideStaIcoBtn, m_killFgBtn}) {
        btn->setProperty("class", "secondary");
        barLay->addWidget(btn);
    }
    m_marginLabel = new QLabel(tr("Margin:"), m_barGrp);
    barLay->addWidget(m_marginLabel);
    barLay->addWidget(m_iconMarginCombo);
    lay->addWidget(m_barGrp);

    lay->addStretch();
    scroll->setWidget(inner);
    tabLay->addWidget(scroll);

    // Connections
    connect(m_chgResBtn,    &QPushButton::clicked, this, &MainWindow::onChangeResolution);
    connect(m_rstResBtn,    &QPushButton::clicked, this, &MainWindow::onRestoreResolution);
    connect(m_chgDpiBtn,    &QPushButton::clicked, this, &MainWindow::onChangeDpi);
    connect(m_rstDpiBtn,    &QPushButton::clicked, this, &MainWindow::onRestoreDpi);
    connect(m_hideBothBtn,  &QPushButton::clicked, this, &MainWindow::onHideBoth);
    connect(m_showBothBtn,  &QPushButton::clicked, this, &MainWindow::onShowBoth);
    connect(m_hideNavBtn,   &QPushButton::clicked, this, &MainWindow::onHideNavigation);
    connect(m_hideStaBtn,   &QPushButton::clicked, this, &MainWindow::onHideStatusBar);
    connect(m_hideStaIcoBtn,&QPushButton::clicked, this, &MainWindow::onHideStatusIcons);
    connect(m_killFgBtn,    &QPushButton::clicked, this, &MainWindow::onKillForeground);

    connect(m_simBackBtn,       &QPushButton::clicked, this, &MainWindow::onSimBack);
    connect(m_simHomeBtn,       &QPushButton::clicked, this, &MainWindow::onSimHome);
    connect(m_simRecentsBtn,    &QPushButton::clicked, this, &MainWindow::onSimRecents);
    connect(m_simLockBtn,       &QPushButton::clicked, this, &MainWindow::onSimLock);
    connect(m_simVolUpBtn,      &QPushButton::clicked, this, &MainWindow::onSimVolUp);
    connect(m_simVolDownBtn,    &QPushButton::clicked, this, &MainWindow::onSimVolDown);
    connect(m_simMuteBtn,       &QPushButton::clicked, this, &MainWindow::onSimMute);
    connect(m_simScreenshotBtn, &QPushButton::clicked, this, &MainWindow::onSimScreenshot);

    m_simLockBtn->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_simLockBtn, &QPushButton::customContextMenuRequested, this, [this](const QPoint &) {
        runAdbOnDevice("shell input keyevent --longpress 26");
        showStatusMessage(tr("Sent key: Long-press Power (Power Menu)"));
    });
    m_simScreenshotBtn->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_simScreenshotBtn, &QPushButton::customContextMenuRequested, this, [this](const QPoint &) {
        QString saveDir = QDir::homePath() + "/Pictures/Screenshots";
        QDir().mkpath(saveDir);
        QDesktopServices::openUrl(QUrl::fromLocalFile(saveDir));
    });

    m_pageStack->addWidget(m_tab2);
}

// ── Tab 3: Battery Control ───────────────────────────────────────────────────

void MainWindow::setupTab3_BatteryControl()
{
    m_tab3 = new QWidget;
    auto *lay = new QVBoxLayout(m_tab3);
    lay->setContentsMargins(18, 16, 18, 16);
    lay->setSpacing(14);

    m_batStatusLabel = new QLabel(tr("N/A"), m_tab3);
    m_batStatusLabel->setStyleSheet(QStringLiteral("font-weight: 500; font-size: 13px;"));
    lay->addWidget(m_batStatusLabel);

    // Percentage & Voltage
    auto *specGrp = new QGroupBox(tr("Battery Level & Voltage"), m_tab3);
    auto *specLay = new QHBoxLayout(specGrp);
    m_batPerBox = new QSpinBox(specGrp);
    m_batPerBox->setRange(0, 100);
    m_batPerBox->setValue(80);
    m_setBatPerBtn = new QPushButton(tr("Set Battery %"), specGrp);
    m_batVolBox = new QSpinBox(specGrp);
    m_batVolBox->setRange(1000, 5000);
    m_batVolBox->setValue(3800);
    m_setBatVolBtn = new QPushButton(tr("Set Voltage"), specGrp);

    specLay->addWidget(new QLabel(tr("Battery %:"), specGrp));
    specLay->addWidget(m_batPerBox);
    specLay->addWidget(m_setBatPerBtn);
    specLay->addSpacing(16);
    specLay->addWidget(new QLabel(tr("Voltage (mV):"), specGrp));
    specLay->addWidget(m_batVolBox);
    specLay->addWidget(m_setBatVolBtn);
    lay->addWidget(specGrp);

    // Charge type
    auto *chgGrp = new QGroupBox(tr("Charge Type Simulation"), m_tab3);
    auto *chgLay = new QHBoxLayout(chgGrp);
    m_acChgRadio  = new QRadioButton(tr("AC Charging"), chgGrp);
    m_usbChgRadio = new QRadioButton(tr("USB Charging"), chgGrp);
    m_wirChgRadio = new QRadioButton(tr("Wireless Charging"), chgGrp);
    m_noChgRadio  = new QRadioButton(tr("Not Charging"), chgGrp);
    m_rstBatBtn   = new QPushButton(tr("Restore Battery"), chgGrp);

    chgLay->addWidget(m_acChgRadio);
    chgLay->addWidget(m_usbChgRadio);
    chgLay->addWidget(m_wirChgRadio);
    chgLay->addWidget(m_noChgRadio);
    chgLay->addWidget(m_rstBatBtn);
    lay->addWidget(chgGrp);
    lay->addStretch();

    connect(m_setBatPerBtn, &QPushButton::clicked, this, &MainWindow::onSetBatteryPercent);
    connect(m_setBatVolBtn, &QPushButton::clicked, this, &MainWindow::onSetBatteryVoltage);
    connect(m_rstBatBtn,    &QPushButton::clicked, this, &MainWindow::onRestoreBattery);
    for (auto *r : {m_acChgRadio, m_usbChgRadio, m_wirChgRadio, m_noChgRadio})
        connect(r, &QRadioButton::clicked, this, &MainWindow::onChargeTypeChanged);

    m_pageStack->addWidget(m_tab3);
}

// ── Tab 4: Battery Monitor ───────────────────────────────────────────────────

void MainWindow::setupTab4_BatteryMonitor()
{
    m_tab4 = new QWidget;
    auto *lay = new QVBoxLayout(m_tab4);
    lay->setContentsMargins(18, 16, 18, 16);
    lay->setSpacing(12);

    auto *monGrp = new QGroupBox(tr("Real-time Battery Information"), m_tab4);
    auto *monLay = new QVBoxLayout(monGrp);

    m_batMonText = new QTextEdit(monGrp);
    m_batMonText->setReadOnly(true);
    monLay->addWidget(m_batMonText);

    lay->addWidget(monGrp);

    m_pageStack->addWidget(m_tab4);
}

// ── Tab 5: UI Tweaks ─────────────────────────────────────────────────────────

void MainWindow::setupTab5_UiTweaks()
{
    m_tab5 = new QWidget;
    auto *tabLay = new QVBoxLayout(m_tab5);
    tabLay->setContentsMargins(18, 16, 18, 16);

    auto *scroll = new QScrollArea(m_tab5);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *inner = new QWidget(scroll);
    auto *lay = new QVBoxLayout(inner);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(12);

    // Font scale
    auto *fontGrp = new QGroupBox(tr("Font Size Scale"), inner);
    auto *fontLay = new QHBoxLayout(fontGrp);
    m_fontSlider = new QSlider(Qt::Horizontal, fontGrp);
    m_fontSlider->setRange(0, 50);
    m_fontSlider->setValue(25);
    m_fontCurLabel = new QLabel(tr("Current: 1.00x"), fontGrp);
    m_fontSetLabel = new QLabel(tr("Set: 1.00x"), fontGrp);
    m_setFontBtn   = new QPushButton(tr("Apply"), fontGrp);
    m_refreshUiBtn = new QPushButton(tr("Refresh"), fontGrp);

    fontLay->addWidget(m_fontSlider, 1);
    fontLay->addWidget(m_fontCurLabel);
    fontLay->addWidget(m_fontSetLabel);
    fontLay->addWidget(m_setFontBtn);
    fontLay->addWidget(m_refreshUiBtn);
    lay->addWidget(fontGrp);

    // Window animation
    {
        auto *grp = new QGroupBox(tr("Window Animation Scale"), inner);
        auto *gl = new QHBoxLayout(grp);
        m_winAniSlider = new QSlider(Qt::Horizontal, grp);
        m_winAniSlider->setRange(0, 50);
        m_winAniSlider->setValue(25);
        m_winAniLabel = new QLabel("1.00x", grp);
        m_setWinAniBtn = new QPushButton(tr("Apply"), grp);
        gl->addWidget(m_winAniSlider, 1);
        gl->addWidget(m_winAniLabel);
        gl->addWidget(m_setWinAniBtn);
        lay->addWidget(grp);
    }
    // Transition animation
    {
        auto *grp = new QGroupBox(tr("Transition Animation Scale"), inner);
        auto *gl = new QHBoxLayout(grp);
        m_transAniSlider = new QSlider(Qt::Horizontal, grp);
        m_transAniSlider->setRange(0, 50);
        m_transAniSlider->setValue(25);
        m_transAniLabel = new QLabel("1.00x", grp);
        m_setTransAniBtn = new QPushButton(tr("Apply"), grp);
        gl->addWidget(m_transAniSlider, 1);
        gl->addWidget(m_transAniLabel);
        gl->addWidget(m_setTransAniBtn);
        lay->addWidget(grp);
    }
    // Animator duration
    {
        auto *grp = new QGroupBox(tr("Animator Duration Scale"), inner);
        auto *gl = new QHBoxLayout(grp);
        m_progAniSlider = new QSlider(Qt::Horizontal, grp);
        m_progAniSlider->setRange(0, 50);
        m_progAniSlider->setValue(25);
        m_progAniLabel = new QLabel("1.00x", grp);
        m_setProgAniBtn = new QPushButton(tr("Apply"), grp);
        gl->addWidget(m_progAniSlider, 1);
        gl->addWidget(m_progAniLabel);
        gl->addWidget(m_setProgAniBtn);
        lay->addWidget(grp);
    }

    // Lock timeout + theme
    auto *bottomRow = new QHBoxLayout;
    m_lockTimeoutCombo = new QComboBox(inner);
    m_lockTimeoutCombo->addItems({"15000","30000","60000","120000","300000","600000","1800000"});
    m_setLockBtn    = new QPushButton(tr("Set Auto Lock Timeout"), inner);
    m_switchThemeBtn= new QPushButton(tr("Cycle Theme"), inner);
    bottomRow->addWidget(new QLabel(tr("Auto Lock Timeout:"), inner));
    bottomRow->addWidget(m_lockTimeoutCombo);
    bottomRow->addWidget(m_setLockBtn);
    bottomRow->addStretch();
    bottomRow->addWidget(m_switchThemeBtn);
    lay->addLayout(bottomRow);
    lay->addStretch();

    scroll->setWidget(inner);
    tabLay->addWidget(scroll);

    connect(m_fontSlider,     &QSlider::valueChanged, this, &MainWindow::onFontSliderChanged);
    connect(m_setFontBtn,     &QPushButton::clicked,  this, &MainWindow::onSetFontScale);
    connect(m_refreshUiBtn,   &QPushButton::clicked,  this, &MainWindow::onRefreshUiTweaks);
    connect(m_winAniSlider,   &QSlider::valueChanged, this, &MainWindow::onWinAniSliderChanged);
    connect(m_setWinAniBtn,   &QPushButton::clicked,  this, &MainWindow::onSetWinAnimation);
    connect(m_transAniSlider, &QSlider::valueChanged, this, &MainWindow::onTransAniSliderChanged);
    connect(m_setTransAniBtn, &QPushButton::clicked,  this, &MainWindow::onSetTransAnimation);
    connect(m_progAniSlider,  &QSlider::valueChanged, this, &MainWindow::onProgAniSliderChanged);
    connect(m_setProgAniBtn,  &QPushButton::clicked,  this, &MainWindow::onSetProgAnimation);
    connect(m_setLockBtn,     &QPushButton::clicked,  this, &MainWindow::onSetLockTimeout);
    connect(m_switchThemeBtn, &QPushButton::clicked,  this, &MainWindow::onSwitchTheme);

    m_pageStack->addWidget(m_tab5);
}

// ── Tab 6: Script Editor ─────────────────────────────────────────────────────

void MainWindow::setupTab6_ScriptEditor()
{
    m_tab6 = new QWidget;
    auto *outerLay = new QHBoxLayout(m_tab6);
    outerLay->setContentsMargins(18, 16, 18, 16);
    outerLay->setSpacing(14);

    // Left: script list + controls
    auto *leftLay = new QVBoxLayout;
    auto *scriptListLabel = new QLabel(tr("Script files"));
    QFont lf = scriptListLabel->font();
    lf.setBold(true);
    scriptListLabel->setFont(lf);
    leftLay->addWidget(scriptListLabel);

    m_scriptList = new QListWidget;
    m_scriptList->setMaximumWidth(220);
    leftLay->addWidget(m_scriptList, 1);

    auto *scriptBtnLay = new QGridLayout;
    m_newScriptBtn  = new QPushButton(tr("New Script"));
    m_saveScriptBtn = new QPushButton(tr("Save"));
    m_runScriptBtn  = new QPushButton(tr("Run"));
    m_stopRunBtn    = new QPushButton(tr("Stop & Run"));
    m_delScriptBtn  = new QPushButton(tr("Delete"));
    m_refreshScrBtn = new QPushButton(tr("Refresh"));
    scriptBtnLay->addWidget(m_newScriptBtn,  0, 0);
    scriptBtnLay->addWidget(m_saveScriptBtn, 0, 1);
    scriptBtnLay->addWidget(m_runScriptBtn,  1, 0);
    scriptBtnLay->addWidget(m_stopRunBtn,    1, 1);
    scriptBtnLay->addWidget(m_delScriptBtn,  2, 0);
    scriptBtnLay->addWidget(m_refreshScrBtn, 2, 1);
    leftLay->addLayout(scriptBtnLay);

    // Simulator buttons
    auto *simGrp = new QGroupBox(tr("Quick Insert"));
    auto *simLay = new QGridLayout(simGrp);
    m_touchSimBtn = new QPushButton(tr("Touch Simulate"));
    m_sysCtlBtn   = new QPushButton(tr("System Control"));
    m_softCtlBtn  = new QPushButton(tr("App Control"));
    m_keySimBtn   = new QPushButton(tr("Key Simulate"));
    m_scrCtlBtn   = new QPushButton(tr("Script Control"));
    m_autoDelayChk= new QCheckBox(tr("Auto Delay"));
    simLay->addWidget(m_touchSimBtn,  0, 0);
    simLay->addWidget(m_sysCtlBtn,    0, 1);
    simLay->addWidget(m_softCtlBtn,   1, 0);
    simLay->addWidget(m_keySimBtn,    1, 1);
    simLay->addWidget(m_scrCtlBtn,    2, 0);
    simLay->addWidget(m_autoDelayChk, 2, 1);
    leftLay->addWidget(simGrp);

    // Center: script editor
    auto *centerLay = new QVBoxLayout;
    auto *scriptEditLabel = new QLabel(tr("Script content"));
    scriptEditLabel->setFont(lf);
    centerLay->addWidget(scriptEditLabel);
    m_scriptEdit = new QTextEdit;
    m_scriptEdit->setFontFamily("monospace");
    m_scriptEdit->setPlaceholderText(tr("Script not selected"));
    centerLay->addWidget(m_scriptEdit, 1);

    // Right: CPU panel
    auto *cpuGrp = new QGroupBox(tr("CPU Cores"));
    auto *cpuLay = new QGridLayout(cpuGrp);
    cpuGrp->setMaximumWidth(160);
    for (int i = 0; i < MAX_CORES; ++i) {
        m_cpuCoreLabels[i] = new QLabel(tr("Core %1").arg(i));
        m_cpuCoreValues[i] = new QLabel("-");
        m_cpuCoreValues[i]->setAlignment(Qt::AlignRight);
        cpuLay->addWidget(m_cpuCoreLabels[i], i, 0);
        cpuLay->addWidget(m_cpuCoreValues[i], i, 1);
    }

    outerLay->addLayout(leftLay);
    outerLay->addLayout(centerLay, 1);
    outerLay->addWidget(cpuGrp);

    connect(m_scriptList,   &QListWidget::itemClicked,  this, &MainWindow::onScriptSelected);
    connect(m_newScriptBtn, &QPushButton::clicked, this, &MainWindow::onNewScript);
    connect(m_saveScriptBtn,&QPushButton::clicked, this, &MainWindow::onSaveScript);
    connect(m_runScriptBtn, &QPushButton::clicked, this, &MainWindow::onRunScript);
    connect(m_stopRunBtn,   &QPushButton::clicked, this, &MainWindow::onStopAndRunScript);
    connect(m_delScriptBtn, &QPushButton::clicked, this, &MainWindow::onDeleteScript);
    connect(m_refreshScrBtn,&QPushButton::clicked, this, &MainWindow::onRefreshScripts);
    connect(m_touchSimBtn,  &QPushButton::clicked, this, &MainWindow::onTouchSimulate);
    connect(m_sysCtlBtn,    &QPushButton::clicked, this, &MainWindow::onSysControl);
    connect(m_softCtlBtn,   &QPushButton::clicked, this, &MainWindow::onSoftControl);
    connect(m_keySimBtn,    &QPushButton::clicked, this, &MainWindow::onKeySimulate);
    connect(m_scrCtlBtn,    &QPushButton::clicked, this, &MainWindow::onScriptControl);

    m_pageStack->addWidget(m_tab6);
}

// ── Tab 7: ADB Terminal ───────────────────────────────────────────────────────

void MainWindow::setupTab7_Terminal()
{
    m_tab7 = new QWidget;
    auto *lay = new QVBoxLayout(m_tab7);
    lay->setContentsMargins(18, 16, 18, 16);
    lay->setSpacing(12);

    auto *topRow = new QHBoxLayout;
    m_cmdInput = new QLineEdit;
    m_cmdInput->setPlaceholderText(tr("ADB Command (e.g. getprop ro.build.version.release)"));
    m_execBtn  = new QPushButton(tr("Execute"));
    topRow->addWidget(m_cmdInput, 1);
    topRow->addWidget(m_execBtn);
    lay->addLayout(topRow);

    m_cmdOutput = new QTextEdit;
    m_cmdOutput->setReadOnly(true);
    m_cmdOutput->setFontFamily("monospace");
    lay->addWidget(m_cmdOutput, 1);

    auto *optRow = new QHBoxLayout;
    m_clrInputChk  = new QCheckBox(tr("Clear Input after Execute"));
    m_clrOutputChk = new QCheckBox(tr("Clear Output before Execute"));
    m_logBtn       = new QPushButton(tr("Log Info"));
    m_clrInputChk->setChecked(AppSettings::instance()->clearInput());
    m_clrOutputChk->setChecked(AppSettings::instance()->clearOutput());
    optRow->addWidget(m_clrInputChk);
    optRow->addWidget(m_clrOutputChk);
    optRow->addStretch();
    optRow->addWidget(m_logBtn);
    lay->addLayout(optRow);

    connect(m_execBtn,  &QPushButton::clicked, this, &MainWindow::onExecuteCommand);
    connect(m_cmdInput, &QLineEdit::returnPressed, this, &MainWindow::onExecuteCommand);
    connect(m_logBtn,   &QPushButton::clicked, this, &MainWindow::onLogInfo);

    m_pageStack->addWidget(m_tab7);
}

// ── Tab 8: Settings ───────────────────────────────────────────────────────────

void MainWindow::setupTab8_Settings()
{
    m_tab8 = new QWidget;
    auto *scroll = new QScrollArea(m_tab8);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *inner = new QWidget(scroll);
    auto *lay = new QVBoxLayout(inner);
    lay->setContentsMargins(18, 16, 18, 16);
    lay->setSpacing(14);

    // General
    auto *genGrp = new QGroupBox(tr("General Settings"), inner);
    auto *genForm = new QFormLayout(genGrp);
    genForm->setSpacing(10);

    m_langCombo = new QComboBox(genGrp);
    m_langCombo->addItem(tr("English"),            "en");
    m_langCombo->addItem(tr("Simplified Chinese"), "zh_CN");
    m_langCombo->addItem(tr("Traditional Chinese"),"zh_TW");
    m_langCombo->addItem(tr("Russian"),            "ru");
    m_langCombo->addItem(tr("Japanese"),           "ja");
    {
        QString cur = AppSettings::instance()->language();
        for (int i = 0; i < m_langCombo->count(); ++i)
            if (m_langCombo->itemData(i).toString() == cur) { m_langCombo->setCurrentIndex(i); break; }
    }

    m_themeCombo = new QComboBox(genGrp);
    m_themeCombo->addItems({
        tr("Light Blue (Default)"),
        tr("Dark Teal"),
        tr("Dark Blue"),
        tr("Dark Purple"),
        tr("Dark Gold"),
        tr("Light Teal")
    });
    m_themeCombo->setCurrentIndex(AppSettings::instance()->themeNum());

    m_startPageCombo = new QComboBox(genGrp);
    m_startPageCombo->addItems({
        tr("Device Info"), tr("App Management"), tr("Display"),
        tr("Battery Control"), tr("Battery Monitor"), tr("UI Tweaks"),
        tr("Script Editor"), tr("ADB Terminal"), tr("Settings")
    });
    m_startPageCombo->setCurrentIndex(AppSettings::instance()->startPage());
    m_saveDefPageBtn = new QPushButton(tr("Save Default Page"), genGrp);

    m_opacitySlider = new QSlider(Qt::Horizontal, genGrp);
    m_opacitySlider->setRange(20, 100);
    m_opacitySlider->setValue(static_cast<int>(AppSettings::instance()->windowOpacity() * 100));
    m_opacityLabel = new QLabel(QString::number(m_opacitySlider->value()) + "%", genGrp);

    auto *opacRow = new QHBoxLayout;
    opacRow->addWidget(m_opacitySlider, 1);
    opacRow->addWidget(m_opacityLabel);

    m_showInfoChk = new QCheckBox(tr("Show device info"), genGrp);
    m_showInfoChk->setChecked(AppSettings::instance()->showInfo());

    m_cpuRefreshBox = new QSpinBox(genGrp);
    m_cpuRefreshBox->setRange(200, 10000);
    m_cpuRefreshBox->setValue(AppSettings::instance()->cpuRefreshMs());
    m_cpuRefreshBox->setSuffix(" ms");

    genForm->addRow(tr("Language:"), m_langCombo);
    genForm->addRow(tr("Theme:"),    m_themeCombo);
    genForm->addRow(tr("Start Page:"), m_startPageCombo);
    genForm->addRow(QString(), m_saveDefPageBtn);
    genForm->addRow(tr("Window Opacity:"), opacRow);
    genForm->addRow(tr("Show device info:"), m_showInfoChk);
    genForm->addRow(tr("Refresh CPU speed:"), m_cpuRefreshBox);
    lay->addWidget(genGrp);

    // Terminal settings
    auto *termGrp = new QGroupBox(tr("ADB Terminal Settings"), inner);
    auto *termLay = new QVBoxLayout(termGrp);
    m_clrInputSettChk  = new QCheckBox(tr("Clear Input After Execute"), termGrp);
    m_clrOutputSettChk = new QCheckBox(tr("Clear Output Before Execute"), termGrp);
    m_clrInputSettChk->setChecked(AppSettings::instance()->clearInput());
    m_clrOutputSettChk->setChecked(AppSettings::instance()->clearOutput());
    termLay->addWidget(m_clrInputSettChk);
    termLay->addWidget(m_clrOutputSettChk);
    lay->addWidget(termGrp);

    // App management
    auto *appGrp = new QGroupBox(tr("App Management Settings"), inner);
    auto *appLay = new QVBoxLayout(appGrp);
    m_allowDgSettChk = new QCheckBox(tr("Allow Downgrade Install"), appGrp);
    m_allowDgSettChk->setChecked(AppSettings::instance()->allowDowngrade());
    appLay->addWidget(m_allowDgSettChk);
    lay->addWidget(appGrp);

    // Push file path
    auto *pushGrp = new QGroupBox(tr("Push to PC Settings"), inner);
    auto *pushLay = new QVBoxLayout(pushGrp);
    auto *pushRow = new QHBoxLayout;
    m_pushPathEdit = new QLineEdit(AppSettings::instance()->pushFilePath(), pushGrp);
    m_browsePushBtn = new QPushButton("...", pushGrp);
    m_browsePushBtn->setMaximumWidth(32);
    pushRow->addWidget(new QLabel(tr("Phone destination path:"), pushGrp));
    pushRow->addWidget(m_pushPathEdit, 1);
    pushRow->addWidget(m_browsePushBtn);
    m_pushToPcChk    = new QCheckBox(tr("Push to PC"), pushGrp);
    m_pushToPcDelChk = new QCheckBox(tr("Delete after Push"), pushGrp);
    m_pushToPcChk->setChecked(AppSettings::instance()->pushToPc());
    m_pushToPcDelChk->setChecked(AppSettings::instance()->pushToPcDel());
    pushLay->addLayout(pushRow);
    pushLay->addWidget(m_pushToPcChk);
    pushLay->addWidget(m_pushToPcDelChk);
    lay->addWidget(pushGrp);

    m_saveSettingsBtn = new QPushButton(tr("Save Settings"), inner);
    lay->addWidget(m_saveSettingsBtn);
    lay->addStretch();

    scroll->setWidget(inner);
    auto *tabLay = new QVBoxLayout(m_tab8);
    tabLay->setContentsMargins(0, 0, 0, 0);
    tabLay->addWidget(scroll);

    connect(m_saveSettingsBtn, &QPushButton::clicked, this, &MainWindow::onSaveSettings);
    connect(m_saveDefPageBtn,  &QPushButton::clicked, this, &MainWindow::onSaveDefaultPage);
    connect(m_browsePushBtn,   &QPushButton::clicked, this, &MainWindow::onBrowsePushPath);
    connect(m_langCombo,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLanguageChanged);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onThemeChanged);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &MainWindow::onOpacityChanged);

    m_pageStack->addWidget(m_tab8);
}

// ── Tab 9: About ─────────────────────────────────────────────────────────────

void MainWindow::setupTab9_About()
{
    m_tab9 = new QWidget;
    auto *scroll = new QScrollArea(m_tab9);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *inner = new QWidget(scroll);
    auto *lay = new QVBoxLayout(inner);
    lay->setContentsMargins(24, 24, 24, 24);
    lay->setSpacing(18);

    m_aboutTitle = new QLabel(QStringLiteral("Android ADB Toolbox"), inner);
    QFont titleFont = m_aboutTitle->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    m_aboutTitle->setFont(titleFont);
    lay->addWidget(m_aboutTitle);

    auto *infoBox = new QGroupBox(tr("About"), inner);
    auto *infoLay = new QFormLayout(infoBox);
    infoLay->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLay->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    infoLay->setSpacing(12);

    m_aboutVersion = new QLabel(QStringLiteral("Version: 1.0.0"), infoBox);
    m_aboutVersion->setTextInteractionFlags(Qt::TextSelectableByMouse);
    infoLay->addRow(tr("Version:"), m_aboutVersion);

    m_aboutLicense = new QLabel(QStringLiteral("GPL v3"), infoBox);
    m_aboutLicense->setTextInteractionFlags(Qt::TextSelectableByMouse);
    infoLay->addRow(tr("License:"), m_aboutLicense);

    m_aboutRepo = new QLabel(QStringLiteral("<a href=\"https://github.com/dismal/toolbox-qt\">https://github.com/dismal/toolbox-qt</a>"), infoBox);
    m_aboutRepo->setTextFormat(Qt::RichText);
    m_aboutRepo->setOpenExternalLinks(true);
    m_aboutRepo->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    infoLay->addRow(tr("GitHub:"), m_aboutRepo);

    lay->addWidget(infoBox);
    lay->addStretch();

    scroll->setWidget(inner);
    auto *tabLay = new QVBoxLayout(m_tab9);
    tabLay->setContentsMargins(0, 0, 0, 0);
    tabLay->addWidget(scroll);

    m_pageStack->addWidget(m_tab9);
}

void MainWindow::retranslateUi()
{
    setWindowTitle(tr("Android Toolbox"));

    // Header bar
    m_headerTitle->setText(tr("Android ADB Toolbox"));
    m_headerSubtitle->setText(tr("Universal Android Modding & ADB Toolkit"));
    m_deviceLabel->setText(tr("Device:"));
    m_refreshDevBtn->setText(tr("Refresh"));
    m_lanDebugBtn->setText(m_lanConnected ? tr("Disconnect") : tr("LAN Debug"));

    if (!m_hasDevice && m_deviceCombo && m_deviceCombo->count() > 0) {
        m_deviceCombo->setItemText(0, tr("No device"));
    }

    // Sidebar items
    if (m_sidebarList && m_sidebarList->count() >= 10) {
        m_sidebarList->item(0)->setText(tr("Device Info"));
        m_sidebarList->item(1)->setText(tr("App Management"));
        m_sidebarList->item(2)->setText(tr("Display"));
        m_sidebarList->item(3)->setText(tr("Battery Control"));
        m_sidebarList->item(4)->setText(tr("Battery Monitor"));
        m_sidebarList->item(5)->setText(tr("UI Tweaks"));
        m_sidebarList->item(6)->setText(tr("Script Editor"));
        m_sidebarList->item(7)->setText(tr("ADB Terminal"));
        m_sidebarList->item(8)->setText(tr("Settings"));
        m_sidebarList->item(9)->setText(tr("About"));
    }

    if (m_aboutVersion) {
        m_aboutVersion->setText(QStringLiteral("Version: %1").arg(QCoreApplication::applicationVersion()));
    }
    if (m_aboutLicense) {
        m_aboutLicense->setText(QStringLiteral("GPL v3"));
    }
    if (m_aboutRepo) {
        m_aboutRepo->setText(QStringLiteral("<a href=\"https://github.com/dismal/toolbox-qt\">https://github.com/dismal/toolbox-qt</a>"));
    }
    if (m_aboutTitle) {
        m_aboutTitle->setText(QStringLiteral("Android ADB Toolbox"));
    }

    // Tab 0
    if (m_topCard) m_topCard->setTitle(tr("Device Overview"));
    if (m_devStatusBadge) {
        if (!m_hasDevice) {
            m_devStatusBadge->setText(QStringLiteral("⚪ ") + tr("No device connected. Please connect via USB or LAN."));
        } else {
            m_devStatusBadge->setText(QStringLiteral("🟢 ") + tr("Connected") + ": " + currentDevice());
        }
    }
    if (m_hwGrp) m_hwGrp->setTitle(tr("Hardware and System"));
    if (m_procGrp) m_procGrp->setTitle(tr("Processor and Platform"));
    if (m_dispGrp) m_dispGrp->setTitle(tr("Display, Battery and Storage"));
    m_brandKeyLabel->setText(tr("Brand:"));
    m_modelKeyLabel->setText(tr("Model:"));
    m_codeKeyLabel->setText(tr("Codename:"));
    m_sysKeyLabel->setText(tr("System:"));
    m_boardKeyLabel->setText(tr("Board ID:"));
    m_cpuPlatKeyLabel->setText(tr("CPU Platform:"));
    m_cpuArchKeyLabel->setText(tr("CPU Arch:"));
    m_kernelKeyLabel->setText(tr("Kernel:"));
    m_buildKeyLabel->setText(tr("Build:"));
    m_dispKeyLabel->setText(tr("Display Info:"));
    m_storKeyLabel->setText(tr("Storage Type:"));
    m_batKeyLabel->setText(tr("Battery:"));
    m_uptimeKeyLabel->setText(tr("Uptime:"));
    m_refreshInfoBtn->setText(tr("Refresh Info"));

    // Tab 1 buttons
    m_installBtn->setText(tr("Install APK"));
    m_uninstallBtn->setText(tr("Uninstall"));
    m_enableBtn->setText(tr("Enable"));
    m_disableBtn->setText(tr("Disable"));
    m_forceStopBtn->setText(tr("Force Stop"));
    m_getInfoBtn->setText(tr("Get Info"));
    m_extractBtn->setText(tr("Extract APK"));
    m_openAppBtn->setText(tr("Open App"));
    m_wipeDataBtn->setText(tr("Wipe Data"));
    m_enableAllBtn->setText(tr("Enable All"));
    m_disableAllBtn->setText(tr("Disable All"));
    m_allowDgChk->setText(tr("Allow Downgrade"));
    m_autoFillChk->setText(tr("Auto-fill"));
    m_selectAllChk->setText(tr("Select All"));
    m_pkgInput->setPlaceholderText(tr("Package Name"));

    // Tab 2 Display & Key Simulation
    if (m_resGrp) m_resGrp->setTitle(tr("Screen Resolution"));
    if (m_dispCurLabel && !m_hasDevice)
        m_dispCurLabel->setText(tr("Resolution: 1080x2400"));
    if (m_resWLabel) m_resWLabel->setText(tr("Horizontal Pixels"));
    if (m_resHLabel) m_resHLabel->setText(tr("Vertical Pixels"));
    if (m_chgResBtn) m_chgResBtn->setText(tr("Modify"));
    if (m_rstResBtn) m_rstResBtn->setText(tr("Restore"));
    if (m_dpiGrp) m_dpiGrp->setTitle(tr("Display Density"));
    if (m_dpiCurLabel && !m_hasDevice)
        m_dpiCurLabel->setText(tr("Density (DPI): 396  Min Width (dp): 436"));
    if (m_unitLabel) m_unitLabel->setText(tr("Unit"));
    if (m_dpiValLabel) m_dpiValLabel->setText(tr("DPI"));
    if (m_chgDpiBtn) m_chgDpiBtn->setText(tr("Modify"));
    if (m_rstDpiBtn) m_rstDpiBtn->setText(tr("Restore"));
    if (m_simGrp) m_simGrp->setTitle(tr("Key Simulation"));
    if (m_simBackBtn) m_simBackBtn->setText(tr("Back"));
    if (m_simHomeBtn) m_simHomeBtn->setText(tr("Home"));
    if (m_simRecentsBtn) m_simRecentsBtn->setText(tr("Recents"));
    if (m_simLockBtn) m_simLockBtn->setText(tr("Lock Screen"));
    if (m_simVolUpBtn) m_simVolUpBtn->setText(tr("Volume +"));
    if (m_simVolDownBtn) m_simVolDownBtn->setText(tr("Volume -"));
    if (m_simMuteBtn) m_simMuteBtn->setText(tr("Mute"));
    if (m_simScreenshotBtn) m_simScreenshotBtn->setText(tr("Screenshot"));
    if (m_simTipLabel1) m_simTipLabel1->setText(tr("* Right-click [Lock Screen] for Power Menu (on supported devices)"));
    if (m_simTipLabel2) m_simTipLabel2->setText(tr("* Right-click [Screenshot] to open the screenshots folder"));
    if (m_barGrp) m_barGrp->setTitle(tr("Navigation and Status Bar"));
    if (m_hideBothBtn) m_hideBothBtn->setText(tr("Hide Both"));
    if (m_showBothBtn) m_showBothBtn->setText(tr("Show Both"));
    if (m_hideNavBtn) m_hideNavBtn->setText(tr("Hide Navigation"));
    if (m_hideStaBtn) m_hideStaBtn->setText(tr("Hide Status Bar"));
    if (m_hideStaIcoBtn) m_hideStaIcoBtn->setText(tr("Hide Status Icons"));
    if (m_killFgBtn) m_killFgBtn->setText(tr("Kill Foreground App"));
    if (m_marginLabel) m_marginLabel->setText(tr("Margin:"));

    // Tab 6 buttons
    m_touchSimBtn->setText(tr("Touch Simulate"));
    m_sysCtlBtn->setText(tr("System Control"));
    m_softCtlBtn->setText(tr("App Control"));
    m_keySimBtn->setText(tr("Key Simulate"));
    m_scrCtlBtn->setText(tr("Script Control"));
    m_autoDelayChk->setText(tr("Auto Delay"));
    m_newScriptBtn->setText(tr("New Script"));
    m_saveScriptBtn->setText(tr("Save"));
    m_runScriptBtn->setText(tr("Run"));
    m_stopRunBtn->setText(tr("Stop & Run"));
    m_delScriptBtn->setText(tr("Delete"));

    // CPU labels
    for (int i = 0; i < MAX_CORES; ++i)
        m_cpuCoreLabels[i]->setText(tr("Core %1").arg(i));

    // Tab 7
    m_cmdInput->setPlaceholderText(tr("ADB Command"));
    m_execBtn->setText(tr("Execute"));
    m_clrInputChk->setText(tr("Clear Input after Execute"));
    m_clrOutputChk->setText(tr("Clear Output before Execute"));
    m_logBtn->setText(tr("Log"));

    // Tab 8
    m_saveSettingsBtn->setText(tr("Save Settings"));
    m_saveDefPageBtn->setText(tr("Save Default Page"));
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();
    QMainWindow::changeEvent(event);
}

// ════════════════════════════════════════════════════════════════════════════
//  Device Management
// ════════════════════════════════════════════════════════════════════

void MainWindow::onRefreshDevices()
{
    m_deviceCombo->blockSignals(true);
    m_deviceCombo->clear();
    QStringList devices = AdbRunner::listDevices();
    m_hasDevice = !devices.isEmpty();
    if (m_hasDevice)
        m_deviceCombo->addItems(devices);
    else
        m_deviceCombo->addItem(tr("No device"));
    m_deviceCombo->blockSignals(false);

    if (m_deviceStatusDot) {
        m_deviceStatusDot->setStyleSheet(m_hasDevice
            ? QStringLiteral("background-color: #4CAF50; border-radius: 5px;")
            : QStringLiteral("background-color: #9E9E9E; border-radius: 5px;"));
    }

    if (m_devStatusBadge) {
        m_devStatusBadge->setText(m_hasDevice
            ? QStringLiteral("🟢 ") + tr("Connected") + ": " + m_deviceCombo->currentText()
            : QStringLiteral("⚪ ") + tr("No device connected. Please connect via USB or LAN."));
    }

    showStatusMessage(tr("Devices refreshed"));
}

void MainWindow::onLanDebugClicked()
{
    if (m_lanConnected) {
        QString dev = currentDevice();
        AdbRunner::runAdb("disconnect " + dev);
        m_lanConnected = false;
        m_lanDebugBtn->setText(tr("LAN Debug"));
        onRefreshDevices();
    } else {
        bool ok;
        QString addr = QInputDialog::getText(this, tr("LAN Debug"),
            tr("Enter IP:port (e.g. 192.168.1.100:5555)"), QLineEdit::Normal, "", &ok);
        if (ok && !addr.isEmpty()) {
            QString result = AdbRunner::runAdb("connect " + addr);
            m_cmdOutput ? m_cmdOutput->append(result) : void();
            m_lanConnected = true;
            m_lanDebugBtn->setText(tr("Disconnect"));
            onRefreshDevices();
        }
    }
}

void MainWindow::onDeviceChanged(int /*index*/)
{
    bool hasDevice = m_hasDevice && !currentDevice().isEmpty();
    if (m_deviceStatusDot) {
        m_deviceStatusDot->setStyleSheet(hasDevice
            ? QStringLiteral("background-color: #4CAF50; border-radius: 5px;")
            : QStringLiteral("background-color: #9E9E9E; border-radius: 5px;"));
    }
    if (m_devStatusBadge) {
        m_devStatusBadge->setText(hasDevice
            ? QStringLiteral("🟢 ") + tr("Connected") + ": " + currentDevice()
            : QStringLiteral("⚪ ") + tr("No device connected. Please connect via USB or LAN."));
    }
    showStatusMessage(tr("Device: ") + currentDevice());
}

QString MainWindow::currentDevice() const
{
    if (!m_hasDevice || !m_deviceCombo) return QString();
    return m_deviceCombo->currentText();
}

QString MainWindow::runAdbOnDevice(const QString &args) const
{
    QString dev = currentDevice();
    if (dev.isEmpty() || dev == tr("No device")) return QString();
    return AdbRunner::runAdb("-s " + dev + " " + args);
}

void MainWindow::showStatusMessage(const QString &msg, int ms)
{
    statusBar()->showMessage(msg, ms);
}

// ════════════════════════════════════════════════════════════════════════════
//  Tab 0: Device Info
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::onRefreshDeviceInfo()
{
    showStatusMessage(tr("Loading device info..."));
    QString dev = currentDevice();
    if (dev.isEmpty() || dev == tr("No device")) {
        m_devStatusBadge->setText(QStringLiteral("⚪ ") + tr("No device connected. Please connect via USB or LAN."));
        return;
    }

    auto *watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, &QFutureWatcher<QStringList>::finished, this, [this, watcher]() {
        onDeviceInfoReady(watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([dev]() -> QStringList {
        QStringList info;
        auto adb = [&](const QString &a) {
            return AdbRunner::runAdb("-s " + dev + " " + a).trimmed();
        };
        info << adb("shell getprop ro.product.brand");
        info << adb("shell getprop ro.product.model");
        info << adb("shell getprop ro.product.odm.name");
        // System + API
        QString ver = adb("shell getprop ro.build.version.release").trimmed();
        QString api = adb("shell getprop ro.build.version.sdk").trimmed();
        info << ("Android " + ver + " (API " + api + ")");
        // CPU platform
        QString cpuInfo = adb("shell cat /proc/cpuinfo");
        QString platform;
        for (const QString &line : cpuInfo.split('\n')) {
            if (line.startsWith("Hardware")) {
                platform = line.section(':', 1).trimmed();
                break;
            }
        }
        if (platform.isEmpty()) platform = adb("shell getprop ro.hardware").trimmed();
        info << platform;
        info << adb("shell getprop ro.product.cpu.abi").trimmed();
        info << adb("shell uname -r").trimmed();
        info << adb("shell getprop ro.build.fingerprint").trimmed();
        // Display
        QString density = adb("shell wm density").trimmed();
        QString size    = adb("shell wm size").trimmed();
        info << (size + " / " + density);
        // Storage
        QString blkDev = adb("shell ls /dev/block");
        info << (blkDev.contains("sda") ? "UFS" : "eMMC");
        // Uptime
        QString uptime = adb("shell cat /proc/uptime");
        int secs = uptime.section('.', 0, 0).toInt();
        info << AdbRunner::formatUptime(secs);
        // Battery
        QString batDump = adb("shell dumpsys battery");
        QString level = adb("shell dumpsys battery get level").trimmed();
        QString tempLine;
        for (const QString &line : batDump.split('\n')) {
            if (line.contains("temperature:")) { tempLine = line; break; }
        }
        double temp = tempLine.section(':', 1).trimmed().toDouble() / 10.0;
        info << (level + "% / " + QString::number(temp) + "°C");
        // Board ID
        info << adb("shell cat /sys/devices/soc0/serial_number").trimmed();
        return info;
    }));
}

void MainWindow::onDeviceInfoReady(const QStringList &info)
{
    if (info.size() >= 13) {
        m_brandLabel->setText(info[0].isEmpty() ? "N/A" : info[0]);
        m_modelLabel->setText(info[1].isEmpty() ? "N/A" : info[1]);
        m_codeLabel->setText(info[2].isEmpty() ? "N/A" : info[2]);
        m_sysLabel->setText(info[3].isEmpty() ? "N/A" : info[3]);
        m_cpuPlatLabel->setText(info[4].isEmpty() ? "N/A" : info[4]);
        m_cpuArchLabel->setText(info[5].isEmpty() ? "N/A" : info[5]);
        m_kernelLabel->setText(info[6].isEmpty() ? "N/A" : info[6]);
        m_buildLabel->setText(info[7].isEmpty() ? "N/A" : info[7]);
        m_dispLabel->setText(info[8].isEmpty() ? "N/A" : info[8]);
        m_storLabel->setText(info[9].isEmpty() ? "N/A" : info[9]);
        m_uptimeLabel->setText(info[10].isEmpty() ? "N/A" : info[10]);
        m_batLabel->setText(info[11].isEmpty() ? "N/A" : info[11]);
        m_boardLabel->setText(info[12].isEmpty() ? "N/A" : info[12]);
        m_devStatusBadge->setText(QStringLiteral("🟢 ") + tr("Connected") + QString(": %1 (%2)").arg(info[0], info[1]));
    }
    showStatusMessage(tr("Device info loaded"));

    // Xiaomi warning
    if (info.size() > 0 && !AppSettings::instance()->miSafeDebug()) {
        QString brand = info[0].toLower();
        if (brand.contains("xiaomi") || brand.contains("redmi")) {
            QMessageBox::information(this, tr("Xiaomi Device Notice"),
                tr("For Xiaomi/Redmi devices, please enable 'USB debugging (Security settings)' "
                   "in Developer Options to allow touch and key simulation."));
            AppSettings::instance()->setMiSafeDebug(true);
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  Tab 1: App Management
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::onRefreshPackages()
{
    showStatusMessage(tr("Loading packages..."));
    QString dev = currentDevice();
    if (dev.isEmpty() || dev == tr("No device")) return;

    auto *watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, &QFutureWatcher<QStringList>::finished, this, [this, watcher]() {
        onPackagesReady(watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([dev]() -> QStringList {
        QString raw = AdbRunner::runAdb("-s " + dev + " shell pm list packages");
        QStringList pkgs;
        for (const QString &line : raw.split('\n', Qt::SkipEmptyParts)) {
            QString trimmed = line.trimmed();
            if (trimmed.startsWith("package:"))
                pkgs << trimmed.mid(8);
        }
        pkgs.sort();
        return pkgs;
    }));
}

void MainWindow::onPackagesReady(const QStringList &packages)
{
    m_pkgList->clear();
    m_pkgList->addItems(packages);
    showStatusMessage(tr("%1 packages loaded").arg(packages.size()));
}

void MainWindow::onInstallApk()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Select APK"), "", "APK Files (*.apk)");
    if (file.isEmpty()) return;
    QString extra = m_allowDgChk->isChecked() ? "-d -r " : "-r ";
    showStatusMessage(tr("Installing APK..."));
    QtConcurrent::run([this, file, extra]() {
        QString out = AdbRunner::runAdb("-s " + currentDevice() + " install " + extra + "\"" + file + "\"");
        QMetaObject::invokeMethod(this, [this, out]() {
            showStatusMessage(out.contains("Success") ? tr("Installed successfully!") : tr("Install failed: ") + out);
        });
    });
}

void MainWindow::onUninstallPkg()
{
    QString pkg = selectedPackage();
    if (pkg.isEmpty()) return;
    if (QMessageBox::question(this, tr("Confirm"), tr("Uninstall %1?").arg(pkg)) != QMessageBox::Yes) return;
    QtConcurrent::run([this, pkg]() {
        QString out = AdbRunner::runAdb("-s " + currentDevice() + " uninstall " + pkg);
        QMetaObject::invokeMethod(this, [this, out]() {
            showStatusMessage(out.contains("Success") ? tr("Uninstalled successfully!") : tr("Uninstall failed: ") + out);
            onRefreshPackages();
        });
    });
}

void MainWindow::onEnablePkg()
{
    QString pkg = selectedPackage();
    if (pkg.isEmpty()) return;
    QtConcurrent::run([=]() { AdbRunner::runAdb("-s " + currentDevice() + " shell pm enable " + pkg); });
}

void MainWindow::onDisablePkg()
{
    QString pkg = selectedPackage();
    if (pkg.isEmpty()) return;
    QtConcurrent::run([=]() { AdbRunner::runAdb("-s " + currentDevice() + " shell pm disable-user --user 0 " + pkg); });
}

void MainWindow::onForceStopPkg()
{
    QString pkg = selectedPackage();
    if (pkg.isEmpty()) return;
    QtConcurrent::run([=]() { AdbRunner::runAdb("-s " + currentDevice() + " shell am force-stop " + pkg); });
}

void MainWindow::onGetInfoPkg()
{
    QString pkg = selectedPackage();
    if (pkg.isEmpty()) return;
    QString result = AdbRunner::runAdb("-s " + currentDevice() + " shell dumpsys package " + pkg);
    QMessageBox::information(this, tr("Package Info"), result.left(2000));
}

void MainWindow::onExtractApk()
{
    QString pkg = selectedPackage();
    if (pkg.isEmpty()) return;
    QString path = AdbRunner::runAdb("-s " + currentDevice() + " shell pm path " + pkg).trimmed().remove("package:");
    QString dest = QFileDialog::getExistingDirectory(this, tr("Save APK to"));
    if (dest.isEmpty()) return;
    AdbRunner::runAdb("-s " + currentDevice() + " pull \"" + path + "\" \"" + dest + "/" + pkg + ".apk\"");
    showStatusMessage(tr("APK extracted to ") + dest);
}

void MainWindow::onOpenApp()
{
    QString pkg = selectedPackage();
    if (pkg.isEmpty()) return;
    AdbRunner::runAdb("-s " + currentDevice() + " shell monkey -p " + pkg + " -c android.intent.category.LAUNCHER 1");
}

void MainWindow::onWipeData()
{
    QString pkg = selectedPackage();
    if (pkg.isEmpty()) return;
    AdbRunner::runAdb("-s " + currentDevice() + " shell pm clear " + pkg);
}

void MainWindow::onEnableAllPkg()
{
    QString dev = currentDevice();
    for (int i = 0; i < m_pkgList->count(); ++i)
        if (m_pkgList->item(i)->isSelected())
            AdbRunner::runAdb("-s " + dev + " shell pm enable " + m_pkgList->item(i)->text());
}

void MainWindow::onDisableAllPkg()
{
    QString dev = currentDevice();
    for (int i = 0; i < m_pkgList->count(); ++i)
        if (m_pkgList->item(i)->isSelected())
            AdbRunner::runAdb("-s " + dev + " shell pm disable-user --user 0 " + m_pkgList->item(i)->text());
}

void MainWindow::onSelectAllPkg(bool checked)
{
    for (int i = 0; i < m_pkgList->count(); ++i)
        m_pkgList->item(i)->setSelected(checked);
}

void MainWindow::onAutoFillToggled(bool checked)
{
    AppSettings::instance()->setAutoFill(checked);
    if (checked && currentTab() == 1)
        m_autoFillTimer->start();
    else
        m_autoFillTimer->stop();
}

void MainWindow::onReadForegroundApp()
{
    QString dev = currentDevice();
    if (dev.isEmpty() || dev == tr("No device")) return;
    QtConcurrent::run([this, dev]() {
        QString dump = AdbRunner::runAdb("-s " + dev + " shell dumpsys window | grep mCurrentFocus");
        // extract package: mCurrentFocus=Window{... pkg/activity}
        int slash = dump.indexOf('/');
        if (slash > 0) {
            int space = dump.lastIndexOf(' ', slash);
            if (space >= 0) {
                QString pkg = dump.mid(space + 1, slash - space - 1);
                QMetaObject::invokeMethod(this, [this, pkg]() {
                    if (!pkg.isEmpty() && m_autoFillChk->isChecked())
                        m_pkgInput->setText(pkg);
                }, Qt::QueuedConnection);
            }
        }
    });
}

// ════════════════════════════════════════════════════════════════════════════
//  Tab 2: Display & Key Simulation
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::onRefreshDisplay()
{
    QString dev = currentDevice();
    if (dev.isEmpty() || dev == tr("No device")) return;
    QtConcurrent::run([this, dev]() {
        QString dens = AdbRunner::runAdb("-s " + dev + " shell wm density").trimmed();
        QString sz   = AdbRunner::runAdb("-s " + dev + " shell wm size").trimmed();
        QMetaObject::invokeMethod(this, [this, dens, sz]() {
            m_dispCurLabel->setText(tr("分辨率: ") + (sz.isEmpty() ? "1080x2400" : sz));
            m_dpiCurLabel->setText(tr("显示密度: ") + (dens.isEmpty() ? "396" : dens));
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onChangeResolution()
{
    int w = m_resWBox->value(), h = m_resHBox->value();
    runAdbOnDevice(QString("shell wm size %1x%2").arg(w).arg(h));
    showStatusMessage(tr("Resolution changed to %1x%2").arg(w).arg(h));
    onRefreshDisplay();
}

void MainWindow::onRestoreResolution()
{
    runAdbOnDevice("shell wm size reset");
    showStatusMessage(tr("Resolution restored"));
    onRefreshDisplay();
}

void MainWindow::onChangeDpi()
{
    runAdbOnDevice(QString("shell wm density %1").arg(m_dpiBox->value()));
    showStatusMessage(tr("DPI changed"));
    onRefreshDisplay();
}

void MainWindow::onRestoreDpi()
{
    runAdbOnDevice("shell wm density reset");
    showStatusMessage(tr("DPI restored"));
    onRefreshDisplay();
}

void MainWindow::onHideBoth()
{
    runAdbOnDevice("shell settings put global policy_control immersive.full=*");
}

void MainWindow::onShowBoth()
{
    runAdbOnDevice("shell settings put global policy_control null*");
}

void MainWindow::onHideNavigation()
{
    runAdbOnDevice("shell settings put global policy_control immersive.navigation=*");
}

void MainWindow::onHideStatusBar()
{
    runAdbOnDevice("shell settings put global policy_control immersive.status=*");
}

void MainWindow::onHideStatusIcons()
{
    runAdbOnDevice("shell settings put secure icon_blacklist clock,alarm_clock,battery,bluetooth,wifi");
}

void MainWindow::onKillForeground()
{
    runAdbOnDevice("shell am kill-all");
}

void MainWindow::onSimBack()
{
    runAdbOnDevice("shell input keyevent 4");
    showStatusMessage(tr("Sent key: Back"));
}

void MainWindow::onSimHome()
{
    runAdbOnDevice("shell input keyevent 3");
    showStatusMessage(tr("Sent key: Home"));
}

void MainWindow::onSimRecents()
{
    runAdbOnDevice("shell input keyevent 187");
    showStatusMessage(tr("Sent key: Recents"));
}

void MainWindow::onSimLock()
{
    runAdbOnDevice("shell input keyevent 26");
    showStatusMessage(tr("Sent key: Lock Screen / Power"));
}

void MainWindow::onSimVolUp()
{
    runAdbOnDevice("shell input keyevent 24");
    showStatusMessage(tr("Sent key: Volume +"));
}

void MainWindow::onSimVolDown()
{
    runAdbOnDevice("shell input keyevent 25");
    showStatusMessage(tr("Sent key: Volume -"));
}

void MainWindow::onSimMute()
{
    runAdbOnDevice("shell input keyevent 164");
    showStatusMessage(tr("Sent key: Mute"));
}

void MainWindow::onSimScreenshot()
{
    QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString saveDir = QDir::homePath() + "/Pictures/Screenshots";
    QDir().mkpath(saveDir);
    QString localFile = saveDir + "/screenshot_" + stamp + ".png";
    runAdbOnDevice("shell screencap -p /sdcard/autotoolbox_screen.png");
    runAdbOnDevice(QString("pull /sdcard/autotoolbox_screen.png \"%1\"").arg(localFile));
    showStatusMessage(tr("Screenshot saved: ") + localFile);
}

// ════════════════════════════════════════════════════════════════════════════
//  Tab 3: Battery Control
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::onSetBatteryPercent()
{
    runAdbOnDevice(QString("shell dumpsys battery set level %1").arg(m_batPerBox->value()));
    showStatusMessage(tr("Battery % set to %1").arg(m_batPerBox->value()));
}

void MainWindow::onSetBatteryVoltage()
{
    runAdbOnDevice(QString("shell dumpsys battery set voltage %1").arg(m_batVolBox->value()));
}

void MainWindow::onRestoreBattery()
{
    runAdbOnDevice("shell dumpsys battery reset");
    showStatusMessage(tr("Battery restored"));
}

void MainWindow::onChargeTypeChanged()
{
    int status = 1;
    if (m_acChgRadio->isChecked())  status = 5;
    if (m_usbChgRadio->isChecked()) status = 2;
    if (m_wirChgRadio->isChecked()) status = 4;
    if (m_noChgRadio->isChecked())  status = 3;
    runAdbOnDevice(QString("shell dumpsys battery set status %1").arg(status));
}

// ════════════════════════════════════════════════════════════════════════════
//  Tab 4: Battery Monitor
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::onBatteryMonitorTick()
{
    QString dev = currentDevice();
    if (dev.isEmpty() || dev == tr("No device")) return;
    QtConcurrent::run([this, dev]() {
        QString out = AdbRunner::runAdb("-s " + dev + " shell dumpsys battery");
        QMetaObject::invokeMethod(this, [this, out]() {
            m_batMonText->setPlainText(out);
        }, Qt::QueuedConnection);
    });
}

// ════════════════════════════════════════════════════════════════════════════
//  Tab 5: UI Tweaks
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::onRefreshUiTweaks()
{
    QString dev = currentDevice();
    if (dev.isEmpty() || dev == tr("No device")) return;
    QtConcurrent::run([this, dev]() {
        auto adb = [&](const QString &a) {
            return AdbRunner::runAdb("-s " + dev + " " + a).trimmed();
        };
        double fontScale = adb("shell settings get system font_scale").toDouble();
        double winAni  = adb("shell settings get global window_animation_scale").toDouble();
        double transAni= adb("shell settings get global transition_animation_scale").toDouble();
        double progAni = adb("shell settings get global animator_duration_scale").toDouble();
        QMetaObject::invokeMethod(this, [=]() {
            int fv = qBound(0, static_cast<int>(fontScale  / 0.04), 50);
            int wv = qBound(0, static_cast<int>(winAni     / 0.04), 50);
            int tv = qBound(0, static_cast<int>(transAni   / 0.04), 50);
            int pv = qBound(0, static_cast<int>(progAni    / 0.04), 50);
            m_fontSlider->setValue(fv);
            m_winAniSlider->setValue(wv);
            m_transAniSlider->setValue(tv);
            m_progAniSlider->setValue(pv);
            m_fontCurLabel->setText(tr("Current: ") + QString::number(fontScale, 'f', 2) + "x");
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onFontSliderChanged(int value)
{
    double scale = value * 0.04;
    m_fontSetLabel->setText(tr("Set: ") + QString::number(scale, 'f', 2) + "x");
}

void MainWindow::onSetFontScale()
{
    double scale = m_fontSlider->value() * 0.04;
    runAdbOnDevice(QString("shell settings put system font_scale %1").arg(scale));
}

void MainWindow::onWinAniSliderChanged(int v) { m_winAniLabel->setText(QString::number(v * 0.04, 'f', 2) + "x"); }
void MainWindow::onSetWinAnimation()
{
    runAdbOnDevice(QString("shell settings put global window_animation_scale %1").arg(m_winAniSlider->value() * 0.04));
}

void MainWindow::onTransAniSliderChanged(int v) { m_transAniLabel->setText(QString::number(v * 0.04, 'f', 2) + "x"); }
void MainWindow::onSetTransAnimation()
{
    runAdbOnDevice(QString("shell settings put global transition_animation_scale %1").arg(m_transAniSlider->value() * 0.04));
}

void MainWindow::onProgAniSliderChanged(int v) { m_progAniLabel->setText(QString::number(v * 0.04, 'f', 2) + "x"); }
void MainWindow::onSetProgAnimation()
{
    runAdbOnDevice(QString("shell settings put global animator_duration_scale %1").arg(m_progAniSlider->value() * 0.04));
}

void MainWindow::onSetLockTimeout()
{
    QString val = m_lockTimeoutCombo->currentText();
    runAdbOnDevice("shell settings put system screen_off_timeout " + val);
    showStatusMessage(tr("Lock timeout set"));
}

void MainWindow::onSwitchTheme()
{
    m_themeColorIndex = (AppSettings::instance()->themeNum() + 1) % 6;
    m_themeCombo->setCurrentIndex(m_themeColorIndex);
}

// ════════════════════════════════════════════════════════════════════════════
//  Tab 6: Script Editor
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::refreshScriptList()
{
    m_scriptList->clear();
    QString scriptDir = QCoreApplication::applicationDirPath() + "/Script";
    QDir dir(scriptDir);
    QStringList filters;
#ifdef Q_OS_WIN
    filters << QStringLiteral("*.cmd") << QStringLiteral("*.bat");
#else
    filters << QStringLiteral("*.sh");
#endif
    for (const QString &f : dir.entryList(filters, QDir::Files))
        m_scriptList->addItem(f);
}

void MainWindow::onRefreshScripts()
{
    refreshScriptList();
}

void MainWindow::onScriptSelected(QListWidgetItem *item)
{
    if (!item) return;
    QString scriptDir = QCoreApplication::applicationDirPath() + "/Script";
    m_currentScriptPath = scriptDir + "/" + item->text();
    QFile f(m_currentScriptPath);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_scriptEdit->setPlainText(QTextStream(&f).readAll());
    }
}

void MainWindow::onNewScript()
{
    QString scriptDir = QCoreApplication::applicationDirPath() + "/Script";
    auto *dlg = new NewScriptDialog(scriptDir, this);
    connect(dlg, &NewScriptDialog::scriptCreated, this, [this](const QString &) {
        refreshScriptList();
    });
    dlg->exec();
}

void MainWindow::onSaveScript()
{
    if (m_currentScriptPath.isEmpty()) return;
    QFile f(m_currentScriptPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&f) << m_scriptEdit->toPlainText();
        showStatusMessage(tr("Script saved!"));
    }
}

void MainWindow::onRunScript()
{
    if (m_currentScriptPath.isEmpty()) return;
    if (m_scriptProc && m_scriptProc->state() == QProcess::Running) return;
    m_scriptProc = new QProcess(this);
    connect(m_scriptProc, &QProcess::readyRead, this, &MainWindow::onScriptProcessOutput);
    connect(m_scriptProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus) { onScriptProcessFinished(code); });
    m_scriptProc->start(shellExecutable(), shellScriptArgs(m_currentScriptPath));
}

void MainWindow::onStopAndRunScript()
{
    if (m_scriptProc) { m_scriptProc->kill(); m_scriptProc->waitForFinished(1000); }
    onRunScript();
}

void MainWindow::onDeleteScript()
{
    if (m_currentScriptPath.isEmpty()) return;
    if (QMessageBox::question(this, tr("Confirm"), tr("Delete this script?")) == QMessageBox::Yes) {
        QFile::remove(m_currentScriptPath);
        m_currentScriptPath.clear();
        m_scriptEdit->clear();
        refreshScriptList();
    }
}

void MainWindow::onScriptProcessOutput()
{
    if (!m_scriptProc) return;
    m_scriptEdit->append(m_scriptProc->readAll());
}

void MainWindow::onScriptProcessFinished(int code)
{
    showStatusMessage(tr("Script finished with code %1").arg(code));
}

void MainWindow::onInsertCommand(const QString &cmd)
{
    m_scriptEdit->setEnabled(true);
    m_scriptEdit->textCursor().insertText(cmd + "\n");
    if (m_autoDelayChk->isChecked())
        m_scriptEdit->textCursor().insertText(scriptDelayCommand());
}

void MainWindow::onTouchSimulate()
{
    auto *dlg = new TouchSimuDialog(currentDevice(), this);
    connect(dlg, &TouchSimuDialog::commandReady, this, &MainWindow::onInsertCommand);
    dlg->exec();
}

void MainWindow::onSysControl()
{
    auto *dlg = new SysCtlDialog(currentDevice(), this);
    connect(dlg, &SysCtlDialog::commandReady, this, &MainWindow::onInsertCommand);
    dlg->exec();
}

void MainWindow::onSoftControl()
{
    auto *dlg = new SoftSimuDialog(currentDevice(), this);
    connect(dlg, &SoftSimuDialog::commandReady, this, &MainWindow::onInsertCommand);
    dlg->exec();
}

void MainWindow::onKeySimulate()
{
    auto *dlg = new KeySimuDialog(currentDevice(), this);
    connect(dlg, &KeySimuDialog::commandReady, this, &MainWindow::onInsertCommand);
    dlg->exec();
}

void MainWindow::onScriptControl()
{
    auto *dlg = new RunControlDialog(this);
    connect(dlg, &RunControlDialog::commandReady, this, &MainWindow::onInsertCommand);
    dlg->exec();
}

// ════════════════════════════════════════════════════════════════════════════
//  Tab 7: ADB Terminal
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::onExecuteCommand()
{
    QString cmd = m_cmdInput->text().trimmed();
    if (cmd.isEmpty()) return;

    if (m_clrOutputChk->isChecked()) m_cmdOutput->clear();

    m_cmdOutput->append("$ " + cmd);

    if (m_adbProc) { m_adbProc->kill(); m_adbProc->deleteLater(); }
    m_adbProc = new QProcess(this);
    m_adbProc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_adbProc, &QProcess::readyRead, this, &MainWindow::onAdbProcessOutput);
    connect(m_adbProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int, QProcess::ExitStatus) { onAdbProcessFinished(); });

    // Run via the OS shell so pipes/redirects keep working on Windows and Linux.
    m_adbProc->start(shellExecutable(), shellCommandArgs(cmd));

    if (m_clrInputChk->isChecked()) m_cmdInput->clear();
}

void MainWindow::onAdbProcessOutput()
{
    if (!m_adbProc) return;
    m_cmdOutput->append(m_adbProc->readAll().trimmed());
}

void MainWindow::onAdbProcessFinished()
{
    m_cmdOutput->append(tr("--- Done ---"));
}

void MainWindow::onLogInfo()
{
    QString dev = currentDevice();
    m_cmdOutput->append(tr("Device: ") + dev);
    m_cmdOutput->append(QDateTime::currentDateTime().toString(Qt::ISODate));
}

// ════════════════════════════════════════════════════════════════════════════
//  Tab 8: Settings
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::onSaveSettings()
{
    auto *s = AppSettings::instance();
    s->setClearInput(m_clrInputSettChk->isChecked());
    s->setClearOutput(m_clrOutputSettChk->isChecked());
    s->setAllowDowngrade(m_allowDgSettChk->isChecked());
    s->setPushFilePath(m_pushPathEdit->text());
    s->setPushToPc(m_pushToPcChk->isChecked());
    s->setPushToPcDel(m_pushToPcDelChk->isChecked());
    s->setShowInfo(m_showInfoChk->isChecked());
    s->setCpuRefreshMs(m_cpuRefreshBox->value());
    s->save();
    m_clrInputChk->setChecked(s->clearInput());
    m_clrOutputChk->setChecked(s->clearOutput());
    m_cpuTimer->setInterval(s->cpuRefreshMs());
    showStatusMessage(tr("Settings saved"));
}

void MainWindow::onSaveDefaultPage()
{
    int idx = m_startPageCombo->currentIndex();
    AppSettings::instance()->setStartPage(idx);
    AppSettings::instance()->save();
    showStatusMessage(tr("Default page saved"));
}

void MainWindow::onBrowsePushPath()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select push destination"));
    if (!dir.isEmpty()) m_pushPathEdit->setText(dir);
}

void MainWindow::onLanguageChanged(int index)
{
    QString lang = m_langCombo->itemData(index).toString();
    AppSettings::instance()->setLanguage(lang);
    AppSettings::instance()->save();
    applyLanguage(lang);
}

void MainWindow::onThemeChanged(int index)
{
    AppSettings::instance()->setThemeNum(index);
    AppSettings::instance()->save();
    applyAppTheme(index);
}

void MainWindow::onOpacityChanged(int value)
{
    double opac = value / 100.0;
    AppSettings::instance()->setWindowOpacity(opac);
    AppSettings::instance()->save();
    setWindowOpacity(opac);
    m_opacityLabel->setText(QString::number(value) + "%");
}

// ════════════════════════════════════════════════════════════════════════════
//  CPU monitoring
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::onCpuTimerTick()
{
    if (currentTab() != 6) return;
    QString dev = currentDevice();
    if (dev.isEmpty() || dev == tr("No device")) return;

    QtConcurrent::run([this, dev]() {
        QStringList freqs;
        for (int i = 0; i < MAX_CORES; ++i) {
            QString path = QString("/sys/devices/system/cpu/cpu%1/cpufreq/scaling_cur_freq").arg(i);
            QString val = AdbRunner::runAdb("-s " + dev + " shell cat " + path).trimmed();
            freqs << val;
        }
        QMetaObject::invokeMethod(this, [this, freqs]() {
            for (int i = 0; i < MAX_CORES && i < freqs.size(); ++i) {
                QString v = freqs[i];
                if (v.isEmpty() || v.contains("No such") || v.contains("error")) {
                    m_cpuCoreValues[i]->setText(tr("Offline"));
                } else {
                    int khz = v.toInt();
                    m_cpuCoreValues[i]->setText(QString::number(khz / 1000) + " MHz");
                }
            }
        }, Qt::QueuedConnection);
    });
}

void MainWindow::updateCpuLabels()
{
}

void MainWindow::onPushFile()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    AppSettings::instance()->save();
    event->accept();
}

// ════════════════════════════════════════════════════════════════════════════
//  Theme helper
// ════════════════════════════════════════════════════════════════════════════

void MainWindow::applyAppTheme(int themeNum)
{
    if (themeNum < 0 || themeNum > 5) themeNum = 0;
    MaterialTheme::applyTheme(static_cast<MaterialTheme::ThemeType>(themeNum));
    updateSidebarIcons();
}

// ════════════════════════════════════════════════════════════════════════════
//  selectedPackage helper
// ════════════════════════════════════════════════════════════════════════════

QString MainWindow::selectedPackage() const
{
    QString txt = m_pkgInput->text().trimmed();
    if (!txt.isEmpty()) return txt;
    auto items = m_pkgList->selectedItems();
    return items.isEmpty() ? QString() : items.first()->text();
}

void MainWindow::updateSidebarIcons()
{
    MaterialTheme::Palette p = MaterialTheme::getPalette(static_cast<MaterialTheme::ThemeType>(AppSettings::instance()->themeNum()));
    QColor norm = p.sidebarText;
    QColor sel = p.sidebarSelectedText;

    const IconHelper::IconType types[] = {
        IconHelper::Home,
        IconHelper::Apps,
        IconHelper::Display,
        IconHelper::Battery,
        IconHelper::Monitor,
        IconHelper::Sliders,
        IconHelper::Script,
        IconHelper::Terminal,
        IconHelper::Settings,
        IconHelper::Info
    };

    if (m_sidebarList) {
        for (int i = 0; i < m_sidebarList->count() && i < 10; ++i) {
            m_sidebarList->item(i)->setIcon(IconHelper::createIcon(types[i], norm, sel, 24));
        }
    }
}

void MainWindow::setCurrentTab(int idx)
{
    if (idx >= 0 && m_pageStack && idx < m_pageStack->count()) {
        if (m_sidebarList) m_sidebarList->setCurrentRow(idx);
        m_pageStack->setCurrentIndex(idx);
    }
}

int MainWindow::currentTab() const
{
    return m_pageStack ? m_pageStack->currentIndex() : 0;
}

void MainWindow::applyLanguage(const QString &lang)
{
    qApp->removeTranslator(&m_translator);

    if (lang.isEmpty()) {
        AppSettings::instance()->setLanguage(QStringLiteral("en"));
    }

    QString selectedLang = lang;
    if (selectedLang.isEmpty()) selectedLang = AppSettings::instance()->language();
    if (selectedLang.isEmpty()) selectedLang = QStringLiteral("en");

    if (selectedLang != QLatin1String("en")) {
        QString qm = QStringLiteral("toolbox_") + selectedLang + QStringLiteral(".qm");
        if (m_translator.load(QStringLiteral(":/translations/") + qm) || m_translator.load(qm)) {
            qApp->installTranslator(&m_translator);
        }
    }

    retranslateUi();
    updateSidebarIcons();
}
