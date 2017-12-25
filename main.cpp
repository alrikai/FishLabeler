
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
