#ifndef RESULTMODEL_H
#define RESULTMODEL_H

#include "Model.h"
#include <QAbstractListModel>

class Match;

class ResultModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role {
        TypeRole = Qt::UserRole,
        FilePathRole,
        UrlRole
    };
    ResultModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;

    void setMatches(const QList<Match>& matches);

private:
    QList<Match> mMatches;
};

#endif // RESULTMODEL_H
