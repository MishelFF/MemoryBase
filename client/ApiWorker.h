#pragma once

#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QImage>
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
    void loadMediaMounts() override;
    void saveMountPoint(const QString &media, const QString &mountPoint) override;
    void loadPersons() override;
    void loadUnresolvedRegions() override;
    void loadRegionsForPerson(int personId) override;
    void createPerson(const QString &displayName) override;
    void assignRegionToPerson(int regionId, int personId) override;
    void setPersonReference(int personId, int regionId) override;
    void unassignRegion(int regionId) override;
    QImage loadChipImage(const QString &id, QSize *size, const QSize & /*requestedSize*/) override;
    void searchPhotos(const PhotoFilter &filter);
    void updatePhotoCountry(int photoId, int countryId);

    virtual void loadCountries();
    virtual void addCountry(const QString &name, const QList<CountryBBox> &bboxes);
    virtual void updateCountryBBoxes(int countryId, const QList<CountryBBox> &bboxes);
    virtual void deleteCountry(int countryId);

    virtual void loadPlaces();
    virtual void addPlace(const QString &name, double lat, double lon, double radiusKm, int countryId);
    virtual void updatePlace(int placeId, const QString &name, double radiusKm, int countryId);
    virtual void deletePlace(int placeId);

    virtual void assignCountriesByCoordinates();
    virtual void assignCountryToFolder(const QString &mediaName, const QString &folderPathPrefix, int countryId) ;

private:

    QNetworkAccessManager *manager;

    QString serverUrl =API_URL;

};