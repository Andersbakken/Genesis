#include "FileIconProvider.h"
#include <QFile>
#include <stdlib.h>
#include <dirent.h>
#include <QDebug>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
typedef QHash<QString, QIcon> iconHash;
Q_GLOBAL_STATIC(iconHash, globalIconCache)

#define MAX_ICON_RECURSE_DEPTH 1

static QIcon readIconFrom(const QByteArray& path, const QByteArray& iconname, int depth = 0)
{
    if (path.isEmpty() || depth > MAX_ICON_RECURSE_DEPTH)
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

    QIcon icn;
    DIR *dir = opendir(path.constData());
    if (!dir)
        return QIcon();
    struct dirent d, *res;
    while (readdir_r(dir, &d, &res) == 0 && res != 0) {
        if (d.d_type == DT_DIR) {
            if (strcmp(d.d_name, ".") && strcmp(d.d_name, "..")) {
                icn = readIconFrom(path + '/' + QByteArray(d.d_name), iconname, depth + 1);
                if (!icn.isNull()) {
                    closedir(dir);
                    return icn;
                }
            }
        }
    }

    closedir(dir);

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
    if (!icon.isNull())
        return icon;
    icon = readIconFrom(QByteArray("/usr/share/icons/gnome/48x48/"), iconname);
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
