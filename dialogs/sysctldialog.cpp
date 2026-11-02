#include "sysctldialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QEvent>

SysCtlDialog::SysCtlDialog(const QString &deviceSerial, QWidget *parent)
    : QDialog(parent)
    , m_deviceSerial(deviceSerial)
{
    setupUi();
    retranslateUi();
}

void SysCtlDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // Section: Reboot
    m_rebootGroup = new QGroupBox(this);
    auto *rebootLayout = new QGridLayout(m_rebootGroup);
    rebootLayout->setSpacing(10);

    m_rebootBtn = new QPushButton(m_rebootGroup);
    m_powerOffBtn = new QPushButton(m_rebootGroup);
    m_recoveryBtn = new QPushButton(m_rebootGroup);
    m_fastbootBtn = new QPushButton(m_rebootGroup);

    const int btnHeight = 38;
    m_rebootBtn->setMinimumHeight(btnHeight);
    m_powerOffBtn->setMinimumHeight(btnHeight);
    m_recoveryBtn->setMinimumHeight(btnHeight);
    m_fastbootBtn->setMinimumHeight(btnHeight);

    rebootLayout->addWidget(m_rebootBtn, 0, 0);
    rebootLayout->addWidget(m_powerOffBtn, 0, 1);
    rebootLayout->addWidget(m_recoveryBtn, 1, 0);
    rebootLayout->addWidget(m_fastbootBtn, 1, 1);

    mainLayout->addWidget(m_rebootGroup);

    // Section: Send Text
    m_sendTextGroup = new QGroupBox(this);
    auto *sendTextLayout = new QVBoxLayout(m_sendTextGroup);
    sendTextLayout->setSpacing(8);

    auto *inputRow = new QHBoxLayout();
    m_textInputEdit = new QLineEdit(m_sendTextGroup);
    m_confirmTextBtn = new QPushButton(m_sendTextGroup);
    inputRow->addWidget(m_textInputEdit, 1);
    inputRow->addWidget(m_confirmTextBtn);
    sendTextLayout->addLayout(inputRow);

    m_noteLabel = new QLabel(m_sendTextGroup);
    m_noteLabel->setStyleSheet(QStringLiteral("color: #d9534f; font-size: 11px;"));
    m_noteLabel->setWordWrap(true);
    sendTextLayout->addWidget(m_noteLabel);

    mainLayout->addWidget(m_sendTextGroup);

    // Connections
    connect(m_rebootBtn, &QPushButton::clicked, this, &SysCtlDialog::onRebootClicked);
    connect(m_powerOffBtn, &QPushButton::clicked, this, &SysCtlDialog::onPowerOffClicked);
    connect(m_recoveryBtn, &QPushButton::clicked, this, &SysCtlDialog::onRecoveryClicked);
    connect(m_fastbootBtn, &QPushButton::clicked, this, &SysCtlDialog::onFastbootClicked);
    connect(m_confirmTextBtn, &QPushButton::clicked, this, &SysCtlDialog::onSendTextClicked);

    resize(420, 260);
}

void SysCtlDialog::retranslateUi()
{
    setWindowTitle(tr("System Control"));

    m_rebootGroup->setTitle(tr("Reboot"));
    m_rebootBtn->setText(tr("Reboot"));
    m_powerOffBtn->setText(tr("Power Off"));
    m_recoveryBtn->setText(tr("Recovery Mode"));
    m_fastbootBtn->setText(tr("Fastboot Mode"));

    m_sendTextGroup->setTitle(tr("Send Text"));
    m_textInputEdit->setPlaceholderText(tr("Enter text to send..."));
    m_confirmTextBtn->setText(tr("Send"));
    m_noteLabel->setText(tr("Only ASCII characters supported"));
}

void SysCtlDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(event);
}

QString SysCtlDialog::adbPrefix() const
{
    return m_deviceSerial.isEmpty()
        ? QStringLiteral("adb")
        : QStringLiteral("adb -s %1").arg(m_deviceSerial);
}

void SysCtlDialog::onRebootClicked()
{
    const QString cmd = QStringLiteral("%1 shell reboot\n"
                                       "echo \"Reboot device\"\n")
                            .arg(adbPrefix());
    emit commandReady(cmd);
    accept();
}

void SysCtlDialog::onPowerOffClicked()
{
    const QString cmd = QStringLiteral("%1 shell reboot -p\n"
                                       "echo \"Power off device\"\n")
                            .arg(adbPrefix());
    emit commandReady(cmd);
    accept();
}

void SysCtlDialog::onRecoveryClicked()
{
    const QString cmd = QStringLiteral("%1 reboot recovery\n"
                                       "echo \"Reboot to recovery mode\"\n")
                            .arg(adbPrefix());
    emit commandReady(cmd);
    accept();
}

void SysCtlDialog::onFastbootClicked()
{
    const QString cmd = QStringLiteral("%1 reboot fastboot\n"
                                       "echo \"Reboot to fastboot mode\"\n")
                            .arg(adbPrefix());
    emit commandReady(cmd);
    accept();
}

void SysCtlDialog::onSendTextClicked()
{
    const QString text = m_textInputEdit->text();
    if (text.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter text to send."));
        return;
    }

    // Escape double quotes and backslashes for bash
    QString escapedText = text;
    escapedText.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    escapedText.replace(QStringLiteral("\""), QStringLiteral("\\\""));

    const QString cmd = QStringLiteral("%1 shell input text \"%2\"\n"
                                       "echo \"Send text: %2\"\n")
                            .arg(adbPrefix(), escapedText);

    emit commandReady(cmd);
    accept();
}
