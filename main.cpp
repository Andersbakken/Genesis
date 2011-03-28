#include <QtGui>
#include "Chooser.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    QFont font;
    font.setPixelSize(20);
    a.setFont(font);

    Chooser chooser;
    chooser.show();

    return a.exec();
}
