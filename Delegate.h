#ifndef Delegate_h
#define Delegate_h

#include <QtGui>

class Delegate : public QItemDelegate
{
    Q_OBJECT
public:
    Delegate(QAbstractItemView *parent = 0);
    virtual void paint(QPainter *painter,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;
private:
    QAbstractItemView *mView;
};


#endif
