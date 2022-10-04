#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <iostream>
#include "wokerthead.h"


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

    WorkerThread *workerthread;

public slots:
    void GetSerialNameChange(QString);
    void ShowCurrentPosition(int);

private slots:

    int GetContorllerId();
    void GetContorllerJog();
    void GetContorllerName();
    void GetTravelLimit();
    void GetControllerStatus();
    void InitActionsConnections();
    void InitContorllerConnection();
    void OnstartGetCurrentPosition();
    void ShowWaringLabel(bool);
    QByteArray WriteDataToSerialResponse(QByteArray command,bool query);

    void on_add_relative_pushButton_clicked();
    void on_ConnectPortButton_clicked();
    void on_contorller_id_comboBox_currentTextChanged(const QString &arg1);
    void on_del_relative_pushButton_clicked();
    void on_motor_pushButton_pressed();
    void on_save_limit_pushButton_clicked();
    void on_set_zero_pushButton_clicked();

    void on_restore_default_pushButton_clicked();

    void TestWriteData(QByteArray command);
    void TastResponseData();

    int GetCurrentPosition(int contoller_id);
    void UpdatePosition();

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    DialogSettingPort *ui_settings = nullptr;
};
#endif // MAINWINDOW_H
