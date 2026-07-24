#include "PhotoTreeModel.h"

PhotoTreeModel::PhotoTreeModel(QObject *parent) : QAbstractItemModel(parent)
{
    m_root = new PhotoTreeItem(PhotoTreeItem::Root,QString(),QString(),QString(),&childIndex,nullptr);
}

PhotoTreeModel::~PhotoTreeModel()
{
    delete m_root;
    m_root = nullptr;
}

PhotoTreeItem *PhotoTreeModel::itemFromIndex(const QModelIndex &index) const
{
    if (index.isValid()) return static_cast<PhotoTreeItem *>(index.internalPointer());
    else return m_root;
}

QModelIndex PhotoTreeModel::index(int row, int column, const QModelIndex &parent) const 
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    PhotoTreeItem *parentItem = itemFromIndex(parent);
    if (row < 0 || row >= parentItem->children.size()) return QModelIndex();
    PhotoTreeItem *child = parentItem->children[row];
    return createIndex(row, column, child);
}

QModelIndex PhotoTreeModel::parent(const QModelIndex &index) const 
{
    if (!index.isValid()) return QModelIndex();
    PhotoTreeItem *child = itemFromIndex(index);
    PhotoTreeItem *parent = child->parent;
    if (parent == nullptr || parent == m_root) return QModelIndex();
    PhotoTreeItem *grand = parent->parent;
    int row = 0;
    if (grand) row = grand->children.indexOf(parent);
    return createIndex(row, 0, parent);
}

int PhotoTreeModel::rowCount(const QModelIndex &parent) const
{
    return itemFromIndex(parent)->children.size();
}


int PhotoTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant PhotoTreeModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();
    PhotoTreeItem *item = itemFromIndex(index);
    switch (role) {
    case Qt::DisplayRole:
        return item->name;
    case IdRole:
        if (item->type == PhotoTreeItem::Photo) return item->photo.id; else return -1;
    case TypeRole:
        return static_cast<int>(item->type);
    case FullPathRole:
        if (item->type == PhotoTreeItem::Photo) return item->photo.fullPath;else return QString();
    case MediaRole:
        if (item->type == PhotoTreeItem::Photo) return item->photo.mediaName;else return QString();
    default:
        return QVariant();
    }
}


QHash<int,QByteArray> PhotoTreeModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "display";
    roles[IdRole] = "id";
    roles[TypeRole] = "type";
    roles[FullPathRole] = "fullPath";
    roles[MediaRole] = "media";
    return roles;
}


Qt::ItemFlags PhotoTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

PhotoTreeItem *PhotoTreeModel::findChild(const QString &childName)
{
        return childIndex.value(childName,nullptr);    
}

int PhotoTreeModel::photoId(const QModelIndex &index) const
{
    if (!index.isValid()) return -1;
    PhotoTreeItem *item =itemFromIndex(index);
    if (item->type != PhotoTreeItem::Photo) return -1;
    return item->photo.id;
}

void PhotoTreeModel::clear()
{
    beginResetModel();
    delete m_root;
    m_root = new PhotoTreeItem(PhotoTreeItem::Root, QString(), QString(), QString(),&childIndex, nullptr);
    endResetModel();
}

void PhotoTreeModel::expand(const QModelIndex &index)
{
    PhotoTreeItem *item =itemFromIndex(index);
    if(!item) return;
    
    if((item->type==PhotoTreeItem::Folder)  && !item->photosLoaded){
        emit requestPhotos(item->mediaName,  item->path.startsWith("/") ? item->path : ("/" + item->path));
    }
}

void PhotoTreeModel::addMedia(const QStringList &media)
{
    beginInsertRows(QModelIndex(),m_root->children.count(), m_root->children.count() + media.count() - 1);
    for (const QString &name : media) {
        auto *item = new PhotoTreeItem(PhotoTreeItem::Media,name,QString(),name,&childIndex,m_root);
    }
    endInsertRows();
}
void PhotoTreeModel::addFolders(const QString &media, const QStringList &folders) {
    
    PhotoTreeItem *mediaItem = m_root->findChild(media);
    if (!mediaItem) return;
    beginInsertRows(indexFromItem(mediaItem), mediaItem->children.count(),folders.count());
    for (const QString &fullPath : folders) {
        QString path = QDir::cleanPath(fullPath);
        QStringList parts = path.split('/', Qt::SkipEmptyParts);
        PhotoTreeItem *parent = mediaItem;
        QString currentPath;
        for (const QString &part : parts) {
            //if (!currentPath.isEmpty()) 
                currentPath += '/';
            currentPath += part;
            PhotoTreeItem *item = findChild(currentPath);
            if (!item) {
                item = new PhotoTreeItem(PhotoTreeItem::Folder, part,currentPath,media,&childIndex,parent);
                new PhotoTreeItem(PhotoTreeItem::Dummy, QString(),currentPath,media,&childIndex, item);
            }
            parent = item;
        }
    }
    endInsertRows();
}
QModelIndex PhotoTreeModel::indexFromItem(PhotoTreeItem *item) const
{
    if (!item || item == m_root)
        return QModelIndex();

    PhotoTreeItem *parent = item->parent;
    if (!parent) return QModelIndex();
    int row = parent->children.indexOf(item);
    if (row < 0) return QModelIndex();

    return createIndex(row, 0, item);
}
void PhotoTreeModel::addPhotos(const QString &media, const QString &path, const QList<PhotoRecord> &photos) {
    
    PhotoTreeItem *folderItem = findChild(path);
    if (!folderItem) return;
    if (folderItem->photosLoaded) return;
    
//    if (folderItem->children.size()> 0 && folderItem->children.first()->type == PhotoTreeItem::Dummy){
//        childIndex.remove(folderItem->children.takeFirst()->name);
//        delete folderItem->children.takeFirst();
//    }
    int first = folderItem->children.size();
    int last  = first + photos.size() - 1;
    beginInsertRows(indexFromItem(folderItem), first, last);
    for (const PhotoRecord &photo : photos) {
        if (!findChild(photo.path+"/"+photo.file)) {
            PhotoTreeItem *item =new PhotoTreeItem(PhotoTreeItem::Photo,photo.file, photo.path,  photo.mediaName,&childIndex,folderItem);
            item->photo = photo;
        }
    }
    folderItem->photosLoaded = true;

    endInsertRows();
}
/*void PhotoTreeModel::buildTree(
        const QList<PhotoRecord> &photos)
{
    beginResetModel();
    delete m_root;
    m_root = new PhotoTreeItem(PhotoTreeItem::Root,QString(),nullptr);

    for (const PhotoRecord &photo : photos) {
        PhotoTreeItem *media = m_root->findChild(photo.mediaName,PhotoTreeItem::Media);
        if (!media){
            media = new PhotoTreeItem(PhotoTreeItem::Media,photo.mediaName,m_root);
        }
        PhotoTreeItem *parent = media;
        QString path = QDir::cleanPath(photo.path);
        QStringList folders = path.split('/',Qt::SkipEmptyParts);

        for (const QString &folder : folders){
            PhotoTreeItem *item = parent->findChild(folder,PhotoTreeItem::Folder);
            if (!item) {
                item = new PhotoTreeItem( PhotoTreeItem::Folder,folder,parent);
            }
            parent = item;
        }

        PhotoTreeItem *image = new PhotoTreeItem(PhotoTreeItem::Photo,photo.file,parent);
        image->photo = photo;
    }
    endResetModel();
}*/