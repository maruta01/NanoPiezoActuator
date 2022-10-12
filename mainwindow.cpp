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
#include <QProgressDialog>
#include <QTimer>

#include <QSettings>


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
    this->setWindowTitle("NanoPZ (Optics)");
    serial = new QSerialPort(this);
    InitActionsConnections();
    connect(ui_settings,SIGNAL(SerialPortChanged(QString)),this,SLOT(GetSerialNameChange(QString)));

}

MainWindow::~MainWindow()
{
    serial->close();
    writeSettings();
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
        QMessageBox msg;
        int cnt =1;
        msg.setText("<p align='center'>Actuator Connecting...</p>");
        msg.setStandardButtons(QMessageBox::NoButton);
        msg.setDefaultButton(QMessageBox::Ok);
        msg.setWindowFlags(Qt::BypassWindowManagerHint);
        msg.setStyleSheet("QLabel{min-width: 200px;}");
        QTimer cntDown;
        QObject::connect(&cntDown, &QTimer::timeout, [&msg,&cnt, &cntDown, this]()->void{
                             if(--cnt < 0){
                                 cntDown.stop();
                                 ConnectSerialport();
                                 msg.accept();
                                 readSettings();
                             }
                         });


        cntDown.start(1000);
        msg.exec();
    }
    else{
        serial->close();
        ui->ConnectPortButton->setText("Connect");
    }
}

void MainWindow::ConnectSerialport(){
    const DialogSettingPort::Settings p = ui_settings->settings();
    sleep(1);
    if(p.settingStatus){
        serial->setPortName(p.name);
        serial->setBaudRate(p.baudRate);
        serial->setDataBits(p.dataBits);
        serial->setParity(p.parity);
        serial->setStopBits(p.stopBits);
        serial->setFlowControl(p.flowControl);
        if (serial->open(QIODevice::ReadWrite)) {
            serial_connect=true;
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

void MainWindow::InitContorllerConnection()
{
    if(GetContorllerId()){
        GetContorllerName();
        sleep(1);
        GetTravelLimit();
        sleep(1);
        ui->contorl_groupBox->setEnabled(true);
        sleep(1);
        UpdatePosition();

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
    serial->write(command+"\r");
    if(!serial->waitForBytesWritten(300)){
    }
    serial->flush();
    while(serial->waitForReadyRead(300)){
    }
    QByteArray data_response = serial->readAll().replace(QByteArray("\n"), QByteArray("")).replace(QByteArray("\r"), QByteArray("")).replace(QByteArray(" "), QByteArray(""));
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
    QMessageBox msg;
    int cnt =0;
    msg.setText("<p align='center'>Waiting " + ui->motor_pushButton->text()+"</p>");
    msg.setStandardButtons(QMessageBox::NoButton);
    msg.setDefaultButton(QMessageBox::Ok);
    msg.setWindowFlags(Qt::BypassWindowManagerHint);
    msg.setStyleSheet("QLabel{min-width: 200px;}");
    QTimer cntDown;
    QObject::connect(&cntDown, &QTimer::timeout, [&msg,&cnt, &cntDown, this]()->void{
                         if(--cnt < 0){
                             cntDown.stop();
                             int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
                             string current_status = ui->motor_status_textBrowser->toPlainText().toStdString();
                             string curent_button = ui->motor_pushButton->text().toStdString();
                             if(current_status == "Motor ON" && curent_button == "Motor OFF"){
                                 WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().MOTOR_OFF);
                             }
                             else if(current_status == "Motor OFF" && curent_button == "Motor ON"){
                                 WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().MOTOR_ON);
                             }
                             sleep(1);
                             GetControllerStatus();
                             msg.accept();
                         } else {
                             msg.setText(QString("Waiting ..."));
                         }
                     });
    cntDown.start(1000);
    msg.exec();


}


void MainWindow::on_add_relative_pushButton_clicked()
{
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    string current_status = ui->motor_status_textBrowser->toPlainText().toStdString();
    int increase_value = ui->increament_spinBox->value();

    if(current_status == "Motor ON"){
        if((ui->current_position_textBrowser->toPlainText().toInt() + increase_value) <= ui->right_travel_limit_spinBox->value())
        {
            QMessageBox msg;
            int cnt =((int)(increase_value/divide_time_wait_actuator_step)<1) ? 1: (int)(increase_value/divide_time_wait_actuator_step);
            msg.setText(QString("<p align='center'>waiting for %1 seconds</p>").arg(cnt));
            msg.setStandardButtons(QMessageBox::NoButton);
            msg.setDefaultButton(QMessageBox::Ok);
            msg.setWindowFlags(Qt::BypassWindowManagerHint);
            msg.setStyleSheet("QLabel{min-width: 150px;}");
            QTimer cntDown;
            QObject::connect(&cntDown, &QTimer::timeout, [&msg,&cnt, &cntDown, this]()->void{
                                 if(--cnt < 0){
                                     cntDown.stop();
                                     UpdatePosition();
                                     msg.accept();
                                 } else {
                                     msg.setText(QString("<p align='center'>waiting for %1 seconds</p>").arg(cnt));
                                 }
                             });
            WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().POSITION_RELATIVE+QByteArray::number(increase_value));
            cntDown.start(1000);
            msg.exec();
        }
        else{
            QMessageBox msg;
            msg.setWindowFlags(Qt::BypassWindowManagerHint);
            msg.setStyleSheet("QLabel{min-width: 100px;}");
            msg.setText(QString("<p align='center'>Over Travel Limit</p>"));

            msg.exec();
        }
    }
}


void MainWindow::on_del_relative_pushButton_clicked()
{
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    string current_status = ui->motor_status_textBrowser->toPlainText().toStdString();
    int decrease_value = ui->increament_spinBox->value();

    if(current_status == "Motor ON"){
        if((ui->current_position_textBrowser->toPlainText().toInt() - decrease_value) >= ui->left_travel_limit_spinBox->value())
        {
            QMessageBox msg;
            int cnt =((int)(decrease_value/divide_time_wait_actuator_step)<1) ? 1: (int)(decrease_value/divide_time_wait_actuator_step);
            msg.setText(QString("<p align='center'>waiting for %1 seconds</p>").arg(cnt));
            msg.setStandardButtons(QMessageBox::NoButton);
            msg.setDefaultButton(QMessageBox::Ok);
            msg.setWindowFlags(Qt::BypassWindowManagerHint);
            msg.setStyleSheet("QLabel{min-width: 150px;}");

            QTimer cntDown;
            QObject::connect(&cntDown, &QTimer::timeout, [&msg,&cnt, &cntDown, this]()->void{
                                 if(--cnt < 0){
                                     cntDown.stop();
                                     UpdatePosition();
                                     msg.accept();
                                 } else {
                                     msg.setText(QString("<p align='center'>waiting for %1 seconds</p>").arg(cnt));
                                 }
                             });
            WriteDataToSerialResponse(QByteArray::number(contoller_id)+ascii_command_set().POSITION_RELATIVE+"-"+QByteArray::number(decrease_value));
            cntDown.start(1000);
            msg.exec();
        }
        else{
            QMessageBox msg;
            msg.setWindowFlags(Qt::BypassWindowManagerHint);
            msg.setStyleSheet("QLabel{min-width: 100px;}");
            msg.setText(QString("<p align='center'>Over Travel Limit</p>"));
            msg.exec();
        }

    }
}


void MainWindow::on_contorller_id_comboBox_currentTextChanged(const QString &arg1)
{
        GetContorllerName();
        GetControllerStatus();
        GetTravelLimit();
}


void MainWindow::on_set_zero_pushButton_clicked()
{
    QMessageBox::StandardButton reply;
     reply = QMessageBox::question(this, "Warning", "Set current position to zero?",
                                   QMessageBox::Yes|QMessageBox::No);
     if (reply == QMessageBox::Yes) {

         int cnt =0;
         QMessageBox msg;
         msg.setText(QString("<p align='center'>waiting for set zero</p>"));
         msg.setStandardButtons(QMessageBox::NoButton);
         msg.setDefaultButton(QMessageBox::Ok);
         msg.setWindowFlags(Qt::BypassWindowManagerHint);
         msg.setStyleSheet("QLabel{min-width: 200px;}");

         QTimer cntDown;
         QObject::connect(&cntDown, &QTimer::timeout, [&msg,&cnt, &cntDown, this]()->void{
                              if(--cnt < 0){
                                  cntDown.stop();
                                  UpdatePosition();
                                  position_history = 0;
                                  msg.accept();
                              }
                          });

         int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
         WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().ZERO_POSITION);
         cntDown.start(1000);
         msg.exec();
     }
}


void MainWindow::on_save_limit_pushButton_clicked()
{
    QMessageBox::StandardButton reply;
     reply = QMessageBox::question(this, "Warning", "Are you want to save travel limit?",
                                   QMessageBox::Yes|QMessageBox::No);
     if (reply == QMessageBox::Yes) {
         int cnt =0;
         QMessageBox msg;
         QTimer cntDown;
         msg.setText(QString("<p align='center'>waiting for set</p>").arg(cnt));
         msg.setStandardButtons(QMessageBox::NoButton);
         msg.setDefaultButton(QMessageBox::Ok);
         msg.setWindowFlags(Qt::BypassWindowManagerHint);
         msg.setStyleSheet("QLabel{min-width: 200px;}");

         QObject::connect(&cntDown, &QTimer::timeout, [&msg,&cnt, &cntDown, this]()->void{
                              if(--cnt < 0){
                                  cntDown.stop();
                                  int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
                                  int right_limit_value = ui->right_travel_limit_spinBox->value();
                                  int left_limit_value = ui->left_travel_limit_spinBox->value();
                                  WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().SET_POSITIVE_TRAVEL_LIMIT + QByteArray::number(right_limit_value));
                                  sleep(1);
                                  WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().SET_NEGATIVE_TRAVEL_LIMIT + QByteArray::number(left_limit_value));
                                  sleep(1);
                                  GetTravelLimit();
                                  msg.accept();
                              }
                          });
         cntDown.start(1000);
         msg.exec();
     }
}


void MainWindow::on_restore_default_pushButton_clicked()
{
    QMessageBox::StandardButton reply;
     reply = QMessageBox::question(this, "Warning", "The program will restart now, Reset controller setting to default ?",
                                   QMessageBox::Yes|QMessageBox::No);
     if (reply == QMessageBox::Yes) {
         int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
         WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().RESET_CONTORLLER);
         qApp->exit();
         QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
     }
}


void MainWindow::UpdatePosition(){
    int new_value = ui->current_position_textBrowser->toPlainText().toInt();
    int old_value = new_value;
    int controller_id = ui->contorller_id_comboBox->currentText().toInt();
    while(true){
      usleep(100);
      new_value = GetCurrentPosition(controller_id);
      if(new_value == old_value) break;
      old_value = new_value;
    }
    new_value = new_value+position_history;
    ui->current_position_textBrowser->setText(QString::number(new_value));
}


int MainWindow::GetCurrentPosition(int contoller_id){
    int value = -999999999;
    QByteArray res_data = WriteDataToSerialResponse(QByteArray::number(contoller_id) + "TP", true);
    if(!res_data.isEmpty()){
        QList res_position = res_data.split('?');
        if(res_position.length()>1){
            value = res_position[1].toInt();
        }
    }
    return value;
}


void MainWindow::on_save_setting_pushButton_clicked()
{

    qDebug()<<"save";
    QMessageBox::StandardButton reply;
     reply = QMessageBox::question(this, "Warning", "Are you want save setting to default ,that these are not lost when controller is powered off ?",
                                   QMessageBox::Yes|QMessageBox::No);
     if (reply == QMessageBox::Yes) {

         int cnt =0;
         QMessageBox msg;
         QTimer cntDown;
         msg.setText(QString("<p align='center'>waiting for save</p>").arg(cnt));
         msg.setStandardButtons(QMessageBox::NoButton);
         msg.setDefaultButton(QMessageBox::Ok);
         msg.setWindowFlags(Qt::BypassWindowManagerHint);
         msg.setStyleSheet("QLabel{min-width: 200px;}");

         QObject::connect(&cntDown, &QTimer::timeout, [&msg,&cnt, &cntDown, this]()->void{
                              if(--cnt < 0){
                                  cntDown.stop();
                                  int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
                                  WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().SAVE_SETTING);
                                  msg.accept();
                              }
                          });
         cntDown.start(1000);
         msg.exec();
     }

}

void MainWindow::writeSettings()
{
    //get current data
    int controller_id = ui->contorller_id_comboBox->currentText().toInt();
    int current_position = ui->current_position_textBrowser->toPlainText().toInt();
    QByteArray controller_name = ui->name_textBrowser->toPlainText().toUtf8();


    QSettings settings("myprogram", "myapp");
    settings.beginGroup("MainWindow");

    //set setting data
    settings.setValue(&"controller_id"[controller_id], controller_id);
    settings.setValue(&"current_position"[controller_id], current_position);
    settings.setValue(&"controller_name"[controller_id], controller_name);
    settings.endGroup();
    qDebug()<<"save";
}

void MainWindow::readSettings()
{
    //Load
    QSettings settings("myprogram", "myapp");
    settings.beginGroup("MainWindow");

    //get current data
    int controller_id = ui->contorller_id_comboBox->currentText().toInt();
    QByteArray controller_name = ui->name_textBrowser->toPlainText().toUtf8();
    QByteArray current_position = ui->current_position_textBrowser->toPlainText().toUtf8();

    //load setting data
    QByteArray setting_controller_name = settings.value(&"controller_name"[controller_id]).toByteArray();
    int setting_controller_id = settings.value(&"controller_id"[controller_id]).toInt();
    QByteArray setting_position = settings.value(&"current_position"[controller_id]).toByteArray();

    if(setting_controller_name==controller_name && setting_controller_id==controller_id && setting_position != current_position){
        QMessageBox::StandardButton reply;
         reply = QMessageBox::question(this, "Warning", "found current position in history!\nAre you want to use current position from history ?",
                                       QMessageBox::Yes|QMessageBox::No);
         if (reply == QMessageBox::Yes) {
                ui->current_position_textBrowser->setText(setting_position);
                position_history = setting_position.toInt();
            }
    }
    settings.endGroup();
    qDebug()<<"Load";
}


void MainWindow::on_move_postition_pushButton_clicked()
{
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    string current_status = ui->motor_status_textBrowser->toPlainText().toStdString();
    int position_value = ui->position_spinBox->value();
    int current_position = ui->current_position_textBrowser->toPlainText().toInt();
    int relative_value = position_value - (current_position);

    if(current_status == "Motor ON"){
        if(relative_value >= 0){
            if((position_value) <= ui->right_travel_limit_spinBox->value())
            {
                QMessageBox msg;
                int cnt =((int)(relative_value/divide_time_wait_actuator_step)<1) ? 1: (int)(relative_value/divide_time_wait_actuator_step);
                msg.setText(QString("<p align='center'>waiting for %1 seconds</p>").arg(cnt));
                msg.setStandardButtons(QMessageBox::NoButton);
                msg.setDefaultButton(QMessageBox::Ok);
                msg.setWindowFlags(Qt::BypassWindowManagerHint);
                msg.setStyleSheet("QLabel{min-width: 150px;}");
                QTimer cntDown;
                QObject::connect(&cntDown, &QTimer::timeout, [&msg,&cnt, &cntDown, this]()->void{
                                     if(--cnt < 0){
                                         cntDown.stop();
                                         UpdatePosition();
                                         msg.accept();
                                     } else {
                                         msg.setText(QString("<p align='center'>waiting for %1 seconds</p>").arg(cnt));
                                     }
                                 });
                WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().POSITION_RELATIVE+QByteArray::number(relative_value));
                cntDown.start(1000);
                msg.exec();
            }
            else{
                QMessageBox msg;
                msg.setWindowFlags(Qt::BypassWindowManagerHint);
                msg.setStyleSheet("QLabel{min-width: 100px;}");
                msg.setText(QString("<p align='center'>Over Travel Limit</p>"));
                msg.exec();
            }
        }
        else{

            if(position_value >= ui->left_travel_limit_spinBox->value())
            {
                relative_value = relative_value*(-1);
                QMessageBox msg;
                int cnt =((int)(relative_value/divide_time_wait_actuator_step)<1) ? 1: (int)(relative_value/divide_time_wait_actuator_step);
                msg.setText(QString("<p align='center'>waiting for %1 seconds</p>").arg(cnt));
                msg.setStandardButtons(QMessageBox::NoButton);
                msg.setDefaultButton(QMessageBox::Ok);
                msg.setWindowFlags(Qt::BypassWindowManagerHint);
                msg.setStyleSheet("QLabel{min-width: 150px;}");

                QTimer cntDown;
                QObject::connect(&cntDown, &QTimer::timeout, [&msg,&cnt, &cntDown, this]()->void{
                                     if(--cnt < 0){
                                         cntDown.stop();
                                         UpdatePosition();
                                         msg.accept();
                                     } else {
                                         msg.setText(QString("<p align='center'>waiting for %1 seconds</p>").arg(cnt));
                                     }
                                 });
                WriteDataToSerialResponse(QByteArray::number(contoller_id)+ascii_command_set().POSITION_RELATIVE+"-"+QByteArray::number(relative_value));
                cntDown.start(1000);
                msg.exec();
            }
            else{
                QMessageBox msg;
                msg.setWindowFlags(Qt::BypassWindowManagerHint);
                msg.setStyleSheet("QLabel{min-width: 100px;}");
                msg.setText(QString("<p align='center'>Over Travel Limit</p>"));
                msg.exec();
            }
        }

    }
}

