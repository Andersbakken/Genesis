#include <QtGui>
#include "Chooser.h"
#include "Config.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    a.setOrganizationName("Genesis");
    a.setApplicationName("Genesis");
    a.setOrganizationDomain("https://github.com/Andersbakken/Genesis");

    Config config;
    QFont font;
    const QString family = config.value<QString>("fontFamily");
    if (family.isEmpty())
        font.setFamily(family);
    font.setPixelSize(config.value<int>("fontSize", 20));
    a.setFont(font);

    QPalette pal = a.palette();
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
