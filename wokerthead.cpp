#include "wokerthead.h"
#include <QtCore>
#include <QDebug>
#include "mainwindow.h"
wokerthead::wokerthead()
{

}

void wokerthead::run()
{
    MainWindow main;
    qDebug()<<"Runnig startd";
    main.GetCurrentPosition();
}
