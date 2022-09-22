#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialogsettingport.h"
#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDebug>
#include <unistd.h>
#include <QtCore/QCoreApplication>
#include "wokerthead.h"
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
    QByteArray MOTOR_ON = "MO";
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
    workerthread = new WorkerThread(this);
    connect(workerthread,SIGNAL(NumberChanged(int)),this,SLOT(onNumChange(int)));


}

MainWindow::~MainWindow()
{
    workerthread->stop = true;
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
    if(GetContorllerId()){
        QMessageBox::information(this,QString("Connected"),QString("NanoPZ Actuator Connected"));
        GetContorllerName();
        OnstartGetCurrentPosition();
    }
    else{
        QMessageBox::warning(this, QString("Error"), "don't find the controller! try to re-plug the serial port or check the controller.");
    }


}

void MainWindow::GetContorllerName(){
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    ui->name_textBrowser->setText(QString::fromStdString(controller_map.at(contoller_id)));
}

int MainWindow::GetContorllerId(){
    QByteArray res_data;
    ui->contorller_id_comboBox->clear();
    int num_controller=0;
    for(int num=0;num<=3;num++){
        res_data = WriteDataToSerialResponse(QByteArray::number(num)[0] + ascii_command_set().ACTUATOR_DESCRIPTION + "?");
        qDebug()<<"GetContorllerId = "<<num<<"/"<< res_data;
        if(!res_data.isEmpty()){
            num_controller++;
            QList controller_name = res_data.split('?');
            controller_map.insert({ num, controller_name[1].toStdString() });
            ui->contorller_id_comboBox->addItem(QString::number(num));
        }
    }
    return num_controller;
}

void MainWindow::GetCurrentPosition(){
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    QByteArray res_data = WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().READ_CURRENT_POSITION + "?");
    if(!res_data.isEmpty()){
        QList controller_name = res_data.split('?');
        qDebug()<<"controller_name"<<controller_name[1];
        ui->current_position_textBrowser->setText(controller_name[1]);
    }
}

void MainWindow::GetControllerStatus(){
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    QByteArray res_data = WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().CONTORLLER_STATUS + "?");
    if(!res_data.isEmpty()){
        QList controller_name = res_data.split('?');
        qDebug()<<"mo status="<<controller_name[1];
        if(controller_name[1] == QByteArray("Q") || controller_name[1] == QByteArray("P")){
            ui->motor_pushButton->setText("Motor OFF");
            ui->motor_status_textBrowser->setText("Motor ON");
        }
        else{
            ui->motor_pushButton->setText("Motor ON");
            ui->motor_status_textBrowser->setText("Motor OFF");
        }
    }
}

void MainWindow::OnstartGetCurrentPosition(){
    workerthread->serial = serial;
    workerthread->controller_id = ui->contorller_id_comboBox->currentText().toInt();
    workerthread->start();
}

void MainWindow::onNumChange(int value)
{
//    ui->label->setText(QString::number(value));
    ui->current_position_textBrowser->setText(QString::number(value));
}



QByteArray MainWindow::WriteDataToSerialResponse(QByteArray command){
    qDebug()<<"commeand= "<<command;
    serial->write(command+"\r");
    while(!serial->waitForBytesWritten(100)){
        usleep(100);
        qDebug()<<"whait write ..";
    }

    while(serial->waitForReadyRead(100)){
        usleep(100);
        qDebug()<<"whait read ..";
    }
    serial->flush();
    return serial->readAll().replace(QByteArray("\n"), QByteArray("")).replace(QByteArray("\r"), QByteArray("")).replace(QByteArray(" "), QByteArray(""));;

}


void MainWindow::GetContorllerJog(){

    serial->write("0" + ascii_command_set().CONTORL_JOG + "?\r");
    serial->flush();
    serial->waitForBytesWritten(100);
    serial->waitForReadyRead(100);
}


void MainWindow::on_motor_pushButton_pressed()
{
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    string current_status = ui->motor_status_textBrowser->toPlainText().toStdString();
    string curent_button = ui->motor_pushButton->text().toStdString();

    if(current_status == "Motor ON" && curent_button == "Motor OFF"){
        WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().MOTOR_OFF);
    }
    else if(current_status == "Motor OFF" && curent_button == "Motor ON"){
        WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().MOTOR_ON);
    }
    GetControllerStatus();
}


void MainWindow::on_add_relative_pushButton_clicked()
{
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    string current_status = ui->motor_status_textBrowser->toPlainText().toStdString();
    int increase_value = ui->increament_spinBox->value();

    if(current_status == "Motor ON"){
        qDebug() << "move+ = "<<increase_value;
        WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().POSITION_RELATIVE+QByteArray::number(increase_value));
    }
}


void MainWindow::on_del_relative_pushButton_clicked()
{
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    string current_status = ui->motor_status_textBrowser->toPlainText().toStdString();
    int decrease_value = ui->increament_spinBox->value();

    if(current_status == "Motor ON"){
        qDebug() << "move- = "<<decrease_value;
        WriteDataToSerialResponse(QByteArray::number(contoller_id)+ascii_command_set().POSITION_RELATIVE+"-"+QByteArray::number(decrease_value));
    }

}



void MainWindow::on_contorller_id_comboBox_currentTextChanged(const QString &arg1)
{
        GetContorllerName();
        GetControllerStatus();
        workerthread->controller_id = arg1.toInt();
}

