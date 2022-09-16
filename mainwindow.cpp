#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialogsettingport.h"
#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>

using namespace std;
QSerialPort* serial;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    ui_settings(new DialogSettingPort)
{
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
}


void MainWindow::on_DisconnectPortButton_clicked()
{
    const DialogSettingPort::Settings p = ui_settings->settings();
    serial->setPortName(p.name);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite)) {
        QMessageBox::information(this,QString("Connected"),QString("Connected to "+p.name));
    } else {
        QMessageBox::critical(this, QString("Error"), serial->errorString());
    }
}

