#pragma once

#include <QPointer>
#include <QMetaObject>
#include <QRunnable>
#include <QThreadPool>
#include "PhotoRecord.h"
#include "ScannerController.h"

class DatabaseWorker;

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
class MissingFileTask : public QRunnable
{
public:
    MissingFileTask(const PhotoRecord &entry, const QString &mountPoint,
                     DatabaseWorker *database, ScannerController *controller, bool reportProgress);
    void run() override;

private:
    PhotoRecord m_photo;
    QString m_mountPoint;
    DatabaseWorker *m_database;
    QPointer<ScannerController> m_controller;
    bool m_reportProgress;
};