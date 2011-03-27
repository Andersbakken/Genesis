#include "LineEdit.h"

LineEdit::LineEdit(QWidget *parent)
    : QLineEdit(parent)
{
}

void LineEdit::keyPressEvent(QKeyEvent *e)
{
    extern Qt::KeyboardModifier numericModifier;
    if (e->modifiers() == numericModifier && e->key() >= Qt::Key_1 && e->key() <= Qt::Key_9) {
        e->ignore();
        return;
    }
    switch (e->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
        e->ignore();
        return;
    default:
        break;
    }
    QLineEdit::keyPressEvent(e);
}
