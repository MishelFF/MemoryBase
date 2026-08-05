#include "LicenseManager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QSysInfo>
#include <QDebug>
#include <sodium.h>

namespace {

const QString PublicKeyBase64 = "WAqt2PPnOQtd1go5NpjIcNKgzVz513L6xmAm19758XM=";
}

LicenseManager::LicenseManager(QString serverUrl, QObject *parent)
    : QObject(parent), m_serverUrl(std::move(serverUrl))
{
}

QString LicenseManager::licenseFilePath() const
{
   
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/license.json";
}

void LicenseManager::setLastError(const QString &error)
{
    if (m_lastError == error)
        return;
    m_lastError = error;
    emit lastErrorChanged();
}

void LicenseManager::checkLicense()
{
    const bool wasValid = m_valid;
    load(licenseFilePath());
    if (m_valid != wasValid)
        emit activationStateChanged();
    if (m_valid)
        emit activationSucceeded();
    else    
        emit activationFailed("Лицензия не найдена или недействительна.");
}

bool LicenseManager::load(const QString &fileName)
{
    m_valid = false;
    m_payload = QJsonObject();

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return false;

    auto root = doc.object();
    if (!root.contains("payload") || !root.contains("signature"))
        return false;

    m_payload = root["payload"].toObject();
    QByteArray payload = QJsonDocument(m_payload).toJson(QJsonDocument::Compact);
    QByteArray signature = QByteArray::fromBase64(root["signature"].toString().toUtf8());

    if (!verify(payload, signature))
        return false;

    auto date = QDate::fromString(m_payload["expires"].toString(), Qt::ISODate);
    if (date.isValid() && date < QDate::currentDate())
        return false;

    m_valid = true;
    return true;
}

bool LicenseManager::verify(const QByteArray &payload, const QByteArray &signature)
{
    if (sodium_init() < 0)
        return false;

    QByteArray publicKey = QByteArray::fromBase64(PublicKeyBase64.toUtf8());
    if (publicKey.size() != crypto_sign_PUBLICKEYBYTES)
        return false;
    if (signature.size() != crypto_sign_BYTES)
        return false;

    return crypto_sign_verify_detached(
               reinterpret_cast<const unsigned char *>(signature.constData()),
               reinterpret_cast<const unsigned char *>(payload.constData()), payload.size(),
               reinterpret_cast<const unsigned char *>(publicKey.constData())) == 0;
}

QString LicenseManager::client() const { return m_payload["client"].toString(); }
QString LicenseManager::edition() const { return m_payload["edition"].toString(); }
QDate LicenseManager::expires() const { return QDate::fromString(m_payload["expires"].toString(), Qt::ISODate); }

void LicenseManager::activate(const QString &licenseKey)
{
    if (m_checking)
        return; 

    m_checking = true;
    emit isCheckingChanged();
    setLastError(QString());

    QString base = m_serverUrl;
    if (!base.endsWith('/'))
        base += '/';
    QUrl url(base + "activate.php");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["license"] = licenseKey;
    json["machine"] = QString::fromLatin1(QSysInfo::machineUniqueId().toHex());

    QNetworkReply *reply = m_network.post(request, QJsonDocument(json).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_checking = false;
        emit isCheckingChanged();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "License activation network error:" << reply->errorString();
            setLastError(tr("Не удалось связаться с сервером активации: %1").arg(reply->errorString()));
            emit activationFailed(m_lastError);
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject() || doc.object().value("status").toString() != "ok") {
            setLastError(tr("Сервер отклонил ключ активации"));
            emit activationFailed(m_lastError);
            return;
        }

        QFile file(licenseFilePath());
        if (!file.open(QIODevice::WriteOnly)) {
            setLastError(tr("Не удалось сохранить файл лицензии на диск"));
            emit activationFailed(m_lastError);
            return;
        }
        file.write(QJsonDocument(doc.object().value("license").toObject()).toJson(QJsonDocument::Indented));
        file.close();

        checkLicense(); // перечитать и проверить подпись/срок только что записанного файла
        if (m_valid) {
            emit activationSucceeded();
        } else {
            setLastError(tr("Сервер вернул некорректный файл лицензии"));
            emit activationFailed(m_lastError);
        }
    });
}
