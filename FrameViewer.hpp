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
    FrameViewer(QObject *parent = 0);

    void display_frame(const VideoFrame<PixelT>& frame);

    QSize get_size_hint() const {
        QSize sz{current_frame.width(), current_frame.height()};
        return sz; 
    }

protected slots:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent * mouseEvent) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QImage current_frame;

    //the (float) coords of the mouse position as the user draws things
    std::vector<QPointF> new_points;
    //the (accumulated) pixel coords on the frame that the user has drawn
    std::vector<QPoint> drawn_points;

    QGraphicsTextItem cursor;
};


class FrameView : public QGraphicsView
{
public:
    FrameView(QGraphicsScene* fview, QObject* parent = 0) 
    {
        fviewer = reinterpret_cast<FrameViewer*>(fview);
    }

    QSize sizeHint() const override {
        return fviewer->get_size_hint(); 
    }
private:
    FrameViewer* fviewer; 
};

#endif
