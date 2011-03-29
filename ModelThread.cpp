#include "ModelThread.h"
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

ModelThread::ModelThread(Model *model)
    : mModel(model)
{
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    connect(this, SIGNAL(finished()), model, SIGNAL(initialized()));
    mWatcher = new QFileSystemWatcher(mModel);
}

static inline QString findIconPath(const QString &)
{
    return QString();
}

void ModelThread::run()
{
    Q_ASSERT(mModel);
    const QList<QByteArray> &roots = mModel->mRoots;
    Q_ASSERT(!roots.isEmpty());
    enum { DefaultRecurseDepth =
#ifdef Q_OS_MAC
           5
#else
           1
#endif
    };

    mLocalItems.clear();

    for (int i=0; i<roots.size(); ++i) {
        recurse(roots.at(i), Config().value<int>("recurseDepth", DefaultRecurseDepth));
    }
    connect(mWatcher, SIGNAL(directoryChanged(QString)), mModel, SLOT(reload()));
    delete mModel->mFileSystemWatcher;
    mWatcher->addPaths(mWatchPaths.toList());
    mModel->mFileSystemWatcher = mWatcher;

    emit itemsReady(mLocalItems);
}

void ModelThread::recurse(const QByteArray &path, int maxDepth)
{
    DIR *dir = opendir(path.constData());
    if (!dir) {
        qWarning("Can't read directory [%s]", path.constData());
        return;
    }
    struct dirent *d;
#ifndef Q_OS_MAC
    struct stat s;
    mWatchPaths.insert(path);
#endif
    char fileBuffer[1024];
    memcpy(fileBuffer, path.constData(), path.size());
    fileBuffer[path.size()] = '/';
    char *file = fileBuffer + path.size() + 1;

    while ((d = readdir(dir))) {
        Q_ASSERT(int(strlen(d->d_name)) < 1024 - path.size());
#ifdef Q_OS_MAC
        if (d->d_type == DT_DIR) {
            if (d->d_namlen > 4 && !strcmp(d->d_name + d->d_namlen - 4, ".app")) {
                mWatchPaths.insert(path);
                strcpy(file, d->d_name);
                const Model::Item item = { QString::fromUtf8(fileBuffer), findIconPath(fileBuffer) };
                mLocalItems.append(item);
            } else if (maxDepth > 1 && (d->d_namlen > 2 || (strcmp(".", d->d_name) && strcmp("..", d->d_name)))) {
                recurse(path + '/' + reinterpret_cast<const char *>(d->d_name), maxDepth - 1);
            }
        }
#else
        if (d->d_type == DT_REG) {
            strcpy(file, d->d_name);
            if (!stat(fileBuffer, &s)) {
                if (s.st_mode & S_IXOTH) {
                    const Model::Item item = { fileBuffer, findIconPath(fileBuffer) };
                    mLocalItems.append(item);
                }
            } else {
                qWarning("Can't stat [%s]", fileBuffer);
            }
        } else if (maxDepth > 1 && strcmp(".", d->d_name) && strcmp("..", d->d_name)) {
            strcpy(file, d->d_name);
            recurse(fileBuffer, maxDepth - 1);
        }
#endif
    }
    closedir(dir);
}
