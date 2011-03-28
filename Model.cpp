#include "Model.h"
#include <ModelThread.h>

static inline bool lessThan(const Match &left, const Match &right)
{
    return left.name.size() > right.name.size();
}

Model::Model(const QStringList &roots, QObject *parent)
    : QObject(parent), mRoots(roots), mFileSystemWatcher(0)
{
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

static inline QIcon googleIcon()
{
    static const QIcon icon(":/google.ico");
    return icon;
}

static inline QIcon wikipediaIcon()
{
    static const QIcon icon(":/wikipedia.ico");
    return icon;
}

QList<Match> Model::matches(const QString &text) const
{
    // ### should match on stuff like "word" for "Microsoft Word"
    QList<Match> matches;
    if (!text.isEmpty()) {
        const int count = mItems.size();
        for (int i=0; i<count; ++i) {
            const Item &item = mItems.at(i);
            const int slash = item.filePath.lastIndexOf('/');
            Q_ASSERT(slash != -1);
            const QString name = ::name(item.filePath);
            if (name.startsWith(text, Qt::CaseInsensitive)) {
                matches.append(Match(Match::Application, name, item.filePath, item.iconPath.isEmpty()
                                     ? mFileIconProvider.icon(QFileInfo(item.filePath))
                                     : QIcon(item.iconPath)));
            }
        }
        qSort(matches.begin(), matches.end(), lessThan);
        enum { MaxCount = 10 };
        while (matches.size() > MaxCount) {
            matches.removeLast();
            // ### could do insertion sort and not add more than MaxCount then
        }
        matches.append(Match(Match::Url, QString("Search Google for '%1'").arg(text),
                             "http://www.google.com/search?ie=UTF-8&q=" + QUrl::toPercentEncoding(text),
                             googleIcon()));
        extern const Qt::KeyboardModifier numericModifier;
        matches.last().keySequence = QKeySequence(numericModifier | Qt::Key_G);
        matches.append(Match(Match::Url, QString("Search Wikipedia for '%1'").arg(text),
                             QString("http://en.wikipedia.org/wiki/Special:Search?search=%1&go=Go").
                             arg(QString::fromUtf8(QUrl::toPercentEncoding(text))),
                             wikipediaIcon()));
    }
    return matches;
}

const QStringList & Model::roots() const
{
    return mRoots;
}

void Model::reload()
{
    printf("%s %d: void Model::reload()\n", __FILE__, __LINE__);
    ModelThread *thread = new ModelThread(this);
    thread->start();
}
