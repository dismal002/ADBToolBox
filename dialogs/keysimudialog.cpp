#include "keysimudialog.h"

#include <QGridLayout>
#include <QPushButton>
#include <QEvent>

KeySimuDialog::KeySimuDialog(const QString &deviceSerial, QWidget *parent)
    : QDialog(parent)
    , m_deviceSerial(deviceSerial)
{
    setupUi();
    retranslateUi();
}

void KeySimuDialog::setupUi()
{
    auto *gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(10);
    gridLayout->setContentsMargins(16, 16, 16, 16);

    m_homeBtn = new QPushButton(this);
    m_backBtn = new QPushButton(this);
    m_recentsBtn = new QPushButton(this);
    m_lockScreenBtn = new QPushButton(this);
    m_screenshotBtn = new QPushButton(this);
    m_muteBtn = new QPushButton(this);
    m_volumeUpBtn = new QPushButton(this);
    m_volumeDownBtn = new QPushButton(this);
    m_powerBtn = new QPushButton(this);

    // Minimum button height for good touch/click targets
    const int btnHeight = 42;
    m_homeBtn->setMinimumHeight(btnHeight);
    m_backBtn->setMinimumHeight(btnHeight);
    m_recentsBtn->setMinimumHeight(btnHeight);
    m_lockScreenBtn->setMinimumHeight(btnHeight);
    m_screenshotBtn->setMinimumHeight(btnHeight);
    m_muteBtn->setMinimumHeight(btnHeight);
    m_volumeUpBtn->setMinimumHeight(btnHeight);
    m_volumeDownBtn->setMinimumHeight(btnHeight);
    m_powerBtn->setMinimumHeight(btnHeight);

    // Row 0
    gridLayout->addWidget(m_homeBtn, 0, 0);
    gridLayout->addWidget(m_backBtn, 0, 1);
    gridLayout->addWidget(m_recentsBtn, 0, 2);

    // Row 1
    gridLayout->addWidget(m_lockScreenBtn, 1, 0);
    gridLayout->addWidget(m_screenshotBtn, 1, 1);
    gridLayout->addWidget(m_muteBtn, 1, 2);

    // Row 2
    gridLayout->addWidget(m_volumeUpBtn, 2, 0);
    gridLayout->addWidget(m_volumeDownBtn, 2, 1);
    gridLayout->addWidget(m_powerBtn, 2, 2);

    connect(m_homeBtn, &QPushButton::clicked, this, &KeySimuDialog::onHomeClicked);
    connect(m_backBtn, &QPushButton::clicked, this, &KeySimuDialog::onBackClicked);
    connect(m_recentsBtn, &QPushButton::clicked, this, &KeySimuDialog::onRecentsClicked);
    connect(m_lockScreenBtn, &QPushButton::clicked, this, &KeySimuDialog::onLockScreenClicked);
    connect(m_screenshotBtn, &QPushButton::clicked, this, &KeySimuDialog::onScreenshotClicked);
    connect(m_muteBtn, &QPushButton::clicked, this, &KeySimuDialog::onMuteClicked);
    connect(m_volumeUpBtn, &QPushButton::clicked, this, &KeySimuDialog::onVolumeUpClicked);
    connect(m_volumeDownBtn, &QPushButton::clicked, this, &KeySimuDialog::onVolumeDownClicked);
    connect(m_powerBtn, &QPushButton::clicked, this, &KeySimuDialog::onPowerClicked);

    resize(420, 200);
}

void KeySimuDialog::retranslateUi()
{
    setWindowTitle(tr("Key Simulation"));

    m_homeBtn->setText(tr("Home"));
    m_backBtn->setText(tr("Back"));
    m_recentsBtn->setText(tr("Recents"));
    m_lockScreenBtn->setText(tr("Lock Screen"));
    m_screenshotBtn->setText(tr("Screenshot"));
    m_muteBtn->setText(tr("Mute"));
    m_volumeUpBtn->setText(tr("Volume +"));
    m_volumeDownBtn->setText(tr("Volume -"));
    m_powerBtn->setText(tr("Power"));
}

void KeySimuDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(event);
}

QString KeySimuDialog::buildKeyCommand(int keycode, const QString &comment, bool longPress) const
{
    QString cmd;
    if (longPress) {
        if (m_deviceSerial.isEmpty()) {
            cmd = QStringLiteral("adb shell input keyevent --longpress %1\n"
                                 "echo \"%2\"\n")
                      .arg(QString::number(keycode), comment);
        } else {
            cmd = QStringLiteral("adb -s %1 shell input keyevent --longpress %2\n"
                                 "echo \"%3\"\n")
                      .arg(m_deviceSerial, QString::number(keycode), comment);
        }
    } else {
        if (m_deviceSerial.isEmpty()) {
            cmd = QStringLiteral("adb shell input keyevent %1\n"
                                 "echo \"%2\"\n")
                      .arg(QString::number(keycode), comment);
        } else {
            cmd = QStringLiteral("adb -s %1 shell input keyevent %2\n"
                                 "echo \"%3\"\n")
                      .arg(m_deviceSerial, QString::number(keycode), comment);
        }
    }
    return cmd;
}

QString KeySimuDialog::buildRawCommand(const QString &subCmd, const QString &comment) const
{
    QString cmd;
    if (m_deviceSerial.isEmpty()) {
        cmd = QStringLiteral("adb shell %1\n"
                             "echo \"%2\"\n")
                  .arg(subCmd, comment);
    } else {
        cmd = QStringLiteral("adb -s %1 shell %2\n"
                             "echo \"%3\"\n")
                  .arg(m_deviceSerial, subCmd, comment);
    }
    return cmd;
}

void KeySimuDialog::onHomeClicked()
{
    emit commandReady(buildKeyCommand(3, QStringLiteral("Keyevent: Home (3)")));
    accept();
}

void KeySimuDialog::onBackClicked()
{
    emit commandReady(buildKeyCommand(4, QStringLiteral("Keyevent: Back (4)")));
    accept();
}

void KeySimuDialog::onRecentsClicked()
{
    emit commandReady(buildKeyCommand(187, QStringLiteral("Keyevent: Recents (187)")));
    accept();
}

void KeySimuDialog::onLockScreenClicked()
{
    emit commandReady(buildKeyCommand(26, QStringLiteral("Keyevent: Lock Screen (26)")));
    accept();
}

void KeySimuDialog::onScreenshotClicked()
{
    emit commandReady(buildRawCommand(
        QStringLiteral("/system/bin/screencap -p /sdcard/screen.png"),
        QStringLiteral("Take screenshot to /sdcard/screen.png")));
    accept();
}

void KeySimuDialog::onMuteClicked()
{
    emit commandReady(buildKeyCommand(164, QStringLiteral("Keyevent: Mute (164)")));
    accept();
}

void KeySimuDialog::onVolumeUpClicked()
{
    emit commandReady(buildKeyCommand(24, QStringLiteral("Keyevent: Volume + (24)")));
    accept();
}

void KeySimuDialog::onVolumeDownClicked()
{
    emit commandReady(buildKeyCommand(25, QStringLiteral("Keyevent: Volume - (25)")));
    accept();
}

void KeySimuDialog::onPowerClicked()
{
    emit commandReady(buildKeyCommand(26, QStringLiteral("Keyevent: Power Longpress (26)"), true));
    accept();
}
