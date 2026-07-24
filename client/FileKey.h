#pragma once

#include <QString>
#include <QDateTime>
#include <QMutex>
#include <QHash>

struct FileKey
{
    QString path;
    QString file;

    qint64 size;

    QDateTime modified;

    bool operator==(const FileKey& other) const
    {
        return path==other.path &&
               file==other.file &&
               size==other.size &&
               modified==other.modified;
    }
};
inline size_t qHash(
    const FileKey &k,
    size_t seed=0)
{
    return qHash(k.path,seed)
         ^ qHash(k.file)
         ^ qHash(k.size)
         ^ qHash(k.modified);
}

using TimgFilesHash = QHash<FileKey,int>;


class FileCache
{

public:

    bool contains(const FileKey& key)
    {
        QMutexLocker lock(&mutex);

        return cache.contains(key);
    }


    void insert(const FileKey& key,int id)
    {
        QMutexLocker lock(&mutex);

        cache.insert(key,id);
    }


    void clear()
    {
        QMutexLocker lock(&mutex);

        cache.clear();
    }

    int size()
    {

        return cache.size();
    }

private:

    QMutex mutex;

    TimgFilesHash cache;
};