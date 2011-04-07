#ifndef ACTIVATE_WINDOW_H
#define ACTIVATE_WINDOW_H

#include <QObject>
#include <QThread>

class PreviousProcessPrivate;

class PreviousProcess : public QObject
{
public:
    PreviousProcess(QObject* parent = 0);

    void activate();
    void compile();

private:
    PreviousProcessPrivate* priv;
};

struct ScriptWrapper
{
    void* script;
};

class ScriptCompiler : public QThread
{
    Q_OBJECT
public:
    ScriptCompiler(QObject* parent);

signals:
    void scriptReady(const ScriptWrapper& wrapper);

protected:
    void run();

private:
    ScriptWrapper platformRun();
};

class PreviousProcessPrivate : public QObject
{
    Q_OBJECT
public:
    PreviousProcessPrivate(QObject* parent);
    ~PreviousProcessPrivate();

    void* script;

public slots:
    void scriptCompiled(const ScriptWrapper& wrapper);

private:
    void platformDestructor();
    void platformSetScript(const ScriptWrapper& wrapper);
};

#endif
