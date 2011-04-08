#include "PreviousWindow.h"
#include <Carbon/Carbon.h>

class PreviousProcessPrivate : public QObject
{
public:
    PreviousProcessPrivate(QObject* parent);

    ProcessSerialNumber PSN;
};

PreviousProcessPrivate::PreviousProcessPrivate(QObject *parent)
    : QObject(parent)
{
}

PreviousProcess::PreviousProcess(QObject *parent)
    : QObject(parent), priv(new PreviousProcessPrivate(this))
{
}

void PreviousProcess::record()
{
    GetFrontProcess(&priv->PSN);
}

void PreviousProcess::activate()
{
    SetFrontProcess(&priv->PSN);
}
