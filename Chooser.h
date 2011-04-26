#ifndef CHOOSER_H
#define CHOOSER_H

#include <QtGui>

class GlobalShortcut;
class Model;
class ResultList;
class QVBoxLayout;
class PreviousProcess;

class Chooser : public QWidget
{
    Q_OBJECT
public:
    Chooser(QWidget* parent = 0);
    void showEvent(QShowEvent *e);
    void keyPressEvent(QKeyEvent *e);
    bool event(QEvent *e);

private slots:
#ifdef ENABLE_SERVER
    void onCommandReceived(const QString &command);
#endif
    void startSearch(const QString& input);
    void invoke(const QModelIndex &index);
    void shortcutActivated(int shortcut);
    void keepAlive();

    void enable();
    void disable();
    void onUnhandledUp();
private:
    void showResultList();
    void hideResultList();

private:
    QLineEdit* mSearchInput;
    Model* mSearchModel;
    ResultList* mResultList;
    GlobalShortcut* mShortcut;
    int mActivateId;
    QTimer mKeepAlive;
    PreviousProcess* mPrevious;

    int mWidth;
    int mResultHiddenHeight;
    int mResultShownHeight;
};

#endif // CHOOSER_H
