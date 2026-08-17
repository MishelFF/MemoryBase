#pragma once

#include <QObject>
#include <QList>
#include "settingsmanager.h"
#include "PhotoRecord.h"
#include "PersonRecord.h"
#include "FaceRegionRecord.h"
#include "PhotoFilter.h"
#include "CountryRecord.h"
#include "PlaceRecord.h"

class PhotoRepository : public QObject
{
    Q_OBJECT

public:

    explicit PhotoRepository(QObject *parent = nullptr): QObject(parent){}
    virtual ~PhotoRepository(){}

public slots:

    virtual void open(SettingsManager *rpSettings) = 0;
    virtual void loadMedia() = 0;
    virtual void loadFolders(const QString &mediaName) = 0;
    virtual void loadPhotos(const QString &mediaName,const QString &path) = 0;
    virtual void loadPhoto(int id) = 0;
    virtual void loadMediaMounts() = 0;                                          
    virtual void saveMountPoint(const QString &media, const QString &mountPoint) = 0;  
    virtual void loadPersons() = 0;
    virtual void loadUnresolvedRegions() = 0;
    virtual void loadRegionsForPerson(int personId) = 0;
    virtual void createPerson(const QString &displayName) = 0;
    virtual void assignRegionToPerson(int regionId, int personId) = 0;
    virtual void setPersonReference(int personId, int regionId) = 0;
    virtual void unassignRegion(int regionId) = 0;
    virtual QImage loadChipImage(const QString &id, QSize *size, const QSize & /*requestedSize*/)= 0;
    virtual void searchPhotos(const PhotoFilter &) =0;
    
    virtual void loadCountries() = 0;
    virtual void addCountry(const QString &name, const QList<CountryBBox> &bboxes) = 0;
    virtual void updateCountryBBoxes(int countryId, const QList<CountryBBox> &bboxes) = 0;
    virtual void deleteCountry(int countryId) = 0;

    virtual void loadPlaces() = 0;
    virtual void addPlace(const QString &name, double lat, double lon, double radiusKm, int countryId) = 0;
    virtual void updatePlace(int placeId, const QString &name, double radiusKm, int countryId) = 0;
    virtual void deletePlace(int placeId) = 0;

    virtual void assignCountriesByCoordinates() = 0;
    virtual void assignCountryToFolder(const QString &mediaName, const QString &folderPathPrefix, int countryId) = 0;
    virtual void updatePhotoCountry(int photoId, int countryId) = 0; 

signals:
    void connected(bool ok);
    void mediaLoaded(QStringList media);
    void foldersLoaded(QString media,QStringList folders);
    void photosLoaded(QList<PhotoRecord> photos);
    void photoLoaded(PhotoRecord photo);
    void error(QString message);
    void status(QString);
    void mediaMountsLoaded(QVariantList mounts);   
    void personsLoaded(QList<PersonRecord> persons);
    void unresolvedRegionsLoaded(QList<FaceRegionRecord> regions);
    void personRegionsLoaded(int personId, QList<FaceRegionRecord> regions);
    void personCreated(PersonRecord person);
    void regionAssigned(int regionId, int personId);
    void personReferenceSet(int personId, int regionId);
    void regionUnassigned(int regionId);
    void photosFound(QList<PhotoRecord> photos);

    void countriesLoaded(QList<CountryRecord> countries);
    void countryAdded(CountryRecord country);
    void countryDeleted(int countryId);
    void countriesAssigned(int updatedCount);

    void placesLoaded(QList<PlaceRecord> places);
    void placeAdded(PlaceRecord place);
    void placeUpdated(PlaceRecord place);
    void placeDeleted(int placeId);

    void photoCountryUpdated(int photoId, int countryId);

};