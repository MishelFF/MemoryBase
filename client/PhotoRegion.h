#pragma once

#include <QString>


// Одна размеченная область (обычно лицо) на фото, извлечённая из
// XMP-тегов ACDSee (namespace acdsee-rs). См. AcdseeRegionParser.h.
//
// Координаты хранятся как в самом XMP — доли (0..1) от размеров, на
// которые ACDSee их изначально накладывала (appliedToWidth/Height), а
// не пересчитанные в пиксели текущего файла: так значения остаются
// верными независимо от последующего изменения размера/поворота фото.
struct PhotoRegion
{
    QString type; // "Face" и т.п. — как в XMP
    QString name;
    QString source;
    QString descriptorModel="dlib_face_recognition_resnet_model_v1";;
    // Финальная область, которую реально показывает/использует ACDSee
    // (после ручной корректировки пользователем, если она была).
    double dlyX = 0;
    double dlyY = 0;
    double dlyW = 0;
    double dlyH = 0;

    // Сырой результат алгоритма детекции, до правок пользователем.
    // Не всегда присутствует в файле — hasAlg это отражает.
    bool hasAlg = false;
    double algX = 0;
    double algY = 0;
    double algW = 0;
    double algH = 0;

    int appliedToWidth = 0;
    int appliedToHeight = 0;
    QVector<float> descriptor;
    QByteArray faceChip;
};
