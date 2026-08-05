// settingsmanager.cpp
#include "settingsmanager.h"
#include "PhotoRecord.h"
#include "secrets.h"
#include <QCoreApplication>
#include <QByteArray>

static const char* kObfuscationKey = SECRETHASH;

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
    , m_settings(QSettings::IniFormat, QSettings::UserScope,
                 QCoreApplication::organizationName(),
                 QCoreApplication::applicationName())
{
}

QString SettingsManager::obfuscate(const QString& plain)
{
    if (plain.isEmpty()) return QString();
    QByteArray data = plain.toUtf8();
    QByteArray key(kObfuscationKey);
    for (int i = 0; i < data.size(); ++i)
        data[i] = data[i] ^ key[i % key.size()];
    return QString::fromLatin1(data.toBase64());
}

QString SettingsManager::deobfuscate(const QString& stored)
{
    if (stored.isEmpty()) return QString();
    QByteArray data = QByteArray::fromBase64(stored.toLatin1());
    QByteArray key(kObfuscationKey);
    for (int i = 0; i < data.size(); ++i)
        data[i] = data[i] ^ key[i % key.size()];
    return QString::fromUtf8(data);
}

QString SettingsManager::dbHost() const { return m_settings.value("db/host", "localhost").toString(); }
void SettingsManager::setDbHost(const QString& v) { m_settings.setValue("db/host", v); emit dbHostChanged(); }

int SettingsManager::dbPort() const { return m_settings.value("db/port", 5432).toInt(); }
void SettingsManager::setDbPort(int v) { m_settings.setValue("db/port", v); emit dbPortChanged(); }

QString SettingsManager::dbName() const { return m_settings.value("db/name").toString(); }
void SettingsManager::setDbName(const QString& v) { m_settings.setValue("db/name", v); emit dbNameChanged(); }

QString SettingsManager::dbUser() const { return m_settings.value("db/user").toString(); }
void SettingsManager::setDbUser(const QString& v) { m_settings.setValue("db/user", v); emit dbUserChanged(); }

QString SettingsManager::dbPassword() const { return deobfuscate(m_settings.value("db/password").toString()); }
void SettingsManager::setDbPassword(const QString& v) { m_settings.setValue("db/password", obfuscate(v)); emit dbPasswordChanged(); }

QString SettingsManager::apiUrl() const { return m_settings.value("api/url", API_URL).toString(); }
void SettingsManager::setApiUrlProp(const QString& v) { m_settings.setValue("api/url", v); emit apiUrlChanged(); }

bool SettingsManager::isValidApiUrl(const QString& url, QString* errorOut) const
{
    if (url.trimmed().isEmpty()) {
        if (errorOut) *errorOut = "URL не может быть пустым";
        return false;
    }
    QUrl u(url, QUrl::StrictMode);
    if (!u.isValid()) {
        if (errorOut) *errorOut = "Некорректный формат URL: " + u.errorString();
        return false;
    }
    const QString scheme = u.scheme().toLower();
    if (scheme != "http" && scheme != "https") {
        if (errorOut) *errorOut = "URL должен использовать схему http или https";
        return false;
    }
    if (u.host().isEmpty()) {
        if (errorOut) *errorOut = "URL должен содержать хост";
        return false;
    }
    return true;
}

bool SettingsManager::saveApiUrl(const QString& url)
{
    QString err;
    if (!isValidApiUrl(url, &err)) {
        emit errorOccurred(err);
        return false;
    }
    setApiUrlProp(url);
    m_settings.sync();
    return true;
}

bool SettingsManager::saveAll(const QString& host, int port, const QString& dbNameV,
                               const QString& user, const QString& password,
                               const QString& apiUrlV)
{   bool isDBOk=true,isAPIOk=true; 
    if (host.trimmed().isEmpty()) {
        emit errorOccurred("Укажите хост базы данных");
        isDBOk = false;
    }
    if (dbNameV.trimmed().isEmpty()) {
        emit errorOccurred("Укажите имя базы данных");
        isDBOk = false;
    }
    if (user.trimmed().isEmpty()) {
        emit errorOccurred("Укажите пользователя БД");
        isDBOk = false;
    }

    QString err;
    if (!isValidApiUrl(apiUrlV, &err)) {
        emit errorOccurred("API URL: " + err);
        isAPIOk = false;
    }
    if (isDBOk){
        setDbHost(host.trimmed());
        setDbPort(port);
        setDbName(dbNameV.trimmed());
        setDbUser(user.trimmed());
        setDbPassword(password);
    }
    if (isAPIOk) setApiUrlProp(apiUrlV);
    if (isDBOk||isAPIOk){
        m_settings.sync();
        emit settingsSaved();
    }
    return isDBOk||isAPIOk;
}