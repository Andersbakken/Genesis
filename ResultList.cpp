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
        mView->selectionModel()->select(mModel->index(0, 0), QItemSelectionModel::Select);
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
