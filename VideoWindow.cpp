#include <iostream>
#include <string>

#include <QTimer>
#include <QFileDialog>

#include "VideoWindow.hpp"


/* TODO: what else to add to the UI? 
 * - more hotkeys for common actions
 * - top toolbar for save, exit, and maybe a help bar (for hotkeys)
 */

/* TODO: what else to do?
 * - ability to use weak labels to jump to specified events (i.e. if we do weak labeleing that there's a fish in a frame, 
 *      then we should have a mode that'll just look at the +- 1 sec around the 'fish in the scene' times.
 * - make image scroll times faster
 * - make mouse capture times for annotations faster
 * - saving anootations out to disk along with the frame
 */

namespace utils {
    inline std::string make_framecount_string(const int findex) {
        return std::string {"Frame #: " + std::to_string(findex)};
    }
}

VideoWindow::VideoWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto filename = QFileDialog::getExistingDirectory(this, 
    tr("Open Fish Video Frame Directory"), QDir::currentPath(), QFileDialog::ShowDirsOnly);
    const std::string vpath = filename.toStdString(); 
    vreader = std::make_unique<VideoReader> (vpath);
    auto initial_frame = vreader->get_next_frame();

    vlogger = std::make_unique<VideoLogger> (vpath);

    main_window = new QWidget(this);
    setCentralWidget(main_window);
    fviewer = std::make_shared<FrameViewer>(initial_frame, main_window);
    init_window();

    //TODO: for whatever reason, this causes a memory leak until the frame is cycled. No idea why though
    //resizes the screen s.t. the frame fits well
    QTimer::singleShot(100, this, SLOT(showFullScreen()));
}

void VideoWindow::init_window()
{
    auto cfg_layout = new QHBoxLayout;
    framenum_label = new QLabel(main_window);
    auto fnum_str = utils::make_framecount_string(0);
    framenum_label->setText(fnum_str.c_str());
    prev_btn = new QPushButton("previous", main_window);
    connect(prev_btn, &QPushButton::clicked, [this]{
        prev_frame();
    });
    next_btn = new QPushButton("next", main_window);
    connect(next_btn, &QPushButton::clicked, [this]{
        next_frame();
    });

    ql_hour = new QLineEdit("0", main_window);
    ql_min = new QLineEdit("0", main_window);
    ql_sec = new QLineEdit("0", main_window);
    offset_btn = new QPushButton("apply offset", main_window);
    connect(offset_btn, &QPushButton::clicked, [this]{
        apply_video_offset();
    });

    ql_paintsz = new QLineEdit(main_window); 
    connect(ql_paintsz, &QLineEdit::editingFinished, [this]{
        adjust_paintbrush_size();
    });

    //set up the layout for all the configuration items
    set_cfgUI_layout(cfg_layout);

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

}

void VideoWindow::set_cfgUI_layout(QHBoxLayout* cfg_layout)
{
    auto ql_hour_txt = new QLabel(main_window);
    ql_hour_txt->setText("hour: ");
    auto ql_min_txt = new QLabel(main_window);
    ql_min_txt->setText("min: ");
    auto ql_sec_txt = new QLabel(main_window);
    ql_sec_txt->setText("sec: ");
    auto ql_paintsz_txt = new QLabel(main_window);
    ql_paintsz_txt->setText("brush size: ");

    constexpr int min_btn_height = 40; 
    constexpr int min_btn_width = 100;
    offset_btn->setMinimumSize(min_btn_width, min_btn_height);
    prev_btn->setMinimumSize(min_btn_width, min_btn_height);
    next_btn->setMinimumSize(min_btn_width, min_btn_height);

    constexpr int max_offset_width = 50;
    ql_hour->setMaximumWidth(max_offset_width);
    ql_min->setMaximumWidth(max_offset_width);
    ql_sec->setMaximumWidth(max_offset_width);
    ql_paintsz->setMaximumWidth(max_offset_width);

    constexpr int max_offset_text_width = 40;
    ql_hour_txt->setMaximumWidth(max_offset_text_width);
    ql_min_txt->setMaximumWidth(max_offset_text_width);
    ql_sec_txt->setMaximumWidth(max_offset_text_width);
    ql_paintsz_txt->setMaximumWidth(2*max_offset_text_width);

    cfg_layout->addWidget(framenum_label);

    cfg_layout->addWidget(ql_paintsz_txt);
    cfg_layout->addWidget(ql_paintsz);

    cfg_layout->addWidget(ql_hour_txt);
    cfg_layout->addWidget(ql_hour);
    cfg_layout->addWidget(ql_min_txt);
    cfg_layout->addWidget(ql_min);
    cfg_layout->addWidget(ql_sec_txt);
    cfg_layout->addWidget(ql_sec);
    cfg_layout->addWidget(offset_btn);

    cfg_layout->addWidget(prev_btn);
    cfg_layout->addWidget(next_btn);
}

void VideoWindow::keyPressEvent(QKeyEvent *evt)
{
    switch(evt->key()) {
        case Qt::Key_N:
            std::cout << "NEXT key" << std::endl;
            next_frame();
            break;
        case Qt::Key_P:
            std::cout << "PREV key" << std::endl;
            prev_frame();
            break;
        default:
            std::cout << "key: " << evt->key() << std::endl;
    }
        
    QWidget::keyPressEvent(evt);
}

void VideoWindow::next_frame()
{
	const int frame_index = vreader->get_current_frame_index();
    if (frame_index < vreader->get_num_frames()) {
        auto vframe = vreader->get_next_frame();

        //we want to get the frame information that is being phased out (so use old frame index)
		auto frame_name = vreader->get_frame_name(frame_index);

		//check the edit box for text
        auto fmeta_text = metadata_edit->toPlainText().toStdString();
        if (fmeta_text.size() > 0) {
            vlogger->write_textmetadata(frame_name, std::move(fmeta_text));
	        //reset the metadata text, if needed
			metadata_edit->clear();
		}

		//check the frame viewer for annotations
        auto fannotation = fview->get_frame_annotations();
		if (fannotation.size() > 0) {
			const int bsz = fviewer->get_brushsz();
			const int fheight = fviewer->get_frame_height();
			const int fwidth = fviewer->get_frame_width();
            vlogger->write_annotations(frame_name, std::move(fannotation), bsz, fheight, fwidth);
		}

        fview->update_frame(vframe);
        auto fnum_str = utils::make_framecount_string(frame_index+1);
        framenum_label->setText(fnum_str.c_str());
        fview->update();
    }
}

void VideoWindow::prev_frame()
{
	const int frame_index = vreader->get_current_frame_index();
    if (frame_index > 0) {
        auto vframe = vreader->get_prev_frame();
        fview->update_frame(vframe);
        auto fnum_str = utils::make_framecount_string(frame_index-1);
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
    std::cout << "H: " << hour_offset << ", M: " << min_offset << ", S: " << sec_offset << std::endl;
    auto vframe = vreader->get_frame(hour_offset, min_offset, sec_offset);
	//upate the current frame index
	auto curr_fidx = vreader->get_current_frame_index();
	auto fnum_str = utils::make_framecount_string(curr_fidx);
    framenum_label->setText(fnum_str.c_str());
	//draw the frame to the UI
    fview->update_frame(vframe);
	
	//TODO: just a sanity check for now...
	//might also be nice to show timestamp of current frame on the UI when jumping like this, in which case this 
	//information would be useful
	auto curr_tstamp = vreader->get_current_timestamp();
	assert(std::get<0>(curr_tstamp) == hour_offset);
	assert(std::get<1>(curr_tstamp) == min_offset);
	assert(std::abs(std::get<2>(curr_tstamp) - sec_offset) <= 1);
}

void VideoWindow::adjust_paintbrush_size()
{
    auto brushsz = ql_paintsz->text().toInt();
    fviewer->set_brushsz(brushsz);
}
