#include "Invoker.h"
#include <QProcess>
#include <QDebug>
#ifdef Q_OS_LINUX
#    include <QSet>
#    include <QX11Info>
#    include <dirent.h>
#    include <stdio.h>
#    include <unistd.h>
#    include <fcntl.h>
#    include <stdlib.h>
#    include <X11/Xlib.h>
#    include <X11/Xutil.h>
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

template<typename T>
static bool readProperty(Display* dpy, Window w, Atom property, QList<T>& data)
{
    data.clear();

    Atom retatom;
    int retfmt;
    unsigned long retnitems, retbytes;
    unsigned char* retprop;

    unsigned long offset = 0;
    int r;
    do {
        r = XGetWindowProperty(dpy, w, property, offset, 5, False, AnyPropertyType,
                               &retatom, &retfmt, &retnitems, &retbytes, &retprop);
        if (r != Success || retatom == None)
            return false;

        Q_ASSERT(retfmt == (sizeof(T) * 8));

        const T* retdata = reinterpret_cast<T*>(retprop);
        for (unsigned long i = 0; i < retnitems; ++i)
            data << retdata[i];

        switch(retfmt) {
        case 8:
            offset += retnitems * 4;
            break;
        case 16:
            offset += retnitems * 2;
            break;
        case 32:
            offset += retnitems;
            break;
        default:
            qFatal("Invalid format returned in readProperty: %d", retfmt);
        }
    } while (retbytes > 0);

    return true;
}

// returns 'true' if the window 'w' belongs to the app 'app'
static bool windowIsApp(Display* dpy, Window w, const QString &app, Atom pidatom)
{
    XClassHint hint;
    if (XGetClassHint(dpy, w, &hint) != 0) {
        if (hint.res_name) {
            QByteArray app8bit = app.toLocal8Bit();
            const int slash = app8bit.lastIndexOf('/');
            if (slash != -1)
                app8bit = app8bit.mid(slash + 1);
            QByteArray res = QByteArray::fromRawData(hint.res_name, strnlen(hint.res_name, 100));
            if (app8bit.toLower() == res.toLower()) {
                XFree(hint.res_name);
                return true;
            }
            XFree(hint.res_name);
        }

        if (hint.res_class)
            XFree(hint.res_class);
    }


    QList<long> pids;
    if (readProperty(dpy, w, pidatom, pids) && !pids.isEmpty()) {
        if (pidIsApp(pids.front(), app))
            return true;
    }

    return false;
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

// returns 'true' if the application named 'app' was found and also puts the window id in 'w'
static bool findAndRaiseWindow(Display* dpy, int screen, const QString &app)
{
    const Atom clientatom = XInternAtom(dpy, "_NET_CLIENT_LIST", True);
    const Atom pidatom = XInternAtom(dpy, "_NET_WM_PID", True);
    const Atom leaderatom = XInternAtom(dpy, "WM_CLIENT_LEADER", True);
    if (clientatom == None || pidatom == None || leaderatom == None)
        return false;

    QList<Window> windows;
    bool ok = readProperty(dpy, RootWindow(dpy, screen), clientatom, windows);
    if (!ok)
        return false;
    ok = false;

    QHash<Window, QList<Window> > clients;
    Window leader = None, w = None;
    QList<Window> currentleader;

    foreach(const Window win, windows) {
        if (readProperty(dpy, win, leaderatom, currentleader) && !currentleader.isEmpty())
            clients[currentleader.front()].append(win);
        if (!ok && windowIsApp(dpy, win, app, pidatom)) {
            ok = true;
            w = win;
            leader = (currentleader.isEmpty() ? None : currentleader.front());
        }
    }

    if (!ok)
        return false;

    if (leader == None)
        raiseWindow(dpy, screen, w);
    else {
        const QList<Window> subs = clients.value(leader);
        foreach(Window win, subs) {
            raiseWindow(dpy, screen, win);
        }
    }

    return true;
}

// returns 'true' if the application 'app' was found and raised
static bool raise(const QString &app, const QWidget* widget)
{
    Display* dpy = QX11Info::display();
    const int scrn = widget->x11Info().screen();
    return findAndRaiseWindow(dpy, scrn, app);
}
#elif !defined(Q_OS_MAC)
#error Do not know how to raise windows on this platform
#endif

void Invoker::invoke()
{
#ifdef Q_OS_MAC
    if (mApplication.endsWith(QLatin1String(".app"))) {
        QStringList args(mArguments);
        if (!args.isEmpty())
            args.prepend(QLatin1String("--args"));
        args.prepend(mApplication);
        QProcess::startDetached(QLatin1String("open"), args);
    } else
        QProcess::startDetached(mApplication, mArguments);
#else
    if (!::raise(mApplication, mWidget))
        QProcess::startDetached(mApplication, mArguments);
#endif
}

void Invoker::raise(QWidget *w)
{
#if defined(Q_WS_X11)
    Display* dpy = QX11Info::display();
    const int screen = w->x11Info().screen();
    raiseWindow(dpy, screen, w->handle());
#elif defined(Q_OS_MAC)
    w->raise();
    w->activateWindow();
#endif
}

void Invoker::hideFromPager(QWidget *w)
{
#if defined(Q_WS_X11)
    Display* dpy = QX11Info::display();
    Window win = w->winId();
    //qDebug() << "hiding from pager" << QByteArray::number((int)win, 16);

    // First, initialize all our atoms
    const Atom stateatom = XInternAtom(dpy, "_NET_WM_STATE", True);
    const Atom atomatom = XInternAtom(dpy, "ATOM", True);
    const Atom hidetaskbaratom = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", True);
    const Atom hidepageratom = XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", True);
    if (stateatom == None || atomatom == None || hidetaskbaratom == None || hidepageratom == None)
        return;

    // Next, get a list of the existing window states
    QList<Atom> states;
    if (!readProperty(dpy, win, stateatom, states))
        return;

    // Now, if the state contains our atoms, just return
    if (states.contains(hidepageratom) && states.contains(hidetaskbaratom))
        return;

    // Insert our new states into the set of existing states
    states << hidepageratom << hidetaskbaratom;

    // Update the state of the window
    Atom statearray[states.size()];
    int statepos = 0;
    foreach(Atom state, states) {
        statearray[statepos++] = state;
    }

    XChangeProperty(dpy, win, stateatom, atomatom, 32, PropModeReplace,
                    reinterpret_cast<unsigned char*>(statearray), states.size());
#else
    Q_UNUSED(w)
#endif
}
