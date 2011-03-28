#include "LineEdit.h"

LineEdit::LineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    setAttribute(Qt::WA_MacShowFocusRect, false);

    setStyleSheet(QLatin1String("QLineEdit { border: 1px solid rgb(160, 160, 160); background: rgb(200, 200, 200) }"));

    setMinimumHeight(qApp->font().pixelSize() + 10);
}

void LineEdit::keyPressEvent(QKeyEvent *e)
{
    extern const Qt::KeyboardModifier numericModifier;
    if (e->modifiers() == int(numericModifier) && int(e->key()) >= Qt::Key_1 && int(e->key()) <= Qt::Key_9) {
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
