#ifndef GLOBALSHORTCUT_H
#define GLOBALSHORTCUT_H

class GlobalShortcutPrivate;
class QWidget;

class GlobalShortcut
{
public:
    GlobalShortcut(QWidget* toplevel);
    ~GlobalShortcut();

    void registerShortcut(int keycode, int modifier);
    void unregisterShortcut();

    void notify();

private:
    GlobalShortcutPrivate* priv;
};

#endif // GLOBALSHORTCUT_H
