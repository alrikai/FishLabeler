#ifndef FISHLABELER_VIDEOWINDOW_HPP
#define FISHLABELER_VIDEOWINDOW_HPP

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

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

    //TODO: figure out if Qt manages the lifetime, or if I do...
	std::shared_ptr<FrameViewer> fviewer;
	QGraphicsView* fview; 

	QWidget* main_window;
	QPlainTextEdit* metadata_edit;
	QPushButton* prev_btn; 
	QPushButton* next_btn; 
	QLabel* framenum_label;

	int frame_index;
};

#endif

