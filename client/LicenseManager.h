#pragma once

#include <QObject>
#include <QDate>
#include <QJsonObject>
#include <QNetworkAccessManager>

// Проверка и активация лицензии.
//
// Схема: офлайн-файл лицензии (payload + подпись Ed25519 через
// libsodium), проверяемый локально при каждом старте (checkLicense());
// активация — сетевой запрос на сервер (activate()), который в ответ
// присылает готовый подписанный файл лицензии и сохраняет его на
// диск, после чего локальная проверка перезапускается.
//
class LicenseManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isActivated READ isActivated NOTIFY activationStateChanged)
    Q_PROPERTY(bool isChecking READ isChecking NOTIFY isCheckingChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString client READ client NOTIFY activationStateChanged)
    Q_PROPERTY(QString edition READ edition NOTIFY activationStateChanged)
    Q_PROPERTY(QDate expires READ expires NOTIFY activationStateChanged)

public:
    explicit LicenseManager(QString serverUrl, QObject *parent = nullptr);

    bool isActivated() const { return m_valid; }
    bool isChecking() const { return m_checking; }
    QString lastError() const { return m_lastError; }
    QString client() const;
    QString edition() const;
    QDate expires() const;

    Q_INVOKABLE void checkLicense();

    Q_INVOKABLE void activate(const QString &licenseKey);

signals:
    void activationStateChanged();
    void isCheckingChanged();
    void lastErrorChanged();
    void activationSucceeded();
    void activationFailed(QString reason);

private:
    bool load(const QString &fileName);
    bool verify(const QByteArray &payload, const QByteArray &signature);
    QString licenseFilePath() const;
    void setLastError(const QString &error);

    QNetworkAccessManager m_network; 
    QString m_serverUrl;
    bool m_valid = false;
    bool m_checking = false;
    QString m_lastError;
    QJsonObject m_payload;
};
