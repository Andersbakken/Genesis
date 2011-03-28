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
    QRect r = option.rect;

    enum { IconMargin = 4, Margin = 4 };

    const int iconSize = r.height() - (IconMargin * 2) - 1;
    const bool isCurrent = (index == mView->currentIndex());

    QColor highlight = option.palette.highlight().color();

    if (isCurrent) {
        painter->fillRect(option.rect, highlight);
        painter->setPen(option.palette.highlightedText().color());
    } else {
        painter->setPen(option.palette.text().color());
    }

    const int iconDiffY = r.height() - iconSize;
    const QRect iconRect(r.x() + IconMargin, r.y() + iconDiffY / 2, iconSize, iconSize);
    icon.paint(painter, iconRect);

    r.setLeft(iconSize + IconMargin + Margin);
    painter->setFont(qApp->font()); // ### why do I have to do this?
    painter->drawText(r, Qt::AlignLeft|Qt::AlignTop, index.data(Qt::DisplayRole).toString());
    r.setRight(r.right() - Margin);
    r.setBottom(r.bottom() - Margin);
    QList<QKeySequence> sequences = qVariantValue<QList<QKeySequence> >(index.data(ResultModel::KeySequencesRole));
    if (isCurrent)
        sequences.prepend(QKeySequence(Qt::Key_Return));
    QString keyString;
    foreach(const QKeySequence &seq, sequences) {
        if (!keyString.isEmpty())
            keyString.append(", ");
        keyString.append(seq.toString(QKeySequence::NativeText));
    }

    painter->drawText(r, Qt::AlignRight|Qt::AlignTop, keyString);

    QFont subFont(qApp->font());
    subFont.setPixelSize(subFont.pixelSize() / 2);
    painter->setFont(subFont);
    painter->setPen(qApp->palette().color(QPalette::BrightText));

    QFontMetrics metrics = QFontMetrics(subFont);
    QString subText = metrics.elidedText(index.data(ResultModel::FilePathRole).toString(), Qt::ElideMiddle, r.width());

    painter->drawText(r, Qt::AlignLeft|Qt::AlignBottom, subText);

    painter->setPen(highlight);
    painter->drawLine(r.bottomLeft().x() - iconSize - IconMargin, r.bottomLeft().y() + Margin,
                      r.bottomRight().x(), r.bottomRight().y() + Margin);
}
QSize Delegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return QSize(1, mPixelSize + (mPixelSize / 2) + 15);
}
