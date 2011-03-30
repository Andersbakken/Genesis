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
public slots:
    void up();
    void down();
    void enter();
private:
    ResultModel* mModel;
};

#endif // RESULTLIST_H
