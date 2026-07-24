#pragma once

#include <QNetworkAccessManager>
#include <QUrlQuery>
#include "PhotoRepository.h"
#include "PhotoRecord.h"

class ApiWorker :  public PhotoRepository
{
    Q_OBJECT

public:

    explicit ApiWorker(QObject *parent=nullptr);

public slots:

    void open(SettingsManager *rpSettings) override;
    void loadMedia() override;
    void loadFolders(const QString &mediaName) override;
    void loadPhotos(const QString &mediaName, const QString &path) override;
    void loadPhoto(int id) override;

private:

    QNetworkAccessManager manager;

    QString serverUrl =API_URL;

};