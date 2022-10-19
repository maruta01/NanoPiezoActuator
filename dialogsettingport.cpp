#include "dialogsettingport.h"
#include "ui_dialogsettingport.h"
#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>

DialogSettingPort::DialogSettingPort(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSettingPort)
{
    ui->setupUi(this);
    this->setWindowTitle("Serial Port Setting");
    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
        if(port.portName().toStdString().find("ttyUSB")==0){
            ui->com_comboBox->addItem(port.portName());
        }
//        ui->com_comboBox->addItem(port.portName());

    }
}

DialogSettingPort::~DialogSettingPort()
{
    if (current_settings.settingStatus){
        delete ui;
    }
    else{
        show();
    }

}

DialogSettingPort::Settings DialogSettingPort::settings() const
{
    return current_settings;
}

void DialogSettingPort::on_buttonBox_accepted()
{
    updateSettings();
    emit SerialPortChanged(ui->com_comboBox->currentText());
    hide();
}


void DialogSettingPort::updateSettings()
{
    current_settings.name = ui->com_comboBox->currentText();
    current_settings.baudRate = QSerialPort::Baud19200;
    current_settings.dataBits = QSerialPort::Data8;
    current_settings.parity = QSerialPort::NoParity;
    current_settings.stopBits = QSerialPort::OneStop;
    current_settings.flowControl = QSerialPort::SoftwareControl;
    current_settings.settingStatus = true;
}

