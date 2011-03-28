#include <QtGui>
#include "Chooser.h"
#include "Config.h"
#include "GlobalShortcut.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    Config config;
    QFont font;
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

    const int keycode = config.value<int>(QLatin1String("shortcutKeycode"), 49); // 49 = space
    const int modifier = config.value<int>(QLatin1String("shortcutModifier"), 256); // 256 = cmd

    GlobalShortcut shortcut(&chooser);
    shortcut.registerShortcut(keycode, modifier);

    return a.exec();
}
