#ifndef WOKERTHEAD_H
#define WOKERTHEAD_H

#include <QThread>
#include <QtSerialPort/QSerialPort>

class wokerthead : public QThread
{

public:
    void run();

    QSerialPort* serial;


signals:
    void NumberChanged(int);

private:
    int GetCurrentPosition(int);
    QByteArray WriteDataToSerialResponse(QByteArray);
};

#endif // WOKERTHEAD_H
