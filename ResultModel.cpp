#include "ResultModel.h"

ResultModel::ResultModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void ResultModel::clearMatches()
{
    mMatches.clear();
    reset();
}

void ResultModel::setMatches(const QList<Match> &matches)
{
    mMatches.clear();
    mMatches = matches;
    reset();
}

int ResultModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return mMatches.size();
}

QVariant ResultModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()
        || index.column() != 0)
        return QVariant();

    int row = index.row();
    if (row < 0 || row >= mMatches.size())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        return mMatches.at(row).name;
    case Qt::DecorationRole:
        return mMatches.at(row).icon;
    default:
        break;
    }
    return QVariant();
}
