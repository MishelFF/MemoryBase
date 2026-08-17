#pragma once

#include <QString>

struct PlaceRecord
{
    int id = -1;
    QString name;
    double latitude = 0;
    double longitude = 0;
    double radiusKm = 5.0;
    int countryId = -1; // -1 = не привязано (NULL в БД)
};
Q_DECLARE_METATYPE(PlaceRecord)
Q_DECLARE_METATYPE(QList<PlaceRecord>)