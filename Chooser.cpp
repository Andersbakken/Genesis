#include "Chooser.h"
#include "LineEdit.h"
#include "Model.h"
#include "ResultList.h"
#include "ResultModel.h"

static void animate(QWidget *target, bool enter)
{
    QParallelAnimationGroup *group = new QParallelAnimationGroup;
    QPropertyAnimation *opacityAnimation = new QPropertyAnimation(target, "windowOpacity", group);
    opacityAnimation->setDuration(400);
    opacityAnimation->setEndValue(enter ? 1.0 : 0.0);
    QPropertyAnimation *positionAnimation = new QPropertyAnimation(target, "pos", group);
    QRect r = target->rect();
    r.moveCenter(qApp->desktop()->screenGeometry(target).center());
    if (!enter) {
        r.moveBottom(0);
        QObject::connect(group, SIGNAL(finished()), target, SLOT(close()));
    }

    positionAnimation->setDuration(200);
    positionAnimation->setEasingCurve(QEasingCurve::InQuad);
    positionAnimation->setEndValue(r.topLeft());
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

Chooser::Chooser(QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint)
    , mSearchInput(new LineEdit(this))
    , mSearchModel(new Model(QStringList() << "/Applications/", this))
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
    QRect r(QPoint(), size());
    r.moveCenter(qApp->desktop()->screenGeometry(this).center());
    r.moveBottom(0);

    move(r.topLeft());
    setWindowOpacity(.0);

    raise();
    activateWindow();
    ::animate(this, true);

    
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
            fadeOut();
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
    ::animate(this, false);
}
