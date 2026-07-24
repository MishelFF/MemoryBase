#include "DatabaseWorker.h"

#include <QSqlQuery>
#include <QSqlField>
#include <QRegularExpression>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include "DatabaseWorker.h"
#include "SettingsManager.h"

DatabaseWorker::DatabaseWorker(QObject *parent) : PhotoRepository(parent)
{
    qDebug() << "DatabaseWorker created";
}

DatabaseWorker::~DatabaseWorker()
{
    close();
}
void DatabaseWorker::open(SettingsManager *rpSettings)
{
    
    QString connectionName ="PhotoDB_connection";
    if(QSqlDatabase::contains(connectionName)){
        db =QSqlDatabase::database(connectionName);
    }
    else{
        db =QSqlDatabase::addDatabase("QPSQL",connectionName);
        db.setHostName(rpSettings->dbHost());
        db.setPort(rpSettings->dbPort());
        db.setDatabaseName(rpSettings->dbName());
        db.setUserName(rpSettings->dbUser());
        db.setPassword(rpSettings->dbPassword());
        if (db.open()) {
            emit connected(true);
            emit status("PostgreSQL connected");
        }
        else{
            emit connected(false);
            emit error(db.lastError().text());
            qDebug()<<"Ошибка подключения :"<<db.lastError().text();
        }
//    loadCache();
    }   
}


void DatabaseWorker::close()
{
    if(db.isOpen())
        db.close();

    QSqlDatabase::removeDatabase("PhotoDB");
}
/*
bool DatabaseWorker::connectDatabase()
{

    QString connectionName ="PhotoDB_connection";
    if(QSqlDatabase::contains(connectionName)){
        db =QSqlDatabase::database(connectionName);
    }
    else{
        db =QSqlDatabase::addDatabase("QPSQL",connectionName);
        db.setHostName("box");
        db.setPort(5432);
        db.setDatabaseName("photobase");
        db.setUserName("photouser");
        db.setPassword("reflected");
    }
    return db.open();
}*/

void DatabaseWorker::loadMedia()
{
    QSqlQuery query(db);
    QStringList media;
    query.prepare(R"(SELECT DISTINCT media_name FROM photobase.photo_images ORDER BY media_name)");
    if(query.exec()){
        while(query.next()){
            media.append(query.value(0).toString());
        }
        emit mediaLoaded(media);
    }
    else{
        emit error(query.lastError().text());
    }
}

void DatabaseWorker::loadFolders(const QString &mediaName)
{
    QStringList folders;
    QSqlQuery query(db);
    query.prepare(R"(SELECT DISTINCT path FROM photobase.photo_images WHERE media_name=:media ORDER BY path)");
    query.bindValue(":media",mediaName);
    if(query.exec()){
        while(query.next()){
            folders.append(query.value(0).toString());
        }
        emit foldersLoaded(mediaName,folders);
    }
    else { emit error(query.lastError().text());}
}

void DatabaseWorker::loadPhotos(const QString &mediaName,const QString &path)
{
    QList<PhotoRecord> photos;
    QSqlQuery query(db);
    query.prepare(R"(SELECT id, file, path, media_name, width, height, ext, date_creation FROM photobase.photo_images WHERE media_name=:media AND path=:path ORDER BY file)");
    query.bindValue(":media",mediaName);
    query.bindValue(":path",path);
    if(query.exec()){
        while(query.next()){
            PhotoRecord photo;
            photo.id =query.value("id").toInt();
            photo.file = query.value("file").toString();
            photo.path = query.value("path").toString();
            photo.mediaName = query.value("media_name").toString();
            photo.width =
                query.value("width")
                .toInt();
            photo.height =
                query.value("height")
                .toInt();
            photo.extension =
                query.value("ext")
                .toString();
            photo.dateCreation =
                query.value("date_creation")
                .toDateTime();
            photos.append(photo);
        }
        emit photosLoaded(photos);
    }
    else
    {
        emit error(query.lastError().text());
    }
}



void DatabaseWorker::loadCache()
{
    if(!m_cache) return;
    m_cache->clear();

    QSqlQuery query(db);
//    query.clear();
    QString safeLabel = QString("'"  MEDIA_LABEL "'");
    QString rawSql = QString( "SELECT id, path, file, filesize, lastmodified FROM photobase.photo_images WHERE media_name = %1").arg(safeLabel);
//    query.prepare(R"(SELECT id,path,file,filesize,lastmodified FROM photo_images WHERE media_name=:media_label)");
//    query.bindValue(":media_label",safeLabel);
    if(!query.exec(rawSql))
    {
        qDebug()<<"Ошибка  :"<<query.lastError().text();
        emit status(query.lastError().text());
    }

    while(query.next())
    {
        FileKey key;


        int id=query.value("id").toInt();

        key.path=query.value("path").toString();
        key.file=query.value("file").toString();
        key.size=query.value("filesize").toLongLong();
        key.modified=query.value("lastmodified").toDateTime();

        m_cache->insert(key,id);
    }

    emit status(QString("Cache loaded: %1 files").arg(m_cache->size()));
}
//bool DatabaseWorker::exists(
//        const FileKey &key)
//{
 //   return m_cache->contains(key);
//}
int DatabaseWorker::insertPhoto(
        PhotoRecord &photo)
{
    QSqlQuery query(db);

    query.prepare(R"(INSERT INTO photobase.photo_images(file,path,ext,filesize,lastmodified,media_name) VALUES (:file,:path:ext,:size,:modified,:media_label) RETURNING id )");

    query.bindValue(":file",photo.file);
    query.bindValue(":path",photo.path);
    query.bindValue(":ext",photo.extension);
    query.bindValue(":size",photo.fileSize);
    query.bindValue(":modified",photo.lastModified);
    query.bindValue(":media_label",MEDIA_LABEL);

    if(!query.exec())
    {
        emit status(query.lastError().text());
        return -1;
    }

    query.next();

    photo.id=query.value(0).toInt();

    FileKey key;

    key.path=photo.path;
    key.file=photo.file;
    key.size=photo.fileSize;
    key.modified=photo.lastModified;

    m_cache->insert(key,photo.id);

    return photo.id;
}
bool DatabaseWorker::updateExif(
        const PhotoRecord &photo)
{
    QSqlQuery query(db);

    query.prepare(R"(UPDATE photobase.photo_images SET maker=:maker,device=:device,
        date_creation=:date,rotation=:rotation,latitude=:lat,longitude=:lon,
        width=:width,height=:height WHERE id=:id)");

    query.bindValue(":id",photo.id);
    query.bindValue(":maker",photo.maker);
    query.bindValue(":device",photo.device);
    query.bindValue(":date",photo.dateCreation);
    query.bindValue(":rotation",photo.rotation);
    query.bindValue(":lat",photo.latitude);
    query.bindValue(":lon",photo.longitude);
    query.bindValue(":width",photo.width);
    query.bindValue(":height",photo.height);

    return query.exec();
}
bool DatabaseWorker::updateMD5(
        const PhotoRecord &photo)
{
    QSqlQuery query(db);

    query.prepare("UPDATE photobase.photo_images SET md5sum=:md5 WHERE id=:id");

    query.bindValue(":id",photo.id);
    query.bindValue(":md5",photo.md5);

    return query.exec();
}
bool DatabaseWorker::insertThumbnail(
        const PhotoRecord &photo)
{
    QSqlQuery query(db);

    query.prepare(R"(INSERT INTO photobase.photo_thumbnails(photo_id,size,width,height,image_data,filesize)
        VALUES(:id,:size,:width,:height,:image,:filesize))");

    query.bindValue(":id",photo.id);
    query.bindValue(":size",512);
    query.bindValue(":width",photo.thumbwidth);
    query.bindValue(":height",photo.thumbheight);
    query.bindValue(":image",photo.thumbnail);
    query.bindValue(":filesize",photo.thumbnail.size());

    return query.exec();
}
/*void DatabaseWorker::beginTransaction()
{
    db.transaction();
}

void DatabaseWorker::commit()
{
    db.commit();
}
*/
QList<PhotoRecord> DatabaseWorker::getPhotosWithoutThumbnail(const QString &rootFolder)
{
    QString rootString=QDir::fromNativeSeparators(rootFolder);
    rootString.replace(QRegularExpression("^[a-zA-Z]:"), "");
    QRegularExpression driveRegex("^([a-zA-Z]:)");
    QRegularExpressionMatch match = driveRegex.match(rootFolder);
    QString driveLetter = "";
    if (match.hasMatch()) {
        driveLetter = match.captured(1); // Запоминаем то, что попало в скобки (например, "D:")
    }
    
    QList<PhotoRecord> list;

    QSqlQuery query(db);
    query.prepare(R"(SELECT id,path,file FROM photobase.photo_images WHERE path LIKE :path AND NOT EXISTS (SELECT 1 FROM photobase.photo_thumbnails WHERE photo_id = photo_images.id) ORDER BY path, file)");

    query.bindValue(":path", rootString + "%");

    if (!query.exec())
    {
        emit status(query.lastError().text());
        return list;
    }

    while (query.next())
    {
        PhotoRecord photo;

        photo.id   = query.value(0).toInt();
        photo.path = query.value(1).toString();
        photo.file = query.value(2).toString();

        QString fPath=QDir::toNativeSeparators(driveLetter+photo.path + "/" + photo.file);
        photo.fullPath = fPath;

        list.append(photo);
    }

    return list;
}

/*QList<PhotoRecord> DatabaseWorker::loadPhotoTree()
{
    QList<PhotoRecord> result;
    if (!db.isOpen()) {
        emit status("Database is not open");
        return result;
    }

    QSqlQuery query(db);
    query.prepare(R"(SELECT id,file,path,media_name FROM photobase.photo_images ORDER BY media_name,path,file)");
    if (!query.exec()){
        emit status("loadPhotoTree error: " + query.lastError().text());
        return result;
    }

    while (query.next()) {
        PhotoRecord photo;
        photo.id = query.value("id").toInt();
        photo.file = query.value("file").toString();
        photo.path = query.value("path").toString();
        photo.mediaName = query.value("media_name").toString();
        photo.fullPath = QDir::cleanPath(photo.path + "/" + photo.file);
        QFileInfo info(photo.file);
        photo.extension = info.suffix().toLower();
        result.append(photo);
    }
    emit status(QString("Photo tree loaded: %1 files").arg(result.size()));
    return result;
}
void DatabaseWorker::loadPhotoTreeAsync()
{
    auto photos = loadPhotoTree();
    emit photoTreeLoaded(photos);
}
*/

void DatabaseWorker::loadPhoto(int id)
{
    PhotoRecord photo;
    if (!db.isOpen())
    {
        emit status("Database is not open");
        return;
    }
    QSqlQuery query(db);
    query.prepare(R"(SELECT id,file,path,media_name,name,comment,maker,device,filesize,width,height,ext,md5sum,rotation,latitude,longitude,date_creation,lastmodified FROM photobase.photo_images WHERE id = :id)");
    query.bindValue(":id",id);
    if (!query.exec()){
        emit status("loadPhoto error: " + query.lastError().text());
        return;
    }
    if (!query.next()){
        emit status(QString("Photo id=%1 not found").arg(id));
        return;
    }
    photo.id = query.value("id").toInt();
    photo.file = query.value("file").toString();
    photo.path = query.value("path").toString();
    photo.mediaName = query.value("media_name").toString();
    photo.fullPath = QDir::cleanPath(photo.path + "/" + photo.file);
//    photo.name = query.value("name").toString();

//    photo.comment = query.value("comment").toString();
    photo.maker = query.value("maker").toString();
    photo.device = query.value("device").toString();
    photo.fileSize = query.value("filesize").toLongLong();
    photo.width = query.value("width").toInt();
    photo.height = query.value("height").toInt();
    photo.extension = query.value("ext").toString();
    photo.md5 = query.value("md5sum").toString();
    photo.rotation = query.value("rotation").toInt();
    photo.latitude = query.value("latitude").toDouble();
    photo.longitude = query.value("longitude").toDouble();
    photo.dateCreation = query.value("date_creation").toDateTime();
    photo.lastModified = query.value("lastmodified").toDateTime();

    QSqlQuery thumbQuery(db);
    thumbQuery.prepare(R"(SELECT image_data	,width,height FROM photobase.photo_thumbnails WHERE photo_id = :id LIMIT 1)");
    thumbQuery.bindValue(":id",id);
    if (thumbQuery.exec() && thumbQuery.next()){
        photo.thumbnail = thumbQuery.value("image_data").toByteArray();
        photo.thumbwidth = thumbQuery.value("width").toInt();
        photo.thumbheight = thumbQuery.value("height").toInt();
    }
    emit photoLoaded(photo);
}