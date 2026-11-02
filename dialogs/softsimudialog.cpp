#include "softsimudialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QEvent>

SoftSimuDialog::SoftSimuDialog(const QString &deviceSerial, QWidget *parent)
    : QDialog(parent)
    , m_deviceSerial(deviceSerial)
{
    setupUi();
    retranslateUi();
}

void SoftSimuDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    m_operationTypeLabel = new QLabel(this);
    m_operationTypeCombo = new QComboBox(this);
    m_operationTypeCombo->addItem(QString());
    m_operationTypeCombo->addItem(QString());
    m_operationTypeCombo->addItem(QString());
    m_operationTypeCombo->addItem(QString());
    formLayout->addRow(m_operationTypeLabel, m_operationTypeCombo);

    m_packageLabel = new QLabel(this);
    m_packageNameEdit = new QLineEdit(this);
    formLayout->addRow(m_packageLabel, m_packageNameEdit);

    mainLayout->addLayout(formLayout);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    m_cancelBtn = new QPushButton(this);
    m_confirmBtn = new QPushButton(this);
    m_confirmBtn->setDefault(true);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_confirmBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_operationTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SoftSimuDialog::onOperationTypeChanged);
    connect(m_confirmBtn, &QPushButton::clicked,
            this, &SoftSimuDialog::onConfirmClicked);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &QDialog::reject);

    resize(420, 160);
}

void SoftSimuDialog::retranslateUi()
{
    setWindowTitle(tr("App Control"));

    m_operationTypeLabel->setText(tr("Operation:"));
    m_operationTypeCombo->setItemText(0, tr("Launch App"));
    m_operationTypeCombo->setItemText(1, tr("Launch Activity"));
    m_operationTypeCombo->setItemText(2, tr("Force Stop"));
    m_operationTypeCombo->setItemText(3, tr("Uninstall"));

    m_packageLabel->setText(tr("Package:"));
    m_confirmBtn->setText(tr("Confirm"));
    m_cancelBtn->setText(tr("Cancel"));

    updatePlaceholder();
}

void SoftSimuDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(event);
}

void SoftSimuDialog::updatePlaceholder()
{
    if (m_operationTypeCombo->currentIndex() == 1) {
        m_packageNameEdit->setPlaceholderText(QStringLiteral("package.name/.ActivityName"));
    } else {
        m_packageNameEdit->setPlaceholderText(QStringLiteral("package.name"));
    }
}

void SoftSimuDialog::onOperationTypeChanged(int index)
{
    Q_UNUSED(index);
    updatePlaceholder();
}

void SoftSimuDialog::onConfirmClicked()
{
    const QString pkg = m_packageNameEdit->text().trimmed();
    if (pkg.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a package name."));
        return;
    }

    const QString adbPrefix = m_deviceSerial.isEmpty()
        ? QStringLiteral("adb shell")
        : QStringLiteral("adb -s %1 shell").arg(m_deviceSerial);

    QString cmd;
    const int opIndex = m_operationTypeCombo->currentIndex();
    switch (opIndex) {
    case 0:
        // Launch App
        cmd = QStringLiteral("%1 monkey -p %2 -c android.intent.category.LAUNCHER 1\n"
                             "echo \"Launch app: %2\"\n")
                  .arg(adbPrefix, pkg);
        break;
    case 1:
        // Launch Activity
        cmd = QStringLiteral("%1 am start -n %2\n"
                             "echo \"Launch activity: %2\"\n")
                  .arg(adbPrefix, pkg);
        break;
    case 2:
        // Force Stop
        cmd = QStringLiteral("%1 am force-stop %2\n"
                             "echo \"Force stop: %2\"\n")
                  .arg(adbPrefix, pkg);
        break;
    case 3:
        // Uninstall
        cmd = QStringLiteral("%1 pm uninstall %2\n"
                             "echo \"Uninstall: %2\"\n")
                  .arg(adbPrefix, pkg);
        break;
    default:
        break;
    }

    emit commandReady(cmd);
    accept();
}
