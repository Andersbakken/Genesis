#include "Model.h"
#include <ModelThread.h>

static inline bool lessThan(const Match &left, const Match &right)
{
    return left.name.size() < right.name.size();
}

static QStringList defaultUrlHandlers()
{
    QStringList ret;
    ret << "%s|http://www.google.com/search?ie=UTF-8&q=%s|:/google.png"
        << "%s|http://en.wikipedia.org/wiki/Special:Search?search=%s&go=Go|:/wikipedia.png"
        << "%s|http://www.amazon.com/s?url=search-alias=aps&field-keywords=%s|:/amazon.png";
    return ret;
}

static QStringList defaultAppHandlers()
{
    QStringList ret;
#ifdef Q_OS_MAC
    ret << "Lock|/System/Library/CoreServices/Menu Extras/User.menu/Contents/Resources/CGSession|:/lock.png|-suspend";
#endif
    return ret;
}

Model::Model(const QByteArray &roots, QObject *parent)
    : QObject(parent), mRoots(roots.split(':')), mFileSystemWatcher(0)
{
    Config config;
    mUserEntries = config.value<QVariantMap>("userEntries");
    bool ok;
    QStringList list = config.value<QStringList>("urlHandlers", defaultUrlHandlers(), &ok);
    if (ok && config.isEnabled("defaultUrlHandlers", true))
        list += defaultUrlHandlers();
    foreach(const QString &string, list) {
        const QStringList list = string.split('|');
        if (list.size() != 3 || list.at(1).isEmpty()) {
            qWarning("Invalid url handler %s", qPrintable(string));
            continue;
        }
        mUrlHandlers.append(list);
    }
    list = config.value<QStringList>("appHandlers", defaultAppHandlers(), &ok);
    if (ok && config.isEnabled("defaultAppHandlers", true))
        list += defaultAppHandlers();
    foreach(const QString &string, list) {
        const QStringList list = string.split('|');
        if (list.size() < 2) {
            qWarning("Invalid app handler %s", qPrintable(string));
            continue;
        }
        mAppHandlers.append(list);
    }

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
        const int appHandlerCount = mAppHandlers.size();
        for (int i = 0; i < appHandlerCount; ++i) {
            const QStringList& handler = mAppHandlers.at(i);
            QString name = handler.at(0);
            QString filepath = handler.at(1);
            const int handlerSize = handler.size();
            QString icon = (handlerSize >= 3) ? handler.at(2) : QString();
            QStringList args;
            for (int i = 3; i < handlerSize; ++i) {
                args << handler.at(i);
            }
            if (name.contains(rx)) {
                const Match m(Match::Application, name, filepath, (icon.isEmpty() ? mFileIconProvider.icon(QFileInfo(filepath)) : QIcon(icon)), args);
                if (userEntryPath == filepath) {
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
            const QStringList &handler = mUrlHandlers.at(i);
            QString url = handler.at(1);
            url.replace("%s", QUrl::toPercentEncoding(text));

            QString name = handler.at(0);
            if (name.isEmpty()) {
                name = text;
            } else {
                name.replace("%s", text);
            }

            // ### clever default icon?
            matches.append(Match(Match::Url, name, url, QIcon(handler.at(2))));
        }
        if (text.startsWith("http://")) {
            matches.append(Match(Match::Url, "Open " + text, text, QIcon()));
        }
    }
    return matches;
}

const QList<QByteArray> & Model::roots() const
{
    return mRoots;
}

void Model::reload()
{
    ModelThread *thread = new ModelThread(this);
    connect(thread, SIGNAL(itemsReady(QList<Model::Item>)), this, SLOT(updateItems(QList<Model::Item>)), Qt::QueuedConnection);
    thread->start();
}

void Model::updateItems(const QList<Item> &newItems)
{
    mItems = newItems;
}

void Model::recordUserEntry(const QString &input, const QString &path)
{
    const int inputLength = input.length();
    if (!inputLength)
        return;

    for (int i=0; i<inputLength; ++i) {
        const QString entry = input.left(i + 1);
        mUserEntries[entry] = path;
    }

    Config config;
    config.setValue("userEntries", mUserEntries);
}

void Model::registerItem()
{
    qRegisterMetaType<QList<Model::Item> >("QList<Model::Item>");
}
