#include "appsettings.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

AppSettings* AppSettings::instance()
{
    static AppSettings inst;
    return &inst;
}

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
{
    load();
}

QString AppSettings::configFilePath() const
{
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QDir(configDir).filePath(QStringLiteral("settings.ini"));
}

void AppSettings::load()
{
    const QString path = configFilePath();
    QSettings settings(path, QSettings::IniFormat);

    setClearInput(settings.value(QStringLiteral("clearInput"), false).toBool());
    setClearOutput(settings.value(QStringLiteral("clearOutput"), false).toBool());
    setAllowDowngrade(settings.value(QStringLiteral("allowDowngrade"), false).toBool());
    setPushFilePath(settings.value(QStringLiteral("pushFilePath"), QStringLiteral("/sdcard/")).toString());
    setWindowOpacity(settings.value(QStringLiteral("windowOpacity"), 1.0).toDouble());
    setStartPage(settings.value(QStringLiteral("startPage"), 0).toInt());
    setThemeNum(settings.value(QStringLiteral("themeNum"), 0).toInt());
    setAutoFill(settings.value(QStringLiteral("autoFill"), false).toBool());
    setCpuRefreshMs(settings.value(QStringLiteral("cpuRefreshMs"), 1000).toInt());
    setShowInfo(settings.value(QStringLiteral("showInfo"), true).toBool());
    setMiSafeDebug(settings.value(QStringLiteral("miSafeDebug"), false).toBool());
    setAssHelp(settings.value(QStringLiteral("assHelp"), false).toBool());
    setShowAndroid10(settings.value(QStringLiteral("showAndroid10"), true).toBool());
    setLanguage(settings.value(QStringLiteral("language"), QStringLiteral("en")).toString());
    setPushToPc(settings.value(QStringLiteral("pushToPc"), false).toBool());
    setPushToPcDel(settings.value(QStringLiteral("pushToPcDel"), false).toBool());

    emit settingsLoaded();
}

void AppSettings::save()
{
    const QString path = configFilePath();
    const QFileInfo fileInfo(path);
    const QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    QSettings settings(path, QSettings::IniFormat);
    settings.setValue(QStringLiteral("clearInput"), m_clearInput);
    settings.setValue(QStringLiteral("clearOutput"), m_clearOutput);
    settings.setValue(QStringLiteral("allowDowngrade"), m_allowDowngrade);
    settings.setValue(QStringLiteral("pushFilePath"), m_pushFilePath);
    settings.setValue(QStringLiteral("windowOpacity"), m_windowOpacity);
    settings.setValue(QStringLiteral("startPage"), m_startPage);
    settings.setValue(QStringLiteral("themeNum"), m_themeNum);
    settings.setValue(QStringLiteral("autoFill"), m_autoFill);
    settings.setValue(QStringLiteral("cpuRefreshMs"), m_cpuRefreshMs);
    settings.setValue(QStringLiteral("showInfo"), m_showInfo);
    settings.setValue(QStringLiteral("miSafeDebug"), m_miSafeDebug);
    settings.setValue(QStringLiteral("assHelp"), m_assHelp);
    settings.setValue(QStringLiteral("showAndroid10"), m_showAndroid10);
    settings.setValue(QStringLiteral("language"), m_language);
    settings.setValue(QStringLiteral("pushToPc"), m_pushToPc);
    settings.setValue(QStringLiteral("pushToPcDel"), m_pushToPcDel);
    settings.sync();

    emit settingsSaved();
}

// Getters
bool AppSettings::clearInput() const { return m_clearInput; }
bool AppSettings::clearOutput() const { return m_clearOutput; }
bool AppSettings::allowDowngrade() const { return m_allowDowngrade; }
QString AppSettings::pushFilePath() const { return m_pushFilePath; }
double AppSettings::windowOpacity() const { return m_windowOpacity; }
int AppSettings::startPage() const { return m_startPage; }
int AppSettings::themeNum() const { return m_themeNum; }
bool AppSettings::autoFill() const { return m_autoFill; }
int AppSettings::cpuRefreshMs() const { return m_cpuRefreshMs; }
bool AppSettings::showInfo() const { return m_showInfo; }
bool AppSettings::miSafeDebug() const { return m_miSafeDebug; }
bool AppSettings::assHelp() const { return m_assHelp; }
bool AppSettings::showAndroid10() const { return m_showAndroid10; }
QString AppSettings::language() const { return m_language; }
bool AppSettings::pushToPc() const { return m_pushToPc; }
bool AppSettings::pushToPcDel() const { return m_pushToPcDel; }

// Setters
void AppSettings::setClearInput(bool clearInput)
{
    if (m_clearInput != clearInput) {
        m_clearInput = clearInput;
        emit clearInputChanged(m_clearInput);
    }
}

void AppSettings::setClearOutput(bool clearOutput)
{
    if (m_clearOutput != clearOutput) {
        m_clearOutput = clearOutput;
        emit clearOutputChanged(m_clearOutput);
    }
}

void AppSettings::setAllowDowngrade(bool allowDowngrade)
{
    if (m_allowDowngrade != allowDowngrade) {
        m_allowDowngrade = allowDowngrade;
        emit allowDowngradeChanged(m_allowDowngrade);
    }
}

void AppSettings::setPushFilePath(const QString &pushFilePath)
{
    if (m_pushFilePath != pushFilePath) {
        m_pushFilePath = pushFilePath;
        emit pushFilePathChanged(m_pushFilePath);
    }
}

void AppSettings::setWindowOpacity(double windowOpacity)
{
    if (qAbs(m_windowOpacity - windowOpacity) > 0.0001) {
        m_windowOpacity = windowOpacity;
        emit windowOpacityChanged(m_windowOpacity);
    }
}

void AppSettings::setStartPage(int startPage)
{
    if (m_startPage != startPage) {
        m_startPage = startPage;
        emit startPageChanged(m_startPage);
    }
}

void AppSettings::setThemeNum(int themeNum)
{
    if (m_themeNum != themeNum) {
        m_themeNum = themeNum;
        emit themeNumChanged(m_themeNum);
    }
}

void AppSettings::setAutoFill(bool autoFill)
{
    if (m_autoFill != autoFill) {
        m_autoFill = autoFill;
        emit autoFillChanged(m_autoFill);
    }
}

void AppSettings::setCpuRefreshMs(int cpuRefreshMs)
{
    if (m_cpuRefreshMs != cpuRefreshMs) {
        m_cpuRefreshMs = cpuRefreshMs;
        emit cpuRefreshMsChanged(m_cpuRefreshMs);
    }
}

void AppSettings::setShowInfo(bool showInfo)
{
    if (m_showInfo != showInfo) {
        m_showInfo = showInfo;
        emit showInfoChanged(m_showInfo);
    }
}

void AppSettings::setMiSafeDebug(bool miSafeDebug)
{
    if (m_miSafeDebug != miSafeDebug) {
        m_miSafeDebug = miSafeDebug;
        emit miSafeDebugChanged(m_miSafeDebug);
    }
}

void AppSettings::setAssHelp(bool assHelp)
{
    if (m_assHelp != assHelp) {
        m_assHelp = assHelp;
        emit assHelpChanged(m_assHelp);
    }
}

void AppSettings::setShowAndroid10(bool showAndroid10)
{
    if (m_showAndroid10 != showAndroid10) {
        m_showAndroid10 = showAndroid10;
        emit showAndroid10Changed(m_showAndroid10);
    }
}

void AppSettings::setLanguage(const QString &language)
{
    if (m_language != language) {
        m_language = language;
        emit languageChanged(m_language);
    }
}

void AppSettings::setPushToPc(bool pushToPc)
{
    if (m_pushToPc != pushToPc) {
        m_pushToPc = pushToPc;
        emit pushToPcChanged(m_pushToPc);
    }
}

void AppSettings::setPushToPcDel(bool pushToPcDel)
{
    if (m_pushToPcDel != pushToPcDel) {
        m_pushToPcDel = pushToPcDel;
        emit pushToPcDelChanged(m_pushToPcDel);
    }
}
