#include "ResultModel.h"

ResultModel::ResultModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void ResultModel::setMatches(const QList<Match> &matches)
{
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

    const Match &match = mMatches.at(row);
    switch (role) {
    case FilePathRole:
        if (match.type != Match::Application)
            return QVariant();
        return match.filePath;
    case UrlRole:
        Q_ASSERT(match.type == Match::Url);
        return match.url;
    case TypeRole:
        return match.type;
    case ArgumentsRole:
        return match.arguments;
    case Qt::DisplayRole:
        return match.name;
    case Qt::DecorationRole:
        return match.icon;
    case KeySequencesRole: {
        extern const Qt::KeyboardModifier numericModifier;
        QList<QKeySequence> ret;
        if (!match.keySequence.isEmpty())
            ret.append(match.keySequence);
        if (index.row() < 9)
            ret.append(QKeySequence(numericModifier | (Qt::Key_1 + index.row())));
        return qVariantFromValue(ret);
    }
    default:
        break;
    }
    return QVariant();
}
