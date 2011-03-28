#include "ModelThread.h"
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>

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
        Q_ASSERT(roots.at(i).endsWith('/'));
#ifdef Q_OS_MAC
        recurse(roots.at(i).toUtf8());
#else
#error Genesis has not been ported to your platform
#endif
    }
}

void ModelThread::recurse(const QByteArray &path, int maxDepth)
{
    DIR *dir = opendir(path.constData());
    Q_ASSERT(dir);
    struct dirent *d;

    while ((d = readdir(dir))) {
        if (d->d_type == DT_DIR) {
            if (d->d_namlen > 4 && !strcmp(d->d_name + d->d_namlen - 4, ".app")) {
                QString file = path;
                file += + d->d_name;
                const Model::Item item = { file, findIconPath(file) };
                mModel->mItems.append(item);
            } else if (maxDepth > 1 && strcmp(".", d->d_name) && strcmp("..", d->d_name)) {
                recurse(path + reinterpret_cast<const char *>(d->d_name) + '/', maxDepth - 1);
            }
        }
    }
    closedir(dir);
}
