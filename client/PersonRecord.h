#pragma once

#include <QString>
#include <QMetaType>

// Один человек из справочника, для передачи через сигналы
// PhotoRepository (аналог PhotoRecord у фото).
struct PersonRecord
{
    int id = 0;
    QString displayName;
    bool hasReference = false;
};

Q_DECLARE_METATYPE(PersonRecord)
