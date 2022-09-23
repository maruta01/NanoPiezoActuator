#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialogsettingport.h"
#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDebug>
#include <QProcess>
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
    InitActionsConnections();
    workerthread = new WorkerThread(this);

    connect(workerthread,SIGNAL(NumberChanged(int)),this,SLOT(ShowCurrentPosition(int)));
    connect(ui_settings,SIGNAL(SerialPortChanged(QString)),this,SLOT(GetSerialNameChange(QString)));

}

MainWindow::~MainWindow()
{
    workerthread->stop = true;
    serial->close();
    delete ui;
    delete ui_settings;
}

void MainWindow::InitActionsConnections()
{
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionConfigure, &QAction::triggered, ui_settings, &DialogSettingPort::show);
}



void MainWindow::on_ConnectPortButton_clicked()
{
    if(ui->ConnectPortButton->text()=="Connect"){
        const DialogSettingPort::Settings p = ui_settings->settings();
        if(p.settingStatus){
            serial->setPortName(p.name);
            serial->setBaudRate(p.baudRate);
            serial->setDataBits(p.dataBits);
            serial->setParity(p.parity);
            serial->setStopBits(p.stopBits);
            serial->setFlowControl(p.flowControl);
            if (serial->open(QIODevice::ReadWrite)) {
                serial_connect=true;
                workerthread->stop = false;
                InitContorllerConnection();
                ui->ConnectPortButton->setText("Connected");
                ui->ConnectPortButton->setEnabled(false);
            } else {
                QMessageBox::critical(this, QString("Error"), serial->errorString());
            }
        }
        else{
            serial_connect=false;
        }
    }
    else{
        workerthread->stop = true;
        serial->close();
        ui->ConnectPortButton->setText("Connect");
    }
}

void MainWindow::InitContorllerConnection()
{
    if(GetContorllerId()){
        QMessageBox::information(this,QString("Connected"),QString("NanoPZ Actuator Connected"));
        GetContorllerName();
        GetTravelLimit();
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
        res_data = WriteDataToSerialResponse(QByteArray::number(num)[0] + ascii_command_set().ACTUATOR_DESCRIPTION, true);
        qDebug()<<"GetContorllerId = "<<num<<"/"<< res_data;
        if(!res_data.isEmpty()){
            num_controller++;
            QList controller_name = res_data.split('?');
            if(controller_name.length()>1)
            {
                controller_map.insert({ num, controller_name[1].toStdString() });
                ui->contorller_id_comboBox->addItem(QString::number(num));
                ShowWaringLabel(true);
            }
            else
            {
                ShowWaringLabel(false);
            }
        }
    }
    return num_controller;
}

void MainWindow::GetControllerStatus(){
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    QByteArray res_data = WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().CONTORLLER_STATUS, true);
    if(!res_data.isEmpty()){
        QList data_status = res_data.split('?');
        if(data_status.length()>1){
            qDebug()<<"mo status="<<data_status[1];
            if(data_status[1] == QByteArray("Q") || data_status[1] == QByteArray("P"))
            {
                ui->motor_pushButton->setText("Motor OFF");
                ui->motor_status_textBrowser->setText("Motor ON");
            }
            else{
                ui->motor_pushButton->setText("Motor ON");
                ui->motor_status_textBrowser->setText("Motor OFF");
            }
            ShowWaringLabel(true);
        }
        else{
            ShowWaringLabel(false);
        }
    }
}

void MainWindow::OnstartGetCurrentPosition(){
    workerthread->serial = serial;
    workerthread->controller_id = ui->contorller_id_comboBox->currentText().toInt();
    workerthread->start();
}

void MainWindow::ShowCurrentPosition(int value)
{
    ui->current_position_textBrowser->setText(QString::number(value));
}

void MainWindow::GetSerialNameChange(QString serial_name)
{
    ui->serialport_name_label->setText(serial_name);
}



QByteArray MainWindow::WriteDataToSerialResponse(QByteArray command,bool query=false){

    if (query) command += "?";
    qDebug()<<"commeand= "<<command;
    serial->write(command+"\r");
    while(!serial->waitForBytesWritten(300)){
        qDebug()<<"whait write ..";
    }
    serial->flush();
    while(serial->waitForReadyRead(300)){
        qDebug()<<"whait read ..";
    }
    QByteArray data_response = serial->readAll().replace(QByteArray("\n"), QByteArray("")).replace(QByteArray("\r"), QByteArray("")).replace(QByteArray(" "), QByteArray(""));
    qDebug()<<"data_response"<<data_response;
    return data_response;
}


void MainWindow::GetContorllerJog()
{
    serial->write("0" + ascii_command_set().CONTORL_JOG + "?\r");
    serial->flush();
    serial->waitForBytesWritten(100);
    serial->waitForReadyRead(100);
}

void MainWindow::GetTravelLimit()
{
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    QByteArray right_res_data = WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().SET_POSITIVE_TRAVEL_LIMIT, true);
    if(!right_res_data.isEmpty()){
        QList right_data = right_res_data.split('?');
        if(right_data.length()>1)
        {
            ui->right_travel_limit_spinBox->setValue(right_data[1].toInt());
            ShowWaringLabel(true);
        }
        else{
            ShowWaringLabel(false);
        }

    }

    QByteArray left_res_data = WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().SET_NEGATIVE_TRAVEL_LIMIT, true);
    if(!left_res_data.isEmpty()){
        QList left_data = left_res_data.split('?');
        if(left_data.length()>1){
            ui->left_travel_limit_spinBox->setValue(left_data[1].toInt());
            ShowWaringLabel(true);
        }
        else{
            ShowWaringLabel(false);
        }
    }
}

void MainWindow::ShowWaringLabel(bool is_access){
    if(is_access){
        ui->warning_label->setStyleSheet("QLabel { color : green; }");
        ui->warning_label->setText("accessed to Nano-piezo Actuators.");
    }
    else{
        ui->warning_label->setStyleSheet("QLabel { color : red; }");
        ui->warning_label->setText("try agin!, we can't access data to Nano-piezo.");
    }

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
        GetTravelLimit();
        workerthread->controller_id = arg1.toInt();
}


void MainWindow::on_set_zero_pushButton_clicked()
{
    QMessageBox::StandardButton reply;
     reply = QMessageBox::question(this, "Warning", "Set current position to zero?",
                                   QMessageBox::Yes|QMessageBox::No);
     if (reply == QMessageBox::Yes) {
         int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
         WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().ZERO_POSITION);
     }

}


void MainWindow::on_save_limit_pushButton_clicked()
{
    QMessageBox::StandardButton reply;
     reply = QMessageBox::question(this, "Warning", "Are you want to save travel limit?",
                                   QMessageBox::Yes|QMessageBox::No);
     if (reply == QMessageBox::Yes) {
         int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
         int right_limit_value = ui->right_travel_limit_spinBox->value();
         int left_limit_value = ui->left_travel_limit_spinBox->value();
         WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().SET_POSITIVE_TRAVEL_LIMIT + QByteArray::number(right_limit_value));
         WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().SET_NEGATIVE_TRAVEL_LIMIT + QByteArray::number(left_limit_value));
     }
}


void MainWindow::on_restore_default_pushButton_clicked()
{
    QMessageBox::StandardButton reply;
     reply = QMessageBox::question(this, "Warning", "The program will restart now, Reset controller to default ?",
                                   QMessageBox::Yes|QMessageBox::No);
     if (reply == QMessageBox::Yes) {
         int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
         WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().RESET_CONTORLLER);
         qApp->exit();
         QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
     }

}

