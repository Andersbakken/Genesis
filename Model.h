#ifndef Model_h
#define Model_h

#include <QtGui>

struct Match
{
    enum Type {
        Application,
        Url
    } type;

    Match(Type t, const QString &tex, const QString &p, const QIcon &i)
        : type(t), text(tex), path(p), icon(i)
    {}

    QString text, path;
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
    QVector<QList<QPair<QString, QIcon> > > mData;
    const QStringList mRoots;
};


#endif
