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

    Match(Type t, const QString &fn, const QString &fp, const QIcon &i, const QStringList &a = QStringList())
        : type(t), name(fn), filePath(t == Application ? fp : QString()), url(t == Url ? fp : QString()), icon(i), arguments(a)
    {}

    QString name, filePath, url;
    QKeySequence keySequence;
    QIcon icon;
    QStringList arguments;
};

class Model : public QObject
{
    Q_OBJECT
public:
    Model(const QByteArray &paths, QObject *parent = 0);
    QList<Match> matches(const QString &text) const;
    const QList<QByteArray> &roots() const;
    void recordUserEntry(const QString &input, const QString &path);

    static void registerItem();

    struct Item {
        QString filePath;
        QString iconPath;
        QString name;
        QStringList arguments;
    };
    struct ItemIndex {
        QString key;
        QList<const Item*> items;

        bool matches(const QString &text) const;
    };
public slots:
    void reload();
signals:
    void initialized();
    void progress(int current);
private slots:
    void updateItems(const QList<Model::Item> &newItems);
private:
    void rebuildIndex();
private:
    friend class ModelThread;
    QList<Item> mItems;
    QList<ItemIndex> mItemIndex;
    const QList<QByteArray> mRoots;
    QFileIconProvider mFileIconProvider;
    QFileSystemWatcher *mFileSystemWatcher;
    QVariantMap mUserEntries;
    QList<QStringList> mUrlHandlers;
    QList<QStringList> mAppHandlers;
};


#endif
