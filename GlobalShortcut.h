#ifndef GLOBALSHORTCUT_H
#define GLOBALSHORTCUT_H

#include <QObject>

class GlobalShortcutPrivate;

class GlobalShortcut : public QObject
{
    Q_OBJECT
public:
    GlobalShortcut(QObject* parent);
    ~GlobalShortcut();

    int registerShortcut(int keycode, int modifier);
    void unregisterShortcut(int id);

signals:
    void activated(int id);

private:
    GlobalShortcutPrivate* priv;

    friend class GlobalShortcutPrivate;
};

#endif // GLOBALSHORTCUT_H
