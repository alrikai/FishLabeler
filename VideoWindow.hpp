#ifndef FISHLABELER_VIDEOWINDOW_HPP
#define FISHLABELER_VIDEOWINDOW_HPP

#include <memory>


#include <QMainWindow>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include "VideoReader.hpp"
#include "FrameViewer.hpp"

class VideoWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit VideoWindow(QWidget *parent = 0);
    
protected:
    void closeEvent(QCloseEvent *evt) override;
/*
private slots:
    void newFile();
    void open();
    bool save();
    bool saveAs();
    void about();
    void documentWasModified();
*/
private:
	void init_window();
	void next_frame();
	void prev_frame();
    void apply_video_offset();

    //TODO: figure out if Qt manages the lifetime, or if I do...
	std::shared_ptr<FrameViewer> fviewer;
	QGraphicsView* fview; 

	QWidget* main_window;
	QPlainTextEdit* metadata_edit;
	QPushButton* prev_btn; 
	QPushButton* next_btn; 
	QLabel* framenum_label;

    QLineEdit* ql_hour;
    QLineEdit* ql_min;
    QLineEdit* ql_sec;
    QPushButton* offset_btn;

	int frame_index;
	std::unique_ptr<VideoReader> vreader;
};

#endif

