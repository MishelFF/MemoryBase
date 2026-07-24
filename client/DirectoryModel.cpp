#include "DirectoryModel.h"

#include <QDir>
#include <QDebug>

DirectoryModel::DirectoryModel(QObject *parent)
    : QFileSystemModel(parent)
{
    setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
}

QString DirectoryModel::filePathFromIndex(const QModelIndex &index) const
{
    return filePath(index);
}
QModelIndex DirectoryModel::indexFromPath(const QString &path) const
{
    return index(path);
}
void DirectoryModel::setRootFolder(const QString &url)
{
    QString path = url;

    if (path.startsWith("file:///"))
        path.remove(0, 8);

    path.replace("/", "\\");

    qDebug() << "Folder:" << path;

    QModelIndex idx = setRootPath(path);

    emit layoutChanged();
}
