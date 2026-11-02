#ifndef SOFTSIMUDIALOG_H
#define SOFTSIMUDIALOG_H

#include <QDialog>
#include <QString>

class QComboBox;
class QLineEdit;
class QPushButton;
class QLabel;
class QEvent;

class SoftSimuDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SoftSimuDialog(const QString &deviceSerial, QWidget *parent = nullptr);
    ~SoftSimuDialog() override = default;

signals:
    void commandReady(const QString &cmd);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onOperationTypeChanged(int index);
    void onConfirmClicked();

private:
    void setupUi();
    void retranslateUi();
    void updatePlaceholder();

    QString m_deviceSerial;

    // UI widgets stored as members
    QLabel *m_operationTypeLabel = nullptr;
    QComboBox *m_operationTypeCombo = nullptr;
    QLabel *m_packageLabel = nullptr;
    QLineEdit *m_packageNameEdit = nullptr;
    QPushButton *m_confirmBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;
};

#endif // SOFTSIMUDIALOG_H
