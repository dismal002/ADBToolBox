#ifndef NEWSCRIPTDIALOG_H
#define NEWSCRIPTDIALOG_H

#include <QDialog>
#include <QString>

class QLineEdit;
class QPushButton;
class QLabel;
class QEvent;

class NewScriptDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewScriptDialog(const QString &scriptDir, QWidget *parent = nullptr);
    ~NewScriptDialog() override = default;

signals:
    void scriptCreated(const QString &filePath);
    void commandReady(const QString &cmd);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onCreateClicked();

private:
    void setupUi();
    void retranslateUi();

    QString m_scriptDir;

    // Form labels and inputs stored as members
    QLabel *m_scriptNameLabel = nullptr;
    QLineEdit *m_scriptNameEdit = nullptr;
    QLabel *m_authorLabel = nullptr;
    QLineEdit *m_authorEdit = nullptr;
    QLabel *m_dateLabel = nullptr;
    QLineEdit *m_dateFieldEdit = nullptr;

    // Buttons
    QPushButton *m_createBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;
};

#endif // NEWSCRIPTDIALOG_H
