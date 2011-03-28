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
    void invoke(int idx);
    QListView *listView() const { return mView; }
    void keyPressEvent(QKeyEvent *e);
signals:
    void clicked(const QModelIndex &index);
private:
    ResultModel* mModel;
    QListView* mView;
};

#endif // RESULTLIST_H
