// GeoLookupService.h
#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include "GeoCache.h"

struct GeoRequest
{
    int photoId = -1;
    double lat = 0;
    double lon = 0;
};

class GeoLookupService : public QObject
{
    Q_OBJECT
public:
    explicit GeoLookupService(QObject *parent = nullptr);
    void setSettleDelay(int ms) { m_debounceTimer.setInterval(ms); }
public slots:
    void lookup(int photoId, double lat, double lon);
signals:
    void placeNameReady(int photoId, QString placeName, QString mapUrl, QString countryName);  
    void lookupError(int photoId, QString message);
private slots:
    void performLookup(); 

private:
    void sendRequest(const GeoRequest &req);
    QNetworkAccessManager m_netManager;
    GeoCache m_cache;
    QTimer m_debounceTimer;
    GeoRequest m_pending;             
    QNetworkReply *m_activeReply = nullptr;
};