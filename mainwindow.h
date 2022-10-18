#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <iostream>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class DialogSettingPort;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    int divide_time_wait_actuator_step = 40000;

public slots:
    void GetSerialNameChange(QString);
    void ShowCurrentPosition(int);

private slots:

    void ConnectSerialport();
    int GetContorllerId();
    void GetContorllerJog();
    void GetContorllerName();
    int GetCurrentPosition(int contoller_id);
    void GetTravelLimit();
    void GetControllerStatus();
    void InitActionsConnections();
    void InitContorllerConnection();
    void MoveToPosition(int position_value);
    void ReadSettingsFile();
    void ShowWaringLabel(bool);
    void UpdatePosition();
    QByteArray WriteDataToSerialResponse(QByteArray command,bool query);
    void WriteSettingsFile();

    void on_add_relative_pushButton_clicked();
    void on_ConnectPortButton_clicked();
    void on_contorller_id_comboBox_currentTextChanged(const QString &arg1);
    void on_del_relative_pushButton_clicked();
    void on_motor_pushButton_pressed();
    void on_set_zero_pushButton_clicked();
    void on_restore_default_pushButton_clicked();
    void on_save_setting_pushButton_clicked();
    void on_move_postition_pushButton_clicked();
    void on_default_in_contact_pushButton_clicked();


    void on_default_out_contact_pushButton_clicked();

    void on_default_in_contact_checkBox_stateChanged(int arg1);

    void on_default_out_contact_checkBox_stateChanged(int arg1);

    void on_right_travel_limit_checkBox_stateChanged(int arg1);

    void on_left_travel_limit_checkBox_stateChanged(int arg1);

private:
    int position_history = 0;
    Ui::MainWindow *ui;
    DialogSettingPort *ui_settings = nullptr;
};
#endif // MAINWINDOW_H
