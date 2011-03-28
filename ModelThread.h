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
private:
    Model *mModel;
    QSet<QString> mWatchPaths;
    QFileSystemWatcher *mWatcher;
};

#endif
