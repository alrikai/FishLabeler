#include <iostream>
#include <string>

#include <QHBoxLayout>
#include <QVBoxLayout>

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
    //fview = new QGraphicsView(fview_p);
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
    //TODO: move forwards
	
	frame_index += 1;
	auto fnum_str = utils::make_framecount_string(frame_index);
    framenum_label->setText(fnum_str.c_str());
}

void VideoWindow::prev_frame()
{
	//TODO: move backwards
	

	frame_index -= 1;
	auto fnum_str = utils::make_framecount_string(frame_index);
    framenum_label->setText(fnum_str.c_str());
}

void VideoWindow::closeEvent(QCloseEvent *evt)
{
    //TODO: do we need to do anything? Flush out un-written annotations, etc?
}

