#ifndef WOKERTHEAD_H
#define WOKERTHEAD_H

#include <QThread>
#include <QtSerialPort/QSerialPort>

class WorkerThread :public QThread
{
    Q_OBJECT

public:
    explicit WorkerThread(QObject *parent=0);
    void run();
    bool stop=false;
    int controller_id=-1;
    QSerialPort* serial;


signals:
    void NumberChanged(int);

private:
    int value_current=0;
    int GetCurrentPosition(int);
    QByteArray WriteDataToSerialResponse(QByteArray);
};

#endif // WOKERTHEAD_H
