#include "PreviousWindow.h"
#include <QMetaType>

ScriptCompiler::ScriptCompiler(QObject *parent)
    : QThread(parent)
{
    qRegisterMetaType<ScriptWrapper>("ScriptWrapper");
}

void ScriptCompiler::run()
{
    ScriptWrapper wrapper = platformRun();
    emit scriptReady(wrapper);
}

PreviousProcessPrivate::PreviousProcessPrivate(QObject *parent)
    : QObject(parent), script(0)
{
}

PreviousProcessPrivate::~PreviousProcessPrivate()
{
    platformDestructor();
}

void PreviousProcessPrivate::scriptCompiled(const ScriptWrapper &wrapper)
{
    platformSetScript(wrapper);
}

PreviousProcess::PreviousProcess(QObject *parent)
    : QObject(parent), priv(new PreviousProcessPrivate(this))
{
}

void PreviousProcess::compile()
{
    ScriptCompiler* compiler = new ScriptCompiler(this);
    connect(compiler, SIGNAL(finished()), compiler, SLOT(deleteLater()));
    connect(compiler, SIGNAL(scriptReady(ScriptWrapper)), priv, SLOT(scriptCompiled(ScriptWrapper)));
    compiler->start();
}
