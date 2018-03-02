#include <iostream>
#include <string>

#include <QTimer>
#include <QFileDialog>

#include "VideoWindow.hpp"
#include "AnnotationTypes.hpp"


/* TODO: what else to add to the UI? 
 * - more hotkeys for common actions --> currently have:
 *   {N, P, --> next / prev frame
 *   cntrl+Z, cntrl+R --> undo / redo annotation
 *   cntrl+B, cntrl+S --> bounding box / pixel-wise label mode
 * - top toolbar for save, exit, and maybe a help bar (for hotkeys)
 */

/* TODO: what else to do?
 * - ability to use weak labels to jump to specified events (i.e. if we do weak labeleing that there's a fish in a frame, 
 *      then we should have a mode that'll just look at the +- 1 sec around the 'fish in the scene' times.
 * - make image scroll times faster
 * - make mouse capture times for annotations faster
 */

VideoWindow::VideoWindow(QWidget *parent)
    : QMainWindow(parent), frame_incamount(1)
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

    label_mode = ANNOTATION_MODE::BOUNDINGBOX;
    fviewer->set_annotation_mode(label_mode);

    init_window();

    //TODO: for whatever reason, this causes a memory leak until the frame is cycled. No idea why though
    //resizes the screen s.t. the frame fits well
    QTimer::singleShot(100, this, SLOT(showFullScreen()));
}

void VideoWindow::init_window()
{
    auto cfg_layout = new QHBoxLayout;
    framenum_label = new QLabel(main_window);
    auto fnum_str = make_framecount_string(0);
    framenum_label->setText(fnum_str.c_str());

    hour_timestamp = new QLabel(main_window);
    hour_timestamp->setText("hour: 0");
    min_timestamp = new QLabel(main_window);
    min_timestamp->setText("min: 0");
    sec_timestamp = new QLabel(main_window);
    sec_timestamp->setText("sec: 0");

    instance_idledit = new QLineEdit("0", main_window);
    connect(instance_idledit, &QLineEdit::editingFinished, [this]{
        set_instanceid();
    });

    prev_btn = new QPushButton("previous", main_window);
    connect(prev_btn, &QPushButton::clicked, [this]{
        prev_frame();
    });
    next_btn = new QPushButton("next", main_window);
    connect(next_btn, &QPushButton::clicked, [this]{
        next_frame();
    });

    frame_incledit = new QLineEdit("1", main_window);
    connect(frame_incledit, &QLineEdit::editingFinished, [this]{
        set_frame_incamount();
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
    labelmode_btn = new QPushButton("Label Mode", main_window);
    connect(labelmode_btn, &QPushButton::clicked, [this]{
        cycle_label_mode();
    });

    QVBoxLayout* rhs_layout = new QVBoxLayout;
    rhs_layout->addWidget(labelmode_btn);
    rhs_layout->addWidget(metadata_textlabel);
    rhs_layout->addWidget(metadata_edit);

    QHBoxLayout* lhs_layout = new QHBoxLayout;
    auto fview_p = fviewer.get();
    fview = new FrameView(fview_p);
    lhs_layout->addWidget(fview);
    lhs_layout->addLayout(rhs_layout);

    QVBoxLayout* main_layout = new QVBoxLayout;
    main_layout->addLayout(lhs_layout);
    main_layout->addLayout(cfg_layout);
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
    auto ql_instanceid_txt = new QLabel(main_window);
    ql_instanceid_txt->setText("instance ID: ");

    constexpr int min_btn_height = 40; 
    constexpr int min_btn_width = 100;
    offset_btn->setMinimumSize(min_btn_width, min_btn_height);
    prev_btn->setMinimumSize(min_btn_width, min_btn_height);
    next_btn->setMinimumSize(min_btn_width, min_btn_height);

    auto ql_framejump_txt = new QLabel(main_window);
    ql_framejump_txt->setText("frame move: ");

    constexpr int max_offset_width = 50;
    ql_hour->setMaximumWidth(max_offset_width);
    ql_min->setMaximumWidth(max_offset_width);
    ql_sec->setMaximumWidth(max_offset_width);
    ql_paintsz->setMaximumWidth(max_offset_width);
    instance_idledit->setMaximumWidth(max_offset_width);
    frame_incledit->setMaximumWidth(max_offset_width);

    constexpr int max_offset_text_width = 40;
    ql_hour_txt->setMaximumWidth(max_offset_text_width);
    ql_min_txt->setMaximumWidth(max_offset_text_width);
    ql_sec_txt->setMaximumWidth(max_offset_text_width);
    ql_paintsz_txt->setMaximumWidth(2*max_offset_text_width);
    ql_instanceid_txt->setMaximumWidth(2*max_offset_text_width);
    ql_framejump_txt->setMaximumWidth(2*max_offset_text_width);

    cfg_layout->addWidget(framenum_label);
    cfg_layout->addWidget(hour_timestamp);
    cfg_layout->addWidget(min_timestamp);
    cfg_layout->addWidget(sec_timestamp);

    cfg_layout->addWidget(ql_instanceid_txt);
    cfg_layout->addWidget(instance_idledit);
   
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
    cfg_layout->addWidget(ql_framejump_txt);
    cfg_layout->addWidget(frame_incledit);
 
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

void VideoWindow::write_frame_metadat(const int old_frame_index)
{
    //we want to get the frame information that is being phased out (so use old frame index)
    auto frame_name = vreader->get_frame_name(old_frame_index);

    //check the edit box for text
    auto fmeta_text = metadata_edit->toPlainText().toStdString();
    if (fmeta_text.size() > 0) {
        vlogger->write_textmetadata(frame_name, std::move(fmeta_text));
        //reset the metadata text, if needed
        metadata_edit->clear();
    }

    //check the frame viewer for user-supplied annotations and write them out to disk
    auto fannotations = fview->get_frame_annotations();
    const int fheight = fviewer->get_frame_height();
    const int fwidth = fviewer->get_frame_width();
    if (fannotations.bboxes.size() > 0) {
        vlogger->write_bboxes(frame_name, std::move(fannotations.bboxes), fheight, fwidth);
    }

    if (fannotations.segm_points.size() > 0) {
        vlogger->write_annotations(frame_name, std::move(fannotations.segm_points), fheight, fwidth);
    }
}

void VideoWindow::retrieve_frame_metadata(const int new_frame_index)
{
    auto nextframe_name = vreader->get_frame_name(new_frame_index);
    //check for pre-existing metadata as well
    if (vlogger->has_annotations(nextframe_name) || vlogger->has_boundingbox(nextframe_name)) {
        auto nfbboxes = vlogger->get_boundingboxes(nextframe_name);
        auto nfannotations = vlogger->get_annotations(nextframe_name);
        FrameAnnotations nframe_annotations {std::move(nfbboxes), std::move(nfannotations)};
        fview->set_frame_annotations(std::move(nframe_annotations));
    }

    //... as well as pre-existing text metadata
    if (vlogger->has_textmetadata(nextframe_name)) {
        auto nfmetadata = vlogger->get_textmetadata(nextframe_name);
        metadata_edit->appendPlainText(QString::fromStdString(nfmetadata));
    }
}

void VideoWindow::frame_change_metadata(const QImage& vframe, const int old_frame_index, const int new_frame_index)
{
    //collect and save existing frame's metadata
    write_frame_metadat(old_frame_index);
    //move to the new frame to be displayed
    fview->update_frame(vframe);
    //retreive and display existing metadata for the new frame (if applicable)
    retrieve_frame_metadata(new_frame_index);

    auto fnum_str = make_framecount_string(new_frame_index);
    framenum_label->setText(fnum_str.c_str());
 
    int h_ts, m_ts, s_ts;
    std::tie(h_ts, m_ts, s_ts) = vreader->get_current_timestamp();
    std::string hour_ts {"hour: " + std::to_string(h_ts)};
    hour_timestamp->setText(hour_ts.c_str());
    std::string min_ts {"min: " + std::to_string(m_ts)};
    min_timestamp->setText(min_ts.c_str());
    std::string sec_ts {"sec: " + std::to_string(s_ts)};
    sec_timestamp->setText(sec_ts.c_str());       
    fview->update();
}

void VideoWindow::next_frame()
{
    const int frame_index = vreader->get_current_frame_index();
    const int target_frame_index = std::min(frame_index + frame_incamount, vreader->get_num_frames()-1);
    //auto vframe = vreader->get_next_frame();
    auto vframe = vreader->get_frame(target_frame_index);
    //save frame's existing metadata, change frame, and (if applicable) load saved metadata for the new frame
    frame_change_metadata(vframe, frame_index, target_frame_index);
}

void VideoWindow::prev_frame()
{
    const int frame_index = vreader->get_current_frame_index();
    const int target_frame_index = std::max(frame_index - frame_incamount, 0);
    //auto vframe = vreader->get_prev_frame();
    auto vframe = vreader->get_frame(target_frame_index);
    //save frame's existing metadata, change frame, and (if applicable) load saved metadata for the new frame
    frame_change_metadata(vframe, frame_index, target_frame_index);
}

void VideoWindow::closeEvent(QCloseEvent *evt)
{
    //TODO: do we need to do anything? Flush out un-written annotations, etc?

    //collect and save existing frame's metadata
    const int frame_index = vreader->get_current_frame_index();
    write_frame_metadat(frame_index);
}

void VideoWindow::apply_video_offset()
{
    auto frame_index = vreader->get_current_frame_index();
    auto hour_offset = ql_hour->text().toInt();
    auto min_offset = ql_min->text().toInt();
    auto sec_offset = ql_sec->text().toInt();
    std::cout << "H: " << hour_offset << ", M: " << min_offset << ", S: " << sec_offset << std::endl;
    QImage vframe;
    try {
        vframe = vreader->get_frame(hour_offset, min_offset, sec_offset);
    } catch (const std::runtime_error& err) {
        //can probably assume that we went past the end of the video -- maybe I should make custom error types for this...
        auto last_frame_idx = vreader->get_num_frames();
        vframe = vreader->get_frame(last_frame_idx-1);
    }
    //upate the current frame index
    auto curr_fidx = vreader->get_current_frame_index();

    //save frame's existing metadata, change frame, and (if applicable) load saved metadata for the new frame
    frame_change_metadata(vframe, frame_index, curr_fidx);
}

void VideoWindow::cycle_label_mode()
{
    //TODO: do I want to have the count as the last annotation mode, or have this stored in some 
    //centralized location?
    static constexpr int NUM_ANNOTATION_MODES = 2;
    auto new_mode_idx = (static_cast<int>(label_mode) + 1) % NUM_ANNOTATION_MODES;
    label_mode = static_cast<ANNOTATION_MODE>(new_mode_idx);

    //this works because we have 2 modes, and because we can abuse automatic conversions from 
    //no-class enums to ints. 

    //TODO: also change the button color?
    if (label_mode == ANNOTATION_MODE::BOUNDINGBOX) {
        labelmode_btn->setText("Detection");
    } else {
        labelmode_btn->setText("Segmentation");
    }

    //notify the frame viewer that the mode changed (this should replace the cntrl + s / contrl + d hotkeys)
    fviewer->set_annotation_mode(label_mode);
}

void VideoWindow::adjust_paintbrush_size()
{
    auto brushsz = ql_paintsz->text().toInt();
    fviewer->set_brushsz(brushsz);
}

void VideoWindow::set_instanceid()
{
    auto instance_id = instance_idledit->text().toInt();
    fviewer->set_instance_id(instance_id);
}

void VideoWindow::set_frame_incamount()
{
    auto frame_jump = frame_incledit->text().toInt();
    frame_incamount = frame_jump;
}
