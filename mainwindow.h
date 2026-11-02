#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QListWidget>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QSplitter>
#include <QTimer>
#include <QTranslator>
#include <QProcess>
#include <QThread>
#include <QFutureWatcher>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void applyLanguage(const QString &lang);

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Device management
    void onRefreshDevices();
    void onLanDebugClicked();
    void onDeviceChanged(int index);

    // Tab 0 - Device Info
    void onRefreshDeviceInfo();
    void onDeviceInfoReady(const QStringList &info);

    // Tab 1 - App Management
    void onRefreshPackages();
    void onPackagesReady(const QStringList &packages);
    void onInstallApk();
    void onUninstallPkg();
    void onEnablePkg();
    void onDisablePkg();
    void onForceStopPkg();
    void onGetInfoPkg();
    void onExtractApk();
    void onOpenApp();
    void onWipeData();
    void onEnableAllPkg();
    void onDisableAllPkg();
    void onSelectAllPkg(bool checked);
    void onAutoFillToggled(bool checked);
    void onReadForegroundApp();

    // Tab 2 - Display
    void onChangeResolution();
    void onRestoreResolution();
    void onChangeDpi();
    void onRestoreDpi();
    void onHideBoth();
    void onShowBoth();
    void onHideNavigation();
    void onHideStatusBar();
    void onHideStatusIcons();
    void onKillForeground();
    void onRefreshDisplay();
    // Tab 2 Key Simulation
    void onSimBack();
    void onSimHome();
    void onSimRecents();
    void onSimLock();
    void onSimVolUp();
    void onSimVolDown();
    void onSimMute();
    void onSimScreenshot();

    // Tab 3 - Battery Control
    void onSetBatteryPercent();
    void onSetBatteryVoltage();
    void onRestoreBattery();
    void onChargeTypeChanged();

    // Tab 4 - Battery Monitor
    void onBatteryMonitorTick();

    // Tab 5 - UI Tweaks
    void onFontSliderChanged(int value);
    void onSetFontScale();
    void onWinAniSliderChanged(int value);
    void onSetWinAnimation();
    void onTransAniSliderChanged(int value);
    void onSetTransAnimation();
    void onProgAniSliderChanged(int value);
    void onSetProgAnimation();
    void onSetLockTimeout();
    void onSwitchTheme();
    void onRefreshUiTweaks();

    // Tab 6 - Script Editor
    void onScriptSelected(QListWidgetItem *item);
    void onNewScript();
    void onSaveScript();
    void onRunScript();
    void onStopAndRunScript();
    void onDeleteScript();
    void onRefreshScripts();
    void onTouchSimulate();
    void onSysControl();
    void onSoftControl();
    void onKeySimulate();
    void onScriptControl();
    void onInsertCommand(const QString &cmd);
    void onScriptProcessOutput();
    void onScriptProcessFinished(int code);

    // Tab 7 - ADB Terminal
    void onExecuteCommand();
    void onAdbProcessOutput();
    void onAdbProcessFinished();
    void onLogInfo();

    // Tab 8 - Settings
    void onSaveSettings();
    void onSaveDefaultPage();
    void onBrowsePushPath();
    void onLanguageChanged(int index);
    void onThemeChanged(int index);
    void onOpacityChanged(int value);

    // CPU monitoring
    void onCpuTimerTick();

    // Push file
    void onPushFile();

private:
    void setupUi();
    void setupHeaderBar();
    void setupSidebar();
    void setupTab0_DeviceInfo();
    void setupTab1_AppManagement();
    void setupTab2_Display();
    void setupTab3_BatteryControl();
    void setupTab4_BatteryMonitor();
    void setupTab5_UiTweaks();
    void setupTab6_ScriptEditor();
    void setupTab7_Terminal();
    void setupTab8_Settings();
    void setupTab9_About();

    void retranslateUi();

    void loadDeviceInfo();
    void loadPackages();
    void refreshScriptList();
    void updateCpuLabels();

    QString currentDevice() const;
    QString runAdbOnDevice(const QString &args) const;
    QString selectedPackage() const;
    void showStatusMessage(const QString &msg, int ms = 3000);
    void applyAppTheme(int themeNum);
    void updateSidebarIcons();
    void setCurrentTab(int idx);
    int currentTab() const;

    // ── Header Bar ───────────────────────────────────────────────
    QWidget         *m_headerBar        = nullptr;
    QLabel          *m_headerLogo       = nullptr;
    QLabel          *m_headerTitle      = nullptr;
    QLabel          *m_headerSubtitle   = nullptr;
    QLabel          *m_deviceStatusDot  = nullptr;
    QLabel          *m_deviceLabel      = nullptr;
    QComboBox       *m_deviceCombo      = nullptr;
    bool             m_hasDevice        = false;
    QPushButton     *m_refreshDevBtn    = nullptr;
    QPushButton     *m_lanDebugBtn      = nullptr;

    // ── Navigation Sidebar & Pages ───────────────────────────────
    QWidget         *m_sidebarContainer = nullptr;
    QListWidget     *m_sidebarList      = nullptr;
    QStackedWidget  *m_pageStack        = nullptr;
    QLabel          *m_sidebarVersion   = nullptr;

    // ── Tab 0: Device Info ───────────────────────────────────────
    QWidget     *m_tab0             = nullptr;
    QGroupBox   *m_topCard          = nullptr;
    QGroupBox   *m_hwGrp            = nullptr;
    QGroupBox   *m_procGrp          = nullptr;
    QGroupBox   *m_dispGrp          = nullptr;
    QLabel      *m_devStatusBadge   = nullptr;
    QLabel      *m_brandKeyLabel    = nullptr;
    QLabel      *m_brandLabel       = nullptr;
    QLabel      *m_modelKeyLabel    = nullptr;
    QLabel      *m_modelLabel       = nullptr;
    QLabel      *m_codeKeyLabel     = nullptr;
    QLabel      *m_codeLabel        = nullptr;
    QLabel      *m_sysKeyLabel      = nullptr;
    QLabel      *m_sysLabel         = nullptr;
    QLabel      *m_cpuPlatKeyLabel  = nullptr;
    QLabel      *m_cpuPlatLabel     = nullptr;
    QLabel      *m_cpuArchKeyLabel  = nullptr;
    QLabel      *m_cpuArchLabel     = nullptr;
    QLabel      *m_kernelKeyLabel   = nullptr;
    QLabel      *m_kernelLabel      = nullptr;
    QLabel      *m_buildKeyLabel    = nullptr;
    QLabel      *m_buildLabel       = nullptr;
    QLabel      *m_dispKeyLabel     = nullptr;
    QLabel      *m_dispLabel        = nullptr;
    QLabel      *m_storKeyLabel     = nullptr;
    QLabel      *m_storLabel        = nullptr;
    QLabel      *m_uptimeKeyLabel   = nullptr;
    QLabel      *m_uptimeLabel      = nullptr;
    QLabel      *m_batKeyLabel      = nullptr;
    QLabel      *m_batLabel         = nullptr;
    QLabel      *m_boardKeyLabel    = nullptr;
    QLabel      *m_boardLabel       = nullptr;
    QPushButton *m_refreshInfoBtn   = nullptr;

    // ── Tab 1: App Management ────────────────────────────────────
    QWidget     *m_tab1         = nullptr;
    QListWidget *m_pkgList      = nullptr;
    QLineEdit   *m_pkgInput     = nullptr;
    QCheckBox   *m_autoFillChk  = nullptr;
    QCheckBox   *m_selectAllChk = nullptr;
    QCheckBox   *m_allowDgChk   = nullptr;
    QPushButton *m_installBtn   = nullptr;
    QPushButton *m_uninstallBtn = nullptr;
    QPushButton *m_enableBtn    = nullptr;
    QPushButton *m_disableBtn   = nullptr;
    QPushButton *m_forceStopBtn = nullptr;
    QPushButton *m_getInfoBtn   = nullptr;
    QPushButton *m_extractBtn   = nullptr;
    QPushButton *m_openAppBtn   = nullptr;
    QPushButton *m_wipeDataBtn  = nullptr;
    QPushButton *m_enableAllBtn = nullptr;
    QPushButton *m_disableAllBtn= nullptr;
    QPushButton *m_refreshPkgBtn= nullptr;
    QTimer      *m_autoFillTimer= nullptr;

    // ── Tab 2: Display ───────────────────────────────────────────
    QWidget     *m_tab2         = nullptr;
    QGroupBox   *m_resGrp       = nullptr;
    QGroupBox   *m_dpiGrp       = nullptr;
    QGroupBox   *m_simGrp       = nullptr;
    QGroupBox   *m_barGrp       = nullptr;
    QSpinBox    *m_resWBox      = nullptr;
    QSpinBox    *m_resHBox      = nullptr;
    QSpinBox    *m_dpiBox       = nullptr;
    QComboBox   *m_densTypeCombo= nullptr;
    QLabel      *m_dispCurLabel = nullptr;
    QLabel      *m_dpiCurLabel  = nullptr;
    QLabel      *m_resWLabel    = nullptr;
    QLabel      *m_resHLabel    = nullptr;
    QLabel      *m_unitLabel    = nullptr;
    QLabel      *m_dpiValLabel  = nullptr;
    QLabel      *m_marginLabel  = nullptr;
    QPushButton *m_chgResBtn    = nullptr;
    QPushButton *m_rstResBtn    = nullptr;
    QPushButton *m_chgDpiBtn    = nullptr;
    QPushButton *m_rstDpiBtn    = nullptr;
    QPushButton *m_hideBothBtn  = nullptr;
    QPushButton *m_showBothBtn  = nullptr;
    QPushButton *m_hideNavBtn   = nullptr;
    QPushButton *m_hideStaBtn   = nullptr;
    QPushButton *m_hideStaIcoBtn= nullptr;
    QComboBox   *m_iconMarginCombo = nullptr;
    QPushButton *m_killFgBtn    = nullptr;

    // Key Simulation buttons
    QPushButton *m_simBackBtn       = nullptr;
    QPushButton *m_simHomeBtn       = nullptr;
    QPushButton *m_simRecentsBtn    = nullptr;
    QPushButton *m_simLockBtn       = nullptr;
    QPushButton *m_simVolUpBtn      = nullptr;
    QPushButton *m_simVolDownBtn    = nullptr;
    QPushButton *m_simMuteBtn       = nullptr;
    QPushButton *m_simScreenshotBtn = nullptr;
    QLabel      *m_simTipLabel1     = nullptr;
    QLabel      *m_simTipLabel2     = nullptr;

    // ── Tab 3: Battery Control ───────────────────────────────────
    QWidget     *m_tab3         = nullptr;
    QLabel      *m_batStatusLabel = nullptr;
    QSpinBox    *m_batPerBox    = nullptr;
    QSpinBox    *m_batVolBox    = nullptr;
    QPushButton *m_setBatPerBtn = nullptr;
    QPushButton *m_setBatVolBtn = nullptr;
    QRadioButton *m_acChgRadio  = nullptr;
    QRadioButton *m_usbChgRadio = nullptr;
    QRadioButton *m_wirChgRadio = nullptr;
    QRadioButton *m_noChgRadio  = nullptr;
    QPushButton *m_rstBatBtn    = nullptr;

    // ── Tab 4: Battery Monitor ───────────────────────────────────
    QWidget     *m_tab4         = nullptr;
    QTextEdit   *m_batMonText   = nullptr;
    QTimer      *m_batMonTimer  = nullptr;

    // ── Tab 5: UI Tweaks ─────────────────────────────────────────
    QWidget     *m_tab5         = nullptr;
    QSlider     *m_fontSlider   = nullptr;
    QLabel      *m_fontCurLabel = nullptr;
    QLabel      *m_fontSetLabel = nullptr;
    QPushButton *m_setFontBtn   = nullptr;
    QSlider     *m_winAniSlider = nullptr;
    QLabel      *m_winAniLabel  = nullptr;
    QPushButton *m_setWinAniBtn = nullptr;
    QSlider     *m_transAniSlider = nullptr;
    QLabel      *m_transAniLabel  = nullptr;
    QPushButton *m_setTransAniBtn = nullptr;
    QSlider     *m_progAniSlider  = nullptr;
    QLabel      *m_progAniLabel   = nullptr;
    QPushButton *m_setProgAniBtn  = nullptr;
    QComboBox   *m_lockTimeoutCombo = nullptr;
    QPushButton *m_setLockBtn     = nullptr;
    QPushButton *m_switchThemeBtn = nullptr;
    QPushButton *m_refreshUiBtn   = nullptr;

    // ── Tab 6: Script Editor ─────────────────────────────────────
    QWidget     *m_tab6         = nullptr;
    QListWidget *m_scriptList   = nullptr;
    QTextEdit   *m_scriptEdit   = nullptr;
    QCheckBox   *m_autoDelayChk = nullptr;
    QPushButton *m_newScriptBtn = nullptr;
    QPushButton *m_saveScriptBtn= nullptr;
    QPushButton *m_runScriptBtn = nullptr;
    QPushButton *m_stopRunBtn   = nullptr;
    QPushButton *m_delScriptBtn = nullptr;
    QPushButton *m_refreshScrBtn= nullptr;
    QPushButton *m_touchSimBtn  = nullptr;
    QPushButton *m_sysCtlBtn    = nullptr;
    QPushButton *m_softCtlBtn   = nullptr;
    QPushButton *m_keySimBtn    = nullptr;
    QPushButton *m_scrCtlBtn    = nullptr;
    // CPU panel
    static constexpr int MAX_CORES = 12;
    QLabel      *m_cpuCoreLabels[MAX_CORES] = {};
    QLabel      *m_cpuCoreValues[MAX_CORES] = {};
    QTimer      *m_cpuTimer     = nullptr;
    QString      m_currentScriptPath;
    QProcess    *m_scriptProc   = nullptr;

    // ── Tab 7: ADB Terminal ──────────────────────────────────────
    QWidget     *m_tab7         = nullptr;
    QLineEdit   *m_cmdInput     = nullptr;
    QTextEdit   *m_cmdOutput    = nullptr;
    QPushButton *m_execBtn      = nullptr;
    QPushButton *m_logBtn       = nullptr;
    QCheckBox   *m_clrInputChk  = nullptr;
    QCheckBox   *m_clrOutputChk = nullptr;
    QProcess    *m_adbProc      = nullptr;

    // ── Tab 8: Settings ──────────────────────────────────────────
    QWidget     *m_tab8         = nullptr;
    QCheckBox   *m_clrInputSettChk  = nullptr;
    QCheckBox   *m_clrOutputSettChk = nullptr;
    QCheckBox   *m_allowDgSettChk   = nullptr;
    QLineEdit   *m_pushPathEdit      = nullptr;
    QPushButton *m_browsePushBtn     = nullptr;
    QCheckBox   *m_pushToPcChk       = nullptr;
    QCheckBox   *m_pushToPcDelChk    = nullptr;
    QComboBox   *m_startPageCombo    = nullptr;
    QPushButton *m_saveDefPageBtn    = nullptr;
    QComboBox   *m_langCombo         = nullptr;
    QComboBox   *m_themeCombo        = nullptr;
    QSlider     *m_opacitySlider     = nullptr;
    QLabel      *m_opacityLabel      = nullptr;
    QCheckBox   *m_showInfoChk       = nullptr;
    QSpinBox    *m_cpuRefreshBox     = nullptr;
    QPushButton *m_saveSettingsBtn   = nullptr;

    // ── Tab 9: About ─────────────────────────────────────────────
    QWidget     *m_tab9         = nullptr;
    QLabel      *m_aboutTitle   = nullptr;
    QLabel      *m_aboutVersion = nullptr;
    QLabel      *m_aboutLicense = nullptr;
    QLabel      *m_aboutRepo    = nullptr;

    // ── State ────────────────────────────────────────────────────
    QTranslator  m_translator;
    bool         m_lanConnected = false;
    int          m_themeColorIndex = 0;
};

#endif // MAINWINDOW_H
