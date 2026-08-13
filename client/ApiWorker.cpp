#include "ApiWorker.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QImage>

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
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
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
void ApiWorker::loadPersons(){}
void ApiWorker::loadUnresolvedRegions(){};
void ApiWorker::loadRegionsForPerson(int personId){Q_UNUSED(personId);};
void ApiWorker::createPerson(const QString &displayName){Q_UNUSED(displayName);};
void ApiWorker::assignRegionToPerson(int regionId, int personId){
    Q_UNUSED(regionId);
    Q_UNUSED(personId);
};
void ApiWorker::setPersonReference(int personId, int regionId){
    Q_UNUSED(regionId);
    Q_UNUSED(personId);
};
void ApiWorker::unassignRegion(int regionId){Q_UNUSED(regionId);};
QImage ApiWorker::loadChipImage(const QString &id, QSize *size, const QSize & /*requestedSize*/) {
    Q_UNUSED(id);
    Q_UNUSED(size);
    return QImage();
}
