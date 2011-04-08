#ifndef PREVIOUSWINDOW_H
#define PREVIOUSWINDOW_H

#include <QObject>

class PreviousProcessPrivate;

class PreviousProcess : public QObject
{
public:
    PreviousProcess(QObject* parent = 0);

    void record();
    void activate();

private:
    PreviousProcessPrivate* priv;
};

#endif
