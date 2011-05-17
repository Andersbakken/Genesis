#ifndef ModelThread_h
#define ModelThread_h

#include <Model.h>

class ModelThread : public QThread
{
    Q_OBJECT
public:
    ModelThread(Model *model);
    virtual void run();
    void scan(const QByteArray &path);

signals:
    void itemsReady(const QList<Model::Item> &newItems);
    void pathsSearched(const QStringList& paths);

private:
    Model *mModel;
    QSet<QString> mWatchPaths;

    QList<Model::Item> mLocalItems;
};

#endif
