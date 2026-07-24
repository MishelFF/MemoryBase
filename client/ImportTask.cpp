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

ImportTask::ImportTask(
        const PhotoRecord &photo,
        DatabaseWorker *database,
        QThreadPool *pool)
    :
      m_photo(photo),
      m_database(database),
      m_pool(pool)
{
    setAutoDelete(true);
}

void ImportTask::run()
{
    PhotoRecord photo = m_photo;

    bool inserted = false;

    //----------------------------------------------------------
    // Вставка записи в БД выполняется
    // в потоке DatabaseWorker
    //----------------------------------------------------------

    QMetaObject::invokeMethod(
        m_database,

        [&]()
        {
            inserted =
                m_database->insertPhoto(photo);
        },

        Qt::BlockingQueuedConnection);

    //----------------------------------------------------------

    if(!inserted)
        return;

    //----------------------------------------------------------
    // Запускаем остальные задачи
    //----------------------------------------------------------

    m_pool->start(
        new ExifTask(
            photo,
            m_database));

    m_pool->start(
        new Md5Task(
            photo,
            m_database));

    m_pool->start(
        new ThumbnailTask(
            photo,
            m_database));
}



ExifTask::ExifTask(
        const PhotoRecord &photo,
        DatabaseWorker *database)
    :
      m_photo(photo),
      m_database(database)
{
    setAutoDelete(true);
}

void ExifTask::run()
{
    PhotoRecord photo = m_photo;

    QFile file(photo.fullPath);

    if (!file.open(QIODevice::ReadOnly))
        return;

    QByteArray data = file.readAll();
    TinyEXIF::EXIFInfo exif;

    int result = exif.parseFrom(
    reinterpret_cast<const uint8_t*>(data.constData()),
    data.size());

    file.close();

    if (result != TinyEXIF::PARSE_SUCCESS)
        return;

    //--------------------------------------------------
    // Камера
    //--------------------------------------------------

    photo.maker = QString::fromStdString(exif.Make);
    photo.device = QString::fromStdString(exif.Model);

    //--------------------------------------------------
    // Размер изображения
    //--------------------------------------------------

    photo.width = exif.ImageWidth;
    photo.height = exif.ImageHeight;

    //--------------------------------------------------
    // Ориентация
    //--------------------------------------------------

    photo.rotation = exif.Orientation;

    //--------------------------------------------------
    // Координаты
    //--------------------------------------------------

    photo.latitude = exif.GeoLocation.Latitude;
    photo.longitude = exif.GeoLocation.Longitude;

    //--------------------------------------------------
    // Дата съемки
    //--------------------------------------------------

    if (!exif.DateTime.empty())
    {
        photo.dateCreation =
            QDateTime::fromString(
                QString::fromStdString(exif.DateTime),
                "yyyy:MM:dd hh:mm:ss");
    }

    //--------------------------------------------------
    // Запись в PostgreSQL
    //--------------------------------------------------

    QMetaObject::invokeMethod(
        m_database,

        [&]()
        {
            m_database->updateExif(photo);
        },

        Qt::BlockingQueuedConnection);
}


Md5Task::Md5Task(
        const PhotoRecord &photo,
        DatabaseWorker *database)
    :
      m_photo(photo),
      m_database(database)
{
    setAutoDelete(true);
}

void Md5Task::run()
{
    QFile file(m_photo.fullPath);

    if (!file.open(QIODevice::ReadOnly))
        return;

    PhotoRecord photo = m_photo;
    QCryptographicHash hash(
            QCryptographicHash::Md5);
    hash.addData(&file);        

    file.close();
    photo.md5 = hash.result().toHex();

    QMetaObject::invokeMethod(
        m_database,

        [&]()
        {
            m_database->updateMD5(photo);
        },

        Qt::BlockingQueuedConnection);
}


ThumbnailTask::ThumbnailTask(
        const PhotoRecord &photo,
        DatabaseWorker *database,
        int size)
    :
      m_photo(photo),
      m_database(database),
      m_size(size)
{
    setAutoDelete(true);
}

void ThumbnailTask::run()
{
    //---------------------------------------------------------
    // Загружаем изображение
    //---------------------------------------------------------

    QImageReader reader(m_photo.fullPath);
    reader.setAutoTransform(true);

    const QSize original = reader.size();
    if (original.isValid()) {
        reader.setScaledSize(original.scaled(m_size, m_size, Qt::KeepAspectRatio));
    }

    QImage thumb = reader.read();  
    
    //---------------------------------------------------------
    // Кодируем в JPEG
    //---------------------------------------------------------

    QByteArray data;

    QBuffer buffer(&data);

    buffer.open(QIODevice::WriteOnly);

    thumb.save(&buffer,"JPEG",85);

    buffer.close();

    
    //---------------------------------------------------------
    // Обновляем PhotoRecord
    //---------------------------------------------------------

    PhotoRecord photo = m_photo;

    photo.thumbnail = data;

    photo.thumbwidth = thumb.width();

    photo.thumbheight = thumb.height();

    //---------------------------------------------------------
    // Записываем в БД
    //---------------------------------------------------------

    QMetaObject::invokeMethod(
        m_database,

        [&]()
        {
            m_database->insertThumbnail(photo);
        },

        Qt::BlockingQueuedConnection);
}