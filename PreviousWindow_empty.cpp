#include "PreviousWindow.h"

class PreviousProcessPrivate : public QObject
{
public:
    PreviousProcessPrivate(QObject* parent);
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
}

void PreviousProcess::activate()
{
}
