#include "GeoLookupService.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

static const int DEFAULT_SETTLE_MS = 500; // пауза 

GeoLookupService::GeoLookupService(QObject *parent) : QObject(parent)
{
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(DEFAULT_SETTLE_MS);
    connect(&m_debounceTimer, &QTimer::timeout, this, &GeoLookupService::performLookup);
}

void GeoLookupService::lookup(int photoId, double lat, double lon)
{
    if (lat == 0 && lon == 0) {
        m_debounceTimer.stop();
        emit lookupError(photoId, "Нет GPS данных");
        return;
    }
    const QString key = GeoCache::gridKey(lat, lon);
    if (m_cache.contains(key)) {
        m_debounceTimer.stop();
        const QString mapUrl = QString("https://www.openstreetmap.org/?mlat=%1&mlon=%2#map=14/%1/%2").arg(lat, 0, 'f', 6).arg(lon, 0, 'f', 6);
        emit placeNameReady(photoId, m_cache.value(key), mapUrl,QString());
        return;
    }
    m_pending = {photoId, lat, lon};
    m_debounceTimer.start(); 
}

void GeoLookupService::performLookup()
{
    if (m_activeReply) {
        m_activeReply->abort();
        m_activeReply = nullptr;
    }
    sendRequest(m_pending);
}

void GeoLookupService::sendRequest(const GeoRequest &req)
{
    const QString key = GeoCache::gridKey(req.lat, req.lon);
    const QString mapUrl = QString("https://www.openstreetmap.org/?mlat=%1&mlon=%2#map=14/%1/%2").arg(req.lat, 0, 'f', 6).arg(req.lon, 0, 'f', 6);

    QUrl url("https://nominatim.openstreetmap.org/reverse");
    QUrlQuery query;
    query.addQueryItem("format", "json");
    query.addQueryItem("lat", QString::number(req.lat, 'f', 6));
    query.addQueryItem("lon", QString::number(req.lon, 'f', 6));
    query.addQueryItem("zoom", "10");
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "MFMediaDBApp/1.0 (adelkahome@proton.me)");

    m_activeReply = m_netManager.get(request);
    connect(m_activeReply, &QNetworkReply::finished, this, [this, req, key, mapUrl]() {
        QNetworkReply *reply = m_activeReply;
        m_activeReply = nullptr;
        reply->deleteLater();
        if (reply->error() == QNetworkReply::OperationCanceledError) return; 
        if (reply->error() != QNetworkReply::NoError) {emit lookupError(req.photoId, reply->errorString());return;}

        const auto obj = QJsonDocument::fromJson(reply->readAll()).object();
        const auto address = obj.value("address").toObject();
        QString city = address.value("city").toString();
        if (city.isEmpty()) city = address.value("town").toString();
        if (city.isEmpty()) city = address.value("village").toString();
        if (city.isEmpty()) city = address.value("county").toString();
        const QString country = address.value("country").toString();
        QString placeName;
        if (!city.isEmpty() && !country.isEmpty())
            placeName = QString("%1, %2").arg(city, country);
        else
            placeName = obj.value("display_name").toString();
        if (placeName.isEmpty())
            placeName = "Место не определено";
        m_cache.insert(key, placeName);
        emit placeNameReady(req.photoId, placeName, mapUrl,country);
    });
}