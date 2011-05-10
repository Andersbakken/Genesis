#include "FileIconProvider.h"
#include <QFile>
#include <stdlib.h>
#include <QDebug>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
typedef QHash<QString, QIcon> iconHash;
Q_GLOBAL_STATIC(iconHash, globalIconCache)

static QIcon readIconFrom(const QByteArray& path, const QByteArray& iconname)
{
    if (path.isEmpty())
        return QIcon();

    QByteArray fn;
    static const char* exts[] = { ".xpm", ".png" };
    for (int i = 0; ; ++i) {
        if (!exts[i])
            break;
        fn = path + iconname + exts[i];
        if (QFile::exists(QString::fromLocal8Bit(fn.constData(), fn.size()))) {
            QIcon icn(QString::fromLocal8Bit(fn.constData(), fn.size()));
            return icn;
        }
    }

    return QIcon();
}

static QByteArray iconEnv(const char* var, const QByteArray& suffix = QByteArray())
{
    QByteArray env(::getenv(var));
    if (env.isEmpty())
        return QByteArray();
    return env + suffix;
}

static QIcon readIcon(const QByteArray& iconname)
{
    if (iconname.startsWith('/'))
        return QIcon(iconname);

    QIcon icon;
    icon = readIconFrom(iconEnv("HOME", "/.icons/"), iconname);
    if (!icon.isNull())
        return icon;
    QList<QByteArray> xdgs = QByteArray(::getenv("XDG_DATA_DIRS")).split(':');
    foreach(const QByteArray& xdg, xdgs) {
        icon = readIconFrom(xdg + "/icons/", iconname);
        if (!icon.isNull())
            return icon;
    }
    icon = readIconFrom(QByteArray("/usr/share/pixmaps/"), iconname);
    return icon;
}
#endif

FileIconProvider::FileIconProvider()
{
}

QIcon FileIconProvider::icon(const QFileInfo &info) const
{
    QIcon defaulticon = QFileIconProvider::icon(info);
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // Try to find icons as defined in the icon theme specification
    QString app = info.fileName();

    iconHash::const_iterator cacheit = globalIconCache()->find(app);
    if (cacheit != globalIconCache()->end())
        return cacheit.value();

    static QString share = QLatin1String("/usr/share/applications/");
    static QString desktop = QLatin1String(".desktop");

    QFile appfile(share + app + desktop);
    if (!appfile.open(QFile::ReadOnly))
        return defaulticon;

    QIcon icn;
    QList<QByteArray> lines = appfile.readAll().split('\n');
    foreach(const QByteArray& line, lines) {
        if (line.toLower().startsWith("icon=")) {
            icn = readIcon(line.mid(5));
            if (!icn.isNull()) {
                globalIconCache()->insert(app, icn);
                return icn;
            }
        }
    }

    icn = readIcon(app.toLocal8Bit());
    if (!icn.isNull()) {
        globalIconCache()->insert(app, icn);
        return icn;
    }
    globalIconCache()->insert(app, defaulticon);
#endif
    return defaulticon;
}
