#ifndef ModelThread_h
#define ModelThread_h

#include <Model.h>

class ModelThread : public QThread
{
    Q_OBJECT
public:
    ModelThread(Model *model);
    virtual void run();
    void recurse(const QByteArray &path, int maxDepth);

signals:
    void itemsReady(const QList<Model::Item> &newItems);

private:
    Model *mModel;
    QSet<QString> mWatchPaths;
    QFileSystemWatcher *mWatcher;

    QList<Model::Item> mLocalItems;
};

#endif
