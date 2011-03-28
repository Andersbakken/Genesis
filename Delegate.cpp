#include "Delegate.h"

Delegate::Delegate(QObject *parent)
    : QItemDelegate(parent)
{
    
}

void Delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QIcon icon = qVariantValue<QIcon>(index.data(Qt::DecorationRole));
    enum { IconSize = 24 };
    const bool isSelected = (option.state & QStyle::State_Selected);
    if (isSelected) {
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setPen(option.palette.highlightedText().color());
    } else {
        painter->setPen(option.palette.foreground().color());
    }
    icon.paint(painter, QRect(0, 0, IconSize, IconSize));
    QRect r = option.rect;
    enum { Margin = 4 };
    r.setLeft(IconSize + Margin);
    painter->drawText(r, Qt::AlignLeft|Qt::AlignVCenter, index.data(Qt::DisplayRole).toString());
    r.setRight(r.right() - Margin);
    painter->drawText(r, Qt::AlignRight|Qt::AlignVCenter,
                      isSelected ? QString("Enter") : QString("%1-%2").arg(QChar(0x2318)).arg(index.row() + 1));
}
