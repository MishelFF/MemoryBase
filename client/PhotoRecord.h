#pragma once

#include <QString>
#include <QByteArray>
#include <QDateTime>

#define MEDIA_LABEL "WD1000"
#define THUMB_SIZE 640
#define CONNECTDATABASE true
#define USESHORTTASKS false
#define API_URL "http://localhost:6780/"
#define MAXFILESIZETOMD5 500000000

struct PhotoRecord
{
    int id = -1;

    QString file;
    QString path;
    QString fullPath;

    QString extension;

    qint64 fileSize = 0;

    QDateTime dateAvailable;

    QString md5;

    QString maker;
    QString device;

    QDateTime dateCreation;
    QString mediaName;
    QString comment;
    int rotation = 0;

    double latitude = 0;
    double longitude = 0;

    int width = 0;
    int height = 0;
    int thumbwidth = 0;
    int thumbheight = 0;
    QByteArray thumbnail;
    int matchCount = 0;
};
Q_DECLARE_METATYPE(PhotoRecord)