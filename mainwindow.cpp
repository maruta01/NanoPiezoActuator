#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialogsettingport.h"
#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDebug>
#include <unistd.h>

using namespace std;

QSerialPort* serial;
bool serial_connect = false;

struct ascii_command_set {
    // Command Set Summary (User Manual : http://shorturl.at/prXYZ
    // Note = add '\r' after finish command example: 1ID?\r
    QByteArray SCAN_SWITCHBOX = "BX";
    QByteArray RESTORE_EEPROM = "BZ";
    QByteArray ACTUATOR_DESCRIPTION = "ID";
    QByteArray CONTORL_JOG = "JA";
    QByteArray MOTOR_OFF = "MF";
    QByteArray MOTOR_NO = "MO";
    QByteArray SELECT_SWITCHBOX = "MX";
    QByteArray ZERO_POSITION = "OR";
    QByteArray GET_HARDWARE_STATUS = "PH";
    QByteArray POSITION_RELATIVE = "PR";
    QByteArray RESET_CONTORLLER = "RS";
    QByteArray SET_CONTORLLER_ADDRESS = "SA";
    QByteArray SET_NEGATIVE_TRAVEL_LIMIT = "SL";
    QByteArray SAVE_SETTING = "SM";
    QByteArray SET_POSITIVE_TRAVEL_LIMIT = "SR";
    QByteArray STOP_MOTION = "ST";
    QByteArray READ_ERROR_CODE = "TE";
    QByteArray READ_CURRENT_POSITION = "TP";
    QByteArray CONTORLLER_STATUS = "TS";
    QByteArray READ_CONTORLLER_FIRMWARE = "VE";
};


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    ui_settings(new DialogSettingPort)
{
    ui_settings->show();
    ui->setupUi(this);
    serial = new QSerialPort(this);
    initActionsConnections();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete ui_settings;
    serial->close();
}


void MainWindow::initActionsConnections()
{
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionConfigure, &QAction::triggered, ui_settings, &DialogSettingPort::show);
//    connect(serial, SIGNAL(readyRead()), this, SLOT(on_Received_Data()));
}

void MainWindow::on_ConnectPortButton_clicked()
{
    const DialogSettingPort::Settings p = ui_settings->settings();
    if(p.settingStatus){
        serial->setPortName(p.name);
        serial->setBaudRate(p.baudRate);
        serial->setDataBits(p.dataBits);
        serial->setParity(p.parity);
        serial->setStopBits(p.stopBits);
        serial->setFlowControl(p.flowControl);
        if (serial->open(QIODevice::ReadWrite)) {
            ui->ConnectPortButton->setText("Disconnect");
            serial_connect=true;
            QMessageBox::information(this,QString("Connected"),QString("Connected to "+p.name));
            InitContorllerConnection();
        } else {
            QMessageBox::critical(this, QString("Error"), serial->errorString());
        }
    }
    else{
        serial_connect=false;
    }
}

void MainWindow::InitContorllerConnection()
{
    GetContorllerName();
    GetContorllerJog();
}

void MainWindow::GetContorllerName(){
    serial->write("0" + ascii_command_set().ACTUATOR_DESCRIPTION + "?\r");
    serial->flush();
    serial->waitForBytesWritten(50);
    serial->waitForReadyRead(50);
    QByteArray data = serial->readAll();
}

void MainWindow::GetContorllerJog(){

    serial->write("0" + ascii_command_set().CONTORL_JOG + "?\r");
    serial->flush();
    serial->waitForBytesWritten(50);
    serial->waitForReadyRead(50);
    qDebug() << serial->readAll();;

}


void MainWindow::on_Received_Data()
{
    //RX
    qDebug() << serial->readAll();;
}



