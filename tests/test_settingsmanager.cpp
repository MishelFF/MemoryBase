#include <QtTest>
#include <QSignalSpy>
#include <QCoreApplication>

#include "settingsmanager.h"

class SettingsManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void validApiUrl();
    void invalidApiUrl();

    void saveAllSuccess();
    void saveAllInvalidUrl();
    void saveAllEmptyHost();

    void settingsSavedSignal();
};

void SettingsManagerTest::initTestCase()
{
    QCoreApplication::setOrganizationName("MemoryBaseTest");
    QCoreApplication::setApplicationName("SettingsTests");
}

void SettingsManagerTest::validApiUrl()
{
    SettingsManager sm;

    QVERIFY(sm.isValidApiUrl("http://localhost"));
    QVERIFY(sm.isValidApiUrl("https://example.com"));
    QVERIFY(sm.isValidApiUrl("https://example.com/api"));
    QVERIFY(sm.isValidApiUrl("http://127.0.0.1:8080"));
}

void SettingsManagerTest::invalidApiUrl()
{
    SettingsManager sm;

    QVERIFY(!sm.isValidApiUrl(""));
    QVERIFY(!sm.isValidApiUrl("localhost"));
    QVERIFY(!sm.isValidApiUrl("ftp://localhost"));
    QVERIFY(!sm.isValidApiUrl("http://"));
}

void SettingsManagerTest::saveAllSuccess()
{
    SettingsManager sm;

    QSignalSpy savedSpy(&sm, &SettingsManager::settingsSaved);
    QSignalSpy errorSpy(&sm, &SettingsManager::errorOccurred);

    QVERIFY(sm.saveAll(
        "localhost",
        5432,
        "MemoryBase",
        "postgres",
        "secret",
        "http://localhost:8080"));

    QCOMPARE(savedSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 0);

    QCOMPARE(sm.dbHost(), QString("localhost"));
    QCOMPARE(sm.dbPort(), 5432);
    QCOMPARE(sm.dbName(), QString("MemoryBase"));
    QCOMPARE(sm.dbUser(), QString("postgres"));
    QCOMPARE(sm.dbPassword(), QString("secret"));
    QCOMPARE(sm.apiUrl(), QString("http://localhost:8080"));

}

void SettingsManagerTest::saveAllInvalidUrl()
{
    SettingsManager sm;

    QSignalSpy errorSpy(&sm, &SettingsManager::errorOccurred);

    QVERIFY(sm.saveAll(
        "localhost",
        5432,
        "MemoryBase",
        "postgres",
        "secret",
        "ftp://localhost"));

    QCOMPARE(sm.dbHost(), QString("localhost"));
    QCOMPARE(sm.dbName(), QString("MemoryBase"));
    QCOMPARE(sm.dbUser(), QString("postgres"));

    // API не должен измениться
    QCOMPARE(errorSpy.count(), 1);
}
void SettingsManagerTest::saveAllEmptyHost()
{
    SettingsManager sm;

    QSignalSpy errorSpy(&sm, &SettingsManager::errorOccurred);

    QVERIFY(sm.saveAll(
        "",
        5432,
        "MemoryBase",
        "postgres",
        "secret",
        "http://localhost"));

    // API должен сохраниться
    QCOMPARE(sm.apiUrl(), QString("http://localhost"));
    QCOMPARE(errorSpy.count(), 1);
}

void SettingsManagerTest::settingsSavedSignal()
{
    SettingsManager sm;

    QSignalSpy spy(&sm, &SettingsManager::settingsSaved);

    sm.saveAll(
        "localhost",
        5432,
        "MemoryBase",
        "postgres",
        "secret",
        "http://localhost");

    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(SettingsManagerTest)
#include "test_settingsmanager.moc"