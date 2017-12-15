#include <iostream>
#include <string>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>

#include "VideoWindow.hpp"

namespace utils {
	inline std::string make_framecount_string(const int findex) {
		return std::string {"Frame #: " + std::to_string(findex)};
	}
}

VideoWindow::VideoWindow(QWidget *parent)
	: QMainWindow(parent)
{
    frame_index = 0;
	main_window = new QWidget(this);
    setCentralWidget(main_window);
    fviewer = std::make_shared<FrameViewer>(main_window);
    init_window();

	//TODO: eventually, will want to load this from the UI
    const std::string vpath {"/home/alrik/Data/NRTFish/20130117144639.mts"};
	vreader = std::make_unique<VideoReader> (vpath);

	QTimer::singleShot(100, this, SLOT(showFullScreen()));
}

void VideoWindow::init_window()
{
    framenum_label = new QLabel(main_window);
    auto fnum_str = utils::make_framecount_string(frame_index);
    framenum_label->setText(fnum_str.c_str());

    prev_btn = new QPushButton("previous", main_window);
    next_btn = new QPushButton("next", main_window);
	metadata_edit = new QPlainTextEdit(main_window);

    QVBoxLayout* rhs_layout = new QVBoxLayout;
    rhs_layout->addWidget(framenum_label);
    rhs_layout->addWidget(prev_btn);
    rhs_layout->addWidget(next_btn);
	rhs_layout->addWidget(metadata_edit);

    QHBoxLayout* lhs_layout = new QHBoxLayout;
	auto fview_p = fviewer.get();
    fview = new FrameView(fview_p);
    lhs_layout->addWidget(fview);

    QVBoxLayout* main_layout = new QVBoxLayout;
    main_layout->addLayout(lhs_layout);
    main_layout->addLayout(rhs_layout);
    main_window->setLayout(main_layout);
    main_window->setWindowTitle("Fish Labeler");

    connect(prev_btn, &QPushButton::clicked, [this]{
        prev_frame();
    });
    connect(next_btn, &QPushButton::clicked, [this]{
        next_frame();
    });
}

void VideoWindow::next_frame()
{
	if (frame_index < vreader->get_num_frames()) {
        auto vframe = vreader->get_next_frame();
		fviewer->display_frame(vframe);
		frame_index += 1;
		auto fnum_str = utils::make_framecount_string(frame_index);
		framenum_label->setText(fnum_str.c_str());
		fview->update();
	}
}

void VideoWindow::prev_frame()
{
	if (frame_index > 0) {
	    frame_index -= 1;
		auto vframe = vreader->get_frame(frame_index);
		fviewer->display_frame(vframe);
		auto fnum_str = utils::make_framecount_string(frame_index);
		framenum_label->setText(fnum_str.c_str());
		fview->update();
	}
}

void VideoWindow::closeEvent(QCloseEvent *evt)
{
    //TODO: do we need to do anything? Flush out un-written annotations, etc?
}

#if 0
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
#endif
