#include "GlobalShortcut.h"
#include <QWidget>
#include <QHash>
#include <QDebug>
#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

class GlobalShortcutPrivate
{
public:
    GlobalShortcutPrivate(GlobalShortcut* s)
        : shortcut(s)
    {
    }

    GlobalShortcut* shortcut;

    void notify(int code)
    {
        emit shortcut->activated(code);
    }
#ifdef Q_OS_MAC
    static QHash<int, EventHotKeyRef*> keyref;
#endif
};

QHash<int, EventHotKeyRef*> GlobalShortcutPrivate::keyref;

#ifdef Q_OS_MAC
static OSStatus hotKeyHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    Q_UNUSED(nextHandler)
    Q_UNUSED(userData)

    EventHotKeyID hotKeyID;
    GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, NULL, sizeof(hotKeyID), NULL, &hotKeyID);

    GlobalShortcutPrivate* shortcut = reinterpret_cast<GlobalShortcutPrivate*>(userData);
    shortcut->notify(hotKeyID.id);

    return noErr;
}
#endif

GlobalShortcut::GlobalShortcut(QObject* parent)
    : QObject(parent), priv(new GlobalShortcutPrivate(this))
{
    static bool first = true;
    if (first) {
        first = false;

#ifdef Q_OS_MAC
        EventTypeSpec spec;

        spec.eventClass = kEventClassKeyboard;
        spec.eventKind = kEventHotKeyPressed;

        InstallApplicationEventHandler(&hotKeyHandler, 1, &spec, priv, NULL);
#endif
    }
}

GlobalShortcut::~GlobalShortcut()
{
    delete priv;
}

int GlobalShortcut::registerShortcut(int keycode, int modifier)
{
    static int id = 1;

#ifdef Q_OS_MAC
    while (GlobalShortcutPrivate::keyref.contains(id))
        ++id;

    EventHotKeyRef* ref = new EventHotKeyRef;
    EventHotKeyID keyid;
    keyid.signature = 1122;
    keyid.id = id;
    RegisterEventHotKey(keycode, modifier, keyid, GetEventDispatcherTarget(), 0, ref);

    GlobalShortcutPrivate::keyref[id] = ref;
#endif

    return id++;
}

void GlobalShortcut::unregisterShortcut(int id)
{
#ifdef Q_OS_MAC
    QHash<int, EventHotKeyRef*>::iterator it = GlobalShortcutPrivate::keyref.find(id);
    if (it != GlobalShortcutPrivate::keyref.end()) {
        UnregisterEventHotKey(**it);
        delete *it;
        GlobalShortcutPrivate::keyref.erase(it);
    }
#endif
}
