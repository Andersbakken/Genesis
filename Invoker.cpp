#include "Invoker.h"
#include <QProcess>
#include <QDebug>
#ifdef Q_OS_LINUX
#    include <QX11Info>
#    include <dirent.h>
#    include <stdio.h>
#    include <unistd.h>
#    include <fcntl.h>
#    include <stdlib.h>
#    include <X11/Xlib.h>
#endif

#define MIN_EQLEN 6

Invoker::Invoker(QWidget *parent)
    : QObject(parent), mWidget(parent)
{
}

void Invoker::setApplication(const QString &application, const QStringList &arguments)
{
    mApplication = application;
    mArguments = arguments;
}

#if defined(Q_OS_LINUX)

// Returns 'true' if the process with the pid 'pid' has the application name 'app'
static bool pidIsApp(long pid, const QString &app)
{
    const QByteArray file = "/proc/" + QByteArray::number(static_cast<qlonglong>(pid)) + "/cmdline";
    const int fd = ::open(file.constData(), O_RDONLY);
    if (fd == -1)
        return false;
    const int maxsize = 1024;
    char data[maxsize + 1];
    ssize_t num, pos = 0;
    do {
        num = ::read(fd, data + pos, maxsize - pos);
        if (num == -1 || pos + num == maxsize) {
            ::close(fd);
            return false;
        }
        pos += num;
    } while (num > 0);
    ::close(fd);

    if (!pos)
        return false;

    const char* start = &data[0];
    const char* end = start + pos;
    while (start != end && *start != ' ')
        ++start;
    data[start - &data[0]] = '\0';
    //qDebug() << "hasPid" << data << "vs input," << app << "from file" << file << num;

    QByteArray app8bit = app.toLocal8Bit();
    const int slash = app8bit.lastIndexOf('/');
    if (slash != -1)
        app8bit = app8bit.mid(slash + 1);
    QByteArray badata(data);
    //qDebug() << "hello" << app8bit << badata << app;
    if (badata.size() >= MIN_EQLEN && badata.contains(app8bit))
        return true;
    else if (app.toLocal8Bit() == badata)
        return true;
    return false;
}

// returns 'true' if the window 'w' belongs to the app 'app'
static bool windowIsApp(Display* dpy, Window w, const QString &app, Atom pidatom, Atom cardinalatom)
{
    Atom retatom;
    int retfmt;
    unsigned long retnitems, retbytes;
    unsigned char* retprop;

    int r = XGetWindowProperty(dpy, w, pidatom, 0, 1, False, cardinalatom, &retatom,
                               &retfmt, &retnitems, &retbytes, &retprop);
    if (r == Success && retatom != None) {
        Q_ASSERT(retfmt == 32 && retnitems == 1);

        long* longs = reinterpret_cast<long*>(retprop);
        //qDebug() << "window" << QByteArray::number((int)w, 16) << "has pid" << *longs;
        if (pidIsApp(*longs, app)) {
            XFree(retprop);
            return true;
        }

        XFree(retprop);
    }

    return false;
}

// returns 'true' if the application named 'app' was found and also puts the window id in 'w'
static bool findWindow(Display* dpy, int screen, const QString &app, Window* w)
{
    Atom retatom;
    int retfmt;
    unsigned long retnitems, retbytes;
    unsigned char* retprop;

    const Atom clientatom = XInternAtom(dpy, "_NET_CLIENT_LIST", True);
    const Atom windowatom = XInternAtom(dpy, "WINDOW", True);
    const Atom pidatom = XInternAtom(dpy, "_NET_WM_PID", True);
    const Atom cardinalatom = XInternAtom(dpy, "CARDINAL", True);
    if (clientatom == None || windowatom == None || pidatom == None || cardinalatom == None)
        return false;

    long offset = 0;
    bool ok = false;
    do {
        int r = XGetWindowProperty(dpy, RootWindow(dpy, screen), clientatom, offset, 5, False,
                                   windowatom, &retatom, &retfmt, &retnitems, &retbytes, &retprop);
        if (r != Success || retatom == None)
            return false;

        Q_ASSERT(retatom == windowatom && retfmt == 32);

        Window* windows = reinterpret_cast<Window*>(retprop);
        for (unsigned int i = 0; i < retnitems; ++i) {
            if (windowIsApp(dpy, windows[i], app, pidatom, cardinalatom)) {
                ok = true;
                *w = windows[i];
                break;
            }
        }

        XFree(retprop);
        offset += retnitems;
    } while (retbytes > 0 && !ok);

    return ok;
}

// raises the window 'w'
static void raiseWindow(Display* dpy, int screen, Window w)
{
    const Atom activeatom = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", True);
    if (activeatom == None)
        return;

    XEvent ev;
    ev.xclient.type = ClientMessage;
    ev.xclient.display = dpy;
    ev.xclient.serial = 0;
    ev.xclient.send_event = True;
    ev.xclient.window = w;
    ev.xclient.message_type = activeatom;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = 0;
    ev.xclient.data.l[1] = 0;
    ev.xclient.data.l[2] = 0;
    ev.xclient.data.l[3] = 0;
    ev.xclient.data.l[4] = 0;

    XSendEvent(dpy, RootWindow(dpy, screen), False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
}

// returns 'true' if the application 'app' was found and raised
static bool raise(const QString &app, const QWidget* widget)
{
    Display* dpy = QX11Info::display();
    const int scrn = widget->x11Info().screen();
    Window w = None;
    if (findWindow(dpy, scrn, app, &w)) {
        raiseWindow(dpy, scrn, w);
        return true;
    }
    return false;
}
#elif !defined(Q_OS_MAC)
#error Do not know how to raise windows on this platform
#endif

void Invoker::invoke()
{
#ifdef Q_OS_MAC
    if (mApplication.endsWith(QLatin1String(".app"))) {
        QStringList args(arguments);
        if (!args.isEmpty())
            args.prepend(QLatin1String("--args"));
        args.prepend(mApplication);
        QProcess::startDetached(QLatin1String("open"), args);
    } else
        QProcess::startDetached(mApplication, arguments);
#else
    if (!::raise(mApplication, mWidget))
        QProcess::startDetached(mApplication, mArguments);
#endif
}

void Invoker::raise(QWidget *w)
{
#ifdef Q_WS_X11
    Display* dpy = QX11Info::display();
    const int screen = w->x11Info().screen();
    raiseWindow(dpy, screen, w->handle());
#endif
}
