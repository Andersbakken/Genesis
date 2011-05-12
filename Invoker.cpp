#include "Invoker.h"
#include "Config.h"
#include "System.h"
#include <QProcess>
#include <QDebug>
#ifdef Q_WS_X11
#    include <QX11Info>
#endif

#define MIN_EQLEN 6

Invoker::Invoker(QWidget *parent)
    : QObject(parent), mWidget(parent)
{
}

void Invoker::setApplication(const QString &application, const QStringList &arguments)
{
    mApplication = application;
    mArguments = arguments;
}

void Invoker::invoke()
{
#ifdef Q_OS_MAC
    if (mApplication.endsWith(QLatin1String(".app"))) {
        QStringList args(mArguments);
        if (!args.isEmpty())
            args.prepend(QLatin1String("--args"));
        args.prepend(mApplication);
        QProcess::startDetached(QLatin1String("open"), args);
    } else
        QProcess::startDetached(mApplication, mArguments);
#else
    if (!System::instance(mWidget->x11Info().screen())->raise(mApplication))
        QProcess::startDetached(mApplication, mArguments);
#endif
}
