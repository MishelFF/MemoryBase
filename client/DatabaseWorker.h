#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QHash>

#include "PhotoRecord.h"
#include "FileKey.h"
#include "PhotoRepository.h"

class DatabaseWorker : public PhotoRepository
{
    Q_OBJECT

public:
    explicit DatabaseWorker(QObject *parent = nullptr);
    ~DatabaseWorker();

    QList<PhotoRecord> getPhotosWithoutThumbnail(const QString &rootFolder);
//    QList<PhotoRecord> loadPhotoTree();
    int insertPhoto(PhotoRecord &photo);
    bool updateExif(const PhotoRecord &photo);
    bool updateMD5(const PhotoRecord &photo);
    bool insertThumbnail(const PhotoRecord &photo);

public slots:
    void open(SettingsManager *rpSettings) override;
    void loadMedia() override;
    void loadFolders(const QString &mediaName) override;
    void loadPhotos(const QString &mediaName,const QString &path) override;
    void loadPhoto(int id) override;
    void close();

//    bool exists(const FileKey &key);


//    void beginTransaction();
//    void commit();
//    void open(const QString &host,int port,const QString &database,const QString &user,const QString &password);
//    void loadPhotoTreeAsync();
//    void loadPhoto(int id);

private:

    QSqlDatabase db;
//    bool connectDatabase();
    void loadCache();
    FileCache* m_cache;    
   
};