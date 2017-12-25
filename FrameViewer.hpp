#ifndef FISHLABELER_FRAMEVIEWER_HPP
#define FISHLABELER_FRAMEVIEWER_HPP

#include <QWidget>
#include <QObject>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QWheelEvent>


class FrameViewer : public QGraphicsScene
{
public:
    using PixelT = uint8_t;
    FrameViewer(const QImage& initial_frame, QObject *parent = 0);

    void display_frame(const QImage& frame);

    QSize get_size_hint() const {
        QSize sz{current_frame.width(), current_frame.height()};
        return sz; 
    }

    void set_brushsz(int brushsz) {
        annotation_brushsz = brushsz;
        this->update();
    }

	int get_brushsz() const {
        return annotation_brushsz;
    }

	int get_frame_width() const {
        return current_frame.width(); 
	}

	int get_frame_height() const {
        return current_frame.height(); 
	}


    std::vector<QPointF> get_frame_annotations() const {
		return annotation_locations;
	}

protected slots:
    void drawBackground(QPainter* painter, const QRectF &rect) override;
    void drawForeground(QPainter* painter, const QRectF &rect) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
    void keyPressEvent(QKeyEvent *evt) override;

private:
    void undo_label();
    void redo_label();

	//hold the current frame to be / being displayed
    QImage current_frame;

    //the (float) coords of the mouse position as the user draws things
    std::vector<QPointF> annotation_locations;
    std::vector<QPointF> limbo_points;

    QGraphicsTextItem cursor;
    int annotation_brushsz;
    bool drawing_annotations;
};


class FrameView : public QGraphicsView
{
public:
    FrameView(QGraphicsScene* fview, QWidget* parent = 0)
        : QGraphicsView(fview, parent)
    {
        fviewer = reinterpret_cast<FrameViewer*>(fview);
        this->update();
    }

    QSize sizeHint() const override {
        return fviewer->get_size_hint(); 
    }

    void update_frame(const QImage& frame) {
        //re-set any viewing transformations
        resetMatrix();
        fviewer->display_frame(frame);
    }

    std::vector<QPointF> get_frame_annotations() const {
        return fviewer->get_frame_annotations();
	}

protected:
    //for zooming into the scene -- hold down control to zoom (versus just scrolling up and down)
    void wheelEvent(QWheelEvent*) override;

private:
    FrameViewer* fviewer; 
};

#endif
