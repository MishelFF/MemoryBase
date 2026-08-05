#pragma once

#include <QPointer>
#include <QMetaObject>
#include <QRunnable>
#include <QThreadPool>
#include "PhotoRecord.h"
#include "ScannerController.h"

class DatabaseWorker;
typedef std::vector<PhotoRecord> PhotoChunk;

class ImportTask : public QRunnable {
  public:
    ImportTask(const PhotoRecord &photo, DatabaseWorker *database, QThreadPool *pool, ScannerController *controller, bool reportProgress);
    void run() override;

  private:
    PhotoRecord m_photo;
    DatabaseWorker *m_database;
    QThreadPool *m_pool;
    QPointer<ScannerController> m_controller;
    bool m_reportProgress;
};
class ExifTask : public QRunnable {
  public:
    ExifTask(const PhotoRecord &photo, DatabaseWorker *database);
    void run() override;

  private:
    PhotoRecord m_photo;
    DatabaseWorker *m_database;
};
class Md5Task : public QRunnable {
  public:
    Md5Task(const PhotoRecord &photo, DatabaseWorker *database);
    void run() override;

  private:
    PhotoRecord m_photo;
    DatabaseWorker *m_database;
};
class ThumbnailTask : public QRunnable {
  public:
    ThumbnailTask(const PhotoRecord &photo, DatabaseWorker *database, ScannerController *controller, bool reportProgress,int size = THUMB_SIZE);
    void run() override;

  private:
    PhotoRecord m_photo;
    DatabaseWorker *m_database;
    int m_size;
    QPointer<ScannerController> m_controller;
    bool m_reportProgress;
};
class ImportComplexTask : public QRunnable {
  public:
    ImportComplexTask(const PhotoChunk &photos, DatabaseWorker *database,  ScannerController *controller, int size, int reportFreq);
    void run() override;

  private:
    PhotoChunk m_photos;
    DatabaseWorker *m_database;
    QPointer<ScannerController> m_controller;
    int m_reportFreq=1;
    int m_size=THUMB_SIZE;
};
class MissingFileTask : public QRunnable
{
public:
    MissingFileTask(const PhotoChunk &photos, const QString &mountPoint,
                     DatabaseWorker *database, ScannerController *controller, bool reportProgress);
    void run() override;

private:
    PhotoChunk m_photos;
    QString m_mountPoint;
    DatabaseWorker *m_database;
    QPointer<ScannerController> m_controller;
    bool m_reportProgress;
};