#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QHash>
#include <QImage>

#include "PhotoRecord.h"
#include "FileKey.h"
#include "PhotoRepository.h"
#include "PhotoRegion.h"
#include "PhotoFilter.h"

class DatabaseWorker : public PhotoRepository
{
    Q_OBJECT

public:
    explicit DatabaseWorker(QObject *parent = nullptr);
    ~DatabaseWorker();

    QList<PhotoRecord> getPhotosWithoutThumbnail(const QString &media,const QString &mountPoint,const QString &rootFolder);
//    QList<PhotoRecord> loadPhotoTree();
    int insertPhoto(PhotoRecord &photo);
    bool updateExif(const PhotoRecord &photo);
    bool updateMD5(const PhotoRecord &photo);
    bool insertThumbnail(const PhotoRecord &photo);
    bool insertRegions(int photoId, const QList<PhotoRegion> &regions);
    void loadCache(const QString &media,const QString &path,FileCache* m_cache);
    void markMissing(int id);
    void clearMissing(int id);
    QList<PhotoRecord> loadPathEntries(const QString &media, const QString &relativePath);
    QSet<int> loadMissingIds(const QString &media, const QString &relativePath);
    bool setReferenceFace(int personId, int regionId);
    QVector<QPair<int, double>> findSimilarFaces(int regionId, double threshold, int limit);


public slots:
    void open(SettingsManager *rpSettings) override;
    void loadMedia() override;
    void loadFolders(const QString &mediaName) override;
    void loadPhotos(const QString &mediaName,const QString &path) override;
    void loadPhoto(int id) override;
    void close();
    void loadMediaMounts();                                            
    void saveMountPoint(const QString &media, const QString &mountPoint);  
    
    void loadPersons();
    void loadUnresolvedRegions();
    void loadRegionsForPerson(int personId);
    void createPerson(const QString &displayName);
    void assignRegionToPerson(int regionId, int personId);
    void setPersonReference(int personId, int regionId);
    void unassignRegion(int regionId);
    QImage loadChipImage(const QString &id, QSize *size, const QSize & /*requestedSize*/);
    void searchPhotos(const PhotoFilter &filter);
    void loadCountries();
    void addCountry(const QString &name, const QList<CountryBBox> &bboxes);
    void updateCountryBBoxes(int countryId, const QList<CountryBBox> &bboxes);
    void deleteCountry(int countryId);
    void loadPlaces();
    void addPlace(const QString &name, double lat, double lon,double radiusKm, int countryId);
    void updatePlace(int placeId, const QString &name, double radiusKm, int countryId);
    void deletePlace(int placeId);
    void assignCountriesByCoordinates();
    void assignCountryToFolder(const QString &mediaName,const QString &folderPathPrefix,int countryId);
    void updatePhotoCountry(int photoId, int countryId);


//    bool exists(const FileKey &key);
//    void beginTransaction();
//    void commit();

private:

    QSqlDatabase db;
};
