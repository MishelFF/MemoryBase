// GeoCache.cpp
#include "GeoCache.h"
#include <QSettings>
#include <cmath>

GeoCache::GeoCache(const QString &settingsGroup, int maxEntries)
    : m_settingsGroup(settingsGroup), m_maxEntries(maxEntries)
{
    load();
}
QString GeoCache::gridKey(double lat, double lon)
{
    const double step = 0.05;
    double gridLat = std::round(lat / step) * step;
    double gridLon = std::round(lon / step) * step;
    return QString("%1_%2").arg(gridLat, 0, 'f', 2).arg(gridLon, 0, 'f', 2);
}
bool GeoCache::contains(const QString &key) const { return m_cache.contains(key); }

QString GeoCache::value(const QString &key)
{
    touch(key);
    return m_cache.value(key);
}
void GeoCache::insert(const QString &key, const QString &placeName)
{
    if (!m_cache.contains(key)) m_order.append(key);
    else touch(key);
    m_cache.insert(key, placeName);
    evictIfNeeded();
    save();
}
void GeoCache::touch(const QString &key)
{
    int idx = m_order.indexOf(key);
    if (idx >= 0) {
        m_order.removeAt(idx);
        m_order.append(key);
    }
}
void GeoCache::evictIfNeeded()
{
    while (m_order.size() > m_maxEntries) {
        const QString oldest = m_order.takeFirst();
        m_cache.remove(oldest);
    }
}
void GeoCache::load()
{
    QSettings settings;
    int size = settings.beginReadArray(m_settingsGroup);
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        const QString key = settings.value("key").toString();
        m_cache.insert(key, settings.value("value").toString());
        m_order.append(key); 
    }
    settings.endArray();
    evictIfNeeded(); 
}

void GeoCache::save() const
{
    QSettings settings;
    settings.beginWriteArray(m_settingsGroup);
    int i = 0;
    for (const QString &key : m_order) {
        settings.setArrayIndex(i++);
        settings.setValue("key", key);
        settings.setValue("value", m_cache.value(key));
    }
    settings.endArray();
}