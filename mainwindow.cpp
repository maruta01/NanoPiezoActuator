#include "dialogsettingport.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <unistd.h>

#include <QDir>
#include <QKeyEvent>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QSerialPortInfo>
#include <QSettings>
#include <QShortcut>
#include <QTimer>
#include <QtCore/QCoreApplication>
#include <QtSerialPort/QSerialPort>

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
    //Sound
    sound_finish->setSource(QUrl("qrc:/sounds/connected.wav"));

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
    if(serial->isOpen() == false && serial_connect == true){
        WriteSettingsFile();
    }
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
        int cnt =0;
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
                                 ReadSettingsFile();
                                 ui->default_in_contact_spinBox->setEnabled(true);
                                 ui->default_out_contact_spinBox->setEnabled(true);
                                 sound_finish->play();
                                 msg.accept();
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
        GetContorllerName();
        usleep(500000);
        GetTravelLimit();
        ui->contorl_groupBox->setEnabled(true);
        usleep(500000);
        UpdatePosition();
        QShortcut *add_reletive_short_cut = new QShortcut(QKeySequence("Up"), ui->add_relative_pushButton);
        QObject::connect(add_reletive_short_cut, SIGNAL(activated()), ui->add_relative_pushButton, SLOT(click()));
        QShortcut *del_reletive_short_cut = new QShortcut(QKeySequence("Down"), ui->add_relative_pushButton);
        QObject::connect(del_reletive_short_cut, SIGNAL(activated()), ui->del_relative_pushButton, SLOT(click()));
        serial_connect=true;
        ui->ConnectPortButton->setText("Connected");
        sound_finish->play();
    }
    else{
        QMessageBox::warning(this, QString("Error"), "don't find the controller! try to re-plug the serial port or check the controller.");
        ui->ConnectPortButton->setText("Disconnected");
        serial_connect=false;
    }
    ui->ConnectPortButton->setEnabled(false);

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
                usleep(300000);
                SetPositiontoZero(num);
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
                             sound_finish->play();
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
    int relative_value = ui->increament_spinBox->value();
    if(!relative_value) return;

    int current_position = ui->current_position_textBrowser->toPlainText().toInt();
    int position_value = current_position+relative_value;
    MoveToPosition(position_value);
}


void MainWindow::on_del_relative_pushButton_clicked()
{
    int relative_value = ui->increament_spinBox->value();
    if(!relative_value) return;

    int current_position = ui->current_position_textBrowser->toPlainText().toInt();
    int position_value = current_position-relative_value;
    MoveToPosition(position_value);
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
                                  sleep(1);
                                  UpdatePosition();
                                  ui->current_position_textBrowser->setText("0");
                                  position_history = 0;
                                  sound_finish->play();
                                  msg.accept();
                              }
                          });

         int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
         WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().ZERO_POSITION);
         cntDown.start(1000);
         msg.exec();
     }
}

void MainWindow::SetPositiontoZero(int contoller_id){
    WriteDataToSerialResponse(QByteArray::number(contoller_id) + ascii_command_set().ZERO_POSITION);
}

void MainWindow::on_restore_default_pushButton_clicked()
{
     QMessageBox::StandardButton reply;
      reply = QMessageBox::question(this, "Warning", "Reset Configuration setting to default ?",
                                    QMessageBox::Yes|QMessageBox::No);
      if (reply == QMessageBox::Yes) {

          int cnt =0;
          QMessageBox msg;
          QTimer cntDown;
          msg.setText(QString("<p align='center'>waiting for Reset</p>"));
          msg.setStandardButtons(QMessageBox::NoButton);
          msg.setDefaultButton(QMessageBox::Ok);
          msg.setWindowFlags(Qt::BypassWindowManagerHint);
          msg.setStyleSheet("QLabel{min-width: 200px;}");

          QObject::connect(&cntDown, &QTimer::timeout, [&msg,&cnt, &cntDown, this]()->void{
                               if(--cnt < 0){
                                   cntDown.stop();
                                   QSettings* settings = new QSettings(QDir::currentPath() + "/actuator_config.ini", QSettings::IniFormat);
                                   int controller_id = ui->contorller_id_comboBox->currentText().toInt();
                                   int current_limit_right = settings->value("default_setting_current_limit_right"+QString::number(controller_id).toUtf8()).toInt();
                                   int current_limit_left = settings->value("default_setting_current_limit_left"+QString::number(controller_id).toUtf8()).toInt();
                                   int default_out_contact = settings->value("default_setting_default_out_contact"+QString::number(controller_id).toUtf8()).toInt();
                                   int default_in_contact = settings->value("default_setting_default_in_contact"+QString::number(controller_id).toUtf8()).toInt();

                                   ui->right_travel_limit_spinBox->setValue(current_limit_right);
                                   ui->left_travel_limit_spinBox->setValue(current_limit_left);
                                   ui->default_out_contact_spinBox->setValue(default_out_contact);
                                   ui->default_in_contact_spinBox->setValue(default_in_contact);
                                   sound_finish->play();
                                   msg.accept();
                               }
                           });
          cntDown.start(1000);
          msg.exec();
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
    int current_value = new_value+position_history;
    ui->current_position_textBrowser->setText(QString::number(current_value));
}


int MainWindow::GetCurrentPosition(int contoller_id){
    int value = position_history;
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
    QMessageBox::StandardButton reply;
     reply = QMessageBox::question(this, "Warning", "Are you want save setting to default ?",
                                   QMessageBox::Yes|QMessageBox::No);
     if (reply == QMessageBox::Yes) {

         int cnt =0;
         QMessageBox msg;
         QTimer cntDown;
         msg.setText(QString("<p align='center'>waiting for save</p>"));
         msg.setStandardButtons(QMessageBox::NoButton);
         msg.setDefaultButton(QMessageBox::Ok);
         msg.setWindowFlags(Qt::BypassWindowManagerHint);
         msg.setStyleSheet("QLabel{min-width: 200px;}");

         QObject::connect(&cntDown, &QTimer::timeout, [&msg,&cnt, &cntDown, this]()->void{
                              if(--cnt < 0){
                                  cntDown.stop();

                                  int controller_id = ui->contorller_id_comboBox->currentText().toInt();
                                  int default_out_contact = ui->default_out_contact_spinBox->value();
                                  int default_in_contact = ui->default_in_contact_spinBox->value();
                                  int current_limit_right = ui->right_travel_limit_spinBox->value();
                                  int current_limit_left = ui->left_travel_limit_spinBox->value();

                                  QSettings* settings = new QSettings(QDir::currentPath() + "/actuator_config.ini", QSettings::IniFormat);
                                  settings->setValue("default_setting_default_in_contact"+QString::number(controller_id).toUtf8(), default_in_contact);
                                  settings->setValue("default_setting_default_out_contact"+QString::number(controller_id).toUtf8(), default_out_contact);

                                  settings->setValue("default_setting_current_limit_right"+QString::number(controller_id).toUtf8(), current_limit_right);
                                  settings->setValue("default_setting_current_limit_left"+QString::number(controller_id).toUtf8(), current_limit_left);
                                  settings->sync();
                                  sound_finish->play();
                                  msg.accept();
                              }
                          });
         cntDown.start(1000);
         msg.exec();
     }

}

void MainWindow::WriteSettingsFile()
{
    //get current data
    int controller_id = ui->contorller_id_comboBox->currentText().toInt();
    int current_position = ui->current_position_textBrowser->toPlainText().toInt();
    QByteArray controller_name = ui->name_textBrowser->toPlainText().toUtf8();
    int current_limit_right = ui->right_travel_limit_spinBox->value();
    int current_limit_left = ui->left_travel_limit_spinBox->value();
    int default_out_contact = ui->default_out_contact_spinBox->value();
    int default_in_contact = ui->default_in_contact_spinBox->value();

    QSettings* settings = new QSettings(QDir::currentPath() + "/actuator_config.ini", QSettings::IniFormat);
    settings->setValue("controller_id"+QString::number(controller_id).toUtf8(), controller_id);
    settings->setValue("current_position"+QString::number(controller_id).toUtf8(), current_position);
    settings->setValue("controller_name"+QString::number(controller_id).toUtf8(), controller_name);
    settings->setValue("current_limit_right"+QString::number(controller_id).toUtf8(), current_limit_right);
    settings->setValue("current_limit_left"+QString::number(controller_id).toUtf8(), current_limit_left);
    settings->setValue("default_out_contact"+QString::number(controller_id).toUtf8(), default_out_contact);
    settings->setValue("default_in_contact"+QString::number(controller_id).toUtf8(), default_in_contact);
    settings->sync();
}

void MainWindow::ReadSettingsFile()
{
    if(!(QFile::exists(QDir::currentPath() + "/actuator_config.ini"))) return;

    //Load
    QSettings* settings = new QSettings(QDir::currentPath() + "/actuator_config.ini", QSettings::IniFormat);
    //get current data
    int controller_id = ui->contorller_id_comboBox->currentText().toInt();
    QByteArray controller_name = ui->name_textBrowser->toPlainText().toUtf8();
    QByteArray current_position = ui->current_position_textBrowser->toPlainText().toUtf8();


    //load setting data
    QByteArray setting_controller_name = settings->value("controller_name"+QString::number(controller_id).toUtf8()).toByteArray();
    int setting_controller_id = settings->value("controller_id"+QString::number(controller_id).toUtf8()).toInt();
    QByteArray setting_position = settings->value("current_position"+QString::number(controller_id).toUtf8()).toByteArray();
    int current_limit_right = settings->value("current_limit_right"+QString::number(controller_id).toUtf8()).toInt();
    int current_limit_left = settings->value("current_limit_left"+QString::number(controller_id).toUtf8()).toInt();
    int default_out_contact = settings->value("default_out_contact"+QString::number(controller_id).toUtf8()).toInt();
    int default_in_contact = settings->value("default_in_contact"+QString::number(controller_id).toUtf8()).toInt();

    if (controller_name==setting_controller_name && controller_id == setting_controller_id && current_position != setting_position){
        ui->current_position_textBrowser->setText(setting_position);
        position_history = setting_position.toInt();
    }
    else{
    }
    ui->right_travel_limit_spinBox->setValue(current_limit_right);
    ui->left_travel_limit_spinBox->setValue(current_limit_left);
    ui->default_out_contact_spinBox->setValue(default_out_contact);
    ui->default_in_contact_spinBox->setValue(default_in_contact);

}


void MainWindow::on_move_postition_pushButton_clicked()
{
    int position_value = ui->position_spinBox->value();
    MoveToPosition(position_value);
}

void MainWindow::MoveToPosition(int position_value)
{
    int contoller_id = ui->contorller_id_comboBox->currentText().toInt();
    string current_status = ui->motor_status_textBrowser->toPlainText().toStdString();
    int current_position = ui->current_position_textBrowser->toPlainText().toInt();
    int relative_value = position_value - (current_position);

    if(current_status == "Motor ON"){
        if(relative_value >= 0){
            if((position_value) <= ui->right_travel_limit_spinBox->value())
            {
                QMessageBox msg;
                int cnt =((int)(relative_value/divide_time_wait_actuator_step)<1) ? 1: (int)(relative_value/divide_time_wait_actuator_step);
                msg.setText(QString("<p align='center'>waiting move...</p>"));
                msg.setStandardButtons(QMessageBox::NoButton);
                msg.setDefaultButton(QMessageBox::Ok);
                msg.setWindowFlags(Qt::BypassWindowManagerHint);
                msg.setStyleSheet("QLabel{min-width: 150px;}");
                QTimer cntDown;
                QObject::connect(&cntDown, &QTimer::timeout, [&msg,&cnt, &cntDown, position_value,this]()->void{
                                     if(--cnt < 0){
                                         cntDown.stop();
                                         sleep(1);
                                         UpdatePosition();
                                         sleep(1);
                                         while(1){
                                             if(ui->current_position_textBrowser->toPlainText().toInt()==position_value){
                                                 break;
                                             }else{
                                                 MoveToPosition(position_value);
                                             }
                                         }
                                         sound_finish->play();
                                         msg.accept();                               
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
                msg.setText(QString("<p align='center'>waiting move...</p>"));
                msg.setStandardButtons(QMessageBox::NoButton);
                msg.setDefaultButton(QMessageBox::Ok);
                msg.setWindowFlags(Qt::BypassWindowManagerHint);
                msg.setStyleSheet("QLabel{min-width: 150px;}");

                QTimer cntDown;
                QObject::connect(&cntDown, &QTimer::timeout, [&msg,&cnt, &cntDown,position_value, this]()->void{
                                     if(--cnt < 0){
                                         cntDown.stop();
                                         sleep(1);
                                         UpdatePosition();
                                         sleep(1);
                                         while(1){
                                             if(ui->current_position_textBrowser->toPlainText().toInt()==position_value){
                                                 break;
                                             }
                                             else{
                                                MoveToPosition(position_value);
                                             }
                                         }
                                         sound_finish->play();
                                         msg.accept();
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


void MainWindow::on_default_in_contact_pushButton_clicked()
{
    int position_value = ui->default_in_contact_spinBox->value();
    MoveToPosition(position_value);
}


void MainWindow::on_default_out_contact_pushButton_clicked()
{
    int position_value = ui->default_out_contact_spinBox->value();
    MoveToPosition(position_value);
}


void MainWindow::on_default_in_contact_checkBox_stateChanged(int arg1)
{
    if(!arg1){
        ui->default_in_contact_spinBox->setReadOnly(false);
    }
    else{
        ui->default_in_contact_spinBox->setReadOnly(true);
    }

}


void MainWindow::on_default_out_contact_checkBox_stateChanged(int arg1)
{
    if(!arg1){
        ui->default_out_contact_spinBox->setReadOnly(false);
    }
    else{
        ui->default_out_contact_spinBox->setReadOnly(true);
    }
}



void MainWindow::on_right_travel_limit_checkBox_stateChanged(int arg1)
{
    if(!arg1){
        ui->right_travel_limit_spinBox->setReadOnly(false);
    }
    else{
        ui->right_travel_limit_spinBox->setReadOnly(true);
    }
}


void MainWindow::on_left_travel_limit_checkBox_stateChanged(int arg1)
{
    if(!arg1){
        ui->left_travel_limit_spinBox->setReadOnly(false);
    }
    else{
        ui->left_travel_limit_spinBox->setReadOnly(true);
    }
}

