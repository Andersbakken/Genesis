#include "ModelThread.h"

ModelThread::ModelThread(Model *model)
    : mModel(model)
{
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    connect(this, SIGNAL(finished()), model, SIGNAL(initialized()));
}

static inline QIcon findIcon(const QString &)
{
    return QIcon();
}

void ModelThread::run()
{
    Q_ASSERT(mModel);
    const QStringList &roots = mModel->mRoots;
    Q_ASSERT(!roots.isEmpty());
    for (int i=0; i<roots.size(); ++i) {
        QList<QPair<QString, QIcon> > &current = mModel->mData[i];
#ifdef Q_OS_MAC
        QDirIterator it(roots.at(i), QStringList() << "*.app", QDir::Dirs, QDirIterator::Subdirectories);
#else
#error Genesis has not been ported to your platform
#endif
        while (it.hasNext()) {
            const QString file = it.next();
            current.append(qMakePair(file, findIcon(file)));
        }
    }
}
