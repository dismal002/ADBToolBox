#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QString>

class AppSettings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool clearInput READ clearInput WRITE setClearInput NOTIFY clearInputChanged)
    Q_PROPERTY(bool clearOutput READ clearOutput WRITE setClearOutput NOTIFY clearOutputChanged)
    Q_PROPERTY(bool allowDowngrade READ allowDowngrade WRITE setAllowDowngrade NOTIFY allowDowngradeChanged)
    Q_PROPERTY(QString pushFilePath READ pushFilePath WRITE setPushFilePath NOTIFY pushFilePathChanged)
    Q_PROPERTY(double windowOpacity READ windowOpacity WRITE setWindowOpacity NOTIFY windowOpacityChanged)
    Q_PROPERTY(int startPage READ startPage WRITE setStartPage NOTIFY startPageChanged)
    Q_PROPERTY(int themeNum READ themeNum WRITE setThemeNum NOTIFY themeNumChanged)
    Q_PROPERTY(bool autoFill READ autoFill WRITE setAutoFill NOTIFY autoFillChanged)
    Q_PROPERTY(int cpuRefreshMs READ cpuRefreshMs WRITE setCpuRefreshMs NOTIFY cpuRefreshMsChanged)
    Q_PROPERTY(bool showInfo READ showInfo WRITE setShowInfo NOTIFY showInfoChanged)
    Q_PROPERTY(bool miSafeDebug READ miSafeDebug WRITE setMiSafeDebug NOTIFY miSafeDebugChanged)
    Q_PROPERTY(bool assHelp READ assHelp WRITE setAssHelp NOTIFY assHelpChanged)
    Q_PROPERTY(bool showAndroid10 READ showAndroid10 WRITE setShowAndroid10 NOTIFY showAndroid10Changed)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(bool pushToPc READ pushToPc WRITE setPushToPc NOTIFY pushToPcChanged)
    Q_PROPERTY(bool pushToPcDel READ pushToPcDel WRITE setPushToPcDel NOTIFY pushToPcDelChanged)

public:
    static AppSettings* instance();

    // Disable copy and move semantics
    AppSettings(const AppSettings &) = delete;
    AppSettings &operator=(const AppSettings &) = delete;
    AppSettings(AppSettings &&) = delete;
    AppSettings &operator=(AppSettings &&) = delete;

    // Getters
    bool clearInput() const;
    bool clearOutput() const;
    bool allowDowngrade() const;
    QString pushFilePath() const;
    double windowOpacity() const;
    int startPage() const;
    int themeNum() const;
    bool autoFill() const;
    int cpuRefreshMs() const;
    bool showInfo() const;
    bool miSafeDebug() const;
    bool assHelp() const;
    bool showAndroid10() const;
    QString language() const;
    bool pushToPc() const;
    bool pushToPcDel() const;

    // Setters
    void setClearInput(bool clearInput);
    void setClearOutput(bool clearOutput);
    void setAllowDowngrade(bool allowDowngrade);
    void setPushFilePath(const QString &pushFilePath);
    void setWindowOpacity(double windowOpacity);
    void setStartPage(int startPage);
    void setThemeNum(int themeNum);
    void setAutoFill(bool autoFill);
    void setCpuRefreshMs(int cpuRefreshMs);
    void setShowInfo(bool showInfo);
    void setMiSafeDebug(bool miSafeDebug);
    void setAssHelp(bool assHelp);
    void setShowAndroid10(bool showAndroid10);
    void setLanguage(const QString &language);
    void setPushToPc(bool pushToPc);
    void setPushToPcDel(bool pushToPcDel);

    // Persistence
    void save();
    void load();
    QString configFilePath() const;

signals:
    void clearInputChanged(bool clearInput);
    void clearOutputChanged(bool clearOutput);
    void allowDowngradeChanged(bool allowDowngrade);
    void pushFilePathChanged(const QString &pushFilePath);
    void windowOpacityChanged(double windowOpacity);
    void startPageChanged(int startPage);
    void themeNumChanged(int themeNum);
    void autoFillChanged(bool autoFill);
    void cpuRefreshMsChanged(int cpuRefreshMs);
    void showInfoChanged(bool showInfo);
    void miSafeDebugChanged(bool miSafeDebug);
    void assHelpChanged(bool assHelp);
    void showAndroid10Changed(bool showAndroid10);
    void languageChanged(const QString &language);
    void pushToPcChanged(bool pushToPc);
    void pushToPcDelChanged(bool pushToPcDel);
    void settingsSaved();
    void settingsLoaded();

private:
    explicit AppSettings(QObject *parent = nullptr);
    ~AppSettings() override = default;

    bool m_clearInput{false};
    bool m_clearOutput{false};
    bool m_allowDowngrade{false};
    QString m_pushFilePath{QStringLiteral("/sdcard/")};
    double m_windowOpacity{1.0};
    int m_startPage{0};
    int m_themeNum{0};
    bool m_autoFill{false};
    int m_cpuRefreshMs{1000};
    bool m_showInfo{true};
    bool m_miSafeDebug{false};
    bool m_assHelp{false};
    bool m_showAndroid10{true};
    QString m_language{QStringLiteral("en")};
    bool m_pushToPc{false};
    bool m_pushToPcDel{false};
};

#endif // APPSETTINGS_H
