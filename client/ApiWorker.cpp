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
            photo.countryId = obj["country_id"].toInt();
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

void ApiWorker::loadCountries()
{
    QUrl url(serverUrl + "countries.php");
    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QList<CountryRecord> result;
        for (const auto &item : doc.array()) {
            QJsonObject obj = item.toObject();
            CountryRecord c;
            c.id = obj["id"].toInt();
            c.name = obj["name"].toString();
            for (const auto &bboxItem : obj["bboxes"].toArray()) {
                QJsonObject bboxObj = bboxItem.toObject();
                CountryBBox b;
                b.id = bboxObj["id"].toInt();
                b.latMin = bboxObj["lat_min"].toDouble();
                b.latMax = bboxObj["lat_max"].toDouble();
                b.lonMin = bboxObj["lon_min"].toDouble();
                b.lonMax = bboxObj["lon_max"].toDouble();
                c.bboxes.append(b);
            }
            result.append(c);
        }
        emit countriesLoaded(result);
    });
}

void ApiWorker::addCountry(const QString &name, const QList<CountryBBox> &bboxes)
{
    QJsonObject payload;
    payload["name"] = name;
    QJsonArray bboxArray;
    for (const auto &b : bboxes) {
        QJsonObject bboxObj;
        bboxObj["lat_min"] = b.latMin;
        bboxObj["lat_max"] = b.latMax;
        bboxObj["lon_min"] = b.lonMin;
        bboxObj["lon_max"] = b.lonMax;
        bboxArray.append(bboxObj);
    }
    payload["bboxes"] = bboxArray;

    QUrl url(serverUrl + "country_add.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        CountryRecord result;
        result.id = obj["id"].toInt();
        result.name = obj["name"].toString();
        emit countryAdded(result);
    });
}

void ApiWorker::updateCountryBBoxes(int countryId, const QList<CountryBBox> &bboxes)
{
    QJsonObject payload;
    payload["country_id"] = countryId;
    QJsonArray bboxArray;
    for (const auto &b : bboxes) {
        QJsonObject bboxObj;
        bboxObj["lat_min"] = b.latMin;
        bboxObj["lat_max"] = b.latMax;
        bboxObj["lon_min"] = b.lonMin;
        bboxObj["lon_max"] = b.lonMax;
        bboxArray.append(bboxObj);
    }
    payload["bboxes"] = bboxArray;

    QUrl url(serverUrl + "country_bboxes_update.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        loadCountries(); // как и в DatabaseWorker - перечитываем целиком
    });
}

void ApiWorker::deleteCountry(int countryId)
{
    QUrl url(serverUrl + QString("country_delete.php?id=%1").arg(countryId));
    QNetworkReply *reply = manager->deleteResource(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, countryId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        emit countryDeleted(countryId);
    });
}

void ApiWorker::loadPlaces()
{
    QUrl url(serverUrl + "places.php");
    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QList<PlaceRecord> result;
        for (const auto &item : doc.array()) {
            QJsonObject obj = item.toObject();
            PlaceRecord p;
            p.id = obj["id"].toInt();
            p.name = obj["name"].toString();
            p.latitude = obj["latitude"].toDouble();
            p.longitude = obj["longitude"].toDouble();
            p.radiusKm = obj["radius_km"].toDouble();
            p.countryId = obj["country_id"].isNull() ? -1 : obj["country_id"].toInt();
            result.append(p);
        }
        emit placesLoaded(result);
    });
}

void ApiWorker::addPlace(const QString &name, double lat, double lon, double radiusKm, int countryId)
{
    QJsonObject payload;
    payload["name"] = name;
    payload["latitude"] = lat;
    payload["longitude"] = lon;
    payload["radius_km"] = radiusKm;
    if (countryId >= 0)
        payload["country_id"] = countryId;
    else
        payload["country_id"] = QJsonValue::Null;

    QUrl url(serverUrl + "place_add.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        PlaceRecord result;
        result.id = obj["id"].toInt();
        result.name = obj["name"].toString();
        result.latitude = obj["latitude"].toDouble();
        result.longitude = obj["longitude"].toDouble();
        result.radiusKm = obj["radius_km"].toDouble();
        result.countryId = obj["country_id"].isNull() ? -1 : obj["country_id"].toInt();
        emit placeAdded(result);
    });
}

void ApiWorker::updatePlace(int placeId, const QString &name, double radiusKm, int countryId)
{
    QJsonObject payload;
    payload["id"] = placeId;
    payload["name"] = name;
    payload["radius_km"] = radiusKm;
    payload["country_id"] = countryId >= 0 ? QJsonValue(countryId) : QJsonValue::Null;

    QUrl url(serverUrl + "place_update.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        PlaceRecord result;
        result.id = obj["id"].toInt();
        result.name = obj["name"].toString();
        result.latitude = obj["latitude"].toDouble();
        result.longitude = obj["longitude"].toDouble();
        result.radiusKm = obj["radius_km"].toDouble();
        result.countryId = obj["country_id"].isNull() ? -1 : obj["country_id"].toInt();
        emit placeUpdated(result);
    });
}

void ApiWorker::deletePlace(int placeId)
{
    QUrl url(serverUrl + QString("place_delete.php?id=%1").arg(placeId));
    QNetworkReply *reply = manager->deleteResource(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, placeId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        emit placeDeleted(placeId);
    });
}

void ApiWorker::assignCountriesByCoordinates()
{
    QUrl url(serverUrl + "assign_countries_by_coordinates.php");
    QNetworkReply *reply = manager->post(QNetworkRequest(url), QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        emit countriesAssigned(obj["updated"].toInt());
    });
}

void ApiWorker::assignCountryToFolder(const QString &mediaName,
                                       const QString &folderPathPrefix,
                                       int countryId)
{
    QJsonObject payload;
    payload["media_name"] = mediaName;
    payload["folder_prefix"] = folderPathPrefix;
    payload["country_id"] = countryId;

    QUrl url(serverUrl + "assign_country_to_folder.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        emit countriesAssigned(obj["updated"].toInt());
    });
}
void ApiWorker::updatePhotoCountry(int photoId, int countryId)
{
    QJsonObject payload;
    payload["photo_id"] = photoId;
    payload["country_id"] = countryId >= 0 ? QJsonValue(countryId) : QJsonValue::Null;

    QUrl url(serverUrl + "photo_country_update.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, photoId, countryId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        emit photoCountryUpdated(photoId, countryId);
    });
}