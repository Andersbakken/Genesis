#include <QtGui>
#include "Chooser.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    Chooser chooser;
    chooser.show();

    return a.exec();
}
