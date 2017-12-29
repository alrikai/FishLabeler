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

    mode = ANNOTATION_MODE::SEGMENTATION;
}

void FrameViewer::display_frame(const QImage& frame) 
{
    current_frame = frame; 

    annotation_locations.clear();
    limbo_points.clear();
    boundingbox_locations.clear();
    limbo_bboxes.clear();
    
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

    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        for (int i = 0; i < annotation_locations.size(); i++) {
            auto npt_loc = annotation_locations[i];
            painter->drawPoint(npt_loc.x(),npt_loc.y());
            std::cout << "drawing pt " << npt_loc.x() << ", " << npt_loc.y() << std::endl;
        }
    } else {
        for (auto bbox : boundingbox_locations) {
            painter->drawRect(bbox);
        }
        //draw the current_bbox only if it is currently being drawn (and thus not in the boundingbox_locations vector)
        if (drawing_annotations) {
            painter->drawRect(current_bbox);
        }
    }
}

void FrameViewer::mouseMoveEvent(QGraphicsSceneMouseEvent* mevt)
{
    if (drawing_annotations) {

        if (mode == ANNOTATION_MODE::SEGMENTATION) {
            //NOTE: could also use e.g. mevt->scenePos().x(), mevt->scenePos().y()
            annotation_locations.emplace_back(mevt->scenePos().x(), mevt->scenePos().y());
            std::cout << "mpos: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << std::endl;
        } else {
            current_bbox.setBottomRight(QPointF(mevt->scenePos().x(), mevt->scenePos().y()));
        }
        this->update();
    }
}

void FrameViewer::mousePressEvent(QGraphicsSceneMouseEvent* mevt)
{
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        annotation_locations.emplace_back(mevt->scenePos().x(), mevt->scenePos().y());
        std::cout << "mpos click: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << std::endl;
    } else {
        static const QSizeF default_bbox_sz {0, 0};
        current_bbox = QRectF(QPointF(mevt->scenePos().x(), mevt->scenePos().y()), default_bbox_sz);
    }

    drawing_annotations = true;
    this->update();
}

void FrameViewer::mouseReleaseEvent(QGraphicsSceneMouseEvent* mevt)
{
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        annotation_locations.emplace_back(mevt->scenePos().x(), mevt->scenePos().y());
        std::cout << "mpos rel: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << std::endl;
    } else {
        current_bbox.setBottomRight(QPointF(mevt->scenePos().x(), mevt->scenePos().y()));
        boundingbox_locations.emplace_back(current_bbox);
    }
    drawing_annotations = false;
    this->update();
}

void FrameViewer::undo_label()
{
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        utils::point_un_redo(annotation_locations, limbo_points);
    } else {
        utils::point_un_redo(boundingbox_locations, limbo_bboxes);
    }
    this->update();
}

void FrameViewer::redo_label()
{
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        utils::point_un_redo(limbo_points, annotation_locations);
    } else {
        utils::point_un_redo(limbo_bboxes, boundingbox_locations);
    }
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
            case Qt::Key_B:
                std::cout << "BOUNDING_BOX key" << std::endl;
                mode = ANNOTATION_MODE::BOUNDINGBOX;
                break;
            case Qt::Key_S:
                std::cout << "SEGMENTATION key" << std::endl;
                mode = ANNOTATION_MODE::SEGMENTATION;
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
