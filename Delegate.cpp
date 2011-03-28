#include "Delegate.h"
#include "ResultModel.h"

Delegate::Delegate(QAbstractItemView* parent)
    : QItemDelegate(parent), mView(parent)
{
    mPixelSize = mConfig.value<int>("fontSize", 20);
    Q_ASSERT(mView);
}

void Delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QIcon icon = qVariantValue<QIcon>(index.data(Qt::DecorationRole));
    QFontMetrics metrics(qApp->font());
    const int iconSize = metrics.height();
    const bool isCurrent = (index == mView->currentIndex());
    if (isCurrent) {
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setPen(option.palette.highlightedText().color());
    } else {
        painter->setPen(option.palette.text().color());
    }
    QRect r = option.rect;

    enum { IconMargin = 2, Margin = 4 };

    const int iconDiffY = r.height() - iconSize;
    QRect iconRect(r.x() + IconMargin, r.y() + iconDiffY / 2, iconSize, iconSize);
    icon.paint(painter, iconRect);

    r.setLeft(iconSize + IconMargin + Margin);
    painter->setFont(qApp->font()); // ### why do I have to do this?
    painter->drawText(r, Qt::AlignLeft|Qt::AlignVCenter, index.data(Qt::DisplayRole).toString());
    r.setRight(r.right() - Margin);
    QList<QKeySequence> sequences = qVariantValue<QList<QKeySequence> >(index.data(ResultModel::KeySequencesRole));
    if (isCurrent)
        sequences.prepend(QKeySequence(Qt::Key_Return));
    QString keyString;
    foreach(const QKeySequence &seq, sequences) {
        if (!keyString.isEmpty())
            keyString.append(", ");
        keyString.append(seq.toString(QKeySequence::NativeText));
    }

    painter->drawText(r, Qt::AlignRight|Qt::AlignVCenter, keyString);
}
QSize Delegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return QSize(1, mPixelSize + 10);
}
