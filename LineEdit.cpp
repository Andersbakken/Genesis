#include "LineEdit.h"

LineEdit::LineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    setAttribute(Qt::WA_MacShowFocusRect, false);

    QColor base = qApp->palette().color(QPalette::Base);
    QColor highlight = qApp->palette().color(QPalette::Highlight);
    setStyleSheet(QString("QLineEdit { border: 1px solid rgb(%1, %2, %3); background: rgb(%4, %5, %6) }")
                  .arg(highlight.red()).arg(highlight.green()).arg(highlight.blue())
                  .arg(base.red()).arg(base.green()).arg(base.blue()));

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
