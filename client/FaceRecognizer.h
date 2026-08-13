#pragma once

#include <QImage>
#include <QVector>
#include <QRect>
#include <sstream>

#include <dlib/array2d.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing.h>
#include <dlib/dnn.h>
//#include <dlib/image_io.h>
#include <dlib/image_transforms.h>
#include "PhotoRegion.h"


// ---------------------------------------------------------------------------
// Описание архитектуры сети ResNet, точно повторяющее структуру, с которой
// был обучен файл dlib_face_recognition_resnet_model_v1.dat.
// Это стандартное определение из примера dlib (dnn_face_recognition_ex.cpp).
// ---------------------------------------------------------------------------

template <template <int, template <typename> class, int, typename> class block,
          int N, template <typename> class BN, typename SUBNET>
using residual = dlib::add_prev1<block<N, BN, 1, dlib::tag1<SUBNET>>>;

template <template <int, template <typename> class, int, typename> class block,
          int N, template <typename> class BN, typename SUBNET>
using residual_down =
    dlib::add_prev2<dlib::avg_pool<2, 2, 2, 2,
        dlib::skip1<dlib::tag2<block<N, BN, 2, dlib::tag1<SUBNET>>>>>>;

template <int N, template <typename> class BN, int stride, typename SUBNET>
using block = BN<dlib::con<N, 3, 3, 1, 1,
                  dlib::relu<BN<dlib::con<N, 3, 3, stride, stride, SUBNET>>>>>;

template <int N, typename SUBNET>
using ares = dlib::relu<residual<block, N, dlib::affine, SUBNET>>;
template <int N, typename SUBNET>
using ares_down = dlib::relu<residual_down<block, N, dlib::affine, SUBNET>>;

template <typename SUBNET> using alevel0 = ares_down<256, SUBNET>;
template <typename SUBNET> using alevel1 = ares<256, ares<256, ares_down<256, SUBNET>>>;
template <typename SUBNET> using alevel2 = ares<128, ares<128, ares_down<128, SUBNET>>>;
template <typename SUBNET> using alevel3 = ares<64, ares<64, ares<64, ares_down<64, SUBNET>>>>;
template <typename SUBNET> using alevel4 = ares<32, ares<32, ares<32, SUBNET>>>;

using anet_type = dlib::loss_metric<dlib::fc_no_bias<128, dlib::avg_pool_everything<
    alevel0<
    alevel1<
    alevel2<
    alevel3<
    alevel4<
    dlib::max_pool<3, 3, 2, 2, dlib::relu<dlib::affine<dlib::con<32, 7, 7, 2, 2,
    dlib::input_rgb_image_sized<150>
    >>>>>>>>>>>>;

// Информация об одном найденном лице Устарело, но пока оставил.
struct FaceInfo
{
    QRect rect;                                // прямоугольник лица на исходном QImage
    dlib::full_object_detection shape;         // 68 (или 5) landmark-точек
    dlib::matrix<float, 0, 1> descriptor;      // 128-мерный дескриптор лица
};

class FaceRecognizer
{
public:
    // shapePredictorPath  - например "models/shape_predictor_68_face_landmarks.dat"
    // resnetModelPath     - "models/dlib_face_recognition_resnet_model_v1.dat"
    FaceRecognizer(const QString& shapePredictorPath, const QString& resnetModelPath);

    // Находит все лица на изображении, их landmark-точки и 128-мерные дескрипторы
    QVector<FaceInfo> processImage(const QImage& image);
    QList<PhotoRegion> detectFaceRegions(dlib::array2d<dlib::rgb_pixel> &img);
    dlib::array2d<dlib::rgb_pixel> preparePhoto(const QByteArray &jpegData);
    int getFacesDescriptions(dlib::array2d<dlib::rgb_pixel> &img,QList<PhotoRegion> &regions);
    static double compareDescriptors(const dlib::matrix<float, 0, 1>& d1, const dlib::matrix<float, 0, 1>& d2);

private:
    static dlib::array2d<dlib::rgb_pixel> qImageToDlib(const QImage& image);
    dlib::array2d<dlib::rgb_pixel> bytesToDlib(const QByteArray& data);

    dlib::frontal_face_detector m_detector;
    dlib::shape_predictor m_shapePredictor;
    anet_type m_net;
    
};
