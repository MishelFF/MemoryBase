#pragma once

#include <QString>
#include <QByteArray>
#include <QDateTime>

#define MEDIA_LABEL "WD1000"
#define THUMB_SIZE 640
#define CONNECTDATABASE true
#define API_URL "http://box/mapi/"
#define MAXFILESIZETOMD5 500000000

struct PhotoRecord
{
    int id = -1;

    QString file;
    QString path;
    QString fullPath;

    QString extension;

    qint64 fileSize = 0;

    QDateTime lastModified;

    QString md5;

    QString maker;
    QString device;

    QDateTime dateCreation;
    QString mediaName;
    int rotation = 0;

    double latitude = 0;
    double longitude = 0;

    int width = 0;
    int height = 0;
    int thumbwidth = 0;
    int thumbheight = 0;

    QByteArray thumbnail;
};
Q_DECLARE_METATYPE(PhotoRecord)