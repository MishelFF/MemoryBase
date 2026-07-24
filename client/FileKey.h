#pragma once

#include <QString>
#include <QDateTime>
//#include <QMutex>
#include <QHash>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>

struct FileKey {
    QString path;
    QString file;
    qint64 size;
    QDateTime modified;
    bool operator==(const FileKey &other) const {
        return path == other.path && file == other.file && size == other.size && modified == other.modified;
    }
};
inline size_t qHash(const FileKey &k, size_t seed = 0) {
    return qHash(k.path, seed) ^ qHash(k.file) ^ qHash(k.size) ^ qHash(k.modified);
}
using TimgFilesHash = QHash<FileKey, int>;
class FileCache {
  public:
    bool contains(const FileKey &key) {
        QReadLocker locker(&m_lock); 
//        QMutexLocker lock(&mutex);
        return cache.contains(key);
    }
    void insert(const FileKey &key, int id) {
        QWriteLocker locker(&m_lock);
//        QMutexLocker lock(&mutex);
        cache.insert(key, id);
    }
    void clear() {
        QWriteLocker locker(&m_lock);
//        QMutexLocker lock(&mutex);
        cache.clear();
    }
    int size() { return cache.size(); }

  private:
//    QMutex mutex;
    QReadWriteLock m_lock;
    TimgFilesHash cache;
};