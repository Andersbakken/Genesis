#ifndef Model_h
#define Model_h

#include <QtGui>

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
    QIcon icon;
};

class Model : public QObject
{
    Q_OBJECT
public:
    static Model *create(const QStringList &roots, QObject *parent);
    QList<Match> matches(const QString &text) const;
    const QStringList &roots() const;
signals:
    void initialized();
    void progress(int current);
private:
    Model(const QStringList &path, QObject *parent = 0);
    friend class ModelThread;
    struct Item {
        QString filePath;
        QString iconPath;
    };
    QList<Item> mItems;
    const QStringList mRoots;
    QFileIconProvider mFileIconProvider;
};


#endif
