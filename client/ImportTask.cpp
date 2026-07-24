#include <QFile>
#include <QDebug>
#include <QCryptographicHash>
#include <QMetaObject>
#include <QImageReader>
#include <QImage>
#include <QBuffer>

#include "TinyEXIF.h"

#include "ImportTask.h"
#include "DatabaseWorker.h"
ImportTask::ImportTask(const PhotoRecord &photo, DatabaseWorker *database, QThreadPool *pool,ScannerController *controller, bool reportProgress)
    : m_photo(photo), m_database(database), m_pool(pool), m_controller(controller), m_reportProgress(reportProgress){
    setAutoDelete(true);
}
void ImportTask::run() {
    PhotoRecord photo = m_photo;
    bool inserted = false;
    QMetaObject::invokeMethod(
        m_database, [&]() { inserted = m_database->insertPhoto(photo); }, Qt::BlockingQueuedConnection);
    if (!inserted) return;
    m_pool->start(new ExifTask(photo, m_database));
    m_pool->start(new Md5Task(photo, m_database));
    m_pool->start(new ThumbnailTask(photo, m_database,m_controller,m_reportProgress));
    if (m_controller)
        QMetaObject::invokeMethod(m_controller, "incrementProcessed", Qt::QueuedConnection);
    if (m_reportProgress && m_controller)
        QMetaObject::invokeMethod(m_controller, "onFileProcessed", Qt::QueuedConnection);

}
ExifTask::ExifTask(const PhotoRecord &photo, DatabaseWorker *database) : m_photo(photo), m_database(database) {
    setAutoDelete(true);
}
void ExifTask::run() {
    PhotoRecord photo = m_photo;
    QFile file(photo.fullPath);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray data = file.readAll();
    TinyEXIF::EXIFInfo exif;
    int result = exif.parseFrom(reinterpret_cast<const uint8_t *>(data.constData()), data.size());
    file.close();
    if (result != TinyEXIF::PARSE_SUCCESS) return;
    photo.maker = QString::fromStdString(exif.Make);
    photo.device = QString::fromStdString(exif.Model);
    photo.width = exif.ImageWidth;
    photo.height = exif.ImageHeight;
    photo.rotation = exif.Orientation;
    photo.latitude = exif.GeoLocation.Latitude;
    photo.longitude = exif.GeoLocation.Longitude;
    if (!exif.DateTime.empty()) {
        photo.dateCreation = QDateTime::fromString(QString::fromStdString(exif.DateTime), "yyyy:MM:dd hh:mm:ss");
    }
    QMetaObject::invokeMethod(m_database, [&]() { m_database->updateExif(photo); }, Qt::BlockingQueuedConnection);
}
Md5Task::Md5Task(const PhotoRecord &photo, DatabaseWorker *database) : m_photo(photo), m_database(database) {
    setAutoDelete(true);
}
void Md5Task::run() {
    QFile file(m_photo.fullPath);
    if (!file.open(QIODevice::ReadOnly)||(file.size()>MAXFILESIZETOMD5)) return;
    PhotoRecord photo = m_photo;
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(&file);
    file.close();
    photo.md5 = hash.result().toHex();
    QMetaObject::invokeMethod(m_database, [&]() { m_database->updateMD5(photo); }, Qt::BlockingQueuedConnection);
}
ThumbnailTask::ThumbnailTask(const PhotoRecord &photo, DatabaseWorker *database,ScannerController *controller, bool reportProgress, int size)
    : m_photo(photo), m_database(database), m_size(size), m_controller(controller), m_reportProgress(reportProgress){
    setAutoDelete(true);
}
void ThumbnailTask::run() {
    
    QImageReader reader(m_photo.fullPath);
    reader.setAutoTransform(true);
    if (!reader.canRead()) {
        if (m_controller)
            QMetaObject::invokeMethod(m_controller, "incrementProcessed", Qt::QueuedConnection);
        return;
    }
    const QSize original = reader.size();
    if (original.isValid()) {
        reader.setScaledSize(original.scaled(m_size, m_size, Qt::KeepAspectRatio));
    }
    QImage thumb = reader.read();
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    thumb.save(&buffer, "JPEG", 85);
    buffer.close();
    PhotoRecord photo = m_photo;
    photo.thumbnail = data;
    photo.thumbwidth = thumb.width();
    photo.thumbheight = thumb.height();
    QMetaObject::invokeMethod(m_database, [&]() { m_database->insertThumbnail(photo); }, Qt::BlockingQueuedConnection);
    if (m_controller) 
         QMetaObject::invokeMethod(m_controller, "incrementProcessed", Qt::QueuedConnection);
   if (m_reportProgress && m_controller)
        QMetaObject::invokeMethod(m_controller, "onFileProcessed", Qt::QueuedConnection);    
}