#include <QFileInfo>
#include <QMetaObject>
#include <QDebug>
#include <QTimer>
#include <QImageReader>
#include "ImportTask.h"
#include "DatabaseWorker.h"
#include "PhotoTreeModel.h"
#include "ScannerController.h"

ScannerController::ScannerController(PhotoRepository *repository,SettingsManager* settings,QObject *parent): 
QObject(parent), m_repository(repository),m_settings(settings)
{
    m_photoTree = new PhotoTreeModel(this);
    
    m_repositoryThread = new QThread(this);
    m_repository->moveToThread(m_repositoryThread);
    connect(m_repositoryThread, &QThread::finished, m_repository, &QObject::deleteLater);
    m_repositoryThread->start();

    qDebug() << "Connecting repository:" << m_repository;
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
    QTimer::singleShot(0, this, [this](){
        this->reConnected(); 
    });
}


ScannerController::~ScannerController()
{
    m_repositoryThread->quit();
    m_repositoryThread->wait();
//    m_pool.waitForDone();
}
PhotoTreeModel* ScannerController::photoTree() const
{
    return m_photoTree;
}

void ScannerController::selectPhoto(int id)
{
    QMetaObject::invokeMethod(m_repository, [this, id]() { m_repository->loadPhoto(id); }, Qt::QueuedConnection);
}
void ScannerController::loadTree()
{
    m_photoTree->clear();
    QMetaObject::invokeMethod(m_repository, &PhotoRepository::loadMedia, Qt::QueuedConnection);
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
    QMetaObject::invokeMethod(m_repository, &PhotoRepository::loadMedia, Qt::QueuedConnection);
}
void ScannerController::mediaLoaded(QStringList media)
{
    m_photoTree->addMedia(media);
    for (const QString &name : media)
    {
//      m_loadedMedia.insert(name);
        QMetaObject::invokeMethod(m_repository, [this, name](){
         m_repository->loadFolders(name);
        }, Qt::QueuedConnection);       
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
    QMetaObject::invokeMethod(m_repository, [this, media](){
    m_repository->loadFolders(media);
    }, Qt::QueuedConnection);
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
    QMetaObject::invokeMethod(m_repository, [this, media, path](){
    m_repository->loadPhotos(media, path);
    }, Qt::QueuedConnection);    
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


void ScannerController::scanFolder(const QString &media,const QString &folder)
{
    // Импорт с диска поддерживается только для локальной БД
    auto *dbWorker = qobject_cast<DatabaseWorker*>(m_repository);
    if (!dbWorker) {
        emit status("Импорт с диска недоступен в режиме API");
        return;
    }

    emit importRunningChanged();
    m_importRunning = true;

    QMetaObject::invokeMethod(m_repository, [this, media,folder,dbWorker](){
        dbWorker->loadCache(media,folder, &(this->m_cache));
    },  Qt::BlockingQueuedConnection);
    QStringList filters;
    //filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp"<< "*.tif" << "*.tiff" << "*.webp"<<"*.gif"<<"*.wmf";
    filters << "*.*";

    QStringList filenames;
    QDirIterator it(folder, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) filenames << it.next();
    m_rootFolder = folder;
    m_totalFiles = filenames.size();
    m_processedFiles.storeRelaxed(0);
    m_reportInterval = qMax(1, m_totalFiles / 200);

    int index = 0;int existCount=0;
    for (const QString &filename : filenames) {
        QFileInfo info(filename);
        FileKey key;
        key.path = info.absolutePath();//убрать букву 
        key.file = info.fileName();
        key.size = info.size();        
        key.modified = info.lastModified();
        if (!m_cache.contains(key)) {
            PhotoRecord photo;
            photo.file = info.fileName();
            photo.path = QDir::fromNativeSeparators(info.absolutePath());
            photo.fullPath = filename;
            photo.fileSize = info.size();
            photo.lastModified = info.lastModified();
            photo.extension = info.suffix().toLower();
            bool shouldReport = (index % m_reportInterval == 0) || (index == m_totalFiles - 1);
            enqueueImport(photo, shouldReport);
        } 
        else {incrementProcessed();++existCount;}   
        ++index;
    }
    emit importProgressChanged();
    emit status(QString("Добавлено файлов: %1 уже существуют загружены %2").arg(m_totalFiles,existCount));
}


void ScannerController::enqueueImport(const PhotoRecord &photo, bool reportProgress)
{
    auto *dbWorker = qobject_cast<DatabaseWorker*>(m_repository);
    if (!dbWorker) return;

    ImportTask *task = new ImportTask(photo, dbWorker, &m_pool, this, reportProgress);
    m_pool.start(task);
}

bool isSupportedImage(const PhotoRecord &photo)
{
    static const QSet<QByteArray> supported = []{
        QSet<QByteArray> set;
        for (const QByteArray &fmt : QImageReader::supportedImageFormats())
            set.insert(fmt.toLower());
        return set;
    }();

    QByteArray ext = photo.extension.toLower().toLatin1();
    if (!supported.contains(ext))
        return false;

    // QImageReader reader(photo.fullPath);
    // if (!reader.canRead())
    //     return false;

    return true;
}

void ScannerController::generateMissingThumbnails(const QString &media,const QString &rootFolder) {
    
    // Импорт с диска поддерживается только для локальной БД
    auto *dbWorker = qobject_cast<DatabaseWorker*>(m_repository);
    if (!dbWorker) {
        emit status("Импорт с диска недоступен в режиме API");
        return;
    }
    QList<PhotoRecord> photos;

    QMetaObject::invokeMethod(dbWorker, [&]() { photos = dbWorker->getPhotosWithoutThumbnail(media,rootFolder); }, Qt::BlockingQueuedConnection); 
    
    QList<PhotoRecord> imagePhotos;
    imagePhotos.reserve(photos.size());
    int skipped = 0;
    for (const PhotoRecord &photo : photos) {
        if (isSupportedImage(photo))
            imagePhotos.append(photo);
        else
            ++skipped;
    }

    m_totalFiles = photos.size();
    m_processedFiles = 0;
    m_importRunning = !photos.isEmpty();
    emit importRunningChanged();
    emit importProgressChanged();
    emit status(QString("Найдено %1 фото без миниатюр (пропущено неподходящих: %2)").arg(imagePhotos.size()).arg(skipped));

    int index = 0;
    for (PhotoRecord &photo : imagePhotos){
        photo.mediaName = media;
        bool shouldReport = (index % m_reportInterval == 0) || (index == m_totalFiles - 1);
        m_pool.start(new ThumbnailTask(photo, dbWorker,this,shouldReport));
        ++index;
    }
}
void ScannerController::onFileProcessed()
{
    emit importProgressChanged();
    if (m_processedFiles.loadRelaxed() >= m_totalFiles) {
        m_importRunning = false;
        emit importRunningChanged();
        emit status("Импорт завершён");
    }
}
void ScannerController::incrementProcessed()
{
    m_processedFiles.fetchAndAddRelaxed(1);
}
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
