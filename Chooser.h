#ifndef CHOOSER_H
#define CHOOSER_H

#include <QtGui>

class Model;
class ResultList;

class Chooser : public QWidget
{
    Q_OBJECT
public:
    Chooser(QWidget* parent = 0);
    void showEvent(QShowEvent *e);
    void keyPressEvent(QKeyEvent *e);

private slots:
    void fadeOut();
    void startSearch(const QString& input);
    void invoke(const QModelIndex &index);

private:
    QLineEdit* mSearchInput;
    Model* mSearchModel;
    ResultList* mResultList;
};

#endif // CHOOSER_H
