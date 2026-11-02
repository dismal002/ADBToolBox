#ifndef RUNCONTROLDIALOG_H
#define RUNCONTROLDIALOG_H

#include <QDialog>
#include <QString>

class QPushButton;
class QLineEdit;
class QSpinBox;
class QLabel;
class QGroupBox;
class QEvent;

class RunControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RunControlDialog(QWidget *parent = nullptr);
    ~RunControlDialog() override = default;

signals:
    void commandReady(const QString &cmd);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onConfirmDelayClicked();
    void onConfirmLoopClicked();

private:
    void setupUi();
    void retranslateUi();

    // Delay section widgets
    QGroupBox *m_delayGroup = nullptr;
    QLabel *m_delayLabel = nullptr;
    QLineEdit *m_delaySecondsEdit = nullptr;
    QPushButton *m_confirmDelayBtn = nullptr;

    // Loop section widgets
    QGroupBox *m_loopGroup = nullptr;
    QLabel *m_varNameLabel = nullptr;
    QLineEdit *m_varNameEdit = nullptr;
    QLabel *m_loopCountLabel = nullptr;
    QSpinBox *m_loopCountSpin = nullptr;
    QPushButton *m_confirmLoopBtn = nullptr;
};

#endif // RUNCONTROLDIALOG_H
