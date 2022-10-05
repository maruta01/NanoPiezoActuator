#include "wokerthead.h"
#include <QtCore>
#include <QDebug>

WorkerThread::WorkerThread(QObject *parent):
    QThread(parent)
{

}

void WorkerThread::run()
{
      stop = false;
      int new_value = 0;
      int old_value = new_value;
      while(true){
        this->msleep(1000);
        new_value = GetCurrentPosition(controller_id);
        emit NumberChanged(QString::number(new_value));
        if(stop || new_value == old_value) break;
        old_value = new_value;

      }
}


QByteArray WorkerThread::WriteDataToSerialResponse(QByteArray command){
    serial->write(command+"\r");
    serial->flush();
    serial->waitForBytesWritten(300);
    QByteArray res_data = "";

    do{
       serial->waitForReadyRead(300);
       res_data = serial->readAll().replace(QByteArray("\n"), QByteArray("")).replace(QByteArray("\r"), QByteArray("")).replace(QByteArray(" "), QByteArray(""));
    }
    while(res_data == "");

    return res_data;
}

int WorkerThread::GetCurrentPosition(int contoller_id){
    int value=value_current;
    QByteArray res_data = WriteDataToSerialResponse(QByteArray::number(contoller_id) + "TP?");
    if(!res_data.isEmpty()){
        QList res_position = res_data.split('?');
        if(res_position.length()>1){
            value = res_position[1].toInt();
        }
    }
    if(value_current != value){
        value_current = value;
    }
    return value_current;
}
