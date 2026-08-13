#include <QFile>
#include <QDebug>
#include <QCryptographicHash>
#include <QMetaObject>
#include <QImageReader>
#include <QImage>
#include <QBuffer>

#include "TinyEXIF.h"
#include "EmbedRegionParser.h"

#include "ImportTask.h"
#include "DatabaseWorker.h"
#ifndef Q_OS_ANDROID
#include "FaceRecognizer.h"
#endif


ImportTask::ImportTask(const PhotoRecord &photo, DatabaseWorker *database, QThreadPool *pool,ScannerController *controller, bool reportProgress)
    : m_photo(photo), m_database(database), m_pool(pool), m_controller(controller), m_reportProgress(reportProgress){
    setAutoDelete(true);
}
void ImportTask::run() {
    PhotoRecord photo = m_photo;
    int inserted = -1;
    QMetaObject::invokeMethod(
        m_database, [&]() { inserted = m_database->insertPhoto(photo); }, Qt::BlockingQueuedConnection);
    if (inserted<=0) return;
    m_pool->start(new ExifTask(photo, m_database));
    m_pool->start(new Md5Task(photo, m_database));
    m_pool->start(new RegionTask(photo, m_database));
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
    DatabaseWorker *database = m_database;
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
    QMetaObject::invokeMethod(m_database, [database,photo]() { database->updateExif(photo); }, Qt::QueuedConnection);
}
Md5Task::Md5Task(const PhotoRecord &photo, DatabaseWorker *database) : m_photo(photo), m_database(database) {
    setAutoDelete(true);
}
void Md5Task::run() {
    QFile file(m_photo.fullPath);
    if (!file.open(QIODevice::ReadOnly)||(file.size()>MAXFILESIZETOMD5)) return;
    DatabaseWorker *database = m_database;
    PhotoRecord photo = m_photo;
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(&file);
    file.close();
    photo.md5 = hash.result().toHex();
    QMetaObject::invokeMethod(m_database, [database,photo]() { database->updateMD5(photo); }, Qt::QueuedConnection);
}

RegionTask::RegionTask(const PhotoRecord &photo, DatabaseWorker *database) : m_photo(photo), m_database(database) {
    setAutoDelete(true);
}
void RegionTask::run() {
    QFile file(m_photo.fullPath);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray data = file.readAll();
    file.close();

    QList<PhotoRegion> regions = parseEmbeddedRegions(data);
    if (regions.isEmpty()) return; // у большинства фото разметки лиц просто нет — не ошибка

    DatabaseWorker *database = m_database;
    int photoId = m_photo.id;
    QMetaObject::invokeMethod(m_database, [database, photoId, regions]() { database->insertRegions(photoId, regions); }, Qt::QueuedConnection);
}

ThumbnailTask::ThumbnailTask(const PhotoRecord &photo, DatabaseWorker *database,ScannerController *controller, bool reportProgress, int size)
    : m_photo(photo), m_database(database), m_size(size), m_controller(controller), m_reportProgress(reportProgress){
    setAutoDelete(true);
}
void ThumbnailTask::run() {
    
    QImageReader reader(m_photo.fullPath);
    reader.setAutoTransform(true);
    ScannerController *controller=m_controller;
    if (!reader.canRead()) {
        if (m_controller)
            QMetaObject::invokeMethod(controller, "incrementProcessed", Qt::QueuedConnection);
        return;
    }
    DatabaseWorker *database = m_database;
    bool reportProgress=m_reportProgress;
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
    QMetaObject::invokeMethod(database, [database,photo]() { database->insertThumbnail(photo); }, Qt::QueuedConnection);
    if (controller) 
         QMetaObject::invokeMethod(controller, "incrementProcessed", Qt::QueuedConnection);
   if (reportProgress && controller)
        QMetaObject::invokeMethod(controller, "onFileProcessed", Qt::QueuedConnection);    
}

ImportComplexTask::ImportComplexTask(const PhotoChunk &photos, DatabaseWorker *database,  ScannerController *controller, int size, int reportFreq)
    : m_photos(std::move(photos)), m_database(database), m_controller(controller), m_size(size),m_reportFreq(reportFreq)
{setAutoDelete(true);}

void ImportComplexTask::run() 
{
#ifndef Q_OS_ANDROID
    int counter=0;
    QByteArray data;
    QBuffer buffer(&data);
    QCryptographicHash hash(QCryptographicHash::Md5);
    DatabaseWorker *database = m_database;
    FaceRecognizer recognizer("models/shape_predictor_68_face_landmarks.dat","models/dlib_face_recognition_resnet_model_v1.dat");
    for (PhotoRecord &m_photo:m_photos){
        PhotoRecord photo=m_photo;
//        qDebug()<<"Import file: "+photo.path+photo.file;
        int inserted = -1;
        QMetaObject::invokeMethod(database, [&]() { inserted = database->insertPhoto(photo); }, Qt::BlockingQueuedConnection);
        if (inserted>0){
            QFile file(photo.fullPath);
            if (file.open(QIODevice::ReadOnly)) {
                data.truncate(0);
                data = file.readAll();
                TinyEXIF::EXIFInfo exif;
                int result = exif.parseFrom(reinterpret_cast<const uint8_t *>(data.constData()), data.size());
                if (result == TinyEXIF::PARSE_SUCCESS) {
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
                    QMetaObject::invokeMethod(database, [database, photo]() { database->updateExif(photo); }, Qt::QueuedConnection);
                }
                if (photo.extension.toUpper()=="JPG"||photo.extension.toUpper()=="JPEG"){
                    dlib::array2d<dlib::rgb_pixel> img=recognizer.preparePhoto(data);
                    if (img.nc()>0&&img.nr()>0){
                        QList<PhotoRegion> regions = parseEmbeddedRegions(data);
                        if (regions.isEmpty()){
                            regions=recognizer.detectFaceRegions(img);
                        }
                        if (!regions.isEmpty()) recognizer.getFacesDescriptions(img,regions);
                        if (!regions.isEmpty()) {
                            int photoId = photo.id;
                            QMetaObject::invokeMethod(database, [database, photoId, regions]() { database->insertRegions(photoId, regions); }, Qt::QueuedConnection);
                        }
                    }
                }
                if (file.size()<=MAXFILESIZETOMD5){
                    file.seek(0);
                    hash.reset();
                    hash.addData(&file);
                    photo.md5 = hash.result().toHex();
                    QMetaObject::invokeMethod(database, [database, photo]() { database->updateMD5(photo); }, Qt::QueuedConnection);
                }
                
                file.seek(0);
                QImageReader reader(&file);
                reader.setAutoTransform(true);
                if (reader.canRead()) {
                    const QSize original = reader.size();
                    if (original.isValid()) {
                        reader.setScaledSize(original.scaled(m_size, m_size, Qt::KeepAspectRatio));
                    }
                    QImage thumb = reader.read();
                    data.truncate(0);
                    buffer.close();
                    buffer.open(QIODevice::WriteOnly);
                    thumb.save(&buffer, "JPEG", 85);
                    buffer.close();
                    photo.thumbnail = data;
                    photo.thumbwidth = thumb.width();
                    photo.thumbheight = thumb.height();
                    QMetaObject::invokeMethod(database, [database, photo]() { database->insertThumbnail(photo); }, Qt::QueuedConnection);
                }    
                file.close();
            }

        } 

        if (m_controller)
            QMetaObject::invokeMethod(m_controller, "incrementProcessed", Qt::QueuedConnection);
        if ((m_reportFreq)&&(counter%m_reportFreq) && m_controller)
            QMetaObject::invokeMethod(m_controller, "onFileProcessed", Qt::QueuedConnection);
        ++counter;
    }
#endif
}

MissingFileTask::MissingFileTask(const PhotoChunk &photos, const QString &mountPoint,DatabaseWorker *database, ScannerController *controller, bool reportProgress)
    : m_photos(std::move(photos)), m_mountPoint(mountPoint), m_database(database),m_controller(controller), m_reportProgress(reportProgress)
{    setAutoDelete(true);}

void MissingFileTask::run()
{
    // Копируем всё нужное в локальные переменные — не трогаем m_photo/this
    // внутри отложенных (QueuedConnection) лямбд
    int index=0;
    QStringList missing;
    for(const PhotoRecord& photo : m_photos){
        int id = photo.id;
        QString path = photo.path;
        QString file = photo.file;
        DatabaseWorker *database = m_database;
        QString fullPath = QDir::cleanPath(m_mountPoint + "/" + path + "/" + file);
        bool exists = QFileInfo::exists(fullPath);
        if (!exists){
            QMetaObject::invokeMethod(database, [database, id, exists](){
                database->markMissing(id);
    //          else
    //            database->clearMissing(id);
            }, Qt::QueuedConnection);
            missing.append(path+"/"+file);
        }

        if (m_controller)
            QMetaObject::invokeMethod(m_controller, "incrementProcessed", Qt::QueuedConnection);
        if (!(index%100)&&(m_reportProgress && m_controller))   //  добавлена проверка
            QMetaObject::invokeMethod(m_controller, "onFileProcessed", Qt::QueuedConnection);
        ++index;
            
    }
    if (m_controller && !missing.isEmpty())
        QMetaObject::invokeMethod(m_controller, [controller = m_controller, missing]() {controller->appendMissingFiles(missing);}, Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_controller, "onFileProcessed", Qt::QueuedConnection);

}