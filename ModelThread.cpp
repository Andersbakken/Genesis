#include "ModelThread.h"

ModelThread::ModelThread(Model *model)
    : mModel(model)
{
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    connect(this, SIGNAL(finished()), model, SIGNAL(initialized()));
}

static inline QString findIconPath(const QString &)
{
    return QString();
}

void ModelThread::run()
{
    Q_ASSERT(mModel);
    const QStringList &roots = mModel->mRoots;
    Q_ASSERT(!roots.isEmpty());
    for (int i=0; i<roots.size(); ++i) {
#ifdef Q_OS_MAC
        QDirIterator it(roots.at(i), QStringList() << "*.app", QDir::Dirs | QDir::NoDotAndDotDot);
#else
#error Genesis has not been ported to your platform
#endif
        while (it.hasNext()) {
            const QString file = it.next();
            const Model::Item item = { file, findIconPath(file) };
            mModel->mItems.append(item);
        }
    }
}
