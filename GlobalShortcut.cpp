#include "GlobalShortcut.h"
#include <QWidget>
#include <QDebug>

#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>

static OSStatus hotKeyHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    Q_UNUSED(nextHandler)
    Q_UNUSED(userData)

    EventHotKeyID hotKeyID;
    GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, NULL, sizeof(hotKeyID), NULL, &hotKeyID);

    if (hotKeyID.id == 1) {
        GlobalShortcut* shortcut = reinterpret_cast<GlobalShortcut*>(userData);
        shortcut->notify();
    }

    return noErr;
}
#endif

class GlobalShortcutPrivate
{
public:
#ifdef Q_OS_MAC
    GlobalShortcutPrivate()
    {
        keyref = 0;
    }

    EventHotKeyRef* keyref;
#endif
    QWidget* toplevel;
};

GlobalShortcut::GlobalShortcut(QWidget* toplevel)
    : priv(new GlobalShortcutPrivate)
{
    priv->toplevel = toplevel;

    static bool first = true;
    if (first) {
        first = false;

#ifdef Q_OS_MAC
        EventTypeSpec spec;

        spec.eventClass = kEventClassKeyboard;
        spec.eventKind = kEventHotKeyPressed;

        InstallApplicationEventHandler(&hotKeyHandler, 1, &spec, this, NULL);
#endif
    }
}

GlobalShortcut::~GlobalShortcut()
{
    delete priv;
}

void GlobalShortcut::registerShortcut(int keycode, int modifier)
{
    unregisterShortcut();
#ifdef Q_OS_MAC
    priv->keyref = new EventHotKeyRef;

    EventHotKeyID keyid;
    keyid.signature = 1000;
    keyid.id = 1;
    RegisterEventHotKey(keycode, modifier, keyid, GetEventDispatcherTarget(), 0, priv->keyref);
#endif
}

void GlobalShortcut::unregisterShortcut()
{
#ifdef Q_OS_MAC
    if (priv->keyref) {
        UnregisterEventHotKey(*(priv->keyref));
        delete priv->keyref;
        priv->keyref = 0;
    }
#endif
}

void GlobalShortcut::notify()
{
    priv->toplevel->show();
}
