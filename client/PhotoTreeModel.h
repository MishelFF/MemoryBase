#pragma once

#include <QAbstractItemModel>
#include <QVector>
#include <QDir>
#include <QDebug>

#include "PhotoRecord.h"

class PhotoTreeItem
{
public:

    enum ItemType {Root,Media,Folder,Photo, Dummy};

    explicit PhotoTreeItem(ItemType t,const QString &n,const QString &p,const QString &media,
        QHash<QString,PhotoTreeItem*>* cIndex,PhotoTreeItem *parentItem  = nullptr):
        type(t),name(n),mediaName(media),path(p),parent(parentItem),childIndex(cIndex)
    {
        if (parent) {
            parent->children.append(this);
            QString key;
            switch (type) {
            case Folder:
                key = path; // полный путь папки
                break;
            case Photo:
                key = path + "/" + name; // полный путь файла
                break;
            default:
                key = name; // Media и Dummy
                break;
            }
            childIndex->insert(key, this);
        }
    }
    
    ~PhotoTreeItem()
    {
        qDeleteAll(children);
    }
    PhotoTreeItem *findChild(const QString &childName)
    {
//        for (PhotoTreeItem *item : children)
//            if (item->type == childType && item->name == childName) return item;
//        return nullptr;
        return childIndex->value(childName,nullptr);    
    }
//    PhotoTreeItem *addChild(ItemType type,const QString &name)
//    {
//        PhotoTreeItem *item = new PhotoTreeItem(type, name, this);
//        children.append(item);
//        parent->childIndex.insert(name, this);
//        return item;
//    }
public:
    ItemType type = Root;
    QString name;
    QString mediaName;
    QString path;
    PhotoRecord photo;
    PhotoTreeItem *parent = nullptr;
    QVector<PhotoTreeItem *> children;
    QHash<QString, PhotoTreeItem*> *childIndex;
    bool photosLoaded = false;
};


class PhotoTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Roles
    {
        IdRole = Qt::UserRole + 1,
        TypeRole,
        FullPathRole,
        MediaRole
    };
    explicit PhotoTreeModel(QObject *parent = nullptr);
    ~PhotoTreeModel() override;
    QModelIndex index(int row,int column,const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int,QByteArray> roleNames() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    //-------------------------------------------------
    // Построение дерева
    //-------------------------------------------------
    void clear();
    //void buildTree(const QList<PhotoRecord> &photos);
    Q_INVOKABLE int photoId(const QModelIndex &index) const;// Получение записи фотографии
    void addMedia(const QStringList &media);
    void addFolders(const QString &media,const QStringList &folders);
    void addPhotos(const QString &media,const QString &path,const QList<PhotoRecord> &photos);
     Q_INVOKABLE void expand(const QModelIndex &index);
signals:
    void requestFolders(QString media);
    void requestPhotos(QString media,QString path);
private:
    PhotoTreeItem *itemFromIndex(const QModelIndex &index) const;
    PhotoTreeItem *m_root = nullptr;
    QHash<QString, PhotoTreeItem*> childIndex;
    PhotoTreeItem *findChild(const QString &childName);
    QModelIndex indexFromItem(PhotoTreeItem *item) const;

};
