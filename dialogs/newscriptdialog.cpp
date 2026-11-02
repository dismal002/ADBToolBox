#include "newscriptdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QEvent>

NewScriptDialog::NewScriptDialog(const QString &scriptDir, QWidget *parent)
    : QDialog(parent)
    , m_scriptDir(scriptDir)
{
    setupUi();
    retranslateUi();
}

void NewScriptDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    m_scriptNameLabel = new QLabel(this);
    m_scriptNameEdit = new QLineEdit(this);

    m_authorLabel = new QLabel(this);
    m_authorEdit = new QLineEdit(this);

    m_dateLabel = new QLabel(this);
    m_dateFieldEdit = new QLineEdit(this);
    m_dateFieldEdit->setReadOnly(true);
    m_dateFieldEdit->setText(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));

    formLayout->addRow(m_scriptNameLabel, m_scriptNameEdit);
    formLayout->addRow(m_authorLabel, m_authorEdit);
    formLayout->addRow(m_dateLabel, m_dateFieldEdit);

    mainLayout->addLayout(formLayout);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    m_cancelBtn = new QPushButton(this);
    m_createBtn = new QPushButton(this);
    m_createBtn->setDefault(true);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_createBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_createBtn, &QPushButton::clicked, this, &NewScriptDialog::onCreateClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    resize(400, 200);
}

void NewScriptDialog::retranslateUi()
{
    setWindowTitle(tr("New Script"));

    m_scriptNameLabel->setText(tr("Script Name:"));
    m_scriptNameEdit->setPlaceholderText(tr("Script name"));

    m_authorLabel->setText(tr("Author:"));
    m_authorEdit->setPlaceholderText(tr("Author"));

    m_dateLabel->setText(tr("Date:"));

    m_createBtn->setText(tr("Create"));
    m_cancelBtn->setText(tr("Cancel"));
}

void NewScriptDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(event);
}

void NewScriptDialog::onCreateClicked()
{
    QString name = m_scriptNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a script name."));
        m_scriptNameEdit->setFocus();
        return;
    }

    // Strip trailing .sh if user typed it
    if (name.endsWith(QStringLiteral(".sh"), Qt::CaseInsensitive)) {
        name.chop(3);
    }
#ifdef Q_OS_WIN
    if (name.endsWith(QStringLiteral(".cmd"), Qt::CaseInsensitive)) {
        name.chop(4);
    }
#endif

    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Invalid script name."));
        return;
    }

    // Determine target directory: use m_scriptDir if specified, otherwise appDir/Script
    QString targetDir = m_scriptDir.trimmed();
    if (targetDir.isEmpty()) {
        targetDir = QCoreApplication::applicationDirPath() + QStringLiteral("/Script");
    }

    QDir dir(targetDir);
    if (!dir.exists()) {
        if (!dir.mkpath(QStringLiteral("."))) {
            QMessageBox::critical(this, tr("Error"),
                tr("Failed to create destination directory:\n%1").arg(targetDir));
            return;
        }
    }

#ifdef Q_OS_WIN
    const QString fullFileName = name + QStringLiteral(".cmd");
#else
    const QString fullFileName = name + QStringLiteral(".sh");
#endif
    const QString filePath = dir.filePath(fullFileName);

    if (QFile::exists(filePath)) {
        QMessageBox::warning(this, tr("Warning"),
            tr("File already exists:\n%1").arg(filePath));
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to create script file:\n%1\nError: %2").arg(filePath, file.errorString()));
        return;
    }

    const QString author = m_authorEdit->text().trimmed();
    const QString dateStr = m_dateFieldEdit->text().trimmed();

    QTextStream out(&file);
#ifdef Q_OS_WIN
    out << "@echo off\r\n"
        << "REM Script: " << fullFileName << "\r\n"
        << "REM Author: " << author << "\r\n"
        << "REM Date: " << dateStr << "\r\n"
        << "REM ===== Edit below =====\r\n\r\n";
#else
    out << "#!/bin/bash\n"
        << "# Script: " << fullFileName << "\n"
        << "# Author: " << author << "\n"
        << "# Date: " << dateStr << "\n"
        << "# ===== Edit below =====\n\n";
#endif
    file.close();

    // chmod +x (rwxr-xr-x)
    const QFileDevice::Permissions permissions =
        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
        QFileDevice::ReadGroup | QFileDevice::ExeGroup |
        QFileDevice::ReadOther | QFileDevice::ExeOther;

    file.setPermissions(permissions);

    emit scriptCreated(filePath);
    emit commandReady(filePath);
    accept();
}
