#if 1
#include <QApplication>
#include "VideoWindow.hpp"

int main(int argc, char *argv[])
{
    //Q_INIT_RESOURCE(application);

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Alrik Firl");
    QCoreApplication::setApplicationName("Fish Labeler");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    VideoWindow video_window;
    video_window.show();
    return app.exec();
}

#else


#include <opencv2/opencv.hpp>

#include <string>
#include <QPoint>
#include <vector>

#include "AnnotationProcessing.hpp"

int main() {

    const std::string contour_path {"/home/alrik/Data/NRTFishAnnotations/20140711121827/Annotations/006324.png"};
    auto contour_img = cv::imread(contour_path);

    //cv::imshow("base image", contour_img);
    //cv::waitKey(0);
    std::vector<QPoint> pts;
    for (int r  = 0; r < contour_img.rows; r++) {
        for (int c  = 0; c < contour_img.cols; c++) {
            if (contour_img.at<uint8_t>(r*contour_img.cols+c) > 0) {
                pts.emplace_back(c,r);
            }
        }
    }
/*
    std::vector<QPoint> pts;
    pts.emplace_back(10, 10);
    pts.emplace_back(10, 100);
    pts.emplace_back(100, 10);
    pts.emplace_back(100, 100);
*/



    QImage out_img = QImage(contour_img.cols, contour_img.rows, QImage::Format_Grayscale8);
    const int brushsz = 8;
    using dtype_t = uint8_t;
    dtype_t instance_id = 255;
    mask_utils::fill_segment<dtype_t>(out_img, pts, brushsz, instance_id);




}
#endif
