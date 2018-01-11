#include "FrameScene.hpp"

#include <cmath>
#include <iostream>
#include <QImage>
#include <QKeyEvent>
#include <QPainter>

namespace utils {
    template <typename SrcT, typename DstT>
    void point_un_redo(std::vector<SrcT>& src, std::vector<DstT>& dst, int num_times=1)
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

    mode = ANNOTATION_MODE::BOUNDINGBOX;
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

void FrameViewer::drawBackground(QPainter* painter, const QRectF&  rect)
{
    this->addPixmap(QPixmap::fromImage(current_frame));
}

void FrameViewer::drawForeground(QPainter* painter, const QRectF& rect)
{
    QPen pen;
    pen.setWidth(annotation_brushsz);
    //pen.setBrush(Qt::lightGray);
    pen.setBrush(Qt::red);
    painter->setPen(pen);   

    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        for (int i = 0; i < annotation_locations.size(); i++) {
            auto smask_inst = annotation_locations[i].smask;
            //TODO: adjust pen color per instance ID
            for (auto npt_loc : smask_inst) {
                painter->drawPoint(npt_loc.x(),npt_loc.y());
                std::cout << "drawing pt " << npt_loc.x() << ", " << npt_loc.y() << std::endl;
            }

            //draw the current mask annotation as well
            for (auto npt_loc : current_mask) {
                painter->drawPoint(npt_loc.x(),npt_loc.y());
                std::cout << "drawing pt " << npt_loc.x() << ", " << npt_loc.y() << std::endl;
            }
        }
    } else {
        for (auto bbox_md : boundingbox_locations) {
            //TODO: adjust pen color based on instance ID
            painter->drawRect(bbox_md.bbox);
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
            QPoint spt {static_cast<int>(std::round(mevt->scenePos().x())), static_cast<int>(std::round(mevt->scenePos().y()))};
            current_mask.emplace_back(spt);
            std::cout << "mpos: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << std::endl;
        } else {
            current_bbox.setBottomRight(QPoint(mevt->scenePos().x(), mevt->scenePos().y()));
        }
        this->update();
    }
}

void FrameViewer::mousePressEvent(QGraphicsSceneMouseEvent* mevt)
{
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        QPoint spt {static_cast<int>(std::round(mevt->scenePos().x())), static_cast<int>(std::round(mevt->scenePos().y()))};
        current_mask.emplace_back(spt);
        std::cout << "mpos click: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << std::endl;
    } else {
        static const QSize default_bbox_sz {0, 0};
        current_bbox = QRect(QPoint(mevt->scenePos().x(), mevt->scenePos().y()), default_bbox_sz);
    }

    drawing_annotations = true;
    this->update();
}

void FrameViewer::mouseReleaseEvent(QGraphicsSceneMouseEvent* mevt)
{
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        QPoint spt {static_cast<int>(std::round(mevt->scenePos().x())), static_cast<int>(std::round(mevt->scenePos().y()))};
        current_mask.emplace_back(spt);
        std::cout << "mpos rel: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << std::endl;
    } else {
        current_bbox.setBottomRight(QPoint(mevt->scenePos().x(), mevt->scenePos().y()));
        boundingbox_locations.emplace_back(current_bbox, current_id);
    }
    drawing_annotations = false;
    this->update();
}

void FrameViewer::undo_label()
{
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        utils::point_un_redo(current_mask, limbo_points);
        //TODO: do we need to propogate this to the annotation_locations as well?
    } else {
        utils::point_un_redo(boundingbox_locations, limbo_bboxes);
    }
    this->update();
}

void FrameViewer::redo_label()
{
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        utils::point_un_redo(limbo_points, current_mask);
        //TODO: do we need to propogate this to the annotation_locations as well?
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
