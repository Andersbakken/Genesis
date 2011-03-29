#include <QtGui>
#include <QtNetwork>
#include "Chooser.h"
#include "Config.h"
#include "Model.h"
#include "Server.h"

#if defined(ENABLE_SIGNAL_HANDLING) && defined (Q_OS_LINUX)
#include <signal.h>
static void signalHandler(int signal)
{
    fprintf(stderr, "signal %d\n", signal);
    delete sInstance;
    exit(signal);
}
#endif

int main(int argc, char **argv)
{
#if defined(ENABLE_SIGNAL_HANDLING) && defined (Q_OS_LINUX)
    signal(SIGINT, signalHandler);
    signal(SIGSEGV, signalHandler);
    signal(SIGBUS, signalHandler);
    signal(SIGABRT, signalHandler);
#endif

    QApplication a(argc, argv);
    a.setOrganizationName("Genesis");
    a.setApplicationName("Genesis");
    a.setOrganizationDomain("https://github.com/Andersbakken/Genesis");
    Config config;
#ifdef ENABLE_SERVER
    if (!Server::instance()->listen()) {
        QString command = "wakeup";
        const QStringList args = a.arguments();
        const int count = args.size();
        for (int i=1; i<count; ++i) {
            const QString &arg = args.at(i);
            if (arg == "--command" || arg == "-c") {
                if (i + 1 == count) {
                    qWarning("%s needs an argument", qPrintable(arg));
                    return 2;
                }
                command = args.at(++i);
                break;
            }
        }

        return Server::write(command) ? 0 : 1;
    }
#endif

    QFont font;
    const QString family = config.value<QString>("fontFamily");
    if (!family.isEmpty())
        font.setFamily(family);
    font.setPixelSize(config.value<int>("fontSize", 20));
    a.setFont(font);

    QPalette pal = a.palette();

    // Default palette entries
    pal.setColor(QPalette::Highlight, QColor(160, 160, 160));
    pal.setColor(QPalette::Base, QColor(200, 200, 200));
    pal.setColor(QPalette::BrightText, QColor(100, 100, 100));

    const QMetaObject &mo = QPalette::staticMetaObject;
    const QMetaEnum e = mo.enumerator(mo.indexOfEnumerator("ColorRole"));
    const int count = e.keyCount();
    for (int i=0; i<count; ++i) {
        QByteArray name("palette-");
        name += e.key(i);
        const QString color = config.value<QString>(name);
        if (!color.isEmpty()) {
            QBrush brush;
            if (QColor::isValidColor(color)) {
                brush = QColor(color);
            } else {
                brush = QPixmap(color);
            }
            if (brush.style() == Qt::NoBrush) {
                qWarning("Invalid color %s=%s", name.constData(), qPrintable(color));
            } else {
                pal.setBrush(static_cast<QPalette::ColorRole>(e.value(i)), brush);
            }
        }
    }

    a.setPalette(pal);

    Model::registerItem();

    Chooser chooser;
    chooser.show();

    return a.exec();
}
