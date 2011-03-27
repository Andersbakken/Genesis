#ifndef RESULTLIST_H
#define RESULTLIST_H

#include <QWidget>
#include <QListView>

class Match;
class ResultModel;

class ResultList : public QWidget
{
    Q_OBJECT
public:
    ResultList(QWidget *parent = 0);

    void clear();

    void setMatches(const QList<Match>& matches);

private:
    ResultModel* mModel;
    QListView* mView;
};

#endif // RESULTLIST_H
