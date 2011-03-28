#include "Chooser.h"
#include "LineEdit.h"
#include "Model.h"
#include "ResultList.h"
#include "ResultModel.h"

Chooser::Chooser(QWidget* parent)
    : QWidget(parent)
    , mSearchInput(new LineEdit(this))
    , mSearchModel(Model::create(QStringList() << "/Applications/" << "/Applications/Microsoft Office 2011/", this))
    , mResultList(new ResultList(this))
{
    connect(mResultList, SIGNAL(clicked(QModelIndex)), this, SLOT(invoke(QModelIndex)));
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
// Should stick this kind of thing into some global config object
extern const Qt::KeyboardModifier numericModifier =
#ifdef Q_OS_MAC
    Qt::ControlModifier
#else
    Qt::AltModifier
#endif
    ;
    
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
        if (e->modifiers() == numericModifier && e->key() >= Qt::Key_1 && e->key() <= Qt::Key_9) {
            const int idx = e->key() - Qt::Key_1;
            mResultList->invoke(idx);
        }
        break;
    }
}

void Chooser::invoke(const QModelIndex &index)
{
    const Match::Type type = static_cast<Match::Type>(index.data(ResultModel::TypeRole).toInt());
    switch (type) {
    case Match::Application:
        QDesktopServices::openUrl(QUrl::fromLocalFile(index.data(ResultModel::FilePathRole).toString()));
        close();
        break;
    case Match::Url:
        QDesktopServices::openUrl(index.data(ResultModel::UrlRole).toString());
        close();
        break;
    case Match::None:
        break;
    }
}
