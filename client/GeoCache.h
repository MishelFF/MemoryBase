#pragma once
#include <QMap>
#include <QStringList>
#include <QString>

class GeoCache
{
public:
    explicit GeoCache(const QString &settingsGroup = "GeoCache", int maxEntries = 200);

    static QString gridKey(double lat, double lon);
    bool contains(const QString &key) const;
    QString value(const QString &key); // не const: обновляет порядок LRU
    void insert(const QString &key, const QString &placeName);
private:
    void load();
    void save() const;
    void touch(const QString &key);       // переместить ключ в конец (последний использованный)
    void evictIfNeeded();

    QMap<QString, QString> m_cache;
    QStringList m_order; 
    QString m_settingsGroup;
    int m_maxEntries;
};