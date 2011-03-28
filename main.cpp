#include <QtGui>
#include <QtNetwork>
#include "Chooser.h"
#include "Config.h"
#include "LocalServer.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    a.setOrganizationName("Genesis");
    a.setApplicationName("Genesis");
    a.setOrganizationDomain("https://github.com/Andersbakken/Genesis");
    Config config;
    const QString serverName = config.value<QString>("localServerName", "Genesis");
    if (!LocalServer::instance()->listen(serverName)) {
        qDebug() << LocalServer::instance()->errorString() << serverName;
        QLocalSocket socket;
        socket.connectToServer(serverName);
        if (!socket.waitForConnected(config.value<int>("localServerConnectionTimeout", 3000))) {
            qWarning("Can't seem to connect to server");
            return 1;
        }
        QByteArray data;
        {
            QDataStream ds(&data, QIODevice::WriteOnly);
            ds << data.size();
            ds << a.arguments();
            ds.device()->seek(0);
            ds << data.size();
        }
        socket.write(data);
        if (!socket.waitForBytesWritten(config.value<int>("localServerConnectionTimeout", 3000))) {
            qWarning("Can't seem to write to server");
            return 1;
        }
        return 0;
    }

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

    Chooser chooser;
    chooser.show();

    return a.exec();
}
