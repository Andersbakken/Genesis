#ifndef CHOOSER_H
#define CHOOSER_H

#include <QtGui>

class GlobalShortcut;
class Model;
class ResultList;

class Chooser : public QWidget
{
    Q_OBJECT
public:
    Chooser(int keycode, int modifier, QWidget* parent = 0);
    void showEvent(QShowEvent *e);
    void keyPressEvent(QKeyEvent *e);
    bool event(QEvent *e);

private slots:
    void fadeOut();
    void startSearch(const QString& input);
    void invoke(const QModelIndex &index);
    void shortcutActivated(int shortcut);

private:
    QLineEdit* mSearchInput;
    Model* mSearchModel;
    ResultList* mResultList;
    GlobalShortcut* mShortcut;
    int mActivateId;
};

#endif // CHOOSER_H
