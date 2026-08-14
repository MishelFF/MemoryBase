#include "ApiWorker.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QImage>
#include <QTimer>
#include <QEventLoop>
#include <QString>


ApiWorker::ApiWorker(QObject *parent) : PhotoRepository(parent) { 
    manager = new QNetworkAccessManager(this);
    qDebug() << "ApiWorker created";
}

void ApiWorker::open(SettingsManager *rpSettings) { 
    QString url=rpSettings->apiUrl().trimmed();
    QUrl u(url, QUrl::StrictMode);
    if (u.isValid()) serverUrl=url;
    emit connected(true); 
}

void ApiWorker::loadMedia() {// Загрузка списка носителей
    QUrl url(serverUrl + "media.php");
    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            reply->deleteLater();
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QStringList result;
        for (auto value : doc.array()) {
            result.append(value.toString());
        }
        emit mediaLoaded(result);
        reply->deleteLater();
    });
}

void ApiWorker::loadFolders(const QString &mediaName) {// Загрузка папок
    QUrlQuery query;
    query.addQueryItem("media", mediaName);
    QUrl url(serverUrl + "folders.php");
    url.setQuery(query);
    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, mediaName]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            reply->deleteLater();
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QStringList folders;
        for (auto value : doc.array()) {
            folders.append(value.toString());
        }
        emit foldersLoaded(mediaName, folders);
        reply->deleteLater();
    });
}
//-----------------------------------------------------
// Загрузка фотографий папки
//-----------------------------------------------------
void ApiWorker::loadPhotos(const QString &mediaName, const QString &path) {
    QUrlQuery query;
    query.addQueryItem("media", mediaName);
    query.addQueryItem("path", path);
    QUrl url(serverUrl + "photos.php");
    url.setQuery(query);
    qDebug()<<"Запрос  :"<<url.toString(QUrl::FullyDecoded);
    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this,mediaName,path, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            reply->deleteLater();
            return;
        }
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QList<PhotoRecord> photos;
        for (auto item : doc.array()) {
            QJsonObject obj = item.toObject();
            PhotoRecord photo;
            photo.id = obj["id"].toInt();
            photo.file = obj["file"].toString();
            photo.path = obj["path"].toString();
            photo.mediaName = obj["media_name"].toString();
            photo.width = obj["width"].toInt();
            photo.height = obj["height"].toInt();
            photo.extension = obj["ext"].toString();
            photo.fileSize = obj["filesize"].toVariant().toLongLong();
            photo.maker = obj["maker"].toString();
            photo.device = obj["device"].toString();
            photo.md5 = obj["md5sum"].toString();
            photo.rotation = obj["rotation"].toInt();
            photo.latitude = obj["latitude"].toDouble();
            photo.longitude = obj["longitude"].toDouble();
            photo.dateCreation = QDateTime::fromString(obj["date_creation"].toString(), Qt::ISODate);
            photo.dateAvailable = QDateTime::fromString(obj["date_available"].toString(), Qt::ISODate);
            photo.comment = obj["comment"].toString();
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
        reply->deleteLater();
    });
}
//-----------------------------------------------------
// Загрузка одной фотографии
//-----------------------------------------------------
void ApiWorker::loadPhoto(int id)
{
    auto photo = QSharedPointer<PhotoRecord>::create();
    auto pending = QSharedPointer<int>::create(2);   

    auto tryEmit = [this, photo, pending]() {if (--(*pending) == 0) emit photoLoaded(*photo);};//Оба запроса выполнятся в одном потоке, отложенно но последовательно

    QUrlQuery propQuery;
    propQuery.addQueryItem("id", QString::number(id));
    QUrl propUrl(serverUrl + "properties.php");
    propUrl.setQuery(propQuery);
    qDebug()<<"Запрос  :"<<propUrl.toString(QUrl::FullyDecoded);
     QNetworkReply *propReply = manager->get(QNetworkRequest(propUrl));
    connect(propReply, &QNetworkReply::finished, this, [this, propReply, photo, tryEmit]() {
        if (propReply->error() != QNetworkReply::NoError) {
            emit error(propReply->errorString());
        } else {
            QJsonObject obj = QJsonDocument::fromJson(propReply->readAll()).object();
            photo->id = obj["id"].toInt();
            photo->file = obj["file"].toString();
            photo->path = obj["path"].toString();
            photo->mediaName = obj["media_name"].toString();
            photo->extension = obj["ext"].toString();
            photo->fileSize = obj["filesize"].toVariant().toLongLong();
            photo->maker = obj["maker"].toString();
            photo->device = obj["device"].toString();
            photo->md5 = obj["md5sum"].toString();
            photo->width = obj["width"].toInt();
            photo->height = obj["height"].toInt();
            photo->rotation = obj["rotation"].toInt();
            photo->latitude = obj["latitude"].toDouble();
            photo->longitude = obj["longitude"].toDouble();
            photo->dateCreation = QDateTime::fromString(obj["date_creation"].toString(), Qt::ISODate);
            photo->dateAvailable = QDateTime::fromString(obj["date_available"].toString(), Qt::ISODate);
            photo->comment = obj["comment"].toString();
        }
        propReply->deleteLater();
        tryEmit();
    });

    QUrlQuery thumbQuery;
    thumbQuery.addQueryItem("id", QString::number(id));
    QUrl thumbUrl(serverUrl + "thumbnail.php");
    thumbUrl.setQuery(thumbQuery);

    QNetworkReply *thumbReply = manager->get(QNetworkRequest(thumbUrl));
    connect(thumbReply, &QNetworkReply::finished, this, [this, thumbReply, photo, tryEmit]() {
        if (thumbReply->error() != QNetworkReply::NoError) {
            emit error(thumbReply->errorString());
        } else {
            photo->thumbnail = thumbReply->readAll();
        }
        thumbReply->deleteLater();
        tryEmit();
    });
}
void ApiWorker::loadMediaMounts()
{
    emit mediaMountsLoaded(QVariantList());   // пустой список — в режиме API не используется
}

void ApiWorker::saveMountPoint(const QString &media, const QString &mountPoint)
{
    Q_UNUSED(media);
    Q_UNUSED(mountPoint);
}

QImage ApiWorker::loadChipImage(const QString &id, QSize *size, const QSize & /*requestedSize*/) {
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
    QUrlQuery query;
    query.addQueryItem("kind", kind);
    query.addQueryItem("id", QString::number(recordId));

    QUrl url(serverUrl + "faceChip.php");
    url.setQuery(query);

    QNetworkReply *reply = manager->get(QNetworkRequest(url));

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    bool timedOut = false;
    connect(&timeoutTimer, &QTimer::timeout, this, [&loop, &timedOut, reply]() {
        timedOut = true; reply->abort(); loop.quit();
    });
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timeoutTimer.start(1000);
    loop.exec();
    timeoutTimer.stop();
    if (timedOut) {
        qDebug() << "loadChipImageSync: таймаут" << kind << recordId;
        reply->deleteLater();
        return QImage();
    }
    reply->deleteLater();

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError && httpStatus != 204) {
        qDebug() << "loadChipImageSync: ошибка сети" << kind << recordId << reply->errorString();
        return QImage();
    }
    if (httpStatus == 204) {
        return QImage();
    }

    const QByteArray data = reply->readAll();
    QImage image = QImage::fromData(data, "JPG");
    if (image.isNull())
        qDebug() << "loadChipImageSync: не удалось декодировать JPEG" << kind << recordId;
    return image;
}

void ApiWorker::searchPhotos(const PhotoFilter &filter)
{
    QJsonObject body;
    QJsonArray mediaArr;

    for (const QString &m : filter.media) mediaArr.append(m);
    body["media"] = mediaArr;
    body["mediaEnabled"] = filter.mediaEnabled;

    QJsonArray personArr;
    for (int pid : filter.personIds) personArr.append(pid);
    body["personIds"] = personArr;
    body["facesEnabled"] = filter.facesEnabled;
    body["facesUseDescriptor"] = filter.facesUseDescriptor;
    body["similarityThreshold"] = filter.similarityThreshold;

    body["dateEnabled"] = filter.dateEnabled;
    if (filter.dateEnabled) {
        body["dateFrom"] = filter.dateFrom.toString(Qt::ISODate);
        body["dateTo"] = filter.dateTo.toString(Qt::ISODate);
    }

    body["limitEnabled"] = filter.limitEnabled;
    body["maxCount"] = filter.maxCount;

    body["sortBy"] = (filter.sortBy == PhotoFilter::SortBy::FaceMatchCount)
                          ? QStringLiteral("face_match_count")
                          : QStringLiteral("date");

    QJsonDocument doc(body);
    QByteArray payload = doc.toJson(QJsonDocument::Compact);

    QUrl url(serverUrl + "searchPhotos.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    qDebug() << "Запрос searchPhotos:" << url.toString(QUrl::FullyDecoded) << payload;

    QNetworkReply *reply = manager->post(request, payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            reply->deleteLater();
            return;
        }
        QByteArray data = reply->readAll();
        QJsonDocument responseDoc = QJsonDocument::fromJson(data);
        QList<PhotoRecord> photos;

        if (responseDoc.isArray()) {
            for (const auto &item : responseDoc.array()) {
                QJsonObject obj = item.toObject();
                PhotoRecord photo;
                photo.id = obj["id"].toInt();
                photo.file = obj["file"].toString();
                photo.path = obj["path"].toString();
                photo.mediaName = obj["media_name"].toString();
                photo.dateCreation = QDateTime::fromString(obj["date_creation"].toString(), Qt::ISODate);
                photo.dateAvailable = QDateTime::fromString(obj["date_available"].toString(), Qt::ISODate);
                photo.matchCount = obj["match_count"].toInt();
                photos.append(photo);
            }
        } else if (responseDoc.isObject() && responseDoc.object().contains("error")) {
            emit error(responseDoc.object()["error"].toString());
            reply->deleteLater();
            return;
        }
        emit photosFound(photos);
        reply->deleteLater();
    });
}
void ApiWorker::loadPersons()
{
    QUrl url(serverUrl + "persons.php");
    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QList<PersonRecord> result;
        for (const auto &item : doc.array()) {
            QJsonObject obj = item.toObject();
            PersonRecord person;
            person.id = obj["id"].toInt();
            person.displayName = obj["display_name"].toString();
            person.hasReference = obj["has_reference"].toBool();
            result.append(person);
        }
        emit personsLoaded(result);
    });
}

void ApiWorker::loadUnresolvedRegions()
{
    QUrl url(serverUrl + "unresolvedRegions.php");
    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QList<FaceRegionRecord> result;
        for (const auto &item : doc.array()) {
            QJsonObject obj = item.toObject();
            FaceRegionRecord region;
            region.id = obj["id"].toInt();
            region.photoId = obj["photo_id"].toInt();
            region.faceName = obj["face_name"].toString();
            result.append(region);
        }
        emit unresolvedRegionsLoaded(result);
    });
}

void ApiWorker::loadRegionsForPerson(int personId)
{
    QUrlQuery query;
    query.addQueryItem("person_id", QString::number(personId));

    QUrl url(serverUrl + "personRegions.php");
    url.setQuery(query);

    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, personId, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        QList<FaceRegionRecord> result;
        for (const auto &item : root["regions"].toArray()) {
            QJsonObject obj = item.toObject();
            FaceRegionRecord region;
            region.id = obj["id"].toInt();
            region.photoId = obj["photo_id"].toInt();
            region.faceName = obj["face_name"].toString();
            result.append(region);
        }
        emit personRegionsLoaded(personId, result);
    });
}

void ApiWorker::createPerson(const QString &displayName)
{
    QJsonObject body;
    body["display_name"] = displayName;

    QUrl url(serverUrl + "createPerson.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }

        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        if (obj.contains("error")) {
            emit error(obj["error"].toString());
            return;
        }
        PersonRecord person;
        person.id = obj["id"].toInt();
        person.displayName = obj["display_name"].toString();
        person.hasReference = obj["has_reference"].toBool();
        emit personCreated(person);
    });
}

void ApiWorker::assignRegionToPerson(int regionId, int personId)
{
    QJsonObject body;
    body["region_id"] = regionId;
    body["person_id"] = personId;

    QUrl url(serverUrl + "assignRegion.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, regionId, personId, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        if (obj.contains("error")) {
            emit error(obj["error"].toString());
            return;
        }
        emit regionAssigned(regionId, personId);
    });
}

void ApiWorker::setPersonReference(int personId, int regionId)
{
    QJsonObject body;
    body["person_id"] = personId;
    body["region_id"] = regionId;

    QUrl url(serverUrl + "setPersonReference.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, personId, regionId, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        if (obj.contains("error")) {
            emit error(obj["error"].toString());
            return;
        }
        emit personReferenceSet(personId, regionId);
    });
}

void ApiWorker::unassignRegion(int regionId)
{
    QJsonObject body;
    body["region_id"] = regionId;

    QUrl url(serverUrl + "unassignRegion.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, regionId, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        if (obj.contains("error")) {
            emit error(obj["error"].toString());
            return;
        }
        emit regionUnassigned(regionId);
    });
}