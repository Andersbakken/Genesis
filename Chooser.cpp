#include "Chooser.h"
#include "Model.h"
#include "ResultList.h"
#include <QLineEdit>
#include <QVBoxLayout>

Chooser::Chooser(QWidget* parent)
    : QWidget(parent)
    , mSearchInput(new QLineEdit(this))
    , mSearchModel(Model::create(QStringList() << "/Applications", this))
    , mResultList(new ResultList(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(mSearchInput);
    layout->addWidget(mResultList);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding));

    connect(mSearchInput, SIGNAL(returnPressed()), this, SLOT(execute()));
    connect(mSearchInput, SIGNAL(textChanged(QString)), this, SLOT(startSearch(QString)));
}

void Chooser::execute()
{
}

void Chooser::startSearch(const QString& input)
{
    QList<Match> matches = mSearchModel->matches(input);
    mResultList->setMatches(matches);
    if (matches.isEmpty())
        mResultList->hide();
    else
        mResultList->show();
}
void Chooser::showEvent(QShowEvent *e)
{
    raise();
    activateWindow();
    QWidget::showEvent(e);
}
void Chooser::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Escape:
        if (mSearchInput->text().isEmpty()) {
            close();
        } else {
            mSearchInput->clear(); // ### undoable?
        }
        break;
    default:
        break;
    }
}
