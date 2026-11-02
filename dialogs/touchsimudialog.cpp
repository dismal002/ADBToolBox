#include "touchsimudialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QIntValidator>
#include <QEvent>
#include <QMessageBox>

#if __has_include("adbrunner.h")
#include "adbrunner.h"
#elif __has_include("../adbrunner.h")
#include "../adbrunner.h"
#else
#include <QProcess>
class AdbRunner
{
public:
    static QString runAdb(const QStringList &args)
    {
        QProcess process;
        process.start(QStringLiteral("adb"), args);
        process.waitForFinished();
        return QString::fromUtf8(process.readAllStandardOutput());
    }
    static QString runAdb(const QString &command)
    {
        return runAdb(command.split(QLatin1Char(' '), Qt::SkipEmptyParts));
    }
};
#endif

TouchSimuDialog::TouchSimuDialog(const QString &deviceSerial, QWidget *parent)
    : QDialog(parent)
    , m_deviceSerial(deviceSerial)
{
    setupUi();
    retranslateUi();
}

void TouchSimuDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // Top horizontal buttons: Show Touch / Hide Touch
    auto *topBtnLayout = new QHBoxLayout();
    m_showTouchBtn = new QPushButton(this);
    m_hideTouchBtn = new QPushButton(this);
    topBtnLayout->addWidget(m_showTouchBtn);
    topBtnLayout->addWidget(m_hideTouchBtn);
    mainLayout->addLayout(topBtnLayout);

    connect(m_showTouchBtn, &QPushButton::clicked, this, &TouchSimuDialog::onShowTouchClicked);
    connect(m_hideTouchBtn, &QPushButton::clicked, this, &TouchSimuDialog::onHideTouchClicked);

    // Section 1: Tap / Long Press
    m_tapGroupBox = new QGroupBox(this);
    auto *tapLayout = new QVBoxLayout(m_tapGroupBox);
    tapLayout->setSpacing(8);

    auto *tapOpLayout = new QHBoxLayout();
    m_operationTypeLabel = new QLabel(m_tapGroupBox);
    m_operationTypeCombo = new QComboBox(m_tapGroupBox);
    m_operationTypeCombo->addItem(QString());
    m_operationTypeCombo->addItem(QString());
    tapOpLayout->addWidget(m_operationTypeLabel);
    tapOpLayout->addWidget(m_operationTypeCombo, 1);
    tapLayout->addLayout(tapOpLayout);

    auto *tapCoordLayout = new QHBoxLayout();
    auto *coordValidator = new QIntValidator(0, 99999, this);

    m_x1Label = new QLabel(m_tapGroupBox);
    m_x1Edit = new QLineEdit(m_tapGroupBox);
    m_x1Edit->setValidator(coordValidator);
    m_x1Edit->setPlaceholderText(QStringLiteral("0"));

    m_y1Label = new QLabel(m_tapGroupBox);
    m_y1Edit = new QLineEdit(m_tapGroupBox);
    m_y1Edit->setValidator(coordValidator);
    m_y1Edit->setPlaceholderText(QStringLiteral("0"));

    tapCoordLayout->addWidget(m_x1Label);
    tapCoordLayout->addWidget(m_x1Edit);
    tapCoordLayout->addWidget(m_y1Label);
    tapCoordLayout->addWidget(m_y1Edit);
    tapLayout->addLayout(tapCoordLayout);

    auto *durationLayout = new QHBoxLayout();
    m_durationLabel = new QLabel(m_tapGroupBox);
    m_durationEdit = new QLineEdit(m_tapGroupBox);
    m_durationEdit->setValidator(new QIntValidator(1, 60000, this));
    m_durationEdit->setPlaceholderText(QStringLiteral("1000"));
    durationLayout->addWidget(m_durationLabel);
    durationLayout->addWidget(m_durationEdit);
    tapLayout->addLayout(durationLayout);

    // Long press duration is hidden by default (Tap selected initially)
    m_durationLabel->setVisible(false);
    m_durationEdit->setVisible(false);

    m_confirmTapBtn = new QPushButton(m_tapGroupBox);
    tapLayout->addWidget(m_confirmTapBtn, 0, Qt::AlignRight);

    mainLayout->addWidget(m_tapGroupBox);

    connect(m_operationTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TouchSimuDialog::onOperationTypeChanged);
    connect(m_confirmTapBtn, &QPushButton::clicked,
            this, &TouchSimuDialog::onConfirmTapClicked);

    // Section 2: Swipe
    m_swipeGroupBox = new QGroupBox(this);
    auto *swipeLayout = new QVBoxLayout(m_swipeGroupBox);
    swipeLayout->setSpacing(8);

    auto *swipeCoordGrid = new QGridLayout();
    m_sx1Label = new QLabel(m_swipeGroupBox);
    m_sx1Edit = new QLineEdit(m_swipeGroupBox);
    m_sx1Edit->setValidator(coordValidator);
    m_sx1Edit->setPlaceholderText(QStringLiteral("0"));

    m_sy1Label = new QLabel(m_swipeGroupBox);
    m_sy1Edit = new QLineEdit(m_swipeGroupBox);
    m_sy1Edit->setValidator(coordValidator);
    m_sy1Edit->setPlaceholderText(QStringLiteral("0"));

    m_sx2Label = new QLabel(m_swipeGroupBox);
    m_sx2Edit = new QLineEdit(m_swipeGroupBox);
    m_sx2Edit->setValidator(coordValidator);
    m_sx2Edit->setPlaceholderText(QStringLiteral("0"));

    m_sy2Label = new QLabel(m_swipeGroupBox);
    m_sy2Edit = new QLineEdit(m_swipeGroupBox);
    m_sy2Edit->setValidator(coordValidator);
    m_sy2Edit->setPlaceholderText(QStringLiteral("0"));

    swipeCoordGrid->addWidget(m_sx1Label, 0, 0);
    swipeCoordGrid->addWidget(m_sx1Edit, 0, 1);
    swipeCoordGrid->addWidget(m_sy1Label, 0, 2);
    swipeCoordGrid->addWidget(m_sy1Edit, 0, 3);
    swipeCoordGrid->addWidget(m_sx2Label, 1, 0);
    swipeCoordGrid->addWidget(m_sx2Edit, 1, 1);
    swipeCoordGrid->addWidget(m_sy2Label, 1, 2);
    swipeCoordGrid->addWidget(m_sy2Edit, 1, 3);
    swipeLayout->addLayout(swipeCoordGrid);

    auto *swipeDurLayout = new QHBoxLayout();
    m_swipeDurationLabel = new QLabel(m_swipeGroupBox);
    m_swipeDurationEdit = new QLineEdit(m_swipeGroupBox);
    m_swipeDurationEdit->setValidator(new QIntValidator(1, 60000, this));
    m_swipeDurationEdit->setPlaceholderText(QStringLiteral("300"));
    swipeDurLayout->addWidget(m_swipeDurationLabel);
    swipeDurLayout->addWidget(m_swipeDurationEdit);
    swipeLayout->addLayout(swipeDurLayout);

    m_confirmSwipeBtn = new QPushButton(m_swipeGroupBox);
    swipeLayout->addWidget(m_confirmSwipeBtn, 0, Qt::AlignRight);

    mainLayout->addWidget(m_swipeGroupBox);

    connect(m_confirmSwipeBtn, &QPushButton::clicked,
            this, &TouchSimuDialog::onConfirmSwipeClicked);

    resize(420, 480);
}

void TouchSimuDialog::retranslateUi()
{
    setWindowTitle(tr("Touch Simulation"));

    m_showTouchBtn->setText(tr("Show Touch Position"));
    m_hideTouchBtn->setText(tr("Hide Touch Position"));

    m_tapGroupBox->setTitle(tr("Tap / Long Press"));
    m_operationTypeLabel->setText(tr("Operation:"));
    m_operationTypeCombo->setItemText(0, tr("Tap"));
    m_operationTypeCombo->setItemText(1, tr("Long Press"));
    m_x1Label->setText(tr("X:"));
    m_y1Label->setText(tr("Y:"));
    m_durationLabel->setText(tr("Duration (ms):"));
    m_confirmTapBtn->setText(tr("Confirm"));

    m_swipeGroupBox->setTitle(tr("Swipe"));
    m_sx1Label->setText(tr("X1:"));
    m_sy1Label->setText(tr("Y1:"));
    m_sx2Label->setText(tr("X2:"));
    m_sy2Label->setText(tr("Y2:"));
    m_swipeDurationLabel->setText(tr("Duration (ms):"));
    m_confirmSwipeBtn->setText(tr("Confirm"));
}

void TouchSimuDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(event);
}

void TouchSimuDialog::onShowTouchClicked()
{
    QString args;
    if (!m_deviceSerial.isEmpty()) {
        args = QStringLiteral("-s %1 shell settings put system pointer_location 1").arg(m_deviceSerial);
    } else {
        args = QStringLiteral("shell settings put system pointer_location 1");
    }
    AdbRunner::runAdb(args);
}

void TouchSimuDialog::onHideTouchClicked()
{
    QString args;
    if (!m_deviceSerial.isEmpty()) {
        args = QStringLiteral("-s %1 shell settings put system pointer_location 0").arg(m_deviceSerial);
    } else {
        args = QStringLiteral("shell settings put system pointer_location 0");
    }
    AdbRunner::runAdb(args);
}

void TouchSimuDialog::onOperationTypeChanged(int index)
{
    const bool isLongPress = (index == 1);
    m_durationLabel->setVisible(isLongPress);
    m_durationEdit->setVisible(isLongPress);
}

void TouchSimuDialog::onConfirmTapClicked()
{
    const QString x = m_x1Edit->text().trimmed();
    const QString y = m_y1Edit->text().trimmed();
    if (x.isEmpty() || y.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter valid X and Y coordinates."));
        return;
    }

    const QString adbPrefix = m_deviceSerial.isEmpty()
        ? QStringLiteral("adb")
        : QStringLiteral("adb -s %1").arg(m_deviceSerial);

    QString cmd;
    if (m_operationTypeCombo->currentIndex() == 0) {
        // Tap
        cmd = QStringLiteral("%1 shell input tap %2 %3\n"
                             "echo \"Tap: %2 %3\"\n")
                  .arg(adbPrefix, x, y);
    } else {
        // Long Press
        QString duration = m_durationEdit->text().trimmed();
        if (duration.isEmpty()) {
            duration = QStringLiteral("1000");
        }
        cmd = QStringLiteral("%1 shell input swipe %2 %3 %2 %3 %4\n"
                             "echo \"Long press: %2 %3 (%4 ms)\"\n")
                  .arg(adbPrefix, x, y, duration);
    }

    emit commandReady(cmd);
    accept();
}

void TouchSimuDialog::onConfirmSwipeClicked()
{
    const QString sx1 = m_sx1Edit->text().trimmed();
    const QString sy1 = m_sy1Edit->text().trimmed();
    const QString sx2 = m_sx2Edit->text().trimmed();
    const QString sy2 = m_sy2Edit->text().trimmed();

    if (sx1.isEmpty() || sy1.isEmpty() || sx2.isEmpty() || sy2.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter start and end coordinates."));
        return;
    }

    QString duration = m_swipeDurationEdit->text().trimmed();
    if (duration.isEmpty()) {
        duration = QStringLiteral("300");
    }

    const QString adbPrefix = m_deviceSerial.isEmpty()
        ? QStringLiteral("adb")
        : QStringLiteral("adb -s %1").arg(m_deviceSerial);

    const QString cmd = QStringLiteral("%1 shell input swipe %2 %3 %4 %5 %6\n"
                                       "echo \"Swipe: (%2, %3) -> (%4, %5) (%6 ms)\"\n")
                            .arg(adbPrefix, sx1, sy1, sx2, sy2, duration);

    emit commandReady(cmd);
    accept();
}
