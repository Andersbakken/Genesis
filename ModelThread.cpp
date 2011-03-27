#include "ModelThread.h"

ModelThread::ModelThread(Model *model)
    : mModel(model);
{
    connect(this, SIGNAL(finished()), SLOT(deleteLater()));
}

void ModelThread::run()
{
    Q_ASSERT(mModel);
    const QStringList &roots = mModel->mRoots;
    for (int i=0; i<roots.size(); ++i) {
        QDirIterator it(roots, QDirIterator::Subdirectories);
        while (it.isValid()) {

        }
    }
}
