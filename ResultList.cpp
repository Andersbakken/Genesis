#include "Model.h"
#include "ResultList.h"
#include "ResultModel.h"
#include "Delegate.h"

ResultList::ResultList(QWidget *parent)
    : QListView(parent), mModel(new ResultModel(this))
{
    setPalette(qApp->palette()); // ### Am I supposed to do this?
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    // ### need to send up and down to mView
    connect(new QShortcut(QKeySequence(Qt::Key_Up), window()), SIGNAL(activated()), this, SLOT(up()));
    connect(new QShortcut(QKeySequence(Qt::Key_Down), window()), SIGNAL(activated()), this, SLOT(down()));
    connect(new QShortcut(QKeySequence(Qt::Key_Enter), window()), SIGNAL(activated()), this, SLOT(enter()));
    connect(new QShortcut(QKeySequence(Qt::Key_Return), window()), SIGNAL(activated()), this, SLOT(enter()));

    setModel(mModel);
    setItemDelegate(new Delegate(this));
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

