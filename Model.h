#ifndef Model_h
#define Model_h

#include <QtGui>
#include "Config.h"

struct Match
{
    enum Type {
        None,
        Application,
        Url
    } type;

    Match(Type t, const QString &fn, const QString &fp, const QIcon &i)
        : type(t), name(fn), filePath(t == Application ? fp : QString()), url(t == Url ? fp : QString()), icon(i)
    {}

    QString name, filePath, url;
    QKeySequence keySequence;
    QIcon icon;
};

class Model : public QObject
{
    Q_OBJECT
    public:
    Model(const QStringList &path, QObject *parent = 0);
    QList<Match> matches(const QString &text) const;
    const QStringList &roots() const;
    void recordUserEntry(const QString &input, const QString &path);
public slots:
    void reload();
signals:
    void initialized();
    void progress(int current);
private:
    friend class ModelThread;
    struct Item {
        QString filePath;
        QString iconPath;
    };
    QList<Item> mItems;
    const QStringList mRoots;
    QFileIconProvider mFileIconProvider;
    QFileSystemWatcher *mFileSystemWatcher;
    QVariantMap mUserEntries;
    QVariantList mUrlHandlers;
};


#endif
