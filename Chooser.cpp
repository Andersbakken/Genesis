#include "Chooser.h"
#include "GlobalShortcut.h"
#include "LineEdit.h"
#include "Model.h"
#include "ResultList.h"
#include "ResultModel.h"

static void animate(QWidget *target, bool enter, int heightdiff = 0)
{
    static const bool enableOpacityAnimation = Config().isEnabled("opacityAnimation", true);
    static const bool enablePositionAnimation = Config().isEnabled("positionAnimation", true);
    QRect r = target->rect();
    r.moveCenter(qApp->desktop()->screenGeometry(target).center());
    if (!enter) {
        r.moveBottom(0);
    } else {
        r.moveBottom(r.bottom() - heightdiff);
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

class RoundedWidget : public QWidget
{
public:
    RoundedWidget(QWidget* parent);

    void setFillColor(const QColor& fill);
    void setRoundedRadius(qreal radius);

protected:
    void paintEvent(QPaintEvent* e);

private:
    QColor mFill;
    qreal mRadius;
};

RoundedWidget::RoundedWidget(QWidget* parent)
    : QWidget(parent), mRadius(15.)
{
}

void RoundedWidget::setFillColor(const QColor& fill)
{
    mFill = fill;
}

void RoundedWidget::setRoundedRadius(qreal radius)
{
    mRadius = radius;
}

void RoundedWidget::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(mFill);
    p.setPen(Qt::NoPen);

    QPainterPath path;
    path.addRoundedRect(rect(), mRadius, mRadius);

    p.drawPath(path);
}

Chooser::Chooser(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint), mSearchInput(new LineEdit(this)),
      mSearchModel(new Model(QStringList() << "/Applications/", this)), mResultList(new ResultList(this)),
      mShortcut(new GlobalShortcut(this))
{
    mLayout = new QVBoxLayout(this);
    RoundedWidget* back = new RoundedWidget(this);
    back->setFillColor(QColor(90, 90, 90, 210));
    mLayout->addWidget(back);
    mLayout->setMargin(0);

    mLayout = new QVBoxLayout(back);
    RoundedWidget* container = new RoundedWidget(back);
    container->setFillColor(QColor(230, 230, 230));
    container->setRoundedRadius(8.);
    mLayout->addWidget(container);
    mLayout->setMargin(10);

    mLayout = new QVBoxLayout(container);

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_QuitOnClose, false);
    new QShortcut(QKeySequence(QKeySequence::Close), this, SLOT(fadeOut()));
    connect(mResultList, SIGNAL(clicked(QModelIndex)), this, SLOT(invoke(QModelIndex)));
    mLayout->setMargin(10);
    mLayout->setSpacing(10);
    mLayout->addWidget(mSearchInput, 0, Qt::AlignTop);
    mLayout->addWidget(mResultList);

    mResultList->hide();

    connect(mSearchInput, SIGNAL(textChanged(QString)), this, SLOT(startSearch(QString)));

    Config config;
    mWidth = config.value<int>("width", 500);
    mResultHiddenHeight = config.value<int>("noresultsHeight", 70);
    mResultShownHeight = config.value<int>("height", 500);

    resize(mWidth, mResultHiddenHeight);

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

    if (matches.isEmpty())
        hideResultList();
    else
        showResultList();
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
    ::animate(this, true, (mResultShownHeight - mResultHiddenHeight) / 2);

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

void Chooser::hideResultList()
{
    if (mResultList->isHidden())
        return;

    mResultList->hide();
    mLayout->removeWidget(mResultList);
    setMinimumHeight(mResultHiddenHeight); // ### why is this needed?
    resize(mWidth, mResultHiddenHeight);
}

void Chooser::showResultList()
{
    if (!mResultList->isHidden())
        return;

    mLayout->addWidget(mResultList);
    mResultList->show();
    resize(mWidth, mResultShownHeight);
}

bool Chooser::event(QEvent *e)
{
    if (e->type() == QEvent::WindowDeactivate) {
        fadeOut();
    }
    return QWidget::event(e);
}
