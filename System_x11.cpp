#include "System.h"
#include "Config.h"
#include <QHash>
#include <QX11Info>
#include <QAbstractEventDispatcher>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define ICON_SUFFICIENT_SIZE 48
#define ICON_MAX_SIZE 1024

struct GenesisSystemInfo
{
    QHash<QByteArray, Window> windownames;
    QSet<Window> allwindows;
    QAbstractEventDispatcher::EventFilter filter;
    Display* dpy;
    int screen;
};
Q_GLOBAL_STATIC(GenesisSystemInfo, genesisInfo)

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
        r = XGetWindowProperty(dpy, w, property, offset, 200, False, AnyPropertyType,
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

static inline QByteArray windowName(Display* dpy, Window w)
{
    QByteArray res;

    XClassHint hint;
    if (XGetClassHint(dpy, w, &hint) != 0) {
        if (hint.res_name) {
            res = QByteArray(hint.res_name, strnlen(hint.res_name, 100)).toLower();
            XFree(hint.res_name);
        }
        if (hint.res_class)
            XFree(hint.res_class);
    }

    return res;
}

static bool eventFilter(void* message)
{
    XEvent* ev = static_cast<XEvent*>(message);
    if (ev->type == PropertyNotify) {
        static Atom clientatom = None;
        if (clientatom == None) {
            clientatom = XInternAtom(genesisInfo()->dpy, "_NET_CLIENT_LIST", True);
            if (clientatom == None)
                return false;
        }

        if (ev->xproperty.atom == clientatom) {
            Display* dpy = genesisInfo()->dpy;
            int screen = genesisInfo()->screen;

            QList<Window> windows;
            if (!readProperty(dpy, RootWindow(dpy, screen), clientatom, windows))
                return false;

            genesisInfo()->allwindows.clear();
            genesisInfo()->windownames.clear();

            foreach(Window w, windows) {
                genesisInfo()->windownames.insert(windowName(dpy, w), w);
                genesisInfo()->allwindows.insert(w);
            }
        }
    }

    return genesisInfo()->filter ? genesisInfo()->filter(message) : false;
}

static inline void initWindows(Display* dpy, int screen)
{
    const Atom clientatom = XInternAtom(dpy, "_NET_CLIENT_LIST", True);
    if (clientatom == None)
        return;

    genesisInfo()->filter = QAbstractEventDispatcher::instance()->setEventFilter(eventFilter);
    genesisInfo()->dpy = dpy;
    genesisInfo()->screen = screen;

    QList<Window> windows;
    if (!readProperty(dpy, RootWindow(dpy, screen), clientatom, windows))
        return;

    foreach(Window w, windows) {
        genesisInfo()->windownames.insert(windowName(dpy, w), w);
        genesisInfo()->allwindows.insert(w);
    }
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
    const Atom leaderatom = XInternAtom(dpy, "WM_CLIENT_LEADER", True);
    if (leaderatom == None)
        return false;

    QByteArray app8bit = app.toLocal8Bit().toLower();
    const int slash = app8bit.lastIndexOf('/');
    if (slash != -1)
        app8bit = app8bit.mid(slash + 1);

    const QHash<QByteArray, Window>::const_iterator winit = genesisInfo()->windownames.find(app8bit);
    if (winit == genesisInfo()->windownames.end())
        return false;

    const Window targetwin = winit.value();
    Window leader = None;
    QHash<Window, QList<Window> > clients;
    QList<Window> currentleader;

    const QSet<Window>& windows = genesisInfo()->allwindows;
    foreach(const Window win, windows) {
        if (readProperty(dpy, win, leaderatom, currentleader) && !currentleader.isEmpty())
            clients[currentleader.front()].append(win);
        if (win == targetwin)
            leader = (currentleader.isEmpty() ? None : currentleader.front());
    }

    if (leader == None)
        raiseWindow(dpy, screen, targetwin);
    else {
        const QList<Window> subs = clients.value(leader);
        foreach(Window win, subs) {
            raiseWindow(dpy, screen, win);
        }
    }

    return true;
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
    static bool first = true;
    if (first) {
        first = false;
        initWindows(QX11Info::display(), mScreen);
    }

    QByteArray app8bit = application.toLocal8Bit().toLower();
    const int slash = app8bit.lastIndexOf('/');
    if (slash != -1)
        app8bit = app8bit.mid(slash + 1);

    const QHash<QByteArray, Window>::const_iterator it = genesisInfo()->windownames.find(app8bit);
    if (it != genesisInfo()->windownames.end()) {
        *winId = it.value();
        return true;
    }
    return false;
}

bool System::raise(const QString &application)
{
    Display* dpy = QX11Info::display();
    return findAndRaiseWindow(dpy, mScreen, application);
}

static inline int parseIcon(const QList<unsigned long>& icons, int pos, QIcon* icon,
                            unsigned long* width, unsigned long* height)
{
    if (pos + 1 >= icons.size())
        return -1;

    const unsigned long w = icons.at(pos++);
    const unsigned long h = icons.at(pos++);

    if (w > ICON_MAX_SIZE || h > ICON_MAX_SIZE)
        return -1;

    QImage img(w, h, QImage::Format_ARGB32);
    unsigned long y, x;
    unsigned int pixel;

    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            pixel = static_cast<unsigned int>(icons.at(pos++));
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
    const Atom iconatom = XInternAtom(dpy, "_NET_WM_ICON", True);
    if (iconatom == None)
        return QIcon();

    // ### Should probably not read icons as a list of longs
    QList<unsigned long> icons;
    readProperty(dpy, winId, iconatom, icons);

    int pos = 0;
    unsigned long w = 0, h = 0, ow = 0, oh = 0;
    QIcon icon, tmpicon;
    for (;;) {
        pos = parseIcon(icons, pos, &tmpicon, &w, &h);
        if (pos == -1)
            break;
        if (w > ow || h > oh) {
            ow = w;
            oh = h;
            icon = tmpicon;
            if (w >= ICON_SUFFICIENT_SIZE || h >= ICON_SUFFICIENT_SIZE)
                break;
        }
    }

    return icon;
}
