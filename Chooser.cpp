#include "Chooser.h"
#include "LineEdit.h"
#include "Model.h"
#include "ResultList.h"
#include "ResultModel.h"

Chooser::Chooser(QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint)
    , mSearchInput(new LineEdit(this))
    , mSearchModel(Model::create(QStringList() << "/Applications/" << "/Applications/Microsoft Office 2011/", this))
    , mResultList(new ResultList(this))
{
    connect(mResultList, SIGNAL(clicked(QModelIndex)), this, SLOT(invoke(QModelIndex)));
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(1);
    layout->setSpacing(1);
    layout->addWidget(mSearchInput, 0, Qt::AlignTop);
    layout->addWidget(mResultList);

    connect(mSearchInput, SIGNAL(textChanged(QString)), this, SLOT(startSearch(QString)));
    resize(500, 500);
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
    setWindowOpacity(.0);
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(400);
    animation->setEndValue(1.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    
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
    default: {
        const QKeySequence key(e->key() | e->modifiers());
        const int count = mResultList->model()->rowCount();
        for (int i=0; i<count; ++i) {
            const QModelIndex idx = mResultList->model()->index(i, 0);
            foreach(const QKeySequence &seq, qVariantValue<QList<QKeySequence> >(idx.data(ResultModel::KeySequencesRole))) {
                if (key.matches(seq) == QKeySequence::ExactMatch) {
                    invoke(idx);
                    e->accept();
                    return;
                }
            }
        }
        break; }
    }
}

void Chooser::invoke(const QModelIndex &index)
{
    const Match::Type type = static_cast<Match::Type>(index.data(ResultModel::TypeRole).toInt());
    switch (type) {
    case Match::Application:
        QDesktopServices::openUrl(QUrl::fromLocalFile(index.data(ResultModel::FilePathRole).toString()));
        fadeOut();
        break;
    case Match::Url:
        QDesktopServices::openUrl(index.data(ResultModel::UrlRole).toString());
        fadeOut();
        break;
    case Match::None:
        break;
    }
}

void Chooser::fadeOut()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(400);
    animation->setEndValue(0.0);
    connect(animation, SIGNAL(finished()), this, SLOT(close()));
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
