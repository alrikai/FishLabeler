#include "FrameViewer.hpp"

#include <iostream>
#include <QImage>

FrameViewer::FrameViewer(const VideoFrame<FrameViewer::PixelT>& initial_frame, QObject* parent)
    : QGraphicsScene(parent) 
{
    //initialize the current frame to a placeholder
    //static const QString placeholder_path {"/home/alrik/Projects/fishlabeler/data/default_placeholder.png"};
    //current_frame = QImage(placeholder_path);

    drawing_annotations = false;
    annotation_brushsz = 8;
    display_frame(initial_frame);
}

void FrameViewer::display_frame(const VideoFrame<FrameViewer::PixelT>& frame) {
    
    //TODO: double check if the data is stored per channel, or interleaved
    //TODO: change channel ordering to be BGR (or change ffmpeg part to be RGB)
    const int line_bytesz = frame.stride * 3 * sizeof(PixelT);
    current_frame = QImage(frame.data.get(), frame.width, frame.height, line_bytesz, QImage::Format_RGB888);

    new_points.clear();

    this->update();
}

void FrameViewer::drawBackground(QPainter* painter, const QRectF &rect)
{
    this->addPixmap(QPixmap::fromImage(current_frame));
}

void FrameViewer::drawForeground(QPainter* painter, const QRectF &rect)
{
    QPen pen;
    pen.setWidth(annotation_brushsz);
    //pen.setBrush(Qt::lightGray);
    pen.setBrush(Qt::red);
    painter->setPen(pen);   
    //TODO: need to convert the new_points to frame coordinates and write them to drawn_points
    for (int i = 0; i < new_points.size(); i++) {
        auto npt_loc = new_points[i];
        painter->drawPoint(npt_loc.x(),npt_loc.y());
        std::cout << "drawing pt " << npt_loc.x() << ", " << npt_loc.y() << std::endl;
    }
}

void FrameViewer::mouseMoveEvent(QGraphicsSceneMouseEvent* mevt)
{
    if (drawing_annotations) {
        //NOTE: could also use e.g. mevt->scenePos().x(), mevt->scenePos().y()
        new_points.emplace_back(mevt->scenePos().x(), mevt->scenePos().y());
        std::cout << "mpos: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << std::endl;
        this->update();
    }
}

void FrameViewer::mousePressEvent(QGraphicsSceneMouseEvent* mevt)
{
    new_points.emplace_back(mevt->scenePos().x(), mevt->scenePos().y());
    std::cout << "mpos click: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << std::endl;
    drawing_annotations = true;
    this->update();
}

void FrameViewer::mouseReleaseEvent(QGraphicsSceneMouseEvent* mevt)
{
    new_points.emplace_back(mevt->scenePos().x(), mevt->scenePos().y());
    std::cout << "mpos rel: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << std::endl;
    drawing_annotations = false;
    this->update();
}

void FrameView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const QGraphicsView::ViewportAnchor prev_anchor = transformationAnchor();
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        //TODO: this is zooming proportionately to how much scrolling is done, but it is a 
        //bit unreliable and ends up with large, sudden jumps sometimes... which is annoying.
        //might want to move to having it just scroll by a constant factor (i.e. 1.2 up/down each time) 
        float zfactor = event->angleDelta().y() / 100.f;
        if (zfactor < 0) {
            zfactor = std::abs(zfactor) - 1.f; 
        }
        std::cout << "zooming " << (zfactor > 1.f ? "IN ":"OUT ") << "by " << zfactor << std::endl;
        scale(zfactor, zfactor);
        setTransformationAnchor(prev_anchor);
    } else {
        QGraphicsView::wheelEvent(event);
    }
}
