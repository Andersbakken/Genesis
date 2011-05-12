#include "System.h"
#include "Config.h"
#include <QHash>
#include <QX11Info>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MIN_EQLEN 6

static inline QByteArray processName(const QByteArray& name)
{
    const QByteArray file = "/proc/" + name + "/cmdline";
    const int fd = ::open(file.constData(), O_RDONLY);
    if (fd == -1)
        return QByteArray();
    const int maxsize = 1024;
    char data[maxsize + 1];
    ssize_t num, pos = 0;
    do {
        num = ::read(fd, data + pos, maxsize - pos);
        if (num == -1 || pos + num == maxsize) {
            ::close(fd);
            return QByteArray();
        }
        pos += num;
    } while (num > 0);
    ::close(fd);

    if (!pos)
        return QByteArray();

    const char* start = &data[0];
    const char* end = start + pos;
    while (start != end && *start != ' ')
        ++start;

    return QByteArray(data, start - &data[0]);
}

static inline bool pidIsApp(long pid, const QString &app)
{
    //qDebug() << "hasPid" << data << "vs input," << app << "from file" << file << num;
    QByteArray cmdline = processName(QByteArray::number(static_cast<qlonglong>(pid)));
    if (cmdline.isEmpty())
        return false;

    QByteArray app8bit = app.toLocal8Bit();
    const int slash = app8bit.lastIndexOf('/');
    if (slash != -1)
        app8bit = app8bit.mid(slash + 1);
    if (cmdline.size() >= MIN_EQLEN && cmdline.contains(app8bit))
        return true;
    else if (app.toLocal8Bit() == cmdline)
        return true;
    return false;
}

template<typename T>
static inline bool readProperty(Display* dpy, Window w, Atom property, QList<T>& data)
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
        XFree(retprop);

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
static inline bool windowIsApp(Display* dpy, Window w, const QString &app, Atom pidatom)
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

    static const bool matchFromPid = Config().isEnabled("matchFromPid", true);
    if (!matchFromPid)
        return false;

    QList<long> pids;
    if (readProperty(dpy, w, pidatom, pids) && !pids.isEmpty()) {
        if (pidIsApp(pids.front(), app))
            return true;
    }

    return false;
}

// raises the window 'w'
static inline void raiseWindow(Display* dpy, int screen, Window w)
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

// returns 'true' if the application named 'app' was found and raised
static inline bool findAndRaiseWindow(Display* dpy, int screen, const QString &app)
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

QSet<QByteArray> System::processes()
{
    QSet<QByteArray> ps;
    DIR* dir = opendir("/proc");
    if (!dir)
        return ps;

    struct dirent d, *dret;
    QByteArray name;
    while (readdir_r(dir, &d, &dret) == 0 && dret != 0) {
        if (d.d_type == DT_DIR) {
            name = processName(QByteArray(d.d_name));
            if (!name.isEmpty())
                ps.insert(name);
        }
    }

    closedir(dir);

    return ps;
}

void System::raise(QWidget *w)
{
    Display* dpy = QX11Info::display();
    const int screen = w->x11Info().screen();
    raiseWindow(dpy, screen, w->handle());
}

void System::hideFromPager(QWidget *w)
{
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
    readProperty(dpy, win, stateatom, states);

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
}

bool System::findWindow(const QString &application, WId *winId)
{
    Display* dpy = QX11Info::display();

    const Atom clientatom = XInternAtom(dpy, "_NET_CLIENT_LIST", True);
    const Atom pidatom = XInternAtom(dpy, "_NET_WM_PID", True);
    const Atom leaderatom = XInternAtom(dpy, "WM_CLIENT_LEADER", True);
    if (clientatom == None || pidatom == None || leaderatom == None)
        return false;

    QList<Window> windows;
    bool ok = readProperty(dpy, RootWindow(dpy, mScreen), clientatom, windows);
    if (!ok)
        return false;
    ok = false;

    foreach(const Window win, windows) {
        if (windowIsApp(dpy, win, application, pidatom)) {
            *winId = win;
            return true;
        }
    }

    return false;
}

bool System::raise(const QString &application)
{
    Display* dpy = QX11Info::display();
    return findAndRaiseWindow(dpy, mScreen, application);
}

static int parseIcon(const QList<unsigned int>& icons, int pos, QIcon* icon, int* width, int* height)
{
    if (pos + 1 >= icons.size())
        return -1;
    int w = icons.at(pos++);
    int h = icons.at(pos++);

    QImage img(w, h, QImage::Format_ARGB32);
    int y, x, pixel;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            pixel = icons.at(pos++);
            img.setPixel(x, y, pixel);
        }
    }

    *width = w;
    *height = h;
    *icon = QIcon(QPixmap::fromImage(img));
    return pos;
}

QIcon System::readIcon(Display *dpy, WId winId)
{
    Atom iconatom = XInternAtom(dpy, "_NET_WM_ICON", True);
    if (iconatom == None)
        return QIcon();

    // Should probably not read icons as a list of ints
    QList<unsigned int> icons;
    readProperty(dpy, winId, iconatom, icons);

    int pos = 0, w, h, ow = 0, oh = 0;
    QIcon icon, tmpicon;
    for (;;) {
        pos = parseIcon(icons, pos, &tmpicon, &w, &h);
        if (pos == -1)
            break;
        if (w > ow || h > oh) {
            ow = w;
            oh = h;
            icon = tmpicon;
        }
    }

    return icon;
}
