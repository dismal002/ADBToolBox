#ifndef SYSCTLDIALOG_H
#define SYSCTLDIALOG_H

#include <QDialog>
#include <QString>

class QPushButton;
class QLineEdit;
class QLabel;
class QGroupBox;
class QEvent;

class SysCtlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SysCtlDialog(const QString &deviceSerial, QWidget *parent = nullptr);
    ~SysCtlDialog() override = default;

signals:
    void commandReady(const QString &cmd);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onRebootClicked();
    void onPowerOffClicked();
    void onRecoveryClicked();
    void onFastbootClicked();
    void onSendTextClicked();

private:
    void setupUi();
    void retranslateUi();
    QString adbPrefix() const;

    QString m_deviceSerial;

    // Reboot section widgets
    QGroupBox *m_rebootGroup = nullptr;
    QPushButton *m_rebootBtn = nullptr;
    QPushButton *m_powerOffBtn = nullptr;
    QPushButton *m_recoveryBtn = nullptr;
    QPushButton *m_fastbootBtn = nullptr;

    // Send text section widgets
    QGroupBox *m_sendTextGroup = nullptr;
    QLineEdit *m_textInputEdit = nullptr;
    QPushButton *m_confirmTextBtn = nullptr;
    QLabel *m_noteLabel = nullptr;
};

#endif // SYSCTLDIALOG_H
