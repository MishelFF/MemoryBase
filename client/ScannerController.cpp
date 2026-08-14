#include <QFileInfo>
#include <QMetaObject>
#include <QDebug>
#include <QTimer>
#include <QImageReader>
#include <QDir>

#include "ImportTask.h"
#include "DatabaseWorker.h"
#include "PhotoTreeModel.h"
#ifdef NEED_LICENSE
#include "LicenseManager.h"
#endif
#include "ScannerController.h"
#include "PersonsModel.h" 
#include "FaceRegionsModel.h"
#include "PhotoFilter.h"
#include "PhotoSearchResultsModel.h"


ScannerController::ScannerController(PhotoRepository *repository,SettingsManager* settings,QObject *parent): 
QObject(parent), m_repository(repository),m_settings(settings)
{
    m_photoTree = new PhotoTreeModel(this);
    
    m_repositoryThread = new QThread(this);
    m_repository->moveToThread(m_repositoryThread);
    connect(m_repositoryThread, &QThread::finished, m_repository, &QObject::deleteLater);
    m_repositoryThread->start();
    m_personsModel = new PersonsModel(this);
    m_unresolvedRegionsModel = new FaceRegionsModel(true,this);
    m_personRegionsModel = new FaceRegionsModel(false,this);
    m_searchResultsModel = new PhotoSearchResultsModel(this);


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
    connect(m_repository,&PhotoRepository::mediaMountsLoaded,this,&ScannerController::mediaMountsLoaded);

    connect(m_personsModel, &PersonsModel:: loadPersons,this, &ScannerController::loadPersons);
    connect(m_repository, &PhotoRepository::personsLoaded,m_personsModel, &PersonsModel::onPersonsLoaded);
    connect(m_repository, &PhotoRepository::personCreated, m_personsModel, &PersonsModel::onPersonCreated);
    connect(m_personRegionsModel, &FaceRegionsModel::loadRegionsForPerson,this, &ScannerController::loadRegionsForPerson);
    connect(m_unresolvedRegionsModel, &FaceRegionsModel::loadUnresolvedRegions,this, &ScannerController::loadUnresolvedRegions);
    connect(m_repository, &PhotoRepository::unresolvedRegionsLoaded,m_unresolvedRegionsModel, &FaceRegionsModel::onUnresolvedRegionsLoaded);
    connect(m_repository, &PhotoRepository::personRegionsLoaded,m_personRegionsModel, &FaceRegionsModel::onPersonRegionsLoaded);
    connect(m_repository, &PhotoRepository::regionAssigned,m_unresolvedRegionsModel, &FaceRegionsModel::onRegionAssigned);
    connect(m_repository, &PhotoRepository::regionAssigned,m_personRegionsModel, &FaceRegionsModel::onRegionAssigned);
    connect(m_repository, &PhotoRepository::regionUnassigned,m_unresolvedRegionsModel, &FaceRegionsModel::onRegionUnassigned);
    connect(m_repository, &PhotoRepository::regionUnassigned,m_personRegionsModel, &FaceRegionsModel::onRegionUnassigned);
    connect(m_repository, &PhotoRepository::photosFound, this, [this](QList<PhotoRecord> photos) { m_searchResultsModel->setPhotos(photos);});//    connect(this,&ScannerController::requestPhotos,m_photoTree,&PhotoTreeModel::requestPhotos);
    //emit connectrepository(m_settings);
    supportedFormats = QImageReader::supportedImageFormats();
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

void ScannerController::selectPhotoInternal(int id, NavigationSource source)
{
    m_navigationSource = source;
    QMetaObject::invokeMethod(m_repository, [this, id]() { m_repository->loadPhoto(id); }, Qt::QueuedConnection);
 
}
void ScannerController::selectPhoto(int id)
{
    selectPhotoInternal(id, NavigationSource::Tree);
}
void ScannerController::selectSearchResult(int index)
{
    const int id = m_searchResultsModel->idAt(index);
    if (id < 0) return;
    m_searchListIndex = index;
    selectPhotoInternal(id, NavigationSource::SearchList);
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
    updateNavigationNeighbors();
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
    m_knownMedia = media;
    emit knownMediaChanged();
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
    QMetaObject::invokeMethod(m_repository, [this, media, path](){m_repository->loadPhotos(media, path);}, Qt::QueuedConnection);    
}
void ScannerController::photosLoaded(QList<PhotoRecord> photos)
{
    if(photos.isEmpty()) return;
    QString media=photos.first().mediaName;
    QString path=photos.first().path;
    m_photoTree->clearDummy(media,path);
    m_photoTree->addPhotos(photos.first().mediaName,photos.first().path,photos);
    emit photoInFolderLoaded();
}
void ScannerController::loadMediaMounts()
{
    QMetaObject::invokeMethod(m_repository, &PhotoRepository::loadMediaMounts, Qt::QueuedConnection);
}
void ScannerController::mediaMountsLoaded(QVariantList mounts)
{
    m_mediaMounts = mounts; // используется в mountPointFor(media)

    QSet<QString> unique;
    QSet<QString> uniqueME;
    for (const QString &m: m_knownMedia) uniqueME.insert(m);
    for (const QVariant &v : mounts) {
        const QString mp = v.toMap().value("mountPoint").toString();
        const QString me = v.toMap().value("media").toString();
        if (!mp.isEmpty())
            unique.insert(mp);
        if (!me.isEmpty())
            uniqueME.insert(me);
    }
    m_knownMountPoints = QStringList(unique.begin(), unique.end());
    m_knownMountPoints.sort(Qt::CaseInsensitive);
    m_knownMedia=QStringList(uniqueME.begin(), uniqueME.end());
    m_knownMedia.sort(Qt::CaseInsensitive);
    emit knownMountPointsChanged();
    emit knownMediaChanged();
}

void ScannerController::reConnected()
{
    emit connectrepository(m_settings);
}


void ScannerController::scanFolder(const QString &media, const QString &mountPoint,const QString &folder)
{
    // Импорт с диска поддерживается только для локальной БД
    auto *dbWorker = qobject_cast<DatabaseWorker*>(m_repository);
    if (!dbWorker) {
        emit status("Импорт с диска недоступен в режиме API");
        return;
    }
    QMetaObject::invokeMethod(dbWorker, [dbWorker, media, mountPoint](){dbWorker->saveMountPoint(media, mountPoint);}, Qt::QueuedConnection);
    
    emit importRunningChanged();
    m_importRunning = true;

    QDir mountDir(mountPoint);
    QString folderRelative = mountDir.relativeFilePath(QDir::fromNativeSeparators(folder));
    QString cachePathFilter = (folderRelative.isEmpty() || folderRelative == ".")? "/" : "/" + folderRelative;
    QMetaObject::invokeMethod(m_repository, [this, media,cachePathFilter,dbWorker](){
        dbWorker->loadCache(media,cachePathFilter, &(this->m_cache));
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
    int existCount=0;
    auto makePhotoRecord = [this, &mountDir, &media ](const QString &filename,PhotoRecord &photo) {
        QFileInfo info(filename);
        QString relativeDir = mountDir.relativeFilePath(info.absolutePath());
        QString normalizedPath = (relativeDir.isEmpty() || relativeDir == ".") ? "/" : "/" + relativeDir;
        FileKey key;
        key.path = normalizedPath;
        key.file = info.fileName();
        key.size = info.size();
        key.modified = info.lastModified();
        bool result=!m_cache.contains(key);
        if (result) {
            photo.file = info.fileName();
            photo.path = normalizedPath;
            photo.fullPath = filename;
            photo.fileSize = info.size();
            photo.dateAvailable = info.lastModified();
            photo.extension = info.suffix().toLower();
            photo.mediaName = media;
        } 
        return result;
    };
    bool shortTasks=USESHORTTASKS;
    if (shortTasks){
        int index = 0;
        for (const QString &filename : filenames) {
            PhotoRecord photo;
            if ( makePhotoRecord(filename,photo)){
                bool shouldReport =(index % m_reportInterval == 0) ||(index == m_totalFiles - 1);
                enqueueImport(photo, shouldReport);
            } 
            else {incrementProcessed();++existCount;}   
            ++index;
        }
    }
    else{
        const int threadCount = qMax(1, m_pool.maxThreadCount());
        const int chunkSize = qMax(500, m_totalFiles / (threadCount * 4));
        int index = 0;
        int flag=chunkSize;
        PhotoChunk chunk;
        chunk.reserve(flag);
        for (const QString &filename : filenames) {
            PhotoRecord photo;
            if ( makePhotoRecord(filename,photo)){
                chunk.push_back(photo);--flag;
            } 
            else {incrementProcessed();++existCount;}   
            if(!flag ||index==(filenames.size()-1)){ 
                
                
                m_pool.start(new ImportComplexTask(chunk, dbWorker, this, THUMB_SIZE, m_reportInterval));
//                  auto task = new ImportComplexTask(chunk, dbWorker, this, THUMB_SIZE, m_reportInterval);
//                  task->run();                
                flag=chunkSize;chunk.clear();
            }
            ++index;
        }
    }

    emit importProgressChanged();
    emit status(QString("Добавлено файлов: %1, уже загружены: %2").arg(m_totalFiles).arg(existCount));
}


void ScannerController::enqueueImport(const PhotoRecord &photo, bool reportProgress)
{
    auto *dbWorker = qobject_cast<DatabaseWorker*>(m_repository);
    if (!dbWorker) return;

    ImportTask *task = new ImportTask(photo, dbWorker, &m_pool, this, reportProgress);
//    task->run();
//    delete task;
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

void ScannerController::generateMissingThumbnails(const QString &media, const QString &mountPoint,const QString &rootFolder) {
    
    // Импорт с диска поддерживается только для локальной БД
    auto *dbWorker = qobject_cast<DatabaseWorker*>(m_repository);
    if (!dbWorker) {
        emit status("Импорт с диска недоступен в режиме API");
        return;
    }
    QList<PhotoRecord> photos;

    QMetaObject::invokeMethod(dbWorker, [&](){ photos = dbWorker->getPhotosWithoutThumbnail(media, mountPoint, rootFolder);}, Qt::BlockingQueuedConnection); 
    QList<PhotoRecord> imagePhotos;
    imagePhotos.reserve(photos.size());
    int skipped = 0;
    for (const PhotoRecord &photo : photos) {
        if (isSupportedImage(photo))
            imagePhotos.append(photo);
        else
            ++skipped;
    }

    m_totalFiles = photos.size()-skipped;
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
  //  qDebug()<<QString("Change progress on form %1").arg(m_processedFiles.loadRelaxed());
    if (m_processedFiles.loadRelaxed() >= m_totalFiles) {
        m_importRunning = false;
        emit importRunningChanged();
        if (!m_missingFilesCount)
            emit status(QString("Проверка завершена. найдено отсутствие %1 файлов").arg(m_missingFilesCount));
        else
            emit status("Обработка завершена");
    }
}
void ScannerController::incrementProcessed()
{
    m_processedFiles.fetchAndAddRelaxed(1);
//    qDebug()<<QString("Iterate progress counter %1").arg(m_processedFiles.loadRelaxed());
}
void ScannerController::databaseConnected(bool ok)
{
#ifdef NEED_LICENSE
    m_licenseManager->checkLicense();
#endif
    if (!ok) {
        emit status("Database connection failed");
        return;
    }
    emit status("Database connected. Loading photo tree...");
    loadTree();
    loadMediaMounts();
    m_personsModel->refresh();
    loadUnresolvedRegions();
}
void ScannerController::photoTreeLoaded(const QList<PhotoRecord> &photos)
{
    if (!m_photoTree) return;
//    m_photoTree->buildTree(photos);
    emit status(QString("Photo tree loaded: %1 files").arg(photos.size()));
}

QVariantList ScannerController::photoInfo() const 
{
    QVariantList list;
    auto addRow = [&list](const QString &label, const QVariant &value) {
        QVariantMap row;
        row["label"] = label;
        row["value"] = value.toString();
        list.append(row);
    };
    addRow("Имя", m_selectedPhoto.file);
    addRow("Дата", m_selectedPhoto.dateCreation.toString("dd.MM.yyyy hh:mm"));
    addRow("Размер", QString("%1 КБ").arg(m_selectedPhoto.fileSize / 1024));
    addRow("Камера", m_selectedPhoto.device);
    addRow("Производитель", m_selectedPhoto.maker);
    addRow("Ширина", m_selectedPhoto.width > 0 ? QString::number(m_selectedPhoto.width) : "");
    addRow("Высота", m_selectedPhoto.height > 0 ? QString::number(m_selectedPhoto.height) : "");
    addRow("MD5", m_selectedPhoto.md5);
    addRow("GPS", (m_selectedPhoto.latitude != 0 || m_selectedPhoto.longitude != 0)?QString("%1, %2").arg(m_selectedPhoto.latitude, 0, 'f', 6).arg(m_selectedPhoto.longitude, 0, 'f', 6):"");
    return list;
}
QString ScannerController::mountPointFor(const QString &media) const
{
    for (const QVariant &v : m_mediaMounts) {
        QVariantMap m = v.toMap();
        if (m["media"].toString() == media)
            return m["mountPoint"].toString();
    }
    return QString();
}
void ScannerController::findMissingFiles(const QString &media, const QString &mountPoint, const QString &folder)
{
    auto *dbWorker = qobject_cast<DatabaseWorker*>(m_repository);
    if (!dbWorker) {
        emit status("Недоступно в режиме API");
        return;
    }

    QDir mountDir(mountPoint);
    QString relative = mountDir.relativeFilePath(QDir::fromNativeSeparators(folder));
    QString relPath = (relative.isEmpty() || relative == ".") ? "/" : "/" + relative;

    QList<PhotoRecord> entries;
    QMetaObject::invokeMethod(dbWorker, [&](){entries = dbWorker->loadPathEntries(media, relPath);}, Qt::BlockingQueuedConnection);

    m_missingFilesText.clear();
    m_missingFilesCount=0;
    emit missingFilesTextChanged();;

    m_totalFiles = entries.size();
    m_processedFiles.storeRelaxed(0);
    m_reportInterval = qMax(1, m_totalFiles / 200);
    m_importRunning = !entries.isEmpty();
    emit importRunningChanged();
    emit importProgressChanged();
    emit status(QString("Проверка %1 записей на наличие файлов...").arg(entries.size()));
    const int threadCount = qMax(1, m_pool.maxThreadCount());
    const int chunkSize = qMax(500, m_totalFiles / (threadCount * 4));

    int index = 0;int flag=chunkSize;
    PhotoChunk chunk;
    chunk.reserve(flag);
    for (const PhotoRecord &entry : entries) {
        chunk.push_back(entry);--flag;
        if(!flag ||index==(entries.size()-1)){ 
//            bool shouldReport = (index % m_reportInterval == 0) || (index == m_totalFiles - 1);
            m_pool.start(new MissingFileTask(chunk, mountPoint, dbWorker, this, true));
            flag=chunkSize;chunk.clear();
        }
        ++index;
    }
}

//void ScannerController::missingFileFound(int id, const QString &path, const QString &file)
//{
//    emit missingFilesChanged();
//}
PhotoTreeItem * ScannerController::findNeighborLevelDown(PhotoTreeItem *item,int increment) 
{
    PhotoTreeItem *result=nullptr;
    if (!item->photosLoaded){

        QEventLoop loop;
        bool success = false;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true); 
        QMetaObject::Connection dataConn = connect(this, &ScannerController::photoInFolderLoaded, 
            [&loop, &timeoutTimer, &success]() {success = true;timeoutTimer.stop();loop.quit();});
        QMetaObject::Connection timeoutConn = connect(&timeoutTimer, &QTimer::timeout,[&loop]() {loop.quit();});
        timeoutTimer.start(5000);
        loadPhotos(item->mediaName,  item->path.startsWith("/") ? item->path : ("/" + item->path));
        loop.exec();
        disconnect(dataConn);
        disconnect(timeoutConn);
    }
    if (item->photosLoaded){
        auto loopBody = [&](auto begin, auto end) {
            for (auto it = begin; it != end; ++it)
                if((*it)->type==PhotoTreeItem::Folder) {
                    PhotoTreeItem *child=findNeighborLevelDown(*it,increment);
                    if (child&&(child->type==PhotoTreeItem::Photo)){result=child;break;}
                }
                else if ((*it)->type==PhotoTreeItem::Photo && isSupportedImage((*it)->photo)) {result=*it;break;} //Пока не все файлы умеет
        };
        if (increment>0) {
            loopBody(item->children.begin(), item->children.end());
        } else {
            loopBody(item->children.rbegin(), item->children.rend());
        }
    }
    return result;
}
PhotoTreeItem * ScannerController::findNeighborLevelUp(PhotoTreeItem *item,int increment) 
{

    PhotoTreeItem *parentItem=item->parent;
    if (!parentItem) return nullptr;

    int neightborIndex=parentItem->children.indexOf(const_cast<PhotoTreeItem*>(item))+increment;
    while ((neightborIndex>=0)&&(neightborIndex<parentItem->children.size())){
        PhotoTreeItem *neightbor=parentItem->children[neightborIndex];
        if (neightbor&&neightbor->type==PhotoTreeItem::Folder){
            PhotoTreeItem *found=findNeighborLevelDown(neightbor,increment);
            if (found) return found;
        }
        else if (neightbor&&neightbor->type==PhotoTreeItem::Photo&&isSupportedImage(neightbor->photo)){
            return neightbor;
        }
        neightborIndex+=increment;
    }
    return findNeighborLevelUp(parentItem,increment);
}

void ScannerController::updateNavigationNeighbors()
{

    if (m_navigationSource == NavigationSource::SearchList) {
        const int count = m_searchResultsModel->rowCount();
        m_nextPhotoId = (m_searchListIndex + 1 < count) ? m_searchResultsModel->idAt(m_searchListIndex + 1) : -1;
        m_previousPhotoId = (m_searchListIndex - 1 >= 0)? m_searchResultsModel->idAt(m_searchListIndex - 1) : -1;
        m_nextPhotoFolder = m_selectedPhoto.path;
        m_previousPhotoFolder = m_selectedPhoto.path;
 
        emit navigationChanged();
        blockNavigation = false;
        return;
    }

    qDebug()<<"StartFindNeibors "<<m_selectedPhoto.id<<" - "<<m_selectedPhoto.path<< m_selectedPhoto.file;
    PhotoTreeItem *currentItem = m_photoTree->itemForPhoto(
        m_selectedPhoto.mediaName, m_selectedPhoto.path, m_selectedPhoto.file);

    if (!currentItem) {
        m_nextPhotoId = -1;
        m_previousPhotoId = -1;
        m_nextPhotoFolder.clear();
        m_previousPhotoFolder.clear();
        emit navigationChanged();
        return;
    }

    PhotoTreeItem *nextItem = findNeighborLevelUp(currentItem, +1);
    PhotoTreeItem *previousItem = findNeighborLevelUp(currentItem, -1);

    m_nextPhotoId = (nextItem && nextItem->type == PhotoTreeItem::Photo) ? nextItem->photo.id : -1;
    m_previousPhotoId = (previousItem && previousItem->type == PhotoTreeItem::Photo) ? previousItem->photo.id : -1;
    m_nextPhotoFolder = nextItem ? nextItem->path : QString();
    m_previousPhotoFolder = previousItem ? previousItem->path : QString();

    emit navigationChanged();
    blockNavigation=false;
    qDebug()<<"EndFindNeibors "<<m_nextPhotoId<<m_nextPhotoFolder<<" - "<<m_previousPhotoId<< m_previousPhotoFolder<<" ! " <<m_selectedPhoto.id;

}

void ScannerController::selectNextPhoto()
{
    if (blockNavigation) {qDebug()<<"Navigate before unblock";return;}
    if (m_nextPhotoId < 0){qDebug()<<"Right border"; return;}
    if (m_navigationSource == NavigationSource::Tree && m_nextPhotoFolder != m_selectedPhoto.path){
        qDebug()<<"Right boundary crossed";
        emit folderBoundaryCrossed(m_nextPhotoFolder);
    }
    if (m_navigationSource == NavigationSource::SearchList) ++m_searchListIndex;
    blockNavigation=true;
    qDebug()<<"selectPhoto";
    selectPhotoInternal(m_nextPhotoId, m_navigationSource);
}
void ScannerController::selectPreviousPhoto()
{
    if (blockNavigation) {qDebug()<<"Navigate before unblock";return;}
    if (m_previousPhotoId < 0){qDebug()<<"Left border"; return;}
    if (m_navigationSource == NavigationSource::Tree && m_previousPhotoFolder != m_selectedPhoto.path){
        qDebug()<<"Left boundary crossed";
        emit folderBoundaryCrossed(m_previousPhotoFolder);
    }
    if (m_navigationSource == NavigationSource::SearchList) --m_searchListIndex;
    blockNavigation=true;
    qDebug()<<"selectPhoto";
    selectPhotoInternal(m_previousPhotoId, m_navigationSource);
}
void ScannerController::appendMissingFiles(const QStringList &rows)
{
    if (rows.isEmpty())
        return;

    for (const QString &v : rows) {
        m_missingFilesText += v + "\n";
        ++m_missingFilesCount;
    }
    emit missingFilesTextChanged();
}
 
void ScannerController::loadPersons(){
    QMetaObject::invokeMethod(m_repository, [this](){ m_repository->loadPersons();}, Qt::QueuedConnection);
}
 
void ScannerController::createPerson(const QString &displayName)
{
    QMetaObject::invokeMethod(m_repository, [this, displayName](){ m_repository->createPerson(displayName);}, Qt::QueuedConnection);
}
void ScannerController::loadRegionsForPerson(int personId)
{   
    QMetaObject::invokeMethod(m_repository, [this, personId](){ m_repository->loadRegionsForPerson(personId);}, Qt::QueuedConnection);
}
void ScannerController::loadUnresolvedRegions()
{
    QMetaObject::invokeMethod(m_repository, [this](){ m_repository->loadUnresolvedRegions();}, Qt::QueuedConnection);
}
 
void ScannerController::assignRegionToPerson(int regionId, int personId)
{
    QMetaObject::invokeMethod(m_repository, [this, regionId, personId](){ m_repository->assignRegionToPerson(regionId, personId);}, Qt::QueuedConnection);
}
 
void ScannerController::setPersonReference(int personId, int regionId)
{
    QMetaObject::invokeMethod(m_repository, [this, personId, regionId](){ m_repository->setPersonReference(personId,regionId);}, Qt::QueuedConnection);
}
 
void ScannerController::unassignRegion(int regionId)
{
    QMetaObject::invokeMethod(m_repository, [this, regionId](){ m_repository->unassignRegion(regionId);}, Qt::QueuedConnection);
}
QImage ScannerController::loadChipImage(const QString &id, QSize *size, const QSize & /*requestedSize*/)
{

    QImage result;
    QSize localSize;

    
    // пока блокирующий запрос m_repositoryThread через BlockingQueuedConnection.
    QMetaObject::invokeMethod(m_repository, [this, id, &result, &localSize]() { result = m_repository->loadChipImage(id, &localSize, QSize());}, Qt::BlockingQueuedConnection);

    if (size) *size = localSize;

    return result;
}
void ScannerController::searchPhotos(const QVariantMap &filterMap)
{
    PhotoFilter filter;
    filter.limitEnabled = filterMap.value("limitEnabled").toBool();
    filter.maxCount = filterMap.value("maxCount").toInt();
    filter.mediaEnabled = filterMap.value("mediaEnabled").toBool();
    filter.media = filterMap.value("media").toStringList();
    filter.dateEnabled = filterMap.value("dateEnabled").toBool();
    filter.dateFrom = filterMap.value("dateFrom").toDateTime();
    filter.dateTo = filterMap.value("dateTo").toDateTime();
    filter.facesEnabled = filterMap.value("facesEnabled").toBool();
    for (const QVariant &v : filterMap.value("personIds").toList()) filter.personIds.append(v.toInt());
    filter.facesUseDescriptor = filterMap.value("facesUseDescriptor").toBool();
    filter.similarityThreshold = filterMap.value("similarityThreshold", 0.6).toDouble();
    filter.sortBy = (filterMap.value("sortBy").toString() == QLatin1String("matchCount"))? PhotoFilter::SortBy::FaceMatchCount: PhotoFilter::SortBy::Date;
     QMetaObject::invokeMethod(m_repository, [this, filter](){m_repository->searchPhotos(filter);}, Qt::QueuedConnection);
}