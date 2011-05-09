#include "PreviousWindow.h"
#include <QApplication>
#include <QX11Info>
#include <QWidget>
#include <X11/Xlib.h>

class PreviousProcessPrivate : public QObject
{
public:
    PreviousProcessPrivate(QObject* parent);

    Window previous;
    int revert;
};

PreviousProcessPrivate::PreviousProcessPrivate(QObject *parent)
    : QObject(parent), previous(None)
{
}

PreviousProcess::PreviousProcess(QObject *parent)
    : QObject(parent), priv(new PreviousProcessPrivate(this))
{
}

void PreviousProcess::record()
{
    XGetInputFocus(QX11Info::display(), &(priv->previous), &(priv->revert));
    QWidgetList all = QApplication::allWidgets();
    foreach(const QWidget* w, all) {
        if (w->handle() == priv->previous) {
            priv->previous = None;
            break;
        }
    }
}

void PreviousProcess::activate()
{
    if (priv->previous != None)
        XSetInputFocus(QX11Info::display(), priv->previous, priv->revert, CurrentTime);
}
