#pragma once

#include <QAbstractListModel>
#include <QVector>
#include "PersonRecord.h"

class ScannerController;

// Список людей из справочника для QML (personsModel в
// FaceMatchingScreen.qml). Данные приходят через PhotoRepository —
// сама модель SQL не выполняет, только подписывается на сигналы
// репозитория и хранит последний полученный список.
class PersonsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        DisplayNameRole,
        HasReferenceRole,
    };

    explicit PersonsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Запросить у репозитория актуальный список. Сама модель
    // обновится асинхронно, когда придёт personsLoaded().
    Q_INVOKABLE void refresh();
signals:
    void loadPersons();
public slots:
    void onPersonsLoaded(QList<PersonRecord> persons);
    void onPersonCreated(PersonRecord person);

private:
    QVector<PersonRecord> m_items;
};
