#include "DatabaseWorker.h"

#include <QSqlQuery>
#include <QSqlField>
#include <QRegularExpression>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QImage>

#include "DatabaseWorker.h"
#include "settingsmanager.h"
#include "PhotoFilter.h"

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
        db.close();
    }
    else{
        db =QSqlDatabase::addDatabase("QPSQL",connectionName);
    }
    db.setHostName(rpSettings->dbHost());
    db.setPort(rpSettings->dbPort());
    db.setDatabaseName(rpSettings->dbName());
    db.setUserName(rpSettings->dbUser());
    db.setPassword(rpSettings->dbPassword());
    if (db.open()) {
            emit status("PostgreSQL connected");
            emit connected(true);
    }
    else{
            emit error(db.lastError().text());
            qDebug()<<"Ошибка подключения :"<<db.lastError().text();
            emit connected(false);
    }
}


void DatabaseWorker::close()
{
    if(db.isOpen())
        db.close();

    QSqlDatabase::removeDatabase("PhotoDB");
}


void DatabaseWorker::loadMedia()
{
    QSqlQuery query(db);
    QStringList media;
    if(query.exec(R"(SELECT DISTINCT media_name FROM photobase.photo_images ORDER BY media_name)")){
        while(query.next()){
            media.append(query.value(0).toString());
        }
        emit mediaLoaded(media);
    }
    else{
        qDebug()<<"Ошибка запроса :"<<query.lastError().text();
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
    else { 
        qDebug()<<"Ошибка  :"<<query.lastError().text();
        emit error(query.lastError().text());
    }
}

void DatabaseWorker::loadPhotos(const QString &mediaName,const QString &path)
{
    QList<PhotoRecord> photos;
    QSqlQuery query(db);
    query.prepare(R"(SELECT id, file, path, media_name, width, height, ext, date_creation,country_id FROM photobase.photo_images WHERE media_name=:media AND path=:path ORDER BY file)");
    query.bindValue(":media",mediaName);
    query.bindValue(":path",path);
    if(query.exec()){
        while(query.next()){
            PhotoRecord photo;
            photo.id =query.value("id").toInt();
            photo.file = query.value("file").toString();
            photo.path = query.value("path").toString();
            photo.mediaName = query.value("media_name").toString();
            photo.width = query.value("width").toInt();
            photo.height = query.value("height").toInt();
            photo.extension = query.value("ext").toString();
            photo.dateCreation = query.value("date_creation").toDateTime();
            photo.countryId =query.value("country_id").toInt();
            photos.append(photo);
        }
        if (!photos.size()){
            PhotoRecord photo;
            photo.id =-1;
            photo.path=path;
            photo.mediaName=mediaName;
            photos.append(photo);
        }
        emit photosLoaded(photos);
    }
    else
    {
        qDebug()<<"Ошибка  :"<<query.lastError().text();
        emit error(query.lastError().text());
    }
}



void DatabaseWorker::loadCache(const QString &media,const QString &relativePath,FileCache* m_cache)
{
    if(!m_cache) return;
    m_cache->clear();

    QSqlQuery query(db);
    query.prepare(R"(SELECT id,path,file,filesize,date_available FROM photobase.photo_images WHERE media_name=:media_label and path like :path)");
    query.bindValue(":media_label",media);
    query.bindValue(":path",relativePath + "%");
    if(!query.exec())
    {
        qDebug()<<"Ошибка  :"<<query.lastError().text();
        emit status(query.lastError().text());
        return; 
    }

    while(query.next())
    {
        FileKey key;

        int id=query.value("id").toInt();
        key.path=query.value("path").toString();
        key.file=query.value("file").toString();
        key.size=query.value("filesize").toLongLong();
        key.modified=query.value("date_available").toDateTime();

        m_cache->insert(key,id);
    }
    emit status(QString("Cache loaded: %1 files").arg(m_cache->size()));
}
//bool DatabaseWorker::exists(
//        const FileKey &key)
//{
 //   return m_cache->contains(key);
//}
int DatabaseWorker::insertPhoto(PhotoRecord &photo)
{
    QSqlQuery query(db);

    query.prepare(R"(SELECT count(*) FROM photobase.photo_images WHERE media_name=:media_label AND path=:path AND file=:file)");
    query.bindValue(":file",photo.file);
    query.bindValue(":path",photo.path);
    query.bindValue(":media_label",photo.mediaName);

    if(!query.exec()) 
    {
        qDebug()<<"Ошибка  :"<<query.lastError().text();
        emit status(query.lastError().text());
        return -1;
    }
    query.next();
    if (query.value(0).toInt()){
        query.finish();
        query.prepare(R"(SELECT id FROM photobase.photo_images WHERE media_name=:media_label AND path=:path AND file=:file)");
        query.bindValue(":file",photo.file);
        query.bindValue(":path",photo.path);
        query.bindValue(":media_label",photo.mediaName);
        if(!query.exec()){
            qDebug()<<"Ошибка  :"<<query.lastError().text();
            emit status(query.lastError().text());
            return -1;
        }
        query.next();
        photo.id=query.value(0).toInt();
        return -2;
    }
    else{    
        query.finish();
        query.prepare(R"(INSERT INTO photobase.photo_images(file,path,ext,filesize,date_available,media_name,name) VALUES (:file,:path,:ext,:size,:modified,:media,:name) RETURNING id )");
        query.bindValue(":file",photo.file);
        query.bindValue(":path",photo.path);
        query.bindValue(":ext",photo.extension.left(20));
        query.bindValue(":size",photo.fileSize);
        query.bindValue(":modified",photo.dateAvailable);
        query.bindValue(":media",photo.mediaName);
        query.bindValue(":name",QFileInfo(photo.file).completeBaseName());
        if(!query.exec()){
            qDebug()<<"Ошибка  :"<<query.lastError().text();
            emit status(query.lastError().text());
            return -1;
        }
        query.next();
        photo.id=query.value(0).toInt();
        return photo.id;
    }
 //   FileKey key;
//
//    key.path=photo.path;
//    key.file=photo.file;
//    key.size=photo.fileSize;
//    key.modified=photo.dateAvailable;

//    m_cache->insert(key,photo.id);

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

QList<PhotoRecord> DatabaseWorker::getPhotosWithoutThumbnail(const QString &media, const QString &mountPoint, const QString &rootFolder)
{
    QList<PhotoRecord> list;

    // Переводим rootFolder в путь относительно точки монтирования 
    QDir mountDir(mountPoint);
    QString relative = mountDir.relativeFilePath(QDir::fromNativeSeparators(rootFolder));
    QString pathPrefix = relative.isEmpty() || relative == "." ? "/" : "/" + relative;

    QSqlQuery query(db);
    query.prepare(R"(SELECT id, path, file, ext FROM photobase.photo_images WHERE media_name = :media_name AND path LIKE :path AND NOT EXISTS (
                    SELECT 1 FROM photobase.photo_thumbnails WHERE photo_id = photo_images.id
                    ) ORDER BY path, file)");

    query.bindValue(":media_name", media);
    query.bindValue(":path", pathPrefix + "%");

    if (!query.exec()) {
        qDebug()<<"Ошибка  :"<<query.lastError().text();
        emit error(query.lastError().text());
        return list;
    }

    while (query.next())
    {
        PhotoRecord photo;
        photo.id = query.value(0).toInt();
        photo.path = query.value(1).toString();
        photo.file = query.value(2).toString();
        photo.extension = query.value(3).toString();
        photo.mediaName = media;
        photo.fullPath = QDir::toNativeSeparators(QDir::cleanPath(mountPoint + "/" + photo.path + "/" + photo.file));
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
    query.prepare(R"(SELECT id,file,path,media_name,name,comment,maker,device,filesize,width,height,ext,md5sum,rotation,latitude,longitude,date_creation,date_available FROM photobase.photo_images WHERE id = :id)");
    query.bindValue(":id",id);
    if (!query.exec()){
        qDebug()<<"Ошибка  :"<<query.lastError().text();
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
    photo.dateAvailable = query.value("date_available").toDateTime();

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
void DatabaseWorker::loadMediaMounts() {
    QSqlQuery query(db);
    query.prepare("SELECT media_name, mount_point FROM photobase.media_mounts ORDER BY media_name");
    if (!query.exec()) { 
        qDebug()<<"Ошибка  :"<<query.lastError().text();
        emit error(query.lastError().text());
        return;
    }
    QVariantList result;
    while (query.next()) {
        QVariantMap row;
        row["media"] = query.value(0).toString();
        row["mountPoint"] = query.value(1).toString();
        result.append(row);
    }
    emit mediaMountsLoaded(result);
}
void DatabaseWorker::saveMountPoint(const QString &media, const QString &mountPoint) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO photobase.media_mounts (media_name, mount_point) VALUES (:media, :mount) ON CONFLICT (media_name) DO UPDATE SET mount_point = :mount");
    query.bindValue(":media", media);
    query.bindValue(":mount", mountPoint);
    if (!query.exec()) {
        qDebug()<<"Ошибка  :"<<query.lastError().text();
        emit error(query.lastError().text());
        return;
    }
    emit status(QString("Точка монтирования для %1 сохранена").arg(media));
}
void DatabaseWorker::markMissing(int id)
{
    QSqlQuery query(db);
    // WHERE missing_since IS NULL — не перезатираем дату первого обнаружения при повторных проверках
    query.prepare("UPDATE photobase.photo_images SET missing_since = now() WHERE id = :id AND missing_since IS NULL");
    query.bindValue(":id", id);
    if (!query.exec()){
        qDebug()<<"Ошибка  :"<<query.lastError().text();
        emit error(query.lastError().text());
    }
}

void DatabaseWorker::clearMissing(int id)
{
    QSqlQuery query(db);
    query.prepare("UPDATE photobase.photo_images SET missing_since = NULL WHERE id = :id AND missing_since IS NOT NULL");
    query.bindValue(":id", id);
    if (!query.exec()){
        qDebug()<<"Ошибка  :"<<query.lastError().text();
        emit error(query.lastError().text());
    }
}

QList<PhotoRecord> DatabaseWorker::loadPathEntries(const QString &media, const QString &relativePath)
{
    QList<PhotoRecord> list;

    QSqlQuery query(db);
    query.prepare(R"(SELECT id, path, file FROM photobase.photo_images WHERE media_name = :media AND path LIKE :path)");
    query.bindValue(":media", media);
    query.bindValue(":path", relativePath + "%");

    if (!query.exec()) {
        qDebug()<<"Ошибка  :"<<query.lastError().text();
        emit error(query.lastError().text());
        return list;
    }
    while (query.next()) {
        PhotoRecord photo;
        photo.id = query.value(0).toInt();
        photo.path = query.value(1).toString();
        photo.file = query.value(2).toString();
        list.append(photo);
    }
    return list;
}
QSet<int> DatabaseWorker::loadMissingIds(const QString &media, const QString &relativePath)
{
    QSet<int> ids;
    QSqlQuery query(db);
    query.prepare(R"(SELECT id FROM photobase.photo_images WHERE media_name = :media AND path LIKE :path AND missing_since IS NOT NULL)");
    query.bindValue(":media", media);
    query.bindValue(":path", relativePath + "%");

    if (!query.exec()) {
        qDebug()<<"Ошибка  :"<<query.lastError().text();
        emit error(query.lastError().text());
        return ids;
    }
    while (query.next())
        ids.insert(query.value(0).toInt());
    return ids;
}

bool DatabaseWorker::insertRegions(int photoId, const QList<PhotoRegion> &regions)
{
    if (regions.isEmpty())
        return true;

    if (!db.transaction()) {
        emit error(db.lastError().text());
        return false;
    }

    QSqlQuery del(db);
    del.prepare("DELETE FROM photobase.photo_regions WHERE photo_id = :id");
    del.bindValue(":id", photoId);
    if (!del.exec()) {
        qDebug()<<"Ошибка  :"<<del.lastError().text();
        db.rollback();
        emit error(del.lastError().text());
        return false;
    }

    for (const PhotoRegion &r : regions) {
        // descriptor вставляем как литерал прямо в текст запроса — параметризация
        // ломается на неизвестном Qt-драйверу типе vector (см. пояснение выше),
        // а тут инъекции взяться неоткуда, значения строим сами из float
        QString descriptorSql = "NULL";
        if (!r.descriptor.isEmpty()) {
            QStringList parts;
            parts.reserve(r.descriptor.size());
            for (float v : r.descriptor)
                parts << QString::number(v, 'g', 9);
            descriptorSql = "CAST('[" + parts.join(',') + "]' AS photobase.vector)";
        }

        QSqlQuery query(db);
        if (!query.prepare(QStringLiteral(R"(
            INSERT INTO photobase.photo_regions
                (photo_id, region_type, source, face_name, face_chip,
                 dly_x, dly_y, dly_w, dly_h, alg_x, alg_y, alg_w, alg_h,
                 applied_to_w, applied_to_h,
                 descriptor, descriptor_bytes, descriptor_model, descriptor_computed_at)
            VALUES
                (:photo_id, :region_type, :source, :face_name, :face_chip,
                 :dly_x, :dly_y, :dly_w, :dly_h, :alg_x, :alg_y, :alg_w, :alg_h,
                 :applied_w, :applied_h,
                 %1, :descriptor_bytes, :descriptor_model, :descriptor_at)
        )").arg(descriptorSql))){
            qDebug() << "Ошибка PREPARE:" << query.lastError().text();
            db.rollback();
            emit error(query.lastError().text());
            return false;
        }
        query.bindValue(":photo_id", photoId);
        query.bindValue(":region_type", r.type);
        query.bindValue(":source", r.source.isEmpty() ? QStringLiteral("acdsee") : r.source);
        query.bindValue(":face_name", r.name.isEmpty() ? QVariant(QVariant::String) : QVariant(r.name));
        query.bindValue(":face_chip", r.faceChip.isEmpty() ? QVariant(QVariant::ByteArray) : QVariant(r.faceChip));
        query.bindValue(":dly_x", r.dlyX);
        query.bindValue(":dly_y", r.dlyY);
        query.bindValue(":dly_w", r.dlyW);
        query.bindValue(":dly_h", r.dlyH);
        query.bindValue(":alg_x", r.hasAlg ? QVariant(r.algX) : QVariant());
        query.bindValue(":alg_y", r.hasAlg ? QVariant(r.algY) : QVariant());
        query.bindValue(":alg_w", r.hasAlg ? QVariant(r.algW) : QVariant());
        query.bindValue(":alg_h", r.hasAlg ? QVariant(r.algH) : QVariant());
        query.bindValue(":applied_w", r.appliedToWidth);
        query.bindValue(":applied_h", r.appliedToHeight);

        if (r.descriptor.isEmpty()) {
            query.bindValue(":descriptor_bytes", QVariant(QVariant::ByteArray));
            query.bindValue(":descriptor_model", QVariant(QVariant::String));
            query.bindValue(":descriptor_at", QVariant(QVariant::DateTime));
        } else {
            QByteArray descBytes(reinterpret_cast<const char *>(r.descriptor.constData()),
                                  int(r.descriptor.size() * sizeof(float)));
            query.bindValue(":descriptor_bytes", descBytes);
            query.bindValue(":descriptor_model", r.descriptorModel);
            query.bindValue(":descriptor_at", QDateTime::currentDateTimeUtc());
        }

        if (!query.exec()) {
            qDebug()<<"Ошибка  :"<<query.lastError().text();
            db.rollback();
            emit error(query.lastError().text());
            return false;
        }
    }

    if (!db.commit()) {
        emit error(db.lastError().text());
        return false;
    }
    return true;
}
bool DatabaseWorker::setReferenceFace(int personId, int regionId)
{
    QSqlQuery query(db);
    if (!query.prepare(R"(
        UPDATE photobase.people
        SET reference_chip = src.face_chip,
            reference_descriptor = src.descriptor,
            reference_descriptor_model = src.descriptor_model,
            reference_source_region_id = src.id
        FROM photobase.photo_regions src
        WHERE photobase.people.id = :person_id
          AND src.id = :region_id
    )")) {
        qDebug() << "Ошибка PREPARE (setReferenceFace):" << query.lastError().text();
        emit error(query.lastError().text());
        return false;
    }

    query.bindValue(":person_id", personId);
    query.bindValue(":region_id", regionId);

    if (!query.exec()) {
        qDebug() << "Ошибка EXEC (setReferenceFace):" << query.lastError().text();
        emit error(query.lastError().text());
        return false;
    }
    return true;
}
QVector<QPair<int, double>> DatabaseWorker::findSimilarFaces(int regionId, double threshold, int limit)
{
    QVector<QPair<int, double>> result;

    QSqlQuery query(db);
    if (!query.prepare(R"(
        SELECT pr.id, pr.descriptor <-> target.descriptor AS distance
        FROM photobase.photo_regions pr
        CROSS JOIN (
            SELECT descriptor FROM photobase.photo_regions WHERE id = :target_id
        ) AS target
        WHERE pr.id <> :target_id
          AND pr.descriptor IS NOT NULL
          AND pr.descriptor <-> target.descriptor < :threshold
        ORDER BY distance
        LIMIT :limit
    )")) {
        qDebug() << "Ошибка PREPARE (similar):" << query.lastError().text();
        emit error(query.lastError().text());
        return result;
    }

    query.bindValue(":target_id", regionId);
    query.bindValue(":threshold", threshold);
    query.bindValue(":limit", limit);

    if (!query.exec()) {
        qDebug() << "Ошибка EXEC (similar):" << query.lastError().text();
        emit error(query.lastError().text());
        return result;
    }

    while (query.next())
        result.append({query.value(0).toInt(), query.value(1).toDouble()});

    return result;
}
void DatabaseWorker::loadPersons()
{
    QList<PersonRecord> result;

    QSqlQuery query(db);
    if (!query.exec(R"(
        SELECT id, display_name, reference_chip IS NOT NULL AS has_reference
        FROM photobase.people
        ORDER BY display_name
    )")) {
        qDebug() << "Ошибка EXEC (loadPersons):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }

    while (query.next()) {
        PersonRecord person;
        person.id = query.value(0).toInt();
        person.displayName = query.value(1).toString();
        person.hasReference = query.value(2).toBool();
        result.append(person);
    }

    emit personsLoaded(result);
}

void DatabaseWorker::loadUnresolvedRegions()
{
    QList<FaceRegionRecord> result;

    QSqlQuery query(db);
    if (!query.exec(R"(
        SELECT id, photo_id, face_name
        FROM photobase.photo_regions
        WHERE person_id IS NULL
        ORDER BY photo_id, id
    )")) {
        qDebug() << "Ошибка EXEC (loadUnresolvedRegions):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }

    while (query.next()) {
        FaceRegionRecord region;
        region.id = query.value(0).toInt();
        region.photoId = query.value(1).toInt();
        region.faceName = query.value(2).toString();
        result.append(region);
    }

    emit unresolvedRegionsLoaded(result);
}

void DatabaseWorker::loadRegionsForPerson(int personId)
{
    QList<FaceRegionRecord> result;

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id, photo_id, face_name
        FROM photobase.photo_regions
        WHERE person_id = :person_id
        ORDER BY photo_id, id
    )");
    query.bindValue(":person_id", personId);

    if (!query.exec()) {
        qDebug() << "Ошибка EXEC (loadRegionsForPerson):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }

    while (query.next()) {
        FaceRegionRecord region;
        region.id = query.value(0).toInt();
        region.photoId = query.value(1).toInt();
        region.faceName = query.value(2).toString();
        result.append(region);
    }

    emit personRegionsLoaded(personId, result);
}

void DatabaseWorker::createPerson(const QString &displayName)
{
    QSqlQuery query(db);
    query.prepare(R"(
        INSERT INTO photobase.people (display_name)
        VALUES (:display_name)
        RETURNING id
    )");
    query.bindValue(":display_name", displayName);

    if (!query.exec() || !query.next()) {
        qDebug() << "Ошибка EXEC (createPerson):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }

    PersonRecord person;
    person.id = query.value(0).toInt();
    person.displayName = displayName;
    person.hasReference = false;

    emit personCreated(person);
}

void DatabaseWorker::assignRegionToPerson(int regionId, int personId)
{
    QSqlQuery query(db);
    query.prepare(R"(
        UPDATE photobase.photo_regions
        SET person_id = :person_id
        WHERE id = :region_id
    )");
    query.bindValue(":person_id", personId);
    query.bindValue(":region_id", regionId);

    if (!query.exec()) {
        qDebug() << "Ошибка EXEC (assignRegionToPerson):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }

    emit regionAssigned(regionId, personId);
}

void DatabaseWorker::setPersonReference(int personId, int regionId)
{
    QSqlQuery query(db);
    if (!query.prepare(R"(
        UPDATE photobase.people
        SET reference_chip = src.face_chip,
            reference_descriptor = src.descriptor,
            reference_descriptor_model = src.descriptor_model,
            reference_source_region_id = src.id
        FROM photobase.photo_regions src
        WHERE photobase.people.id = :person_id
          AND src.id = :region_id
    )")) {
        qDebug() << "Ошибка PREPARE (setPersonReference):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }
    query.bindValue(":person_id", personId);
    query.bindValue(":region_id", regionId);

    if (!query.exec()) {
        qDebug() << "Ошибка EXEC (setPersonReference):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }

    emit personReferenceSet(personId, regionId);
}

void DatabaseWorker::unassignRegion(int regionId)
{
    QSqlQuery query(db);
    query.prepare(R"(
        UPDATE photobase.photo_regions
        SET person_id = NULL
        WHERE id = :region_id
    )");
    query.bindValue(":region_id", regionId);

    if (!query.exec()) {
        qDebug() << "Ошибка EXEC (unassignRegion):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }

    emit regionUnassigned(regionId);
}
QImage DatabaseWorker::loadChipImage(const QString &id, QSize *size, const QSize & /*requestedSize*/)
{
    // id приходит без "image://facechip/" — например "region/42" или "person/7"
    const QStringList parts = id.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (parts.size() != 2) {
        qDebug() << "FaceChipImageProvider: неожиданный id" << id;
        return QImage();
    }

    const QString kind = parts.at(0);
    bool ok = false;
    const int recordId = parts.at(1).toInt(&ok);
    if (!ok) {
        qDebug() << "FaceChipImageProvider: не число в id" << id;
        return QImage();
    }

    QString sql;
    if (kind == QLatin1String("region"))
        sql = QStringLiteral("SELECT face_chip FROM photobase.photo_regions WHERE id = :id");
    else if (kind == QLatin1String("person"))
        sql = QStringLiteral("SELECT reference_chip FROM photobase.people WHERE id = :id");
    else {
        qDebug() << "FaceChipImageProvider: неизвестный тип" << kind;
        return QImage();
    }

    QSqlQuery query(db);
    query.prepare(sql);
    query.bindValue(":id", recordId);

    if (!query.exec() || !query.next()) {
        qDebug() << "FaceChipImageProvider: не найдено" << id << query.lastError().text();
        return QImage();
    }

    const QByteArray jpegBytes = query.value(0).toByteArray();
    if (jpegBytes.isEmpty())
        return QImage(); // чип ещё не посчитан/не выбран — нормальный случай, не ошибка

    QImage image = QImage::fromData(jpegBytes, "JPG");
    if (size) *size = image.size();
    return image;
}

void DatabaseWorker::searchPhotos(const PhotoFilter &filter)
{
    QStringList mediaPlaceholders;
    for (int i = 0; i < filter.media.size(); ++i)
        mediaPlaceholders << QStringLiteral(":media%1").arg(i);
 
    QStringList personPlaceholders;
    for (int i = 0; i < filter.personIds.size(); ++i)
        personPlaceholders << QStringLiteral(":person%1").arg(i);
 
    const bool needsScores = filter.facesEnabled && !filter.personIds.isEmpty();
 
    // photo_scores(photo_id, match_count) — считается в зависимости от режима
    QString scoresCte;
    if (needsScores) {
        if (filter.facesUseDescriptor) {
            // берём reference_descriptor, ищем минимальную дистанцию среди ВСЕХ регионовнаходим ещё не сопоставленные.
            scoresCte = QStringLiteral(R"(
                WITH selected_people AS (SELECT id, reference_descriptor FROM photobase.people WHERE id IN (%1) AND reference_descriptor IS NOT NULL),
                matches AS (
                    SELECT pr.photo_id, sp.id AS person_id,MIN(pr.descriptor::photobase.vector <-> sp.reference_descriptor::photobase.vector) AS best_distance FROM photobase.photo_regions pr
                    CROSS JOIN selected_people sp WHERE pr.descriptor IS NOT NULL GROUP BY pr.photo_id, sp.id),
                photo_scores AS (SELECT photo_id, COUNT(*) AS match_count FROM matches WHERE best_distance < :threshold GROUP BY photo_id)
            )").arg(personPlaceholders.join(QLatin1Char(',')));
        } else {
            scoresCte = QStringLiteral(R"( WITH photo_scores AS (
                SELECT photo_id, COUNT(DISTINCT person_id) AS match_count FROM photobase.photo_regions WHERE person_id IN (%1) GROUP BY photo_id)
            )").arg(personPlaceholders.join(QLatin1Char(',')));
        }
    }
 
    QString sql = scoresCte + QStringLiteral(R"(
        SELECT pi.id, pi.file, pi.path, pi.media_name, pi.date_creation, pi.date_available, %1 AS match_count FROM photobase.photo_images pi %2 WHERE pi.missing_since IS NULL
    )").arg(needsScores ? QStringLiteral("ps.match_count") : QStringLiteral("0"),needsScores ? QStringLiteral("JOIN photo_scores ps ON ps.photo_id = pi.id") : QString());
 
    if (filter.mediaEnabled && !filter.media.isEmpty())
        sql += QStringLiteral(" AND pi.media_name IN (%1)").arg(mediaPlaceholders.join(QLatin1Char(',')));
    sql+=(" AND (pi.ext='jpg' OR pi.ext='jpeg') " );    
    if (filter.dateEnabled)
        sql += QStringLiteral(" AND COALESCE(pi.date_creation, pi.date_available) BETWEEN :date_from AND :date_to");
    if (filter.countryEnabled && filter.countryId >= 0) sql += QStringLiteral(" AND pi.country_id = :country_id");
    // haversine: расстояние photo <-> place.lat/lon, сравниваем с радиусом места
    if (filter.placeEnabled && filter.placeId >= 0) {
        sql += QStringLiteral(R"( AND EXISTS (SELECT 1 FROM photobase.places pl WHERE pl.id = :place_id
               AND (pi.latitude != 0 OR pi.longitude != 0) AND POWER((pi.latitude - pl.latitude) * 111.32, 2)
               + POWER((pi.longitude - pl.longitude) * 111.32 * cos(radians(pl.latitude)), 2) <= POWER(pl.radius_km, 2)))");
    }
    sql += (filter.sortBy == PhotoFilter::SortBy::FaceMatchCount)
        ? QStringLiteral(" ORDER BY match_count DESC, COALESCE(pi.date_creation, pi.date_available) DESC")
        : QStringLiteral(" ORDER BY COALESCE(pi.date_creation, pi.date_available) DESC");
     
    if (filter.limitEnabled) sql += QStringLiteral(" LIMIT :max_count");
     QSqlQuery query(db);
    if (!query.prepare(sql)) {
        qDebug() << "Ошибка PREPARE (searchPhotos):" << query.lastError().text();
        qDebug() << "SQL:" << sql;
        emit error(query.lastError().text());
        return;
    }
 
    if (needsScores) {
        for (int i = 0; i < filter.personIds.size(); ++i)
            query.bindValue(personPlaceholders.at(i), filter.personIds.at(i));
        if (filter.facesUseDescriptor)
            query.bindValue(":threshold", filter.similarityThreshold);
    }
 
    if (filter.mediaEnabled)
        for (int i = 0; i < filter.media.size(); ++i)
            query.bindValue(mediaPlaceholders.at(i), filter.media.at(i));
 
    if (filter.dateEnabled) {
        query.bindValue(":date_from", filter.dateFrom);
        query.bindValue(":date_to", filter.dateTo);
    }
    if (filter.countryEnabled && filter.countryId >= 0)
        query.bindValue(":country_id", filter.countryId);
    if (filter.placeEnabled && filter.placeId >= 0)
        query.bindValue(":place_id", filter.placeId);
    if (filter.limitEnabled)
        query.bindValue(":max_count", filter.maxCount);
 
    if (!query.exec()) {
        qDebug() << "Ошибка EXEC (searchPhotos):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }
 
    QList<PhotoRecord> result;
    while (query.next()) {
        PhotoRecord photo;
        photo.id = query.value(0).toInt();
        photo.file = query.value(1).toString();
        photo.path = query.value(2).toString();
        photo.mediaName = query.value(3).toString();
        photo.matchCount = query.value(6).toInt(); 
        result.append(photo);
    }
 
    emit photosFound(result);
}
void DatabaseWorker::loadCountries()
{
    QHash<int, CountryRecord> countries;
    QList<int> order;

    QSqlQuery countryQuery(db);
    if (!countryQuery.exec(R"(
        SELECT id, name FROM photobase.countries ORDER BY name
    )")) {
        qDebug() << "Ошибка EXEC (loadCountries):" << countryQuery.lastError().text();
        emit error(countryQuery.lastError().text());
        return;
    }
    while (countryQuery.next()) {
        CountryRecord c;
        c.id = countryQuery.value(0).toInt();
        c.name = countryQuery.value(1).toString();
        countries.insert(c.id, c);
        order.append(c.id);
    }

    QSqlQuery bboxQuery(db);
    if (!bboxQuery.exec(R"(
        SELECT id, country_id, lat_min, lat_max, lon_min, lon_max
        FROM photobase.country_bboxes
    )")) {
        qDebug() << "Ошибка EXEC (loadCountries/bboxes):" << bboxQuery.lastError().text();
        emit error(bboxQuery.lastError().text());
        return;
    }
    while (bboxQuery.next()) {
        int countryId = bboxQuery.value(1).toInt();
        if (!countries.contains(countryId))
            continue;

        CountryBBox b;
        b.id = bboxQuery.value(0).toInt();
        b.latMin = bboxQuery.value(2).toDouble();
        b.latMax = bboxQuery.value(3).toDouble();
        b.lonMin = bboxQuery.value(4).toDouble();
        b.lonMax = bboxQuery.value(5).toDouble();
        countries[countryId].bboxes.append(b);
    }

    QList<CountryRecord> result;
    for (int id : order)
        result.append(countries.value(id));

    emit countriesLoaded(result);
}

void DatabaseWorker::addCountry(const QString &name, const QList<CountryBBox> &bboxes)
{
    QSqlQuery query(db);
    query.prepare(R"(
        INSERT INTO photobase.countries (name) VALUES (:name) RETURNING id
    )");
    query.bindValue(":name", name);

    if (!query.exec() || !query.next()) {
        qDebug() << "Ошибка EXEC (addCountry):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }

    CountryRecord result;
    result.id = query.value(0).toInt();
    result.name = name;

    for (const auto &b : bboxes) {
        QSqlQuery bboxInsert(db);
        bboxInsert.prepare(R"(
            INSERT INTO photobase.country_bboxes (country_id, lat_min, lat_max, lon_min, lon_max)
            VALUES (:cid, :latMin, :latMax, :lonMin, :lonMax)
            RETURNING id
        )");
        bboxInsert.bindValue(":cid", result.id);
        bboxInsert.bindValue(":latMin", b.latMin);
        bboxInsert.bindValue(":latMax", b.latMax);
        bboxInsert.bindValue(":lonMin", b.lonMin);
        bboxInsert.bindValue(":lonMax", b.lonMax);

        if (!bboxInsert.exec() || !bboxInsert.next()) {
            qDebug() << "Ошибка EXEC (addCountry/bbox):" << bboxInsert.lastError().text();
            continue;
        }
        CountryBBox saved = b;
        saved.id = bboxInsert.value(0).toInt();
        result.bboxes.append(saved);
    }

    emit countryAdded(result);
}

void DatabaseWorker::updateCountryBBoxes(int countryId, const QList<CountryBBox> &bboxes)
{
    QSqlQuery del(db);
    del.prepare(R"(DELETE FROM photobase.country_bboxes WHERE country_id = :cid)");
    del.bindValue(":cid", countryId);
    if (!del.exec()) {
        qDebug() << "Ошибка EXEC (updateCountryBBoxes/delete):" << del.lastError().text();
        emit error(del.lastError().text());
        return;
    }

    for (const auto &b : bboxes) {
        QSqlQuery ins(db);
        ins.prepare(R"(
            INSERT INTO photobase.country_bboxes (country_id, lat_min, lat_max, lon_min, lon_max)
            VALUES (:cid, :latMin, :latMax, :lonMin, :lonMax)
        )");
        ins.bindValue(":cid", countryId);
        ins.bindValue(":latMin", b.latMin);
        ins.bindValue(":latMax", b.latMax);
        ins.bindValue(":lonMin", b.lonMin);
        ins.bindValue(":lonMax", b.lonMax);
        if (!ins.exec())
            qDebug() << "Ошибка EXEC (updateCountryBBoxes/insert):" << ins.lastError().text();
    }

    loadCountries(); // проще перечитать и переэмитить целиком
}

void DatabaseWorker::deleteCountry(int countryId)
{
    QSqlQuery query(db);
    query.prepare(R"(DELETE FROM photobase.countries WHERE id = :id)");
    query.bindValue(":id", countryId);
    if (!query.exec()) {
        qDebug() << "Ошибка EXEC (deleteCountry):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }
    emit countryDeleted(countryId);
}

void DatabaseWorker::loadPlaces()
{
    QList<PlaceRecord> result;

    QSqlQuery query(db);
    if (!query.exec(R"(
        SELECT id, name, latitude, longitude, radius_km, country_id
        FROM photobase.places
        ORDER BY name
    )")) {
        qDebug() << "Ошибка EXEC (loadPlaces):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }

    while (query.next()) {
        PlaceRecord p;
        p.id = query.value(0).toInt();
        p.name = query.value(1).toString();
        p.latitude = query.value(2).toDouble();
        p.longitude = query.value(3).toDouble();
        p.radiusKm = query.value(4).toDouble();
        p.countryId = query.value(5).isNull() ? -1 : query.value(5).toInt();
        result.append(p);
    }

    emit placesLoaded(result);
}

void DatabaseWorker::addPlace(const QString &name, double lat, double lon,
                               double radiusKm, int countryId)
{
    QSqlQuery query(db);
    query.prepare(R"(
        INSERT INTO photobase.places (name, latitude, longitude, radius_km, country_id)
        VALUES (:name, :lat, :lon, :radius, :countryId)
        RETURNING id
    )");
    query.bindValue(":name", name);
    query.bindValue(":lat", lat);
    query.bindValue(":lon", lon);
    query.bindValue(":radius", radiusKm);
    query.bindValue(":countryId", countryId >= 0 ? QVariant(countryId)
                                                   : QVariant(QMetaType(QMetaType::Int)));

    if (!query.exec() || !query.next()) {
        qDebug() << "Ошибка EXEC (addPlace):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }

    PlaceRecord result;
    result.id = query.value(0).toInt();
    result.name = name;
    result.latitude = lat;
    result.longitude = lon;
    result.radiusKm = radiusKm;
    result.countryId = countryId;

    emit placeAdded(result);
}

void DatabaseWorker::updatePlace(int placeId, const QString &name, double radiusKm, int countryId)
{
    QSqlQuery query(db);
    query.prepare(R"(
        UPDATE photobase.places
        SET name = :name, radius_km = :radius, country_id = :countryId
        WHERE id = :id
        RETURNING latitude, longitude
    )");
    query.bindValue(":name", name);
    query.bindValue(":radius", radiusKm);
    query.bindValue(":countryId", countryId >= 0 ? QVariant(countryId) : QVariant(QMetaType(QMetaType::Int)));
    query.bindValue(":id", placeId);

    if (!query.exec() || !query.next()) {
        qDebug() << "Ошибка EXEC (updatePlace):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }

    PlaceRecord result;
    result.id = placeId;
    result.name = name;
    result.radiusKm = radiusKm;
    result.countryId = countryId;
    result.latitude = query.value(0).toDouble();
    result.longitude = query.value(1).toDouble();

    emit placeUpdated(result);
}

void DatabaseWorker::deletePlace(int placeId)
{
    QSqlQuery query(db);
    query.prepare(R"(DELETE FROM photobase.places WHERE id = :id)");
    query.bindValue(":id", placeId);
    if (!query.exec()) {
        qDebug() << "Ошибка EXEC (deletePlace):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }
    emit placeDeleted(placeId);
}

void DatabaseWorker::assignCountriesByCoordinates()
{
    QSqlQuery query(db);
    query.prepare(R"(
        UPDATE photobase.photo_images
        SET country_id = (
            SELECT cb.country_id FROM photobase.country_bboxes cb
            WHERE photos.latitude  BETWEEN cb.lat_min AND cb.lat_max
              AND photos.longitude BETWEEN cb.lon_min AND cb.lon_max
            LIMIT 1
        )
        WHERE country_id IS NULL
          AND (latitude != 0 OR longitude != 0)
    )");

    if (!query.exec()) {
        qDebug() << "Ошибка EXEC (assignCountriesByCoordinates):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }

    emit countriesAssigned(query.numRowsAffected());
}

void DatabaseWorker::assignCountryToFolder(const QString &mediaName,const QString &folderPathPrefix,int countryId)
{
    QSqlQuery query(db);
    query.prepare(R"(
        UPDATE photobase.photo_images SET country_id = :countryId WHERE media_name = :media
          AND (path = :prefix OR path LIKE :prefixLike))");
    query.bindValue(":countryId", countryId);
    query.bindValue(":media", mediaName);
    query.bindValue(":prefix", folderPathPrefix);
    query.bindValue(":prefixLike", folderPathPrefix + "/%");

    if (!query.exec()) {
        qDebug() << "Ошибка EXEC (assignCountryToFolder):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }
    emit countriesAssigned(query.numRowsAffected());
}
void DatabaseWorker::updatePhotoCountry(int photoId, int countryId)
{
    QSqlQuery query(db);
    query.prepare(R"(UPDATE photobase.photo_images SET country_id = :countryId WHERE id = :id)");
    query.bindValue(":countryId", countryId >= 0 ? QVariant(countryId) : QVariant(QMetaType(QMetaType::Int)));
    query.bindValue(":id", photoId);
    if (!query.exec()) {
        qDebug() << "Ошибка EXEC (updatePhotoCountry):" << query.lastError().text();
        emit error(query.lastError().text());
        return;
    }
    emit photoCountryUpdated(photoId, countryId);
}