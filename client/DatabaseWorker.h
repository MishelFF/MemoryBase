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

    QList<PhotoRecord> getPhotosWithoutThumbnail(const QString &media,const QString &mountPoint,const QString &rootFolder);
//    QList<PhotoRecord> loadPhotoTree();
    int insertPhoto(PhotoRecord &photo);
    bool updateExif(const PhotoRecord &photo);
    bool updateMD5(const PhotoRecord &photo);
    bool insertThumbnail(const PhotoRecord &photo);
    void loadCache(const QString &media,const QString &path,FileCache* m_cache);
    void markMissing(int id);
    void clearMissing(int id);
    QList<PhotoRecord> loadPathEntries(const QString &media, const QString &relativePath);
    QSet<int> loadMissingIds(const QString &media, const QString &relativePath);

public slots:
    void open(SettingsManager *rpSettings) override;
    void loadMedia() override;
    void loadFolders(const QString &mediaName) override;
    void loadPhotos(const QString &mediaName,const QString &path) override;
    void loadPhoto(int id) override;
    void close();
    void loadMediaMounts();                                            
    void saveMountPoint(const QString &media, const QString &mountPoint);  

//    bool exists(const FileKey &key);
//    void beginTransaction();
//    void commit();

private:

    QSqlDatabase db;
};