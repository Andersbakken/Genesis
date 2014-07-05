#include "Chooser.h"
#include "GlobalShortcut.h"
#include "LineEdit.h"
#include "Model.h"
#include "ResultList.h"
#include "ResultModel.h"
#include "Server.h"
#include "PreviousWindow.h"
#include "Invoker.h"
#include "System.h"

static void animate(QWidget *target, bool enter, int heightdiff = 0)
{
    static const bool enableOpacityAnimation = Config().isEnabled("opacityAnimation",
#ifdef Q_OS_MAC
                                                                  true
#else
                                                                  false
#endif
        );

    static const bool enablePositionAnimation = Config().isEnabled("positionAnimation", false);
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

static inline QByteArray defaultSearchPaths()
{
    QByteArray paths = qgetenv("PATH");
#ifdef Q_OS_MAC
    if (!paths.isEmpty())
        paths += ":";
    paths += "/Applications:/Developer/Applications:/System/Library/CoreServices";
#endif
    return paths;
}

Chooser::Chooser(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint), mSearchInput(new LineEdit(this)),
      mSearchModel(new Model(Config().value<QByteArray>("searchPaths", ::defaultSearchPaths()), this)),
      mResultList(new ResultList(this)), mShortcut(new GlobalShortcut(this)),
      mPrevious(new PreviousProcess(this)), mInvoker(new Invoker(this))
{
#ifdef ENABLE_SERVER
    connect(Server::instance(), SIGNAL(commandReceived(QString)), this, SLOT(onCommandReceived(QString)));
#endif
    QVBoxLayout* layout = new QVBoxLayout(this);
    RoundedWidget* back = new RoundedWidget(this);
    back->setFillColor(QColor(90, 90, 90, 210));
    layout->addWidget(back);
    layout->setMargin(0);

    layout = new QVBoxLayout(back);
    RoundedWidget* container = new RoundedWidget(back);
    container->setFillColor(QColor(230, 230, 230));
    container->setRoundedRadius(8.);
    layout->addWidget(container);
    layout->setMargin(10);

    layout = new QVBoxLayout(container);

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_QuitOnClose, false);

    new QShortcut(QKeySequence(QKeySequence::Close), this, SLOT(disable()));
    connect(mResultList, SIGNAL(clicked(QModelIndex)), this, SLOT(invoke(QModelIndex)));
    connect(mSearchModel, SIGNAL(indexRebuilt()), this, SLOT(search()));
    layout->setMargin(10);
    layout->setSpacing(10);
    layout->addWidget(mSearchInput, 0, Qt::AlignTop);
    layout->addWidget(mResultList);

    mResultList->hide();

    connect(mSearchInput, SIGNAL(textChanged(QString)), this, SLOT(search()));

    Config config;
    mWidth = config.value<int>("width", 500);
    mResultHiddenHeight = config.value<int>("noresultsHeight", mSearchInput->minimumHeight() + 40);
    mResultShownHeight = config.value<int>("height", 500);

    resize(mWidth, mResultHiddenHeight);

#if defined(Q_OS_MAC)
    const int keycode = config.value<int>(QLatin1String("shortcutKeycode"), 49); // 49 = space
    const int modifier = config.value<int>(QLatin1String("shortcutModifier"), 256); // 256 = cmd
#elif defined(Q_WS_X11)
    const int keycode = config.value<int>(QLatin1String("shortcutKeycode"), 65); // 65 = space
    const int modifier = config.value<int>(QLatin1String("shortcutModifier"), 1); // 1 = shift
#endif
    connect(mShortcut, SIGNAL(activated(int)), this, SLOT(shortcutActivated(int)));
    mActivateId = mShortcut->registerShortcut(keycode, modifier);

    connect(&mKeepAlive, SIGNAL(timeout()), this, SLOT(keepAlive()));
    mKeepAlive.setInterval(1000 * 60 * 5); // five minutes
    mKeepAlive.start();

    System::hideFromPager(this);
}

void Chooser::shortcutActivated(int shortcut)
{
    if (shortcut != mActivateId)
        return;

    static const bool showHide = Config().isEnabled("showHide", false);
    if ((showHide && !isVisible()) || windowOpacity() < 1.) {
        mPrevious->record();
        enable();
    } else if ((showHide && isVisible()) || windowOpacity() > 0.) {
        disable();
        mPrevious->activate();
    }
}

void Chooser::search()
{
    QList<Match> matches = mSearchModel->matches(mSearchInput->text());
    mResultList->setMatches(matches);

    if (matches.isEmpty())
        hideResultList();
    else
        showResultList();
}

void Chooser::showEvent(QShowEvent *e)
{
    static QColor foreground = Config().value<QColor>("foregroundColor", QColor(Qt::black));
    setStyleSheet(QString("border: none; color: rgb(%1, %2, %3)")
                  .arg(foreground.red()).arg(foreground.green()).arg(foreground.blue()));

    QRect r(QPoint(), size());
    r.moveCenter(qApp->desktop()->screenGeometry(this).center());
    r.moveBottom(0);

    setWindowOpacity(0.);

    move(r.topLeft());

    QWidget::showEvent(e);

    static const bool showHide = Config().isEnabled("showHide", false);
    if (showHide) {
        static bool initial = true;
        if (initial) {
            hide();
            initial = false;
            return;
        }
    }
}

void Chooser::hideEvent(QHideEvent *e)
{
    QWidget::hideEvent(e);
    mSearchModel->reload();
}

void Chooser::enable()
{
    static const bool showHide = Config().isEnabled("showHide", false);
    if (showHide) {
        System::hideFromPager(this);
        show();
        QApplication::flush();
        QApplication::syncX();
    }

    setWindowOpacity(0.);
    System::raise(this);

    const int animateHeight = mResultList->isHidden() ? (mResultShownHeight - mResultHiddenHeight) / 2 : 0;
    ::animate(this, true, animateHeight);
    mSearchInput->setText(Config().value<QString>("last"));
    mSearchInput->selectAll();
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
            disable();
            mPrevious->activate();
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
    Config().setValue("last", mSearchInput->text());
    switch (type) {
    case Match::Application: {
        const QString path = index.data(ResultModel::FilePathRole).toString();
        const QStringList args = index.data(ResultModel::ArgumentsRole).toStringList();

        mInvoker->setApplication(path, args);
        mInvoker->invoke();

        mSearchModel->recordUserEntry(mSearchInput->text(), path);
        disable();
        //mPrevious->activate();
        break; }
    case Match::Url:
        QDesktopServices::openUrl(index.data(ResultModel::UrlRole).toString());
        disable();
        //mPrevious->activate();
        break;
    case Match::None:
        break;
    }
}

void Chooser::disable()
{
    static const bool showHide = Config().isEnabled("showHide", false);
    if (showHide) {
        hide();
        QApplication::flush();
        QApplication::syncX();
        mSearchInput->clear();
        return;
    }
    ::animate(this, false);
    mSearchInput->clear();
}

void Chooser::hideResultList()
{
    if (mResultList->isHidden())
        return;

    mResultList->hide();
    setMinimumHeight(mResultHiddenHeight); // ### why is this needed?
    resize(mWidth, mResultHiddenHeight);
}

void Chooser::showResultList()
{
    const int newHeight = mResultHiddenHeight + mResultList->sizeHint().height();
    setMinimumHeight(newHeight); // ### why oh why?

    mResultList->show();
    resize(mWidth, newHeight);
}

bool Chooser::event(QEvent *e)
{
    if (e->type() == QEvent::WindowDeactivate) {
        disable();
    }
    return QWidget::event(e);
}

#ifdef ENABLE_SERVER
void Chooser::onCommandReceived(const QString &command)
{
    if (command == "wakeup") {
        enable();
    } else if (command == "quit") {
        close();
        QCoreApplication::quit();
    } else if (command == "restart") {
        Server::instance()->close();
        QProcess::startDetached(QCoreApplication::arguments().first());
        close();
        QCoreApplication::quit();
    } else {
        qWarning("Unknown command received [%s]", qPrintable(command));
    }
}
#endif

void Chooser::keepAlive()
{
    // Keep the process in active memory
    if (windowOpacity() > 0.)
        return;

    QList<Match> matches = mSearchModel->matches(QLatin1String("a"));
    foreach(const Match& m, matches) {
        Q_UNUSED(m)
    }
}

