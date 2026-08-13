
#include <QBuffer>
#include <QImage>

#include "FaceRecognizer.h"

QByteArray encodeFaceChip(const dlib::matrix<dlib::rgb_pixel> &chip)
{
    const int w = int(chip.nc());
    const int h = int(chip.nr());

    QImage image(w, h, QImage::Format_RGB888);
    for (int y = 0; y < h; ++y) {
        uchar *line = image.scanLine(y);
        for (int x = 0; x < w; ++x) {
            const dlib::rgb_pixel &p = chip(y, x);
            line[x * 3 + 0] = p.red;
            line[x * 3 + 1] = p.green;
            line[x * 3 + 2] = p.blue;
        }
    }

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPG", 90);
    return bytes;
}
FaceRecognizer::FaceRecognizer(const QString& shapePredictorPath,const QString& resnetModelPath)
{
    m_detector = dlib::get_frontal_face_detector();
    dlib::deserialize(shapePredictorPath.toStdString()) >> m_shapePredictor;
    dlib::deserialize(resnetModelPath.toStdString()) >> m_net;
}

dlib::array2d<dlib::rgb_pixel> FaceRecognizer::qImageToDlib(const QImage& image)
{
    // dlib::rgb_pixel хранит байты в порядке R,G,B - приводим QImage к этому формату
    const QImage img = image.convertToFormat(QImage::Format_RGB888);

    dlib::array2d<dlib::rgb_pixel> dlibImage;
    dlibImage.set_size(img.height(), img.width());

    for (int y = 0; y < img.height(); ++y)
    {
        const uchar* line = img.scanLine(y);
        for (int x = 0; x < img.width(); ++x)
        {
            const uchar* px = line + x * 3;
            dlibImage[y][x] = dlib::rgb_pixel(px[0], px[1], px[2]);
        }
    }
    return dlibImage;
}
dlib::array2d<dlib::rgb_pixel> FaceRecognizer::bytesToDlib(const QByteArray &data) {
    QImage image;
    if (!image.loadFromData(data)) throw std::runtime_error("Failed to decode image");
    image = image.convertToFormat(QImage::Format_RGB888);
    dlib::array2d<dlib::rgb_pixel> result(image.height(), image.width());
    for (int y = 0; y < image.height(); ++y) {
        const uchar *src = image.constScanLine(y);
        for (int x = 0; x < image.width(); ++x) {
            const uchar *p = src + x * 3;
            result[y][x] = dlib::rgb_pixel(p[0], p[1], p[2]);
        }
    }
    return result;
}
/*dlib::array2d<dlib::rgb_pixel> FaceRecognizer::bytesToDlib(const QByteArray& data)
{
    dlib::array2d<dlib::rgb_pixel> image;
//    std::istringstream stream(std::string(data.constData(), data.size()),std::ios::binary);
    dlib::load_jpeg(image, data.constData(), data.size());
    return image;
}*/
dlib::array2d<dlib::rgb_pixel> FaceRecognizer::preparePhoto(const QByteArray &jpegData){
    if (jpegData.isEmpty()) return {};
    return bytesToDlib(jpegData);
}

QList<PhotoRegion> FaceRecognizer::detectFaceRegions(dlib::array2d<dlib::rgb_pixel> &img)
{
    QList<PhotoRegion> result;
    if (img.nc() == 0 || img.nr() == 0) return result; // не удалось декодировать
    const double imgW = img.nc();
    const double imgH = img.nr();
    std::vector<dlib::rectangle> dets = m_detector(img);
    for (const dlib::rectangle &det : dets) {
        PhotoRegion region;
        region.type = "Face";
        region.source = "dlib";
        region.dlyX = (det.left()+det.width()/2.0) / imgW;
        region.dlyY = (det.top()+det.height()/2.0) / imgH;
        region.dlyW = det.width() / imgW;
        region.dlyH = det.height() / imgH;
        region.hasAlg = false; 
        region.appliedToWidth = img.nc();
        region.appliedToHeight = img.nr();
        result.push_back(region);
    }
    return result;
}

int FaceRecognizer::getFacesDescriptions(dlib::array2d<dlib::rgb_pixel> &img,QList<PhotoRegion> &regions)
{
    if (regions.isEmpty()) return -1;

    const long imgW = img.nc();
    const long imgH = img.nr();

    std::vector<dlib::matrix<dlib::rgb_pixel>> faceChips;
    faceChips.reserve(regions.size());

    QList<PhotoRegion> workRegions;
    workRegions.reserve(regions.size());

    for (PhotoRegion &region : regions) {
        int left   = std::lround((region.dlyX - region.dlyW / 2.0) * imgW);
        int top    = std::lround((region.dlyY - region.dlyH / 2.0) * imgH);
        int right  = std::lround((region.dlyX + region.dlyW / 2.0) * imgW) - 1;
        int bottom = std::lround((region.dlyY + region.dlyH / 2.0) * imgH) - 1;
        left   = std::max(0, left);
        top    = std::max(0, top);
        right  = std::min(int(imgW) - 1, right);
        bottom = std::min(int(imgH) - 1, bottom);
        if (right <= left || bottom <= top) continue; // вырожденный прямоугольник — пропускаем

        dlib::rectangle rect(left, top, right, bottom);
        dlib::full_object_detection shape = m_shapePredictor(img, rect);
        dlib::matrix<dlib::rgb_pixel> chip;
        dlib::extract_image_chip(img, dlib::get_face_chip_details(shape, 150, 0.25), chip);

        region.faceChip = encodeFaceChip(chip);
        region.descriptorModel="dlib_face_recognition_resnet_model_v1";
        faceChips.push_back(std::move(chip));
        workRegions.push_back(std::move(region));
    }
    if (workRegions.isEmpty()) return -1; // все регионы оказались вырожденными
    std::vector<dlib::matrix<float, 0, 1>> descriptors = m_net(faceChips);
    for (int i = 0; i < workRegions.size(); ++i) {
        workRegions[i].descriptor.resize(descriptors[i].size());
        std::copy(descriptors[i].begin(), descriptors[i].end(), workRegions[i].descriptor.begin());
    }
    regions = std::move(workRegions); // вернуть отфильтрованный и заполненный список наружу
    return 0;
}

QVector<FaceInfo> FaceRecognizer::processImage(const QImage& image)
{
    QVector<FaceInfo> result;

    if (image.isNull())
        return result;

    dlib::array2d<dlib::rgb_pixel> img = qImageToDlib(image);

    // 1. Детекция лиц
    std::vector<dlib::rectangle> dets = m_detector(img);
    if (dets.empty())
        return result;

    // 2. Landmarks + подготовка "чипов" лица (выровненные по face chip 150x150)
    std::vector<dlib::full_object_detection> shapes;
    std::vector<dlib::matrix<dlib::rgb_pixel>> faceChips;
    shapes.reserve(dets.size());
    faceChips.reserve(dets.size());

    for (const auto& det : dets)
    {
        dlib::full_object_detection shape = m_shapePredictor(img, det);
        shapes.push_back(shape);

        dlib::matrix<dlib::rgb_pixel> chip;
        dlib::extract_image_chip(
            img,
            dlib::get_face_chip_details(shape, 150, 0.25),
            chip);
        faceChips.push_back(std::move(chip));
    }

    // 3. Дескрипторы - сеть умеет считать сразу пачкой (batch), это быстрее,
    //    чем вызывать её по одному лицу за раз
    std::vector<dlib::matrix<float, 0, 1>> descriptors = m_net(faceChips);

    // 4. Собираем результат
    result.reserve(static_cast<int>(dets.size()));
    for (size_t i = 0; i < dets.size(); ++i)
    {
        FaceInfo info;
        info.rect = QRect(dets[i].left(),
                           dets[i].top(),
                           static_cast<int>(dets[i].width()),
                           static_cast<int>(dets[i].height()));
        info.shape = shapes[i];
        info.descriptor = descriptors[i];
        result.push_back(info);
    }

    return result;
}

double FaceRecognizer::compareDescriptors(const dlib::matrix<float, 0, 1>& d1,
                                            const dlib::matrix<float, 0, 1>& d2)
{
    return dlib::length(d1 - d2);
}
