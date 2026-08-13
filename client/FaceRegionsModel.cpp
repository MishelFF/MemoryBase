#include "FaceRegionsModel.h"
#include "PhotoRepository.h"

FaceRegionsModel::FaceRegionsModel(bool showingUnresolved,QObject *parent) :QAbstractListModel(parent),m_showingUnresolved(showingUnresolved) 
    
{
}

int FaceRegionsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant FaceRegionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const FaceRegionRecord &item = m_items.at(index.row());
    switch (role) {
    case IdRole:       return item.id;
    case PhotoIdRole:  return item.photoId;
    case FaceNameRole: return item.faceName;
    default:           return QVariant();
    }
}

QHash<int, QByteArray> FaceRegionsModel::roleNames() const
{
    return {
        { IdRole,       "id" },
        { PhotoIdRole,  "photoId" },
        { FaceNameRole, "faceName" },
    };
}


void FaceRegionsModel::onUnresolvedRegionsLoaded(QList<FaceRegionRecord> regions)
{
    if (!m_showingUnresolved)
        return; // ответ на устаревший запрос — режим модели уже сменился

    beginResetModel();
    m_items = QVector<FaceRegionRecord>(regions.cbegin(), regions.cend());
    endResetModel();
}

void FaceRegionsModel::onPersonRegionsLoaded(int personId, QList<FaceRegionRecord> regions)
{
    if (m_showingUnresolved )
        return; // ответ на устаревший запрос — либо режим сменился, либо это другой человек

    beginResetModel();
    m_items = QVector<FaceRegionRecord>(regions.cbegin(), regions.cend());
    endResetModel();
}

void FaceRegionsModel::onRegionAssigned(int regionId, int personId)
{
    if (m_showingUnresolved)
        removeItemById(regionId); 
    else 
        emit loadRegionsForPerson(personId);

}

void FaceRegionsModel::onRegionUnassigned(int regionId)
{
    if (!m_showingUnresolved) 
        removeItemById(regionId); // ушёл из списка "регионы человека"
    else 
        emit loadUnresolvedRegions();
}

void FaceRegionsModel::removeItemById(int regionId)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).id == regionId) {
            beginRemoveRows(QModelIndex(), i, i);
            m_items.removeAt(i);
            endRemoveRows();
            return;
        }
    }
}
