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
    void ShowWaringLabel(bool);
    void UpdatePosition();
    QByteArray WriteDataToSerialResponse(QByteArray command,bool query);

    void on_add_relative_pushButton_clicked();
    void on_ConnectPortButton_clicked();
    void on_contorller_id_comboBox_currentTextChanged(const QString &arg1);
    void on_del_relative_pushButton_clicked();
    void on_motor_pushButton_pressed();
    void on_save_limit_pushButton_clicked();
    void on_set_zero_pushButton_clicked();
    void on_restore_default_pushButton_clicked();

    void on_save_setting_pushButton_clicked();
    void writeSettings();
    void readSettings();

    void on_move_postition_pushButton_clicked();

private:
    int position_history = 0;
    Ui::MainWindow *ui;
    DialogSettingPort *ui_settings = nullptr;
};
#endif // MAINWINDOW_H
