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
    void GetCurrentPosition();
    void onNumChange(int);

private slots:

    void initActionsConnections();

    void on_ConnectPortButton_clicked();
    void InitContorllerConnection();
    void GetContorllerName();
    void GetContorllerJog();
    int GetContorllerId();
    void OnstartGetCurrentPosition();
    void GetSerailportName();
    void GetControllerStatus();
    QByteArray WriteDataToSerialResponse(QByteArray command);

    void on_motor_pushButton_pressed();

    void on_add_relative_pushButton_clicked();

    void on_del_relative_pushButton_clicked();

    void on_contorller_id_comboBox_currentTextChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
    DialogSettingPort *ui_settings = nullptr;
};
#endif // MAINWINDOW_H
