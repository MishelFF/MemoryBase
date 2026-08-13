#pragma once

#include <QObject>
#include <QThread>
#include <QThreadPool>
#include <QHash>
#include <QDirIterator>
#include <QAtomicInteger>
#include <QVariantList>

#include "PhotoRecord.h"
#include "FileKey.h"
#include "settingsmanager.h"
#include "PhotoTreeModel.h"
#ifdef NEED_LICENSE
#include "LicenseManager.h"
#endif
#include "PhotoRegion.h"
#include "PhotoFilter.h"


class PhotoRepository;
class PhotoTreeModel;
class PersonsModel;
class FaceRegionsModel;
class PhotoSearchResultsModel;


class ScannerController : public QObject
{
    Q_OBJECT


public:
    Q_PROPERTY(PhotoTreeModel* photoTree  READ photoTree  CONSTANT)
    Q_PROPERTY(PersonsModel* personsModel READ personsModel CONSTANT)
    Q_PROPERTY(FaceRegionsModel* unresolvedRegionsModel READ unresolvedRegionsModel CONSTANT)
    Q_PROPERTY(FaceRegionsModel* personRegionsModel READ personRegionsModel CONSTANT)
    Q_PROPERTY(PhotoSearchResultsModel* searchResultsModel READ searchResultsModel CONSTANT)

    Q_PROPERTY(QString thumbnailSource  READ thumbnailDataSource  NOTIFY selectedPhotoChanged)
    Q_PROPERTY(int importTotal READ importTotal NOTIFY importProgressChanged)
    Q_PROPERTY(int importProcessed READ importProcessed NOTIFY importProgressChanged)
    Q_PROPERTY(bool importRunning READ importRunning NOTIFY importRunningChanged)
    Q_PROPERTY(QVariantList photoInfo READ photoInfo NOTIFY selectedPhotoChanged)
    Q_PROPERTY(bool hasNextPhoto READ hasNextPhoto NOTIFY navigationChanged)
    Q_PROPERTY(bool hasPreviousPhoto READ hasPreviousPhoto NOTIFY navigationChanged)
    Q_PROPERTY(int searchListCurrentIndex READ searchListCurrentIndex NOTIFY navigationChanged)
 
    Q_PROPERTY(QString selectedPhotoKey READ selectedPhotoKey NOTIFY selectedPhotoChanged)
    Q_PROPERTY(QString selectedPhotoName READ selectedPhotoName NOTIFY selectedPhotoChanged)
    Q_PROPERTY(QString selectedPhotoFolder READ selectedPhotoFolder NOTIFY selectedPhotoChanged)
    Q_PROPERTY(QString missingFilesText READ missingFilesText NOTIFY missingFilesTextChanged)
    Q_PROPERTY(QStringList knownMedia READ knownMedia NOTIFY knownMediaChanged)
    Q_PROPERTY(QStringList knownMountPoints READ knownMountPoints NOTIFY knownMountPointsChanged)

    bool hasNextPhoto() const { return m_nextPhotoId >= 0; }
    bool hasPreviousPhoto() const { return m_previousPhotoId >= 0; }
    
    Q_INVOKABLE void selectNextPhoto();
    Q_INVOKABLE void selectPreviousPhoto();
    Q_INVOKABLE void createPerson(const QString &displayName);
    Q_INVOKABLE void assignRegionToPerson(int regionId, int personId);
    Q_INVOKABLE void setPersonReference(int personId, int regionId);
    Q_INVOKABLE void unassignRegion(int regionId);

    int importTotal() const { return m_totalFiles; }
    int importProcessed() const { return m_processedFiles.loadRelaxed(); }
    bool importRunning() const { return m_importRunning; }
#ifdef NEED_LICENSE
    void setlicenseManager(LicenseManager *licenseManager){m_licenseManager=licenseManager}; 
#endif
    Q_INVOKABLE QString mountPointFor(const QString &media) const;

    explicit ScannerController(PhotoRepository *repository,SettingsManager* settings,QObject *parent = nullptr);
    ~ScannerController();

    PhotoTreeModel* photoTree() const;
    PersonsModel* personsModel() const { return m_personsModel; }
    FaceRegionsModel* unresolvedRegionsModel() const { return m_unresolvedRegionsModel; }
    FaceRegionsModel* personRegionsModel() const { return m_personRegionsModel; }
    PhotoSearchResultsModel* searchResultsModel() const { return m_searchResultsModel; }
    int searchListCurrentIndex() const { return m_searchListIndex;}

    QString thumbnailDataSource() const;
    QVariantList photoInfo() const;
    QString selectedPhotoKey() const {
        return PhotoTreeItem::GetKey(m_selectedPhoto.mediaName, m_selectedPhoto.path, m_selectedPhoto.file);
    }
    QString selectedPhotoName() const { return m_selectedPhoto.file; }
    QString selectedPhotoFolder() const { return m_selectedPhoto.path; }
    QString missingFilesText() const { return m_missingFilesText; }
    QStringList knownMedia() const { return m_knownMedia; }
    QStringList knownMountPoints() const { return m_knownMountPoints; }
    QImage loadChipImage(const QString &id, QSize *size, const QSize & /*requestedSize*/);

public slots:
    void loadTree();
    void selectPhoto(int id);
    void loadMedia();
    void loadFolders(const QString &media);
    void loadPhotos(const QString &media,const QString &path);
    void loadMediaMounts();
    void scanFolder(const QString &media, const QString &mountPoint, const QString &folder);
    void generateMissingThumbnails(const QString &media, const QString &mountPoint,const QString &rootFolder);
    void onFileProcessed();
    void incrementProcessed();
    void findMissingFiles(const QString &media, const QString &mountPoint, const QString &folder);
//    void missingFileFound(int id, const QString &path, const QString &file);
    void appendMissingFiles(const QStringList &rows);
    void loadPersons();
    void loadRegionsForPerson(int personId);
    void loadUnresolvedRegions();
    void searchPhotos(const QVariantMap &filterMap);
    void selectSearchResult(int index);

signals:
    void selectedPhotoChanged();
//    void requestPhotos(QString media,QString path);
    void photoInFolderLoaded();
    void status(QString message);
    void connectrepository(SettingsManager*);
    void importProgressChanged();
    void importRunningChanged();
    void navigationChanged();               // hasNextPhoto/hasPreviousPhoto
    void folderBoundaryCrossed(QString folderName); // Folder changed
    void knownMediaChanged();
    void missingFilesTextChanged();
    void knownMountPointsChanged();

private slots:
    void databaseConnected(bool ok);
    void mediaLoaded(QStringList media);
    void foldersLoaded(QString media,QStringList folders);
    void photosLoaded(QList<PhotoRecord> photos);
    void photoLoaded(PhotoRecord photo);
    void photoTreeLoaded(const QList<PhotoRecord> &photos);
    void reConnected();
    void mediaMountsLoaded(QVariantList mounts);

//    void selectPhotoLoaded(const PhotoRecord &photo);
//    void treeItemExpanded(int type,QString media,QString path);
//    void progressChanged(int value);
//    void finished();

private:
    void enqueueImport(const PhotoRecord &photo, bool reportProgress);
    PhotoTreeItem *findNeighborLevelUp(PhotoTreeItem *item,int increment);
    PhotoTreeItem *findNeighborLevelDown(PhotoTreeItem *item,int increment);
    void updateNavigationNeighbors();

    PhotoRepository *m_repository = nullptr;
    QThread *m_repositoryThread = nullptr;
    PhotoTreeModel *m_photoTree = nullptr;
    PersonsModel *m_personsModel = nullptr;
    FaceRegionsModel *m_unresolvedRegionsModel = nullptr;
    FaceRegionsModel *m_personRegionsModel = nullptr;
    PhotoSearchResultsModel *m_searchResultsModel = nullptr;
    SettingsManager *m_settings = nullptr;
#ifdef NEED_LICENSE
    LicenseManager *m_licenseManager=nullptr; 
#endif
    PhotoRecord m_selectedPhoto;
    QString m_thumbnailSource;
    QThreadPool m_pool;
    FileCache m_cache;
    QString m_rootFolder;
    enum class NavigationSource { Tree, SearchList };
    NavigationSource m_navigationSource = NavigationSource::Tree;
    int m_searchListIndex = -1;
    void selectPhotoInternal(int id, NavigationSource source);
 

    int m_totalFiles = 0;
    int m_reportInterval = 1;  
    QAtomicInteger<int> m_processedFiles{0};
    bool m_importRunning = false;
    QVariantList m_mediaMounts;
    QStringList m_knownMedia;
    QString m_missingFilesText;
    int m_missingFilesCount=0;
    int m_nextPhotoId = -1;
    int m_previousPhotoId = -1;
    bool blockNavigation;
    QString m_nextPhotoFolder;
    QString m_previousPhotoFolder;
    QList<QByteArray> supportedFormats;
    QStringList m_knownMountPoints;
};
