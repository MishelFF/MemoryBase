#pragma once

#include <QString>
#include <QList>

struct CountryBBox
{
    int id = -1;
    double latMin = 0;
    double latMax = 0;
    double lonMin = 0;
    double lonMax = 0;
};

struct CountryRecord
{
    int id = -1;
    QString name;
    QList<CountryBBox> bboxes;
};
Q_DECLARE_METATYPE(CountryRecord)
Q_DECLARE_METATYPE(QList<CountryRecord>)