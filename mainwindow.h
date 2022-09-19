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

private slots:
    void initActionsConnections();

    void on_ConnectPortButton_clicked();
    void InitContorllerConnection();
    void on_Received_Data();
    void GetContorllerName();
    void GetContorllerJog();
    void GetContorllerId();
    void GetCurrentPosition();
    void GetSerailportName();
    QByteArray WriteDataToSerialResponse(QByteArray command);

    void on_contorller_id_comboBox_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    DialogSettingPort *ui_settings = nullptr;
};
#endif // MAINWINDOW_H
