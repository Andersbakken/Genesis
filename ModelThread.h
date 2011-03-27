#ifndef ModelThread_h
#define ModelThread_h

#include <Model.h>

class ModelThread : public QThread
{
    Q_OBJECT
public:
    ModelThread(Model *model);
    virtual void run();
private:
    Model *mModel;
};

#endif
