#pragma once

#include <QAbstractListModel>
#include <QVector>
#include "PhotoRecord.h"

class PhotoSearchResultsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        FileRole,
        PathRole,
        MediaNameRole,
        MatchCountRole,
    };

    explicit PhotoSearchResultsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setPhotos(const QList<PhotoRecord> &photos);
    Q_INVOKABLE int idAt(int row) const;
private:
    QVector<PhotoRecord> m_items;
};
