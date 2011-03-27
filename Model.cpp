#include "Model.h"
#include <ModelThread.h>

Model *Model::create(const QStringList &roots, QObject *parent)
{
    Model *model = new Model(roots, parent);
    ModelThread *thread = new ModelThread(model);
    thread->start();
    return model;
}

static inline bool lessThan(const Match &left, const Match &right)
{
    return left.text.size() > right.text.size();
}

Model::Model(const QStringList &roots, QObject *parent)
    : QObject(parent), mRoots(roots)
{
}

static inline QString fileName(const QString &path)
{
    Q_ASSERT(path.contains(QLatin1Char('/')));
    return path.mid(path.lastIndexOf(QLatin1Char('/')));
}

static inline QIcon iconForPath(const QString &path)
{
    if (path.isEmpty()) {
        return QIcon(path);
    } else {
        Q_ASSERT(qApp);
        return qApp->style()->standardIcon(QStyle::SP_FileIcon);
    }
}

QList<Match> Model::matches(const QString &text) const
{
    QList<Match> matches;
    const int count = mItems.size();
    for (int i=0; i<count; ++i) {
        const Item &item = mItems.at(i);
        const int slash = item.filePath.lastIndexOf('/');
        Q_ASSERT(slash != -1);
        const QString name = item.filePath.mid(slash + 1);
        if (name.startsWith(text)) {
            matches.append(Match(Match::Application, item.filePath, name, iconForPath(item.iconPath)));
        }
    }
    qSort(matches.begin(), matches.end(), lessThan);
    return matches;
}

const QStringList & Model::roots() const
{
    return mRoots;
}
