#ifndef WOKERTHEAD_H
#define WOKERTHEAD_H
#include <QtCore>

class wokerthead :public QThread
{
public:
    wokerthead();
    void run();
    QString name;
};

#endif // WOKERTHEAD_H
