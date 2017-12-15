#include "FrameViewer.hpp"

#include <iostream>
#include <QImage>

FrameViewer::FrameViewer(QObject* parent)
    : QGraphicsScene(parent) 
{
    //initialize the current frame to a placeholder
    static const QString placeholder_path {"/home/alrik/Projects/fishlabeler/data/default_placeholder.png"};
    current_frame = QImage(placeholder_path);
    this->update();
}

void FrameViewer::display_frame(const VideoFrame<FrameViewer::PixelT>& frame) {
    
    //TODO: double check if the data is stored per channel, or interleaved
    //TODO: change channel ordering to be BGR (or change ffmpeg part to be RGB)
    const int line_bytesz = frame.stride * 3 * sizeof(PixelT);
    current_frame = QImage(frame.data.get(), frame.width, frame.height, line_bytesz, QImage::Format_RGB888);
    this->update();
}

void FrameViewer::drawBackground(QPainter* painter, const QRectF &rect)
{
    this->addPixmap(QPixmap::fromImage(current_frame));
    QPen pen;
    pen.setWidth(1);
    pen.setBrush(Qt::lightGray);
    painter->setPen(pen);   
    //TODO: need to convert the new_points to frame coordinates and write them to drawn_points
    for (int i = 0; i < new_points.size(); i++) {
        auto npt_loc = new_points[i];
        painter->drawPoint(npt_loc.x(),npt_loc.y());
    }
    new_points.clear();
}

void FrameViewer::mouseMoveEvent(QGraphicsSceneMouseEvent* mevt)
{
    //NOTE: could also use e.g. mevt->scenePos().x(), mevt->scenePos().y()
    new_points.emplace_back(mevt->scenePos().x(), mevt->scenePos().y());
    std::cout << "mpos: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << std::endl;
    this->update();
}

void FrameViewer::mousePressEvent(QGraphicsSceneMouseEvent* mevt)
{
    this->update();
}

void FrameViewer::mouseReleaseEvent(QGraphicsSceneMouseEvent* mevt)
{
    this->update();
}


