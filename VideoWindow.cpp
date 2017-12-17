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
	//TODO: eventually, will want to load this from the UI
    const std::string vpath {"/home/alrik/Data/NRTFish/20130117144639.mts"};
	vreader = std::make_unique<VideoReader> (vpath);

	auto initial_frame = vreader->get_next_frame();

    frame_index = 0;
	main_window = new QWidget(this);
    setCentralWidget(main_window);
    fviewer = std::make_shared<FrameViewer>(initial_frame, main_window);
    init_window();

	//resizes the screen s.t. the frame fits well
	QTimer::singleShot(100, this, SLOT(showFullScreen()));
}

void VideoWindow::init_window()
{
    auto cfg_layout = new QHBoxLayout;
    framenum_label = new QLabel(main_window);
    auto fnum_str = utils::make_framecount_string(frame_index);
    framenum_label->setText(fnum_str.c_str());
    prev_btn = new QPushButton("previous", main_window);
    connect(prev_btn, &QPushButton::clicked, [this]{
        prev_frame();
    });
    next_btn = new QPushButton("next", main_window);
    connect(next_btn, &QPushButton::clicked, [this]{
        next_frame();
    });

    auto ql_hour_txt = new QLabel("0", main_window);
    ql_hour_txt->setText("hour: ");
    ql_hour = new QLineEdit(main_window);
    /*
    connect(ql_hour, &QLineEdit::editingFinished, [this]{
        hour_offset();
    });
    */

    auto ql_min_txt = new QLabel("0", main_window);
    ql_min_txt->setText("min: ");
    ql_min = new QLineEdit(main_window);
    /*
    connect(ql_min, &QLineEdit::editingFinished, [this]{
        minute_offset();
    });
    */

    auto ql_sec_txt = new QLabel("0", main_window);
    ql_sec_txt->setText("sec: ");
    ql_sec = new QLineEdit(main_window);
    /*
    connect(ql_sec, &QLineEdit::editingFinished, [this]{
        second_offset();
    });
    */
    constexpr int min_btn_height = 40; 
    constexpr int min_btn_width = 100;
    offset_btn = new QPushButton("apply offset", main_window);
    connect(offset_btn, &QPushButton::clicked, [this]{
        apply_video_offset();
    });
    offset_btn->setMinimumSize(min_btn_width, min_btn_height);
    prev_btn->setMinimumSize(min_btn_width, min_btn_height);
    next_btn->setMinimumSize(min_btn_width, min_btn_height);

    constexpr int max_offset_width = 50;
    ql_hour->setMaximumWidth(max_offset_width);
    ql_min->setMaximumWidth(max_offset_width);
    ql_sec->setMaximumWidth(max_offset_width);
    constexpr int max_offset_text_width = 40;
    ql_hour_txt->setMaximumWidth(max_offset_text_width);
    ql_min_txt->setMaximumWidth(max_offset_text_width);
    ql_sec_txt->setMaximumWidth(max_offset_text_width);

    cfg_layout->addWidget(framenum_label);
    cfg_layout->addWidget(ql_hour_txt);
    cfg_layout->addWidget(ql_hour);
    cfg_layout->addWidget(ql_min_txt);
    cfg_layout->addWidget(ql_min);
    cfg_layout->addWidget(ql_sec_txt);
    cfg_layout->addWidget(ql_sec);
    cfg_layout->addWidget(offset_btn);

    cfg_layout->addWidget(prev_btn);
    cfg_layout->addWidget(next_btn);

	auto metadata_textlabel = new QLabel(main_window);
    metadata_textlabel->setText("Frame Metadata:");
	metadata_edit = new QPlainTextEdit(main_window);

    QVBoxLayout* rhs_layout = new QVBoxLayout;
    rhs_layout->addWidget(metadata_textlabel);
	rhs_layout->addWidget(metadata_edit);

    QHBoxLayout* lhs_layout = new QHBoxLayout;
	auto fview_p = fviewer.get();
    fview = new FrameView(fview_p);
    lhs_layout->addWidget(fview);

    QVBoxLayout* main_layout = new QVBoxLayout;
    main_layout->addLayout(lhs_layout);
    main_layout->addLayout(cfg_layout);
    main_layout->addLayout(rhs_layout);
    main_window->setLayout(main_layout);
    main_window->setWindowTitle("Fish Labeler");

    /* TODO: what else to add? 
	 * - edit box for pen size (for annotations -- have it default to something (like, 8) 
	 * - keyboard listener -- left and right arrows to do the next / prev button functionality 
	 * - zoom in / out of the frame 
	 */

    /* TODO: what else to do?
     * - proper video seeking -- doesn't seem to work currently
     * - ability to use weak labels to jump to specified events (i.e. if we do weak labeleing that there's a fish in a frame, 
     *      then we should have a mode that'll just look at the +- 1 sec around the 'fish in the scene' times.
     */

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

void VideoWindow::apply_video_offset()
{
    auto hour_offset = ql_hour->text().toInt();
    auto min_offset = ql_min->text().toInt();
    auto sec_offset = ql_sec->text().toInt();

    int64_t offset_in_sec = 60 * (60 * hour_offset + min_offset) + sec_offset; 
    //TODO: need to ensure this is the correct FPS for the video. We will *assume* our videos
    //are a uniform frame rate (this is not always the case, but I think we can assume this in our case)

    int64_t frame_offset = static_cast<int64_t>(offset_in_sec * vreader->get_video_fps());
    std::cout << "H: " << hour_offset << ", M: " << min_offset << ", S: " << sec_offset << std::endl;
    auto vframe = vreader->get_frame(frame_offset);
	fviewer->display_frame(vframe);
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
