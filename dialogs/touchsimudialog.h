#ifndef TOUCHSIMUDIALOG_H
#define TOUCHSIMUDIALOG_H

#include <QDialog>
#include <QString>

class QPushButton;
class QComboBox;
class QLineEdit;
class QLabel;
class QGroupBox;
class QEvent;

class TouchSimuDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TouchSimuDialog(const QString &deviceSerial, QWidget *parent = nullptr);
    ~TouchSimuDialog() override = default;

signals:
    void commandReady(const QString &cmd);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onShowTouchClicked();
    void onHideTouchClicked();
    void onOperationTypeChanged(int index);
    void onConfirmTapClicked();
    void onConfirmSwipeClicked();

private:
    void setupUi();
    void retranslateUi();

    QString m_deviceSerial;

    // Top control buttons
    QPushButton *m_showTouchBtn = nullptr;
    QPushButton *m_hideTouchBtn = nullptr;

    // Tap / Long Press section widgets
    QGroupBox *m_tapGroupBox = nullptr;
    QLabel *m_operationTypeLabel = nullptr;
    QComboBox *m_operationTypeCombo = nullptr;
    QLabel *m_x1Label = nullptr;
    QLineEdit *m_x1Edit = nullptr;
    QLabel *m_y1Label = nullptr;
    QLineEdit *m_y1Edit = nullptr;
    QLabel *m_durationLabel = nullptr;
    QLineEdit *m_durationEdit = nullptr;
    QPushButton *m_confirmTapBtn = nullptr;

    // Swipe section widgets
    QGroupBox *m_swipeGroupBox = nullptr;
    QLabel *m_sx1Label = nullptr;
    QLineEdit *m_sx1Edit = nullptr;
    QLabel *m_sy1Label = nullptr;
    QLineEdit *m_sy1Edit = nullptr;
    QLabel *m_sx2Label = nullptr;
    QLineEdit *m_sx2Edit = nullptr;
    QLabel *m_sy2Label = nullptr;
    QLineEdit *m_sy2Edit = nullptr;
    QLabel *m_swipeDurationLabel = nullptr;
    QLineEdit *m_swipeDurationEdit = nullptr;
    QPushButton *m_confirmSwipeBtn = nullptr;
};

#endif // TOUCHSIMUDIALOG_H
