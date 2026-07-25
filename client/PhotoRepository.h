#pragma once

#include <QObject>
#include <QList>
#include "settingsmanager.h"
#include "PhotoRecord.h"


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

signals:

    void connected(bool ok);
    void mediaLoaded(QStringList media);
    void foldersLoaded(QString media,QStringList folders);
    void photosLoaded(QList<PhotoRecord> photos);
    void photoLoaded(PhotoRecord photo);
    void error(QString message);
    void status(QString);
    void mediaMountsLoaded(QVariantList mounts);   
};