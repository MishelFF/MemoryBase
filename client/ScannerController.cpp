#include <QFileInfo>
#include <QMetaObject>
#include <QDebug>
#include "ImportTask.h"
#include "DatabaseWorker.h"
#include "PhotoTreeModel.h"
#include "ScannerController.h"

ScannerController::ScannerController(PhotoRepository *repository,SettingsManager* settings,QObject *parent): 
QObject(parent), m_repository(repository),m_settings(settings)
{
    m_photoTree = new PhotoTreeModel(this);
    connect(m_repository,&PhotoRepository::mediaLoaded,this,&ScannerController::mediaLoaded);
    connect(m_repository,&PhotoRepository::foldersLoaded,this,&ScannerController::foldersLoaded);
    connect(m_repository,&PhotoRepository::photosLoaded,this,&ScannerController::photosLoaded);
    connect(this,&ScannerController::connectrepository,m_repository,&PhotoRepository::open);
    connect(m_repository,&PhotoRepository::connected,this,&ScannerController::databaseConnected);
    connect( m_repository,&PhotoRepository::photoLoaded, this, &ScannerController::photoLoaded);
    connect( m_repository,&PhotoRepository::photosLoaded,this, &ScannerController::photosLoaded);
    connect(m_repository,&PhotoRepository::error,this,&ScannerController::status);
    connect(m_photoTree,&PhotoTreeModel::requestFolders,this,&ScannerController::loadFolders);
    connect(m_photoTree,&PhotoTreeModel::requestPhotos,this,&ScannerController::loadPhotos);
    connect(m_settings,&SettingsManager::settingsSaved,this,&ScannerController::reConnected);
    //emit connectrepository(m_settings);
    reConnected();
}
/*ScannerController::ScannerController(QObject *parent)
    : QObject(parent)
{
    m_photoTree = new PhotoTreeModel(this);

    m_database = new DatabaseWorker(&m_cache);

    m_database->moveToThread(&m_databaseThread);

    connect(&m_databaseThread,&QThread::finished,m_database,&QObject::deleteLater);

    connect(m_database,&DatabaseWorker::status,this,&ScannerController::status);

    m_databaseThread.start();

    m_pool.setMaxThreadCount(QThread::idealThreadCount());
//    connect(m_database,&DatabaseWorker::photoTreeLoaded,this,[this](const QList<PhotoRecord> &photos)
//        {
//          m_photoTree->buildTree(photos);
//        }
//    );
    void photoLoaded(const PhotoRecord &photo);
    void photoLoaded(const PhotoRecord &photo);
    connect(m_database, &DatabaseWorker::photoLoaded, this, &ScannerController::selectPhotoLoaded);
    connect(m_database, &DatabaseWorker::connected, this, &ScannerController::databaseConnected);
    connect(m_database, &DatabaseWorker::photoTreeLoaded,this, &ScannerController::photoTreeLoaded);
}*/

ScannerController::~ScannerController()
{
//    m_pool.waitForDone();

//    m_databaseThread.quit();
//    m_databaseThread.wait();
}
PhotoTreeModel* ScannerController::photoTree() const
{
    return m_photoTree;
}

void ScannerController::selectPhoto(int id)
{
    if(id>0)   m_repository->loadPhoto(id);
}
void ScannerController::loadTree()
{
//    m_loadedMedia.clear();
//    m_loadedFolders.clear();
    m_photoTree->clear();
    
    m_repository->loadMedia();
}
void ScannerController::photoLoaded(PhotoRecord photo)
{
    m_selectedPhoto = photo;
    if(photo.thumbnail.isEmpty()) {
        m_thumbnailSource.clear();
    }
    else {
        m_thumbnailSource = "data:image/jpeg;base64," + QString::fromLatin1( photo.thumbnail.toBase64());
    }
    emit selectedPhotoChanged();
}
QString ScannerController::thumbnailDataSource() const
{
    return m_thumbnailSource;
}
void ScannerController::loadMedia()
{
    m_repository->loadMedia();
}
void ScannerController::mediaLoaded(QStringList media)
{
    m_photoTree->addMedia(media);
    for (const QString &name : media)
    {
//        m_loadedMedia.insert(name);
        m_repository->loadFolders(name);
    }
    emit status(QString("Media count: %1").arg(media.size()));
}

//void ScannerController::selectPhotoLoaded(const PhotoRecord &photo)
//{
//    m_selectedPhoto = photo;
//    if (photo.thumbnail.isEmpty()) {m_thumbnailSource.clear();}
//    else {
//        m_thumbnailSource = "data:image/jpeg;base64," + QString::fromLatin1(photo.thumbnail.toBase64());
//    }
//
//    emit selectedPhotoChanged();
//}
void ScannerController::loadFolders(const QString &media)
{
//    if(!m_loadedMedia.contains(media)) m_loadedMedia.insert(media);
    m_repository->loadFolders(media);
}
void ScannerController::foldersLoaded(QString media,QStringList folders)
{
//    if(!m_loadedFolders.contains(media)) m_loadedMedia.insert(media);
    m_photoTree->addFolders(media,folders);
}
void ScannerController::loadPhotos(const QString &media,const QString &path)
{
//    QString key = media + path;
//    if(m_loadedFolders.contains(key)) return;
//    m_loadedFolders.insert(key);
    m_repository->loadPhotos(media,path);
}
void ScannerController::photosLoaded(QList<PhotoRecord> photos)
{
    if(photos.isEmpty()) return;
    m_photoTree->addPhotos(photos.first().mediaName,photos.first().path,photos);
}
void ScannerController::reConnected()
{
    emit connectrepository(m_settings);
}



/*bool ScannerController::connectDatabase(
        const QString &host,
        int port,
        const QString &database,
        const QString &user,
        const QString &password)
{
    bool ok = false;

    QMetaObject::invokeMethod(
        m_database,
        [&]()
        {
            m_database->open(host,port,database,user,password);
        },
        Qt::BlockingQueuedConnection);

    return ok;
}
void ScannerController::scanFolder(
        const QString &folder)
{
    m_rootFolder = folder;

    m_totalFiles = 0;
    m_processedFiles = 0;

    QStringList filters;

    filters
            << "*.jpg"
            << "*.jpeg"
            << "*.png"
            << "*.bmp"
            << "*.tif"
            << "*.tiff"
            << "*.webp";

    QDirIterator it(
            folder,
            filters,
            QDir::Files,
            QDirIterator::Subdirectories);

    while(it.hasNext())
    {
        QString filename = it.next();

        QFileInfo info(filename);

        FileKey key;

        key.path = info.absolutePath();
        key.file = info.fileName();
        key.size = info.size();
        key.modified = info.lastModified();

        if(m_cache.contains(key))
            continue;

        PhotoRecord photo;

        photo.file = key.file;
        photo.path = QDir::fromNativeSeparators(key.path);
        photo.fullPath = filename;
        photo.fileSize = key.size;
        photo.lastModified = key.modified;
        photo.extension = info.suffix().toLower();

        enqueueImport(photo);

        ++m_totalFiles;
    }

    emit status(
        QString("Добавлено задач: %1")
        .arg(m_totalFiles));
}
void ScannerController::enqueueImport(
        const PhotoRecord &photo)
{
    ImportTask *task =new ImportTask(photo,m_database,&m_pool);

    m_pool.start(task);
}
void ScannerController::generateMissingThumbnails(const QString &rootFolder)
{
    QList<PhotoRecord> photos;

    QMetaObject::invokeMethod(m_database,[&](){photos = m_database->getPhotosWithoutThumbnail(rootFolder);},Qt::BlockingQueuedConnection);
    emit status(QString("Найдено %1 фотографий без миниатюр").arg(photos.size()));

    for (const PhotoRecord &photo : photos){
        m_pool.start(new ThumbnailTask(photo,m_database));
    }
}*/
void ScannerController::databaseConnected(bool ok)
{
    if (!ok) {
        emit status("Database connection failed");
        return;
    }
    emit status("Database connected. Loading photo tree...");
    loadTree();
}
void ScannerController::photoTreeLoaded(const QList<PhotoRecord> &photos)
{
    if (!m_photoTree) return;
//    m_photoTree->buildTree(photos);
    emit status(QString("Photo tree loaded: %1 files").arg(photos.size()));
}
/*void ScannerController::treeItemExpanded(int type, QString media, QString path)
{
    switch (type){
//    case PhotoTreeItem::Media:
//    {
//      if (m_loadedMedia.contains(media)) return;
//        m_loadedMedia.insert(media);
//        m_repository->loadFolders(media);
//        break;
//    }

    case PhotoTreeItem::Folder:
    {
//        QString key = media + "|" + path;
//        if (m_loadedFolders.contains(key)) return;
//        m_loadedFolders.insert(key);
        m_repository->loadPhotos(media, path);
        break;
    }
    default:
        break;
    }
}*/
