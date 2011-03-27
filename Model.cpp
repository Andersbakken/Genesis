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
    mData.resize(mRoots.size());
}

static inline QString fileName(const QString &path)
{
    Q_ASSERT(path.contains(QLatin1Char('/')));
    return path.mid(path.lastIndexOf(QLatin1Char('/')));
}

QList<Match> Model::matches(const QString &text) const
{
    QList<Match> matches;
    const int rootCount = mRoots.size();
    for (int r=0; r<rootCount; ++r) {
        const QList<QPair<QString, QIcon> > &list = mData.at(r);
        const int count = list.size();
        for (int i=0; i<count; ++i) {
            const QPair<QString, QIcon> &item = list.at(i);
            if (item.first.startsWith(text)) {
                matches.append(Match(Match::Application, mRoots.at(r) + QLatin1Char('/') + text, text, item.second));
            }
        }
    }
    qSort(matches.begin(), matches.end(), lessThan);
    return matches;
}

const QStringList & Model::roots() const
{
    return mRoots;
}
