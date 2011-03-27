#include "Model.h"

Model * Model::create(const QStringList &roots, QObject *parent)
{
    Model *model = new Model(roots, parent);
    ModelThread *thread = new ModelThread(model);
    thread->run();
    return model;
}

QList<Match> Model::matches(const QString &text) const
{
    QList<Match> matches;
    const int rootCount = mRoots.size();
    for (int r=0; r<rootCount; ++r) {
        const QStringList &list = mMatches.at(i);
        const int count = list.size();
        for (int i=0; i<count; ++i) {
            const QString &item = list.at(i);
            if (text == item) {
                matches.prepend(mRoots.at(i) item
            }
    }


}

const QStringList & Model::all() const
{
    return mAll;
}

const QStringList & Model::roots() const
{
    return mMatches;
}
