#ifndef RESULTLIST_H
#define RESULTLIST_H

#include <QtGui>

class Match;
class ResultModel;

class ResultList : public QListView
{
    Q_OBJECT
public:
    ResultList(QWidget *parent = 0);

    void clear();
    void setMatches(const QList<Match>& matches);

    QSize sizeHint() const;
    void scrollTo(const QModelIndex &index, ScrollHint hint);
public slots:
    void up();
    void down();
    void enter();
signals:
    void unhandledUp();
private:
    ResultModel* mModel;
};

#endif // RESULTLIST_H
