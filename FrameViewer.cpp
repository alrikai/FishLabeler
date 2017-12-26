#include "FrameViewer.hpp"

#include <iostream>
#include <QImage>

namespace utils {
    template <typename PtListT>
    void point_un_redo(PtListT& src, PtListT& dst, int num_times=1)
    {
        for (int i = 0; i < num_times; i++) {
            if (src.size() > 0) {
                auto inv_point = src.back();
                src.pop_back();
                dst.push_back(inv_point);
            }
        }
    }
}

FrameViewer::FrameViewer(const QImage& initial_frame, QObject* parent)
    : QGraphicsScene(parent) 
{
    //initialize the current frame to a placeholder
    //static const QString placeholder_path {"/home/alrik/Projects/fishlabeler/data/default_placeholder.png"};
    //current_frame = QImage(placeholder_path);

    drawing_annotations = false;
    annotation_brushsz = 8;
    display_frame(initial_frame);
}

void FrameViewer::display_frame(const QImage& frame) 
{
    //TODO: before changing out the current frame for the new one, marshal all of its information, and (optionally) write it out to disk?
    //Need to figure out how exactly I want to handle this part. 
    
    current_frame = frame; 
    annotation_locations.clear();
    limbo_points.clear();
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
    for (int i = 0; i < annotation_locations.size(); i++) {
        auto npt_loc = annotation_locations[i];
        painter->drawPoint(npt_loc.x(),npt_loc.y());
        std::cout << "drawing pt " << npt_loc.x() << ", " << npt_loc.y() << std::endl;
    }
}

void FrameViewer::mouseMoveEvent(QGraphicsSceneMouseEvent* mevt)
{
    if (drawing_annotations) {
        //NOTE: could also use e.g. mevt->scenePos().x(), mevt->scenePos().y()
        annotation_locations.emplace_back(mevt->scenePos().x(), mevt->scenePos().y());
        std::cout << "mpos: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << std::endl;
        this->update();
    }
}

void FrameViewer::mousePressEvent(QGraphicsSceneMouseEvent* mevt)
{
    annotation_locations.emplace_back(mevt->scenePos().x(), mevt->scenePos().y());
    std::cout << "mpos click: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << std::endl;
    drawing_annotations = true;
    this->update();
}

void FrameViewer::mouseReleaseEvent(QGraphicsSceneMouseEvent* mevt)
{
    annotation_locations.emplace_back(mevt->scenePos().x(), mevt->scenePos().y());
    std::cout << "mpos rel: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << std::endl;
    drawing_annotations = false;
    this->update();
}

void FrameViewer::undo_label()
{
    utils::point_un_redo(annotation_locations, limbo_points);
    this->update();
}

void FrameViewer::redo_label()
{
    utils::point_un_redo(limbo_points, annotation_locations);
    this->update();
}

void FrameViewer::keyPressEvent(QKeyEvent *evt)
{
    if (evt->modifiers() & Qt::ControlModifier) {
        switch(evt->key()) {
            case Qt::Key_Z:
                std::cout << "UNDO key" << std::endl;
                undo_label();
                break;
            case Qt::Key_R:
                std::cout << "REDO key" << std::endl;
                redo_label();
                break;
            default:
                std::cout << "key: " << evt->key() << std::endl;
        }
    } else {
        QGraphicsScene::keyPressEvent(evt);
    }
}

void FrameView::wheelEvent(QWheelEvent *evt)
{
    if (evt->modifiers() & Qt::ControlModifier) {
        const QGraphicsView::ViewportAnchor prev_anchor = transformationAnchor();
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        //TODO: this is zooming proportionately to how much scrolling is done, but it is a 
        //bit unreliable and ends up with large, sudden jumps sometimes... which is annoying.
        //might want to move to having it just scroll by a constant factor (i.e. 1.2 up/down each time) 
        float zfactor = evt->angleDelta().y() / 100.f;
        if (zfactor < 0) {
            zfactor = std::abs(zfactor) - 1.f; 
        }
        std::cout << "zooming " << (zfactor > 1.f ? "IN ":"OUT ") << "by " << zfactor << std::endl;
        scale(zfactor, zfactor);
        setTransformationAnchor(prev_anchor);
    } else {
        QGraphicsView::wheelEvent(evt);
    }
}
