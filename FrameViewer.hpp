#ifndef FISHLABELER_FRAMEVIEWER_HPP
#define FISHLABELER_FRAMEVIEWER_HPP

#include <QWidget>
#include "VideoFrame.hpp"

#include <QObject>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>


class FrameViewer : public QGraphicsScene
{
public:
    using PixelT = uint8_t;
    FrameViewer(const VideoFrame<PixelT>& initial_frame, QObject *parent = 0);

    void display_frame(const VideoFrame<PixelT>& frame);

    QSize get_size_hint() const {
        QSize sz{current_frame.width(), current_frame.height()};
        return sz; 
    }

    void set_brushsz(int brushsz) {
        annotation_brushsz = brushsz;
    }

protected slots:
    void drawBackground(QPainter* painter, const QRectF &rect) override;
    void drawForeground(QPainter* painter, const QRectF &rect) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;

private:
    QImage current_frame;

    //the (float) coords of the mouse position as the user draws things
    std::vector<QPointF> new_points;
    //the (accumulated) pixel coords on the frame that the user has drawn
    std::vector<QPoint> drawn_points;

    QGraphicsTextItem cursor;
    int annotation_brushsz;
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
private:
    FrameViewer* fviewer; 
};

#endif
