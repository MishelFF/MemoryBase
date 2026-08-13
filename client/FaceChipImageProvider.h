#pragma once
#include <QQuickImageProvider>

class ScannerController;

class FaceChipImageProvider : public QQuickImageProvider
{
public:
    explicit FaceChipImageProvider(ScannerController *controller)
        : QQuickImageProvider(QQuickImageProvider::Image)
        , m_controller(controller)
    {}

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        if (!m_controller)
            return QImage();
        return m_controller->loadChipImage(id, size, requestedSize);
    }

private:
    ScannerController *m_controller = nullptr;
};