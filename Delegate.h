#ifndef Delegate_h
#define Delegate_h

#include <QtGui>
#include "Config.h"

class Delegate : public QItemDelegate
{
    Q_OBJECT
public:
    Delegate(QAbstractItemView *parent = 0);
    virtual void paint(QPainter *painter,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
private:
    QAbstractItemView *mView;
    Config mConfig;
    int mPixelSize;
};


#endif
