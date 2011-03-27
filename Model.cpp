#include "Model.h"
#include <ModelThread.h>

Model *Model::create(const QStringList &roots, QObject *parent)
{
    Model *model = new Model(roots, parent);
    ModelThread *thread = new ModelThread(model);
    thread->run();
    return model;
}

static inline bool lessThan(const QString &left, const QString &right)
{
    return left.size() > right.size();
}

Model::Model(const QStringList &roots, QObject *parent)
    : QObject(parent), mRoots(roots)
{
    mData.resize(mRoots.size());
}

QList<Match> Model::matches(const QString &text) const
{
    QList<Match> matches;
    const int rootCount = mRoots.size();
    for (int r=0; r<rootCount; ++r) {
        const QList<QPair<QString, QIcon> > &list = mData.at(r);
        const int count = list.size();
        for (int i=0; i<count; ++i) {
            const QString &item = list.at(i);
            if (item.startsWith(text)) {
                matches.append(mRoots.at(r) + QLatin1Char('/') + text);
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
