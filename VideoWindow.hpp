#ifndef FISHLABELER_VIDEOWINDOW_HPP
#define FISHLABELER_VIDEOWINDOW_HPP

#include <memory>


#include <QMainWindow>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "VideoReader.hpp"
#include "FrameViewer.hpp"
#include "VideoLogger.hpp"

class VideoWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit VideoWindow(QWidget *parent = 0);
    
protected:
    void closeEvent(QCloseEvent *evt) override;
    void keyPressEvent(QKeyEvent *evt) override;

private:
    inline std::string make_framecount_string(const int findex) {
        //NOTE: we index from frame 0, hence the -1
        return std::string {"Frame #: " + std::to_string(findex) + " / " + std::to_string(vreader->get_num_frames()-1)};
    }

    void init_window();
    void set_cfgUI_layout(QHBoxLayout* layout);
    void next_frame();
    void prev_frame();
    void set_frame_incamount();

    void set_instanceid();
    void apply_video_offset();
    void adjust_paintbrush_size();
    void frame_change_metadata(const QImage& vframe, const int old_frame_index, const int new_frame_index);

    void write_frame_metadat(const int old_frame_index);
    void retrieve_frame_metadata(const int new_frame_index);

    //TODO: figure out if Qt manages the lifetime, or if I do...
    std::shared_ptr<FrameViewer> fviewer;
    FrameView* fview;

    QWidget* main_window;
    QPlainTextEdit* metadata_edit;
    QPushButton* prev_btn; 
    QPushButton* next_btn; 
    QLabel* framenum_label;
    QLabel* hour_timestamp;
    QLabel* min_timestamp;
    QLabel* sec_timestamp;

    QLineEdit* instance_idledit;
    QLineEdit* frame_incledit;
        
    QLineEdit* ql_hour;
    QLineEdit* ql_min;
    QLineEdit* ql_sec;
    QPushButton* offset_btn;
    QLineEdit* ql_paintsz;

    int frame_incamount;
    std::unique_ptr<VideoReader> vreader;
    std::unique_ptr<VideoLogger> vlogger;
};

#endif

