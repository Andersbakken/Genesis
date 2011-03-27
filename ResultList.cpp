#include "Model.h"
#include "ResultList.h"
#include "ResultModel.h"
#include <QHBoxLayout>

ResultList::ResultList(QWidget *parent)
    : QWidget(parent), mModel(new ResultModel(this)), mView(new QListView(this))
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(mView);

    mView->setModel(mModel);
}

void ResultList::clear()
{
    mModel->clearMatches();
}

void ResultList::setMatches(const QList<Match> &matches)
{
    mModel->setMatches(matches);
}
