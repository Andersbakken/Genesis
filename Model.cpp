#include "Model.h"
#include <ModelThread.h>

static inline bool lessThan(const Match &left, const Match &right)
{
    return left.name.size() > right.name.size();
}

static QVariantList defaultUrlHandlers()
{
    QVariantList ret;
    {
        QMap<QString, QVariant> map;
        map["url"] = "http://www.google.com/search?ie=UTF-8&q=%s";
        map["icon"] = ":/google.ico";
        ret.append(map);
    }
    {
        QMap<QString, QVariant> map;
        map["url"] = "http://en.wikipedia.org/wiki/Special:Search?search=%s&go=Go";
        map["icon"] = ":/wikipedia.ico";
        ret.append(map);
    }
    {
        QMap<QString, QVariant> map;
        map["url"] = "http://www.amazon.com/s?url=search-alias=aps&field-keywords=%s";
        map["icon"] = ":/amazon.ico";
        ret.append(map);
    }

    return ret;
}

Model::Model(const QStringList &roots, QObject *parent)
    : QObject(parent), mRoots(roots), mFileSystemWatcher(0)
{
    Config config;
    mUserEntries = config.value<QVariantMap>("userEntries");
    mUrlHandlers = config.value<QVariantList>("urlHandlers", defaultUrlHandlers());
    reload();
}

static inline QString name(const QString &path)
{
    const int lastSlash = path.lastIndexOf(QLatin1Char('/'));
    Q_ASSERT(lastSlash != -1);
#ifdef Q_OS_MAC
    return path.mid(lastSlash + 1, path.size() - lastSlash - 5);
#else
    return path.mid(lastSlash + 1);
#endif
}

static inline QIcon iconForPath(const QString &path)
{
    if (!path.isEmpty()) {
        return QIcon(path);
    } else {
        Q_ASSERT(qApp);
        return qApp->style()->standardIcon(QStyle::SP_FileIcon);
    }
}

QList<Match> Model::matches(const QString &text) const
{
    // ### should match on stuff like "word" for "Microsoft Word"
    QList<Match> matches;
    QRegExp rx("\\b" + text);
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    if (!text.isEmpty()) {
        const int count = mItems.size();
        bool foundPreviousUserEntry = false;
        const QString userEntryPath = mUserEntries.value(text).toString();
        // $$$ This really should be a QMap<QString, QString> or a sorted stringlist or something
        for (int i=0; i<count; ++i) {
            const Item &item = mItems.at(i);
            const int slash = item.filePath.lastIndexOf('/');
            Q_ASSERT(slash != -1);
            const QString name = ::name(item.filePath);
            if (name.contains(rx)) {
                const Match m(Match::Application, name, item.filePath, item.iconPath.isEmpty()
                              ? mFileIconProvider.icon(QFileInfo(item.filePath))
                              : QIcon(item.iconPath));
                if (userEntryPath == item.filePath) {
                    foundPreviousUserEntry = true;
                    matches.prepend(m);
                } else {
                    matches.append(m);
                }
            }
        }
        if (matches.size() > 1) {
            QList<Match>::iterator it = matches.begin();
            if (foundPreviousUserEntry)
                ++it;
            qSort(it, matches.end(), lessThan); // ### hm, what to do about this one
        }
        const int urlHandlerCount = mUrlHandlers.size(); // ### could store the matches and modify them here
        for (int i=0; i<urlHandlerCount; ++i) {
            QVariantMap map = qVariantValue<QVariantMap>(mUrlHandlers.at(i));
            QString url = map.value("url").toString();
            if (url.isEmpty()) {
                qWarning() << "Invalid urlhandler" << map;
                continue;
            }
            url.replace("%s", QUrl::toPercentEncoding(text));

            QString name = map.value("name", text).toString();
            name.replace("%s", text);

            // ### clever default icon?
            matches.append(Match(Match::Url, name, url, QIcon(map.value("icon").toString())));
        }
    }
    return matches;
}

const QStringList & Model::roots() const
{
    return mRoots;
}

void Model::reload()
{
    ModelThread *thread = new ModelThread(this);
    thread->start();
}

void Model::recordUserEntry(const QString &input, const QString &path)
{
    mUserEntries[input] = path;
    Config config;
    config.setValue("userEntries", mUserEntries);
}
