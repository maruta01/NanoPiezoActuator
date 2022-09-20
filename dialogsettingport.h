#ifndef DIALOGSETTINGPORT_H
#define DIALOGSETTINGPORT_H

#include <QDialog>
#include <QSerialPort>

namespace Ui {
class DialogSettingPort;
}

class DialogSettingPort : public QDialog
{
    Q_OBJECT

public:
    struct Settings {
        QString name;
        qint32 baudRate;
        QString stringBaudRate;
        QSerialPort::DataBits dataBits;
        QString stringDataBits;
        QSerialPort::Parity parity;
        QString stringParity;
        QSerialPort::StopBits stopBits;
        QString stringStopBits;
        QSerialPort::FlowControl flowControl;
        QString stringFlowControl;
        bool settingStatus=false;
    };


    explicit DialogSettingPort(QWidget *parent = nullptr);
    ~DialogSettingPort();

    Settings settings() const;
    Ui::DialogSettingPort *ui;

public slots:
    void on_buttonBox_accepted();


private:
    void updateSettings();

private:

    Settings current_settings;
};

#endif // DIALOGSETTINGPORT_H
