#ifndef KEYSIMUDIALOG_H
#define KEYSIMUDIALOG_H

#include <QDialog>
#include <QString>

class QPushButton;
class QEvent;

class KeySimuDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeySimuDialog(const QString &deviceSerial, QWidget *parent = nullptr);
    ~KeySimuDialog() override = default;

signals:
    void commandReady(const QString &cmd);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onHomeClicked();
    void onBackClicked();
    void onRecentsClicked();
    void onLockScreenClicked();
    void onScreenshotClicked();
    void onMuteClicked();
    void onVolumeUpClicked();
    void onVolumeDownClicked();
    void onPowerClicked();

private:
    void setupUi();
    void retranslateUi();
    QString buildKeyCommand(int keycode, const QString &comment, bool longPress = false) const;
    QString buildRawCommand(const QString &subCmd, const QString &comment) const;

    QString m_deviceSerial;

    // Grid buttons
    QPushButton *m_homeBtn = nullptr;
    QPushButton *m_backBtn = nullptr;
    QPushButton *m_recentsBtn = nullptr;
    QPushButton *m_lockScreenBtn = nullptr;
    QPushButton *m_screenshotBtn = nullptr;
    QPushButton *m_muteBtn = nullptr;
    QPushButton *m_volumeUpBtn = nullptr;
    QPushButton *m_volumeDownBtn = nullptr;
    QPushButton *m_powerBtn = nullptr;
};

#endif // KEYSIMUDIALOG_H
