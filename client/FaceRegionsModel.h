#pragma once

#include <QAbstractListModel>
#include <QVector>
#include "FaceRegionRecord.h"

class PhotoRepository;

// Список регионов лиц для QML. Один класс на два сценария
// FaceMatchingScreen.qml, различие только в том, какой слот
// репозитория вызывается при refresh:
//
//   loadUnresolved()   -> repository->loadUnresolvedRegions()
//   loadForPerson(id)  -> repository->loadRegionsForPerson(id)
//
// Как и PersonsModel — сама SQL не выполняет, только слушает сигналы.
class FaceRegionsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        PhotoIdRole,
        FaceNameRole,
    };

    explicit FaceRegionsModel(bool showingUnresolved,QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;


signals:
    void loadRegionsForPerson(int personId);
    void loadUnresolvedRegions();
public slots:
    void onUnresolvedRegionsLoaded(QList<FaceRegionRecord> regions);
    void onPersonRegionsLoaded(int personId, QList<FaceRegionRecord> regions);

    // Мутации приходят от других частей UI (панель сопоставления
    // вызывает их на репозитории напрямую), но эта модель должна
    // остаться консистентной без ручного refresh() после каждой —
    // проще всего убрать регион из списка "нераспознанных" сразу по
    // приходу сигнала, а не ждать, пока кто-то явно перечитает список.
    void onRegionAssigned(int regionId, int personId);
    void onRegionUnassigned(int regionId);
private:
    // true, пока модель показывает "нераспознанные" (нужно, чтобы
    // onRegionAssigned/onRegionUnassigned знали, актуальны ли они
    // для текущего режима модели — режим "для человека" не должен
    // реагировать на assign/unassign чужих регионов)
    bool m_showingUnresolved = false;

    void removeItemById(int regionId);

    QVector<FaceRegionRecord> m_items;
};
