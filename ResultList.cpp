#include "Model.h"
#include "ResultList.h"
#include "ResultModel.h"
#include "Delegate.h"

ResultList::ResultList(QWidget *parent)
    : QListView(parent), mModel(new ResultModel(this))
{
    setAttribute(Qt::WA_MacShowFocusRect, false);

    setPalette(qApp->palette()); // ### Am I supposed to do this?
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    // ### need to send up and down to mView
    connect(new QShortcut(QKeySequence(Qt::Key_Up), window()), SIGNAL(activated()), this, SLOT(up()));
    connect(new QShortcut(QKeySequence(Qt::Key_Down), window()), SIGNAL(activated()), this, SLOT(down()));
    connect(new QShortcut(QKeySequence(Qt::Key_Enter), window()), SIGNAL(activated()), this, SLOT(enter()));
    connect(new QShortcut(QKeySequence(Qt::Key_Return), window()), SIGNAL(activated()), this, SLOT(enter()));

    setModel(mModel);
    setItemDelegate(new Delegate(this));

    QColor base = qApp->palette().color(QPalette::Base);
    QColor highlight = qApp->palette().color(QPalette::Highlight);
    setStyleSheet(QString("QListView { border: 1px solid rgb(%1, %2, %3); background: rgb(%4, %5, %6) }")
                  .arg(highlight.red()).arg(highlight.green()).arg(highlight.blue())
                  .arg(base.red()).arg(base.green()).arg(base.blue()));

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void ResultList::clear()
{
    mModel->setMatches(QList<Match>());
}

void ResultList::setMatches(const QList<Match> &matches)
{
    mModel->setMatches(matches);
    if (mModel->rowCount()) {
        setCurrentIndex(mModel->index(0, 0));
    }
}

void ResultList::up()
{
    const int current = currentIndex().row();
    if (current > 0) {
        setCurrentIndex(mModel->index(current - 1, 0));
    }
}

void ResultList::down()
{
    const int current = currentIndex().row();
    const int count = mModel->rowCount();
    if (current + 1 < count) {
        setCurrentIndex(mModel->index(current + 1, 0));
    }
}

void ResultList::enter()
{
    if (currentIndex().isValid())
        emit clicked(currentIndex());
}

QSize ResultList::sizeHint() const
{
    static QSize sizePerRow;
    if (sizePerRow.isEmpty()) {
        const QStyleOptionViewItem opt;
        const QModelIndex idx;
        sizePerRow = itemDelegate()->sizeHint(opt, idx);
    }

    if (sizePerRow.isEmpty())
        return QListView::sizeHint();

     // ### where does the number 11 come from here?
    return QSize(width(), (sizePerRow.height() * model()->rowCount()) + 11);
}

void ResultList::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    // This function is reimplemented to avoid the list
    // scrolling down when entering the last item in the list

    Q_UNUSED(index)
    Q_UNUSED(hint)
}
