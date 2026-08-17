#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QList>
#include <QMetaType>

struct PhotoFilter
{
    bool limitEnabled = false;
    int maxCount = 200;

    bool mediaEnabled = false;
    QStringList media;

    bool dateEnabled = false;
    QDateTime dateFrom;
    QDateTime dateTo;

    bool countryEnabled = false;
    int countryId = -1;
    bool placeEnabled = false;
    int placeId = -1;

    bool facesEnabled = false;
    QList<int> personIds;
    bool facesUseDescriptor = false;
    double similarityThreshold = 0.6; // используется только при facesUseDescriptor
    enum class SortBy { Date, FaceMatchCount };
    SortBy sortBy = SortBy::Date;
};

Q_DECLARE_METATYPE(PhotoFilter)
