#ifndef Model_h
#define Model_h

#include <QtCore>

struct Match
{
    enum Type {
        Application,
        Url
    };

    QString text;
    QIcon icon;
};

class Model : public QObject
{
    Q_OBJECT
public:
    static Model *create(const QStringList &roots);
    int matches(const QString &text, Match *matches, int max) const;
    const QStringList &roots() const;
signals:
    void initialized();
    void progress(int current);
public slots:
    void setMatches(const QList<QStringList> &data);
private:
    Model(const QString &path, QObject *parent = 0);
    friend class ModelThread;
    QList<QStringList> mMatches;
    const QStringList mRoots;
};


#endif
