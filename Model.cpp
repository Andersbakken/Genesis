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

bool Model::ItemIndex::matches(const QString &text) const
{
    return key.toLower().startsWith(text);
}

bool Model::UserEntry::matches(const QString &text) const
{
    return input.startsWith(text);
}

Model::Model(const QByteArray &roots, QObject *parent)
    : QObject(parent), mRoots(roots.split(':')), mFileSystemWatcher(0)
{
    Config config;
    restoreUserEntries(&config);
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
                    const Match m(Match::Application, item->name, item->filePath, item->iconPath.isEmpty()
                                  ? mFileIconProvider.icon(QFileInfo(item->filePath))
                                  : QIcon(item->iconPath), item->arguments);
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

        // ### could store the matches and modify them here
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
    rebuildIndex();
}

void Model::rebuildIndex()
{
    mItemIndex.clear();

    // Add the default application entries
    Item item;
    const int appHandlerCount = mAppHandlers.size();
    for (int i = 0; i < appHandlerCount; ++i) {
        const QStringList& handler = mAppHandlers.at(i);
        const int handlerSize = handler.size();

        item.name = handler.at(0);
        item.filePath = handler.at(1);
        item.iconPath = (handlerSize > 2) ? handler.at(2) : QString();
        item.arguments.clear();
        for (int i = 3; i < handlerSize; ++i) {
            item.arguments << handler.at(i);
        }
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
