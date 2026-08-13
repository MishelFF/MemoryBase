#pragma once

#include <QByteArray>
#include <QList>

#include "PhotoRegion.h"

// Извлекает области лиц, встроенные в JPEG сторонними программами
// каталогизации, из уже прочитанного в память содержимого файла.
//
// Понимает разметку ACDSee (XMP, namespace acdsee-rs) и digiKam (XMP,
// схема Metadata Working Group, namespace mwg-rs). Обе схемы лежат в
// одном и том же XMP-пакете JPEG — extractXmpPacket в .cpp их не
// различает, различие только в разборе RDF внутри уже извлечённого
// текста. 

QList<PhotoRegion> parseEmbeddedRegions(const QByteArray &jpegData);
