#ifndef LineEdit_h
#define LineEdit_h

#include <QtGui>

class LineEdit : public QLineEdit
{
    Q_OBJECT
public:
    LineEdit(QWidget *parent = 0);
    void keyPressEvent(QKeyEvent *e);
};


#endif
