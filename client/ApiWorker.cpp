#include "ApiWorker.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

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
            photo.dateCreation = QDateTime::fromString(obj["date_creation"].toString(), Qt::ISODate);
            photos.append(photo);
        }
        emit photosLoaded(photos);
        reply->deleteLater();
    });
}
//-----------------------------------------------------
// Загрузка одной фотографии
//-----------------------------------------------------
void ApiWorker::loadPhoto(int id) {
    QUrlQuery query;
    query.addQueryItem("id", QString::number(id));
    QUrl url(serverUrl + "thumbnail.php");
    url.setQuery(query);
    qDebug()<<"Запрос  :"<<url.toString(QUrl::FullyDecoded);
    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        PhotoRecord photo;
        QByteArray thumb = reply->readAll();
        photo.thumbnail = thumb;
        
        emit photoLoaded(photo);
        reply->deleteLater();
    });
}