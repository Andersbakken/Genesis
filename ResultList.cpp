#include "Model.h"
#include "ResultList.h"
#include "ResultModel.h"
#include <QHBoxLayout>

ResultList::ResultList(QWidget *parent)
    : QWidget(parent), mModel(new ResultModel(this)), mView(new QListView(this))
{
    // ### need to send up and down to mView
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(mView);
    connect(mView, SIGNAL(clicked(QModelIndex)), this, SIGNAL(clicked(QModelIndex)));

    mView->setModel(mModel);
}

void ResultList::clear()
{
    mModel->setMatches(QList<Match>());
}

void ResultList::setMatches(const QList<Match> &matches)
{
    mModel->setMatches(matches);
}
void ResultList::invoke(int idx)
{
    if (idx < mModel->rowCount(QModelIndex()))
        emit clicked(mModel->index(idx));
}
