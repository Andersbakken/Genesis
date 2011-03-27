#include "ResultModel.h"
#include "Model.h"

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
    return mMatches.size();
}

QVariant ResultModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()
        || index.column() != 0)
        return QVariant();

    int row = index.row();
    if (row < 0 || row >= mMatches.size())
        return QVariant();

    if (role == Qt::DisplayRole)
        return mMatches.at(row).text;
    else if (role == Qt::DecorationRole)
        return mMatches.at(row).icon;
    return QVariant();
}
