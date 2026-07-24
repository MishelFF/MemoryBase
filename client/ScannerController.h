#pragma once

#include <QObject>
#include <QThread>
#include <QThreadPool>
#include <QHash>
#include <QDirIterator>

#include "PhotoRecord.h"
#include "FileKey.h"
#include "SettingsManager.h"

class PhotoRepository;
class PhotoTreeModel;

class ScannerController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(PhotoTreeModel* photoTree  READ photoTree  CONSTANT)
    Q_PROPERTY(QString thumbnailSource  READ thumbnailDataSource  NOTIFY selectedPhotoChanged)

public:

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
signals:
    void selectedPhotoChanged();
    void status(QString message);
    void connectrepository(SettingsManager*);
private slots:
    void mediaLoaded(QStringList media);
    void foldersLoaded(QString media,QStringList folders);
    void photosLoaded(QList<PhotoRecord> photos);
    void photoLoaded(PhotoRecord photo);
    
//    void progressChanged(int value);
//    void finished();
    void databaseConnected(bool ok);
    void photoTreeLoaded(const QList<PhotoRecord> &photos);
    void reConnected();
//    void selectPhotoLoaded(const PhotoRecord &photo);
//    void treeItemExpanded(int type,QString media,QString path);
private:
//    void enqueueImport(const PhotoRecord &photo);

    PhotoRepository *m_repository = nullptr;;
    PhotoTreeModel *m_photoTree = nullptr;
    SettingsManager *m_settings = nullptr;
    PhotoRecord m_selectedPhoto;
    QString m_thumbnailSource;
//    QSet<QString> m_loadedFolders;
//    QSet<QString> m_loadedMedia; 
//    QThread m_databaseThread;
//    QThreadPool m_pool;
//    FileCache m_cache;
//    QString m_rootFolder;

//    int m_totalFiles = 0;
//    int m_processedFiles = 0;
};