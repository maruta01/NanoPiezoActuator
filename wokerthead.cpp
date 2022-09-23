#include "wokerthead.h"
#include <QtCore>

WorkerThread::WorkerThread(QObject *parent):
    QThread(parent)
{

}

void WorkerThread::run()
{
    int value = value_current;
    while(controller_id>=0){
        if(stop) break;
        value = GetCurrentPosition(controller_id);
        emit NumberChanged(value);
        this->msleep(500);

    }
}


QByteArray WorkerThread::WriteDataToSerialResponse(QByteArray command){
    serial->write(command+"\r");
    serial->flush();
    serial->waitForBytesWritten(300);
    if(!serial->waitForReadyRead(300)){
        return serial->readAll().replace(QByteArray("\n"), QByteArray("")).replace(QByteArray("\r"), QByteArray("")).replace(QByteArray(" "), QByteArray(""));
    };
    return "";
}

int WorkerThread::GetCurrentPosition(int contoller_id){
    int value=value_current;
    QByteArray res_data = WriteDataToSerialResponse(QByteArray::number(contoller_id) + "TP?");
    if(!res_data.isEmpty()){
        QList res_position = res_data.split('?');
        if(res_position.length()>1){
            qDebug()<<"controller_position= "<<res_position[1];
            value = res_position[1].toInt();
        }
    }
    if(value_current != value){
        value_current = value;
    }
    return value_current;
}
