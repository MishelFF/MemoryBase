#include "PhotoSearchResultsModel.h"

PhotoSearchResultsModel::PhotoSearchResultsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PhotoSearchResultsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant PhotoSearchResultsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const PhotoRecord &item = m_items.at(index.row());
    switch (role) {
    case IdRole:        return item.id;
    case FileRole:      return item.file;
    case PathRole:      return item.path;
    case MediaNameRole: return item.mediaName;
    case MatchCountRole:return item.matchCount;
    default:            return QVariant();
    }
}

QHash<int, QByteArray> PhotoSearchResultsModel::roleNames() const
{
    return {
        { IdRole,        "id" },
        { FileRole,      "file" },
        { PathRole,      "path" },
        { MediaNameRole, "mediaName" },
        { MatchCountRole,"matchCount" },
    };
}

void PhotoSearchResultsModel::setPhotos(const QList<PhotoRecord> &photos)
{
    beginResetModel();
    m_items = QVector<PhotoRecord>(photos.cbegin(), photos.cend());
    endResetModel();
}
int PhotoSearchResultsModel::idAt(int row) const
{
    if (row < 0 || row >= m_items.size()) return -1;
    return m_items.at(row).id;
}