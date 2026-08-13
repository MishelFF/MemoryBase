#include "EmbedRegionParser.h"
#include <QDebug>
#include <cstring>
#include <tinyxml2.h>

namespace {

// Находит сегмент APP1 с XMP-пакетом внутри JPEG и возвращает его
// содержимое как есть (текст XML, начиная с "<?xpacket ..."). Пустой
// массив — если XMP не найден или файл не похож на валидный JPEG.
//
// Формат JPEG: после SOI (0xFFD8) идёт последовательность сегментов
// маркер(1 байт 0xFF + 1 байт кода) + длина(2 байта, big-endian,
// включая сами эти 2 байта) + данные. XMP лежит в APP1 (0xFFE1) с
// сигнатурой "http://ns.adobe.com/xap/1.0/\0" в начале данных сегмента 
QByteArray extractXmpPacket(const QByteArray &data)
{
    static const char xmpSignature[] = "http://ns.adobe.com/xap/1.0/";
    const int sigLen = int(strlen(xmpSignature)) + 1; // +1 — завершающий \0 тоже часть сигнатуры в файле

    if (data.size() < 4 || quint8(data[0]) != 0xFF || quint8(data[1]) != 0xD8) return QByteArray(); // нет JPEG SOI — не JPEG или файл повреждён

    int pos = 2;
    while (pos + 4 <= data.size()) {
        if (quint8(data[pos]) != 0xFF) break; // испорченный поток сегментов — дальше не лезем
        int m = pos + 1;
        while (m < data.size() && quint8(data[m]) == 0xFF) m++; // съедаем padding
        if (m >= data.size()) break;
        quint8 marker = quint8(data[m]);
        pos = m + 1;                                    
        if (marker == 0xD8 || marker == 0xD9) continue; 
        if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) continue;
        if (marker == 0xDA) break;
        if (pos + 2 > data.size()) break;
        int segLen = (quint8(data[pos]) << 8) | quint8(data[pos + 1]); // длина сразу после маркера
        int segStart = pos + 2;
        int segDataLen = segLen - 2;
        if (segDataLen < 0 || segStart + segDataLen > data.size()) break;
        if (marker == 0xE1 && segDataLen > sigLen && memcmp(data.constData() + segStart, xmpSignature, strlen(xmpSignature)) == 0) {
            qDebug() << "raw signature bytes:" << data.mid(segStart, sigLen).toHex(' ');
            return data.mid(segStart + sigLen, segDataLen - sigLen);
        }
        pos = segStart + segDataLen;
    }
    return QByteArray();
}

// ACDSee: XMP namespace acdsee-rs. `exiftool -X file.jpg`, ветка rdf:Description/acdsee-rs:Regions.
QList<PhotoRegion> parseAcdseeRegions(tinyxml2::XMLElement *description)
{
    QList<PhotoRegion> result;

    auto *regionsEl = description->FirstChildElement("acdsee-rs:Regions");
    if (!regionsEl)
        return result; // в файле просто нет разметки лиц ACDSee — нормальный случай

    int appliedW = 0, appliedH = 0;
    if (auto *applied = regionsEl->FirstChildElement("acdsee-rs:AppliedToDimensions")) {
        applied->QueryIntAttribute("acdsee-stDim:w", &appliedW);
        applied->QueryIntAttribute("acdsee-stDim:h", &appliedH);
    }

    auto *list = regionsEl->FirstChildElement("acdsee-rs:RegionList");
    auto *bag = list ? list->FirstChildElement("rdf:Bag") : nullptr;
    if (!bag)
        return result;

    for (auto *li = bag->FirstChildElement("rdf:li"); li; li = li->NextSiblingElement("rdf:li")) {
        auto *itemDesc = li->FirstChildElement("rdf:Description");
        if (!itemDesc)
            continue;

        PhotoRegion region;
        const char *type = itemDesc->Attribute("acdsee-rs:Type");
        region.type = type ? QString::fromUtf8(type) : QString();
        region.name=QString();
        region.source=QString("acdsee");
        region.appliedToWidth = appliedW;
        region.appliedToHeight = appliedH;

        if (auto *dly = itemDesc->FirstChildElement("acdsee-rs:DLYArea")) {
            dly->QueryDoubleAttribute("acdsee-stArea:x", &region.dlyX);
            dly->QueryDoubleAttribute("acdsee-stArea:y", &region.dlyY);
            dly->QueryDoubleAttribute("acdsee-stArea:w", &region.dlyW);
            dly->QueryDoubleAttribute("acdsee-stArea:h", &region.dlyH);
        }
        if (auto *alg = itemDesc->FirstChildElement("acdsee-rs:ALGArea")) {
            region.hasAlg = true;
            alg->QueryDoubleAttribute("acdsee-stArea:x", &region.algX);
            alg->QueryDoubleAttribute("acdsee-stArea:y", &region.algY);
            alg->QueryDoubleAttribute("acdsee-stArea:w", &region.algW);
            alg->QueryDoubleAttribute("acdsee-stArea:h", &region.algH);
        }

        result.append(region);
    }

    return result;
}

 
// digiKam: XMP namespace mwg-rs, схема Metadata Working Group. См.
// дамп через `exiftool -X file.jpg`, ветка rdf:Description/mwg-rs:Regions.
//
// В отличие от acdsee-rs, тут нет разделения на "финал после правки
// пользователем" (DLYArea) и "сырой результат детектора" (ALGArea) —
// только один набор координат на регион, кладём его в dlyX/Y/W/H,
// hasAlg остаётся false. Зато есть mwg-rs:Name — конкретное имя
// человека, а не тип региона (в acdsee-rs такого поля нет
QList<PhotoRegion> parseMwgRegions(tinyxml2::XMLElement *description)
{
    QList<PhotoRegion> result;
 
    auto *regionsEl = description->FirstChildElement("mwg-rs:Regions");
    if (!regionsEl) return result; // в файле просто нет разметки лиц digiKam — нормальный случай
 
    int appliedW = 0, appliedH = 0;
    if (auto *applied = regionsEl->FirstChildElement("mwg-rs:AppliedToDimensions")) {
        applied->QueryIntAttribute("stDim:w", &appliedW);
        applied->QueryIntAttribute("stDim:h", &appliedH);
    }
 
    auto *list = regionsEl->FirstChildElement("mwg-rs:RegionList");
    auto *bag = list ? list->FirstChildElement("rdf:Bag") : nullptr;
    if (!bag) return result;
 
    for (auto *li = bag->FirstChildElement("rdf:li"); li; li = li->NextSiblingElement("rdf:li")) {
        auto *itemDesc = li->FirstChildElement("rdf:Description");
        if (!itemDesc) continue;
 
        PhotoRegion region;
        const char *name = itemDesc->Attribute("mwg-rs:Name");
        const char *type = itemDesc->Attribute("mwg-rs:Type");
        region.name = name ? QString::fromUtf8(name) : QString();
        region.type = type ? QString::fromUtf8(type) : QString();
        region.source = QStringLiteral("mwg-rs");
        region.appliedToWidth = appliedW;
        region.appliedToHeight = appliedH;
 
        if (auto *area = itemDesc->FirstChildElement("mwg-rs:Area")) {
            area->QueryDoubleAttribute("stArea:x", &region.dlyX);
            area->QueryDoubleAttribute("stArea:y", &region.dlyY);
            area->QueryDoubleAttribute("stArea:w", &region.dlyW);
            area->QueryDoubleAttribute("stArea:h", &region.dlyH);
        }
        // hasAlg остаётся false — у mwg-rs нет отдельного "сырого" набора координат
 
        result.append(region);
    }
 
    return result;
}
 
} // namespace

QList<PhotoRegion> parseEmbeddedRegions(const QByteArray &jpegData)
{
    QByteArray xmp = extractXmpPacket(jpegData);
    if (xmp.isEmpty())
        return {};
    tinyxml2::XMLDocument doc;
    int rdfStart = xmp.indexOf("<x:xmpmeta");
    if (rdfStart < 0) rdfStart = xmp.indexOf("<rdf:RDF"); 
    int rdfEnd = xmp.lastIndexOf("</x:xmpmeta>");
    if (rdfEnd < 0) rdfEnd = xmp.lastIndexOf("</rdf:RDF>");
    if (rdfEnd >= 0) {
        rdfEnd = xmp.indexOf('>', rdfEnd) + 1;// сдвигаем конец на закрывающий тег целиком
    }
    if (rdfStart < 0 || rdfEnd <= rdfStart) return {};
    QByteArray cleanXml = xmp.mid(rdfStart, rdfEnd - rdfStart);
    auto parseResult = doc.Parse(cleanXml.constData(), cleanXml.size());
    qDebug() << "result:" << parseResult
         << "name:" << doc.ErrorName()
         << "str:" << doc.ErrorStr()
         << "line:" << doc.ErrorLineNum();
    if ( parseResult!= tinyxml2::XML_SUCCESS) return {};

    auto *meta = doc.FirstChildElement("x:xmpmeta");
    auto *rdf = meta ? meta->FirstChildElement("rdf:RDF") : nullptr;
    auto *desc = rdf ? rdf->FirstChildElement("rdf:Description") : nullptr;
    if (!desc) return {};

    QList<PhotoRegion> regions = parseAcdseeRegions(desc);
    if (regions.isEmpty())
         regions = parseMwgRegions(desc); // DigiKam — когда будет реализовано

    return regions;
}
