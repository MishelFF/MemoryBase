#pragma once

#include <QRunnable>
#include <QMetaObject>
#include <QThreadPool>

#include "PhotoRecord.h"
#include "ScannerController.h"

class DatabaseWorker;

class ImportTask : public QRunnable
{
public:

    ImportTask(
            const PhotoRecord &photo,
            DatabaseWorker *database,
            QThreadPool *pool);

    void run() override;

private:

    PhotoRecord m_photo;

    DatabaseWorker *m_database;

    QThreadPool *m_pool;
};

class ExifTask : public QRunnable
{
public:

    ExifTask(
            const PhotoRecord &photo,
            DatabaseWorker *database);

    void run() override;

private:

    PhotoRecord m_photo;

    DatabaseWorker *m_database;
};

class Md5Task : public QRunnable
{
public:

    Md5Task(
            const PhotoRecord &photo,
            DatabaseWorker *database);

    void run() override;

private:

    PhotoRecord m_photo;

    DatabaseWorker *m_database;
};

class ThumbnailTask : public QRunnable
{
public:

    ThumbnailTask(
            const PhotoRecord &photo,
            DatabaseWorker *database,
            int size = THUMB_SIZE);

    void run() override;

private:

    PhotoRecord m_photo;

    DatabaseWorker *m_database;

    int m_size;
};
