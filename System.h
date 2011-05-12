#ifndef SYSTEM_H
#define SYSTEM_H

#include <QString>
#include <QVector>
#include <QSet>
#include <QWidget>
#ifdef Q_WS_X11
#    include <QIcon>
#    include <QX11Info>
#endif

class System
{
public:
    static System* instance(int screen);

    static void raise(QWidget* w);
    static void hideFromPager(QWidget* w);

#ifdef Q_WS_X11
    static QIcon readIcon(Display* dpy, WId winId);
#endif

    bool raise(const QString& application);
    bool findWindow(const QString& application, WId* winId);

private:
    System(int scrn);

    static QVector<System*> sInsts;
    int mScreen;
};

#endif // SYSTEM_H
