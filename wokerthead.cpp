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
        this->msleep(1000);
    }
}


QByteArray WorkerThread::WriteDataToSerialResponse(QByteArray command){
    serial->write(command+"\r");
    serial->flush();
    serial->waitForBytesWritten(100);
    if(!serial->waitForReadyRead(200)){
        return serial->readAll().replace(QByteArray("\n"), QByteArray("")).replace(QByteArray("\r"), QByteArray("")).replace(QByteArray(" "), QByteArray(""));
    };
    return "";
}

int WorkerThread::GetCurrentPosition(int contoller_id){
    int value=value_current;
    QByteArray res_data = WriteDataToSerialResponse(QByteArray::number(contoller_id) + "TP?");
    if(!res_data.isEmpty()){
        QList controller_name = res_data.split('?');
        qDebug()<<"controller_position= "<<controller_name[1];
        value = controller_name[1].toInt();
        qDebug()<<"value PT= "<<value;

    }
    if(value_current != value){
        value_current = value;
    }
    return value_current;



}
