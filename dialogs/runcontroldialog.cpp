#include "runcontroldialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QDoubleValidator>
#include <QMessageBox>
#include <QEvent>

RunControlDialog::RunControlDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    retranslateUi();
}

void RunControlDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // Section 1: Delay
    m_delayGroup = new QGroupBox(this);
    auto *delayLayout = new QHBoxLayout(m_delayGroup);
    delayLayout->setSpacing(8);

    m_delayLabel = new QLabel(m_delayGroup);
    m_delaySecondsEdit = new QLineEdit(m_delayGroup);
    auto *delayValidator = new QDoubleValidator(0.1, 86400.0, 2, this);
    delayValidator->setNotation(QDoubleValidator::StandardNotation);
    m_delaySecondsEdit->setValidator(delayValidator);

    m_confirmDelayBtn = new QPushButton(m_delayGroup);

    delayLayout->addWidget(m_delayLabel);
    delayLayout->addWidget(m_delaySecondsEdit, 1);
    delayLayout->addWidget(m_confirmDelayBtn);

    mainLayout->addWidget(m_delayGroup);

    // Section 2: Loop
    m_loopGroup = new QGroupBox(this);
    auto *loopLayout = new QVBoxLayout(m_loopGroup);
    loopLayout->setSpacing(10);

    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(8);

    m_varNameLabel = new QLabel(m_loopGroup);
    m_varNameEdit = new QLineEdit(m_loopGroup);
    m_varNameEdit->setText(QStringLiteral("i"));

    m_loopCountLabel = new QLabel(m_loopGroup);
    m_loopCountSpin = new QSpinBox(m_loopGroup);
    m_loopCountSpin->setRange(1, 9999);
    m_loopCountSpin->setValue(10);

    formLayout->addRow(m_varNameLabel, m_varNameEdit);
    formLayout->addRow(m_loopCountLabel, m_loopCountSpin);
    loopLayout->addLayout(formLayout);

    m_confirmLoopBtn = new QPushButton(m_loopGroup);
    loopLayout->addWidget(m_confirmLoopBtn, 0, Qt::AlignRight);

    mainLayout->addWidget(m_loopGroup);

    // Connections
    connect(m_confirmDelayBtn, &QPushButton::clicked,
            this, &RunControlDialog::onConfirmDelayClicked);
    connect(m_confirmLoopBtn, &QPushButton::clicked,
            this, &RunControlDialog::onConfirmLoopClicked);

    resize(380, 260);
}

void RunControlDialog::retranslateUi()
{
    setWindowTitle(tr("Script Control"));

    m_delayGroup->setTitle(tr("Delay"));
    m_delayLabel->setText(tr("Seconds:"));
    m_delaySecondsEdit->setPlaceholderText(tr("Seconds"));
    m_confirmDelayBtn->setText(tr("Confirm"));

    m_loopGroup->setTitle(tr("Loop"));
    m_varNameLabel->setText(tr("Variable:"));
    m_varNameEdit->setPlaceholderText(tr("Variable name"));
    m_loopCountLabel->setText(tr("Count:"));
    m_confirmLoopBtn->setText(tr("Confirm"));
}

void RunControlDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(event);
}

void RunControlDialog::onConfirmDelayClicked()
{
    const QString seconds = m_delaySecondsEdit->text().trimmed();
    if (seconds.isEmpty() || seconds.toDouble() <= 0) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a valid delay duration in seconds."));
        return;
    }

    const QString cmd = QStringLiteral("echo \"Delay %1 second(s)\"\n"
                                       "sleep %1\n")
                            .arg(seconds);

    emit commandReady(cmd);
    accept();
}

void RunControlDialog::onConfirmLoopClicked()
{
    QString varName = m_varNameEdit->text().trimmed();
    if (varName.isEmpty()) {
        varName = QStringLiteral("i");
    }

    // Validate bash variable name: must start with letter/underscore and contain alphanumeric/underscores
    static const QRegularExpression varRegex(QStringLiteral("^[a-zA-Z_][a-zA-Z0-9_]*$"));
    if (!varRegex.match(varName).hasMatch()) {
        QMessageBox::warning(this, tr("Warning"),
            tr("Invalid variable name. Variable names must start with a letter or underscore and contain only alphanumeric characters and underscores."));
        return;
    }

    const int count = m_loopCountSpin->value();
    const QString cmd = QStringLiteral("for %1 in $(seq 1 %2); do\n"
                                       "    echo \"Iteration $%1\"\n"
                                       "done\n")
                            .arg(varName, QString::number(count));

    emit commandReady(cmd);
    accept();
}
