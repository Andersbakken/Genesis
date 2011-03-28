#include "Chooser.h"
#include "GlobalShortcut.h"
#include "LineEdit.h"
#include "Model.h"
#include "ResultList.h"
#include "ResultModel.h"

static void animate(QWidget *target, bool enter)
{
    static const bool enableOpacityAnimation = Config().isEnabled("opacityAnimation", true);
    static const bool enablePositionAnimation = Config().isEnabled("positionAnimation", true);
    QRect r = target->rect();
    r.moveCenter(qApp->desktop()->screenGeometry(target).center());
    if (!enter) {
        r.moveBottom(0);
    }

    if (!enablePositionAnimation)
        target->move(r.topLeft());
    if (!enableOpacityAnimation)
        target->setWindowOpacity(enter ? 1.0 : 0.0);
    if (!enablePositionAnimation && !enableOpacityAnimation)
        return;
    QParallelAnimationGroup *group = new QParallelAnimationGroup;
    if (enableOpacityAnimation) {
        QPropertyAnimation *opacityAnimation = new QPropertyAnimation(target, "windowOpacity", group);
        opacityAnimation->setDuration(400);
        opacityAnimation->setEndValue(enter ? 1.0 : 0.0);
    }
    if (enablePositionAnimation) {
        QPropertyAnimation *positionAnimation = new QPropertyAnimation(target, "pos", group);
        positionAnimation->setDuration(200);
        positionAnimation->setEasingCurve(QEasingCurve::InQuad);
        positionAnimation->setEndValue(r.topLeft());
    }

    if (!enter)
        QObject::connect(group, SIGNAL(finished()), target, SLOT(close()));

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

Chooser::Chooser(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint), mSearchInput(new LineEdit(this)),
      mSearchModel(new Model(QStringList() << "/Applications/", this)), mResultList(new ResultList(this)),
      mShortcut(new GlobalShortcut(this))
{
    setAttribute(Qt::WA_QuitOnClose, false);
    new QShortcut(QKeySequence(QKeySequence::Close), this, SLOT(fadeOut()));
    connect(mResultList, SIGNAL(clicked(QModelIndex)), this, SLOT(invoke(QModelIndex)));
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(1);
    layout->setSpacing(1);
    layout->addWidget(mSearchInput, 0, Qt::AlignTop);
    layout->addWidget(mResultList);

    connect(mSearchInput, SIGNAL(textChanged(QString)), this, SLOT(startSearch(QString)));
    Config config;
    resize(config.value<int>("width", 500), config.value<int>("height", 500));

    const int keycode = config.value<int>(QLatin1String("shortcutKeycode"), 49); // 49 = space
    const int modifier = config.value<int>(QLatin1String("shortcutModifier"), 256); // 256 = cmd
    connect(mShortcut, SIGNAL(activated(int)), this, SLOT(shortcutActivated(int)));
    mActivateId = mShortcut->registerShortcut(keycode, modifier);
}

void Chooser::shortcutActivated(int shortcut)
{
    if (shortcut != mActivateId)
        return;

    show();
    raise();
}

void Chooser::startSearch(const QString& input)
{
    QList<Match> matches = mSearchModel->matches(input);
    mResultList->setMatches(matches);
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

    if (!mSearchInput->text().isEmpty()) {
        mSearchInput->selectAll();
        startSearch(mSearchInput->text());
    }

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
                    mSearchInput->clear();
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
    case Match::Application: {
        const QString path = index.data(ResultModel::FilePathRole).toString();
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        mSearchModel->recordUserEntry(mSearchInput->text(), path);
        fadeOut();
        break; }
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
bool Chooser::event(QEvent *e)
{
    if (e->type() == QEvent::WindowDeactivate) {
        fadeOut();
    }
    return QWidget::event(e);
}
