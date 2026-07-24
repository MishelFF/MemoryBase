#pragma once

#include <QFileSystemModel>

class DirectoryModel : public QFileSystemModel
{
    Q_OBJECT

public:
    explicit DirectoryModel(QObject *parent = nullptr);

    Q_INVOKABLE QString filePathFromIndex(const QModelIndex &index) const;
    Q_INVOKABLE QModelIndex indexFromPath(const QString &path) const;
    Q_INVOKABLE void setRootFolder(const QString &path);
};