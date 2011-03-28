#include "Model.h"
#include "ResultList.h"
#include "ResultModel.h"
#include "Delegate.h"

ResultList::ResultList(QWidget *parent)
    : QWidget(parent), mModel(new ResultModel(this)), mView(new QListView(this))
{
    // ### need to send up and down to mView
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(mView);
    connect(mView, SIGNAL(clicked(QModelIndex)), this, SIGNAL(clicked(QModelIndex)));
    connect(new QShortcut(QKeySequence(Qt::Key_Up), window()), SIGNAL(activated()), this, SLOT(up()));
    connect(new QShortcut(QKeySequence(Qt::Key_Down), window()), SIGNAL(activated()), this, SLOT(down()));
    connect(new QShortcut(QKeySequence(Qt::Key_Enter), window()), SIGNAL(activated()), this, SLOT(enter()));
    connect(new QShortcut(QKeySequence(Qt::Key_Return), window()), SIGNAL(activated()), this, SLOT(enter()));

    mView->setModel(mModel);
    mView->setItemDelegate(new Delegate(mView));
}

void ResultList::clear()
{
    mModel->setMatches(QList<Match>());
}

void ResultList::setMatches(const QList<Match> &matches)
{
    mModel->setMatches(matches);
    if (mModel->rowCount()) {
        mView->setCurrentIndex(mModel->index(0, 0));
    }
}
void ResultList::invoke(int idx)
{
    if (idx < mModel->rowCount(QModelIndex()))
        emit clicked(mModel->index(idx));
}
void ResultList::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
        e->accept();
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(e);
}
void ResultList::up()
{
    printf("%s %d: void ResultList::up()\n", __FILE__, __LINE__);
    const int current = mView->currentIndex().row();
    if (current > 0) {
        mView->setCurrentIndex(mModel->index(current - 1, 0));
    }
}

void ResultList::down()
{
    printf("%s %d: void ResultList::down()\n", __FILE__, __LINE__);
    const int current = mView->currentIndex().row();
    const int count = mModel->rowCount();
    if (current + 1 < count) {
        mView->setCurrentIndex(mModel->index(current + 1, 0));
    }
}

void ResultList::enter()
{
    
}
