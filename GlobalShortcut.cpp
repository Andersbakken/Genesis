#include "GlobalShortcut.h"
#include <QWidget>
#include <QHash>
#include <QDebug>
#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>

struct Shortcut
{
    GlobalShortcut* creator;
    EventHotKeyRef* keyref;
};

#endif

class GlobalShortcutPrivate
{
public:
    GlobalShortcutPrivate(GlobalShortcut* s)
        : shortcut(s)
    {
    }
    ~GlobalShortcutPrivate()
    {
#ifdef Q_OS_MAC
        QHash<int, Shortcut>::iterator it = shortcuts.begin();
        while (it != shortcuts.end()) {
            if (it.value().creator == shortcut) {
                UnregisterEventHotKey(*(it.value().keyref));
                delete it.value().keyref;
                it = shortcuts.erase(it);
            } else
                ++it;
        }
#endif
    }

    GlobalShortcut* shortcut;

    void notify(int code)
    {
        emit shortcut->activated(code);
    }
#ifdef Q_OS_MAC
    static QHash<int, Shortcut> shortcuts;
#endif
};

QHash<int, Shortcut> GlobalShortcutPrivate::shortcuts;

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

int GlobalShortcut::registerShortcut(int keycode, int modifier)
{
    static int id = 1;

#ifdef Q_OS_MAC
    while (GlobalShortcutPrivate::shortcuts.contains(id))
        ++id;

    EventHotKeyRef* ref = new EventHotKeyRef;
    EventHotKeyID keyid;
    keyid.signature = 1122;
    keyid.id = id;
    RegisterEventHotKey(keycode, modifier, keyid, GetEventDispatcherTarget(), 0, ref);

    Shortcut shortcut;
    shortcut.creator = this;
    shortcut.keyref = ref;
    GlobalShortcutPrivate::shortcuts[id] = shortcut;
#endif

    return id++;
}

void GlobalShortcut::unregisterShortcut(int id)
{
#ifdef Q_OS_MAC
    QHash<int, Shortcut>::iterator it = GlobalShortcutPrivate::shortcuts.find(id);
    if (it != GlobalShortcutPrivate::shortcuts.end()) {
        if (it.value().creator != this)
            return;

        UnregisterEventHotKey(*(it.value().keyref));
        delete it.value().keyref;
        GlobalShortcutPrivate::shortcuts.erase(it);
    }
#endif
}
