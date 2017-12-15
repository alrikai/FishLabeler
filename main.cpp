
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "VideoWindow.hpp"

int main(int argc, char *argv[])
{
    //Q_INIT_RESOURCE(application);

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Alrik Firl");
    QCoreApplication::setApplicationName("Fish Labeler");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
/*
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The file to open.");
    parser.process(app);
    if (!parser.positionalArguments().isEmpty())
        video_window;.loadFile(parser.positionalArguments().first());

*/
    VideoWindow video_window;
    video_window.show();
    return app.exec();
}

/*
int main() {
    const std::string vpath {"/home/alrik/Data/NRTFish/20130117144639.mts"};
    VideoReader vreader {vpath};

    std::cout << "video has " << vreader.get_num_frames() << " #frames" << std::endl;
    for (int i = 0; i < 10; i++) {
        auto vframe = vreader.get_next_frame();
        std::cout << "vf " << i << ": [" << vframe.height << " x " << vframe.width << "]" << std::endl;
        cv::Mat cv_frame (vframe.height, vframe.width, CV_8UC3, vframe.data.get());
        std::string fname {"vframe_" + std::to_string(i) + ".png"};
        cv::imwrite(fname, cv_frame);
    }

    int foffset = 30 * 60 * 60 * 4;
    auto vframe = vreader.get_frame(foffset);
    std::string fname {"vframe_" + std::to_string(20) + ".png"};
    for (int i = 0; i < 10; i++) {
        auto vframe = vreader.get_next_frame();
        std::cout << "vf " << foffset + i << ": [" << vframe.height << " x " << vframe.width << "]" << std::endl;
        cv::Mat cv_frame (vframe.height, vframe.width, CV_8UC3, vframe.data.get());
        std::string fname {"vframe_" + std::to_string(foffset+i) + ".png"};
        cv::imwrite(fname, cv_frame);
    }



    vreader.get_cache_stats();
}
*/
