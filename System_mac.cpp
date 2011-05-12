#include "System.h"

void System::raise(QWidget *w)
{
    w->raise();
    w->activateWindow();
}

void System::hideFromPager(QWidget *w)
{
    Q_UNUSED(w)
}

bool System::findWindow(const QString &application, WId *winId)
{
    Q_UNUSED(application)
    Q_UNUSED(winId)
    return false;
}

bool System::raise(const QString &application)
{
    Q_UNUSED(application)
    return false;
}
