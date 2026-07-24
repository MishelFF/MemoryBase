// settingsmanager.h
#pragma once
#include <QObject>
#include <QSettings>
#include <QString>
#include <QUrl>

class SettingsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString dbHost     READ dbHost     WRITE setDbHost     NOTIFY dbHostChanged)
    Q_PROPERTY(int     dbPort     READ dbPort     WRITE setDbPort     NOTIFY dbPortChanged)
    Q_PROPERTY(QString dbName     READ dbName     WRITE setDbName     NOTIFY dbNameChanged)
    Q_PROPERTY(QString dbUser     READ dbUser     WRITE setDbUser     NOTIFY dbUserChanged)
    Q_PROPERTY(QString dbPassword READ dbPassword WRITE setDbPassword NOTIFY dbPasswordChanged)
    Q_PROPERTY(QString apiUrl     READ apiUrl     WRITE setApiUrlProp NOTIFY apiUrlChanged)

public:
    explicit SettingsManager(QObject* parent = nullptr);

    QString dbHost() const;
    void setDbHost(const QString& v);

    int dbPort() const;
    void setDbPort(int v);

    QString dbName() const;
    void setDbName(const QString& v);

    QString dbUser() const;
    void setDbUser(const QString& v);

    QString dbPassword() const;
    void setDbPassword(const QString& v);

    QString apiUrl() const;
    void setApiUrlProp(const QString& v); // "молчаливый" сеттер для property binding

    // Вызываются из QML явно — с валидацией и обратной связью об ошибке
    Q_INVOKABLE bool isValidApiUrl(const QString& url, QString* errorOut = nullptr) const;
    Q_INVOKABLE bool saveApiUrl(const QString& url);
    Q_INVOKABLE bool saveAll(const QString& host, int port, const QString& dbName,
                              const QString& user, const QString& password,
                              const QString& apiUrl);

signals:
    void dbHostChanged();
    void dbPortChanged();
    void dbNameChanged();
    void dbUserChanged();
    void dbPasswordChanged();
    void apiUrlChanged();
    void errorOccurred(const QString& message);
    void openSettingsRequested();
    void settingsSaved();
private:
    static QString obfuscate(const QString& plain);
    static QString deobfuscate(const QString& stored);

    QSettings m_settings;
};