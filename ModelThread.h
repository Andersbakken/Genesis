#ifndef ModelThread_h
#define ModelThread_h

#include <Model.h>

class ModelThread : public QThread
{
    Q_OBJECT
public:
    ModelThread(Model *model);
    virtual void run();
    void recurse(const QByteArray &path, int maxDepth = 3);
private:
    Model *mModel;
};

#endif
