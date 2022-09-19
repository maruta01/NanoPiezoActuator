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
std::map<int, string> controller_map;

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

void MainWindow::GetSerailportName(){

}

void MainWindow::initActionsConnections()
{
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionConfigure, &QAction::triggered, ui_settings, &DialogSettingPort::show);
    connect(ui->actionConfigure,SIGNAL(clicked()),SLOT(GetSerailportName()));
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
            InitContorllerConnection();
            QMessageBox::information(this,QString("Connected"),QString("Connected to "+p.name));
            ui->serialport_name_label->setText(p.name);
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
    GetContorllerId();
    GetContorllerName();
    GetCurrentPosition();
//    GetContorllerJog();
}

void MainWindow::GetContorllerName(){
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    ui->name_textBrowser->setText(QString::fromStdString(controller_map.at(contoller_id)));
}

void MainWindow::GetContorllerId(){
    QByteArray res_data;
    for(int num=0;num<=2;num++){
        qDebug() << "num = "<<num;
        res_data = WriteDataToSerialResponse(QByteArray::number(num)[0] + ascii_command_set().ACTUATOR_DESCRIPTION + "?");
        if(res_data.length()>1){
            QList controller_name = res_data.split('?');
            controller_map.insert({ num, controller_name[1].toStdString() });
            ui->contorller_id_comboBox->addItem(QString::number(num));
        }
    }
}
void MainWindow::GetCurrentPosition(){
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    QByteArray res_data = WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().READ_CURRENT_POSITION + "?");
    if(res_data.length()>1){
        QList controller_name = res_data.split('?');
        controller_name[1] = controller_name[1].replace(QByteArray("\n"), QByteArray(""));
        controller_name[1] = controller_name[1].replace(QByteArray("\r"), QByteArray(""));
        ui->current_position_textBrowser->setText(controller_name[1]);
    }
}



QByteArray MainWindow::WriteDataToSerialResponse(QByteArray command){
    serial->write(command+"\r");
    serial->flush();
    serial->waitForBytesWritten(100);
    serial->waitForReadyRead(100);
    return serial->readAll();
}



void MainWindow::GetContorllerJog(){

    serial->write("0" + ascii_command_set().CONTORL_JOG + "?\r");
    serial->flush();
    serial->waitForBytesWritten(50);
    serial->waitForReadyRead(100);
}


void MainWindow::on_Received_Data()
{
    //RX
    qDebug() << serial->readAll();;
}




void MainWindow::on_contorller_id_comboBox_currentIndexChanged(int index)
{
    GetContorllerName();
    GetCurrentPosition();
}

