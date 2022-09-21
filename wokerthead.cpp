#include "wokerthead.h"
#include <QtCore>
#include <QDebug>
#include "mainwindow.h"

int value_current;

void wokerthead::run()
{
    int value;
    while (true) {
        qDebug()<<"run..";
        value = GetCurrentPosition(1);
        this->msleep(1000);
//        NumberChanged(value);
    }

//    main.GetCurrentPosition();
}

//void wokerthead::NumberChanged(int value){
//    emit NumberChanged(value);
//}

QByteArray wokerthead::WriteDataToSerialResponse(QByteArray command){
    serial->write(command+"\r");
    serial->flush();
    serial->waitForBytesWritten(100);
    if(!serial->waitForReadyRead(200)){
        return serial->readAll().replace(QByteArray("\n"), QByteArray("")).replace(QByteArray("\r"), QByteArray("")).replace(QByteArray(" "), QByteArray(""));
    };
    return "";
}

int wokerthead::GetCurrentPosition(int contoller_id){
    int value=value_current;
    if (!serial->waitForReadyRead(200)){
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
    }
    return value_current;



}
