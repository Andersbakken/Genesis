#ifndef Invoker_H
#define Invoker_H

#include <QWidget>
#include <QStringList>

class Invoker : public QObject
{
    Q_OBJECT
public:
    Invoker(QWidget *parent);

    void setApplication(const QString& application, const QStringList& arguments);
    void invoke();

    void raise(QWidget* w);

private:
    QString mApplication;
    QStringList mArguments;
    QWidget* mWidget;
};

#endif // INVOKER_H
