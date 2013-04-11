#include "Model.h"
#include <ModelThread.h>

#define MAX_MATCH_COUNT 9

static inline bool matchLessThan(const Match &left, const Match &right)
{
    return left.name.size() < right.name.size();
}

static inline bool indexLessThan(const Model::ItemIndex &left, const Model::ItemIndex &right)
{
    return left.key < right.key;
}

static inline bool userEntryLessThan(const Model::UserEntry &left, const Model::UserEntry &right)
{
    return left.input < right.input;
}

static QVariantList defaultUrlHandlers()
{
    QVariantList ret;

    QVariantMap map;
    map["url"] = "http://www.google.com/search?ie=UTF-8&q=%s";
    map["icon"] = QIcon(":/google.png");
    ret.append(map);

    map["url"] = "http://en.wikipedia.org/wiki/Special:Search?search=%s&go=Go";
    map["icon"] = QIcon(":/wikipedia.png");
    ret.append(map);

    map["url"] = "http://www.amazon.com/s?url=search-alias=aps&field-keywords=%s";
    map["icon"] = QIcon(":/amazon.png");
    ret.append(map);
    return ret;
}

static QVariantList defaultAppHandlers()
{
    QVariantList ret;

#ifdef Q_OS_MAC
    QVariantMap map;
    map["name"] = "Lock";
    map["command"] = "/System/Library/CoreServices/Menu Extras/User.menu/Contents/Resources/CGSession";
    map["arguments"] = (QStringList() << "-suspend");
    map["icon"] = ":/lock.png";
    ret.append(map);
#endif

    return ret;
}

bool Model::ItemIndex::matches(const QString &text) const
{
    return key.toLower().startsWith(text);
}

bool Model::UserEntry::matches(const QString &text) const
{
    return input.startsWith(text);
}

Model::Model(const QByteArray &roots, QWidget *parent)
    : QObject(parent), mRoots(roots.split(':')), mFileIconProvider(parent), mFileSystemWatcher(0)
{
    Config config;
    restoreUserEntries(&config);
    {
        QVariantList list = config.value<QVariantList>("urlHandlers");
        if (config.isEnabled("defaultUrlHandlers", true))
            list += defaultUrlHandlers();
        foreach(const QVariant &map, list) {
            if (map.type() != QVariant::Map) {
                qWarning("Invalid url handler %s", qPrintable(map.toString()));
                continue;
            }
            const QVariantMap m = map.toMap();
            if (!m.contains("url")) {
                qWarning("Invalid url handler %s", qPrintable(map.toString()));
                continue;
            }
            mUrlHandlers.append(m);
        }
    }
    {
        QVariantList list = config.value<QVariantList>("appHandlers");
        if (config.isEnabled("defaultAppHandlers", true))
            list += defaultAppHandlers();
        foreach(const QVariant &map, list) {
            if (map.type() != QVariant::Map) {
                qWarning("Invalid app handler %s", qPrintable(map.toString()));
                continue;
            }
            const QVariantMap m = map.toMap();
            if (!m.contains("command") || !m.contains("name")) {
                qWarning("Invalid app handler %s", qPrintable(map.toString()));
                continue;
            }
            mAppHandlers.append(m);
        }
    }

    reload();
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
    if (!text.isEmpty()) {
        const QString match = text.toLower();

        const QHash<QString, int> userEntries = findUserEntries(match);
        const QHash<QString, int>::const_iterator userEntriesEnd = userEntries.end();
        QHash<QString, int>::const_iterator userEntriesPos;
        QMap<int, Match> usermatches;

        const int urlHandlerCount = mUrlHandlers.size();
        const int userEntriesSize = userEntries.size();
        const int maxMatches = MAX_MATCH_COUNT - urlHandlerCount;

        QHash<Match, int> matchcount;
        ItemIndex findIndex;

        QStringList tofind = match.split(QLatin1Char(' '), QString::SkipEmptyParts);

        // ### should be a better way at looking up multiple indices and only keeping the intersection than this

        // loop for each index
        foreach(const QString& strfind, tofind) {
            findIndex.key = strfind;

            // find the index and keep going until the next item in the list does not match the index string
            QList<ItemIndex>::const_iterator found = qLowerBound(mItemIndex.begin(), mItemIndex.end(), findIndex, indexLessThan);
            const QList<ItemIndex>::const_iterator indexEnd = mItemIndex.end();
            while (found != indexEnd && (*found).matches(strfind)) {
                foreach(const Item* item, (*found).items) {
                    const Match m(Match::Application, item->name, item->filePath, QIcon(item->iconPath), item->arguments);
                    if (matchcount.contains(m))
                        matchcount[m] += 1;
                    else {
                        if ((userEntriesPos = userEntries.find(item->filePath)) == userEntriesEnd) {
                            matches.append(m);
                        } else {
                            usermatches.insert(userEntriesSize - userEntriesPos.value(), m);
                        }
                        matchcount[m] = 1;
                    }
                }

                ++found;
            }
        }

        // now remove all items that were not found the same amount of times as we have indices in the tofind list
        const int tofindsize = tofind.size();
        if (tofindsize > 1) {
            QList<Match>::iterator mit = matches.begin();
            while (mit != matches.end()) {
                if (matchcount.value(*mit) != tofindsize)
                    mit = matches.erase(mit);
                else
                    ++mit;
            }
            QMap<int, Match>::iterator umit = usermatches.begin();
            while (umit != usermatches.end()) {
                if (matchcount.value(umit.value()) != tofindsize)
                    umit = usermatches.erase(umit);
                else
                    ++umit;
            }
        }

        if (matches.size() > 1) {
            qSort(matches.begin(), matches.end(), matchLessThan); // ### hm, what to do about this one
        }

        foreach(const Match &m, usermatches) {
            matches.prepend(m);
        }

        // ### not optimal at all
        while (matches.size() > maxMatches)
            matches.removeLast();

        QList<Match>::iterator matchit = matches.begin();
        QList<Match>::const_iterator matchend = matches.end();
        while (matchit != matchend) {
            if ((*matchit).icon.isNull()) {
                (*matchit).icon = mFileIconProvider.icon(QFileInfo((*matchit).filePath));
            }
            ++matchit;
        }

        // ### could store the matches and modify them here
        for (int i=0; i<urlHandlerCount; ++i) {
            const QVariantMap &handler = mUrlHandlers.at(i);
            QString url = handler.value("url").toString();
            url.replace("%s", QUrl::toPercentEncoding(text));

            QString name = handler.value("name").toString();
            if (name.isEmpty()) {
                name = text;
            } else {
                name.replace("%s", text);
            }

            // ### clever default icon?
            QVariant icon = handler.value("icon");
            QIcon icn;
            if (icon.type() == QVariant::Icon) {
                icn = qVariantValue<QIcon>(icon);
            } else {
                icn = QIcon(icon.toString());
            }
            matches.append(Match(Match::Url, name, url, icn));
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
    connect(thread, SIGNAL(pathsSearched(QStringList)), this, SLOT(searchPathsChanged(QStringList)));
    connect(thread, SIGNAL(itemsReady(QList<Model::Item>)), this, SLOT(updateItems(QList<Model::Item>)), Qt::QueuedConnection);
    thread->start();
}

void Model::searchPathsChanged(const QStringList &paths)
{
    delete mFileSystemWatcher;
    mFileSystemWatcher = new QFileSystemWatcher(this);
    connect(mFileSystemWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(reload()));
    mFileSystemWatcher->addPaths(paths);
}

void Model::updateItems(const QList<Item> &newItems)
{
    mItems = newItems;
    rebuildIndex();
}

void Model::rebuildIndex()
{
    mItemIndex.clear();

    // Add the default application entries
    Item item;
    const int appHandlerCount = mAppHandlers.size();
    for (int i = 0; i < appHandlerCount; ++i) {
        const QVariant& handler = mAppHandlers.at(i);
        const QVariantMap& map = handler.toMap();

        item.name = map.value("name").toString();
        item.filePath = map.value("command").toString();
        item.iconPath = map.value("icon").toString();
        item.arguments = map.value("arguments").toStringList();
        mItems.append(item);
    }

    // Build the index
    QStringList words;
    QList<Model::ItemIndex>::iterator pos;
    Model::ItemIndex idx;
    bool found;
    foreach(const Item& item, mItems) {
        words = item.name.split(QLatin1Char(' '));

        foreach(const QString& key, words) {
            idx.key = key.toLower();
            found = false;
            pos = qLowerBound(mItemIndex.begin(), mItemIndex.end(), idx, indexLessThan);
            if (pos != mItemIndex.end()) {
                // Check if we found an exact match
                if ((*pos).key == key) {
                    // yay!
                    (*pos).items.append(&item);
                    found = true;
                }
            }
            if (!found) { // insert a index
                idx.items.clear();
                idx.items.append(&item);
                mItemIndex.insert(pos, idx);
            }
        }
    }
    emit indexRebuilt();
}

QHash<QString, int> Model::findUserEntries(const QString &input) const
{
    QHash<QString, int> paths;

    UserEntry entry;
    entry.input = input;

    int count = -1; // to be able to use prefix increase below

    const QList<UserEntry>::const_iterator posend = mUserEntries.end();
    QList<UserEntry>::const_iterator pos = qLowerBound(mUserEntries.begin(), posend, entry, userEntryLessThan);
    while (pos != posend && (*pos).matches(input)) {
        foreach(const QString &path, (*pos).paths) {
            // ### this is not really optimal, should have some way of being able to do a conditional insert
            if (paths.contains(path))
                continue;
            paths[path] = ++count;
        }

        ++pos;
    }

    return paths;
}

void Model::recordUserEntry(const QString &input, const QString &path)
{
    const int inputLength = input.length();
    if (!inputLength)
        return;

    const QString match = input.toLower();
    UserEntry entry;
    entry.input = match;

    bool found = false;
    QList<UserEntry>::iterator pos = qLowerBound(mUserEntries.begin(), mUserEntries.end(), entry, userEntryLessThan);
    if (pos != mUserEntries.end()) {
        if ((*pos).input == match) {
            const QList<QString>::iterator pathit = qFind((*pos).paths.begin(), (*pos).paths.end(), path);
            if (pathit != (*pos).paths.end())
                (*pos).paths.erase(pathit);
            (*pos).paths.prepend(path);
            found = true;
        }
    }
    if (!found) {
        entry.paths.append(path);
        mUserEntries.insert(pos, entry);
    }

    Config config;
    saveUserEntries(&config);
}

void Model::restoreUserEntries(Config *config)
{
    QVariantList list = config->value<QVariantList>("userEntries");

    UserEntry entry;
    while (list.size() >= 2) {
        entry.input = list.takeFirst().toString();
        entry.paths = list.takeFirst().toStringList();
        mUserEntries.append(entry);
    }
}

void Model::saveUserEntries(Config *config)
{
    QVariantList list;
    foreach(const UserEntry& entry, mUserEntries) {
        list.append(entry.input);
        list.append(entry.paths);
    }

    config->setValue("userEntries", list);
}

void Model::registerItem()
{
    qRegisterMetaType<QList<Model::Item> >("QList<Model::Item>");
}
