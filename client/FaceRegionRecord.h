#pragma once

#include <QString>
#include <QMetaType>

struct FaceRegionRecord  
{
    int id = 0;
    int photoId = 0;
    QString faceName;
};

Q_DECLARE_METATYPE(FaceRegionRecord)
