#pragma once

#include <QObject>
#include <QThread>
#include <QThreadPool>
#include <QHash>
#include <QDirIterator>
#include <QAtomicInteger>

#include "PhotoRecord.h"
#include "FileKey.h"
#include "SettingsManager.h"

class PhotoRepository;
class PhotoTreeModel;

class ScannerController : public QObject
{
    Q_OBJECT


public:
    Q_PROPERTY(PhotoTreeModel* photoTree  READ photoTree  CONSTANT)
    Q_PROPERTY(QString thumbnailSource  READ thumbnailDataSource  NOTIFY selectedPhotoChanged)
    Q_PROPERTY(int importTotal READ importTotal NOTIFY importProgressChanged)
    Q_PROPERTY(int importProcessed READ importProcessed NOTIFY importProgressChanged)
    Q_PROPERTY(bool importRunning READ importRunning NOTIFY importRunningChanged)

    int importTotal() const { return m_totalFiles; }
    int importProcessed() const { return m_processedFiles.loadRelaxed(); }
    bool importRunning() const { return m_importRunning; }

    explicit ScannerController(PhotoRepository *repository,SettingsManager* settings,QObject *parent = nullptr);
    ~ScannerController();

    PhotoTreeModel* photoTree() const;
    QString thumbnailDataSource() const;
public slots:
    void loadTree();
    void selectPhoto(int id);
    void loadMedia();
    void loadFolders(const QString &media);
    void loadPhotos(const QString &media,const QString &path);
    void scanFolder(const QString &media,const QString &folder);
    void generateMissingThumbnails(const QString &media,const QString &rootFolder);
    void onFileProcessed();
signals:
    void selectedPhotoChanged();
    void status(QString message);
    void connectrepository(SettingsManager*);
    void importProgressChanged();
    void importRunningChanged();

private slots:
    void databaseConnected(bool ok);
    void mediaLoaded(QStringList media);
    void foldersLoaded(QString media,QStringList folders);
    void photosLoaded(QList<PhotoRecord> photos);
    void photoLoaded(PhotoRecord photo);
    void photoTreeLoaded(const QList<PhotoRecord> &photos);
    void reConnected();
//    void selectPhotoLoaded(const PhotoRecord &photo);
//    void treeItemExpanded(int type,QString media,QString path);
//    void progressChanged(int value);
//    void finished();

private:
    void enqueueImport(const PhotoRecord &photo, bool reportProgress);
    void incrementProcessed();

    PhotoRepository *m_repository = nullptr;
    QThread *m_repositoryThread = nullptr;
    PhotoTreeModel *m_photoTree = nullptr;
    SettingsManager *m_settings = nullptr;
    PhotoRecord m_selectedPhoto;
    QString m_thumbnailSource;
    QThreadPool m_pool;
    FileCache m_cache;
    QString m_rootFolder;

    int m_totalFiles = 0;
    int m_reportInterval = 1;  
    QAtomicInteger<int> m_processedFiles{0};
    bool m_importRunning = false;
};