#include "PersonsModel.h"
#include "PhotoRepository.h"

PersonsModel::PersonsModel(QObject *parent)
    : QAbstractListModel(parent)
    
{
}

int PersonsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant PersonsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const PersonRecord &item = m_items.at(index.row());
    switch (role) {
    case IdRole:            return item.id;
    case DisplayNameRole:   return item.displayName;
    case HasReferenceRole:  return item.hasReference;
    default:                return QVariant();
    }
}

QHash<int, QByteArray> PersonsModel::roleNames() const
{
    return {
        { IdRole,           "id" },
        { DisplayNameRole,  "displayName" },
        { HasReferenceRole, "hasReference" },
    };
}

void PersonsModel::refresh()
{
    emit loadPersons();
}

void PersonsModel::onPersonsLoaded(QList<PersonRecord> persons)
{
    beginResetModel();
    m_items = QVector<PersonRecord>(persons.cbegin(), persons.cend());
    endResetModel();
}

void PersonsModel::onPersonCreated(PersonRecord person)
{
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(person);
    endInsertRows();
}
