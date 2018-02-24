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

    Qt::GlobalColor get_qt_color(const int id) {
        //TODO: in theory, this would be a good case for a full-blown colormap
        static constexpr int NUM_COLORS = 17;
        static std::array<Qt::GlobalColor, NUM_COLORS> annotation_color = {{
            Qt::white,
            Qt::black,
            Qt::red,
            Qt::darkRed,
            Qt::green,
            Qt::darkGreen,
            Qt::blue,
            Qt::darkBlue,
            Qt::cyan,
            Qt::darkCyan,
            Qt::magenta,
            Qt::darkMagenta,
            Qt::yellow,
            Qt::darkYellow,
            Qt::gray,
            Qt::darkGray,
            Qt::lightGray
        }};

        const int color_idx = id % NUM_COLORS;
        return annotation_color[color_idx];
    }

    void add_segbrush_pixels(std::vector<QPoint>& current_mask, const int rowpos, const int colpos, const int brushsz)
    {
        for (int row_offset = 0; row_offset < brushsz; row_offset++) {
            //TODO: should we just round down? i.e. make it s.t. all odd-values for brushsz == itself-1?
            const int row_offset_factor = row_offset - brushsz / 2; 
            const int rowpos_px = rowpos + row_offset_factor;
            for (int col_offset = 0; col_offset < brushsz; col_offset++) {
                const int col_offset_factor = col_offset - brushsz / 2; 
                QPoint spt {rowpos_px, colpos + col_offset_factor};
                current_mask.emplace_back(spt);
            }
        }
    }
}

FrameViewer::FrameViewer(const QImage& initial_frame, QObject* parent)
    : QGraphicsScene(parent) 
{
    drawing_annotations = false;
    annotation_brushsz = 8;
    display_frame(initial_frame);
    mode = ANNOTATION_MODE::BOUNDINGBOX;
    current_pixframe = nullptr;
}

void FrameViewer::display_frame(const QImage& frame) 
{
    //if we are doing segmentation, write out whatever the current mask is as well
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        set_instance_id(current_id);
    }

    current_frame = QPixmap::fromImage(frame); 

    //moving to the next frame, so clear out the current frame's annotations
    annotation_locations.clear();
    limbo_points.clear();
    boundingbox_locations.clear();
    limbo_bboxes.clear();
    this->update();
}

void FrameViewer::drawBackground(QPainter* painter, const QRectF& rect)
{
    if (current_pixframe == nullptr) { 
        current_pixframe = this->addPixmap(current_frame);
    } else { 
        current_pixframe->setPixmap(current_frame);
    }
}

void FrameViewer::drawForeground(QPainter* painter, const QRectF& rect)
{
    QPen pen;
    pen.setWidth(annotation_brushsz);

    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        //we only use the brushsz for the drawing stage to reduce user burden
        pen.setWidth(1);
        for (int i = 0; i < annotation_locations.size(); i++) {
            auto smask_inst = annotation_locations[i].smask;
            //adjust pen color per instance ID
            pen.setBrush(utils::get_qt_color(annotation_locations[i].instance_id));
            painter->setPen(pen);   
            for (auto npt_loc : smask_inst) {
                painter->drawPoint(npt_loc.x(),npt_loc.y());
            }
        }

        pen.setBrush(Qt::lightGray);
        painter->setPen(pen);   
        //draw the current mask annotation as well
        for (auto npt_loc : current_mask) {
            painter->drawPoint(npt_loc.x(),npt_loc.y());
        }
    } else {
        for (auto bbox_md : boundingbox_locations) {
            //adjust pen color based on instance ID
            pen.setBrush(utils::get_qt_color(bbox_md.instance_id));
            painter->setPen(pen);   
            painter->drawRect(bbox_md.bbox);
        }

        pen.setBrush(Qt::lightGray);
        painter->setPen(pen);   
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
            std::cout << "mpos: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << " @bsz " << annotation_brushsz << std::endl;
            const int rowpos_click = static_cast<int>(std::round(mevt->scenePos().x()));
            const int colpos_click = static_cast<int>(std::round(mevt->scenePos().y()));
            utils::add_segbrush_pixels(current_mask, rowpos_click, colpos_click, annotation_brushsz);
        } else {
            current_bbox.setBottomRight(QPoint(mevt->scenePos().x(), mevt->scenePos().y()));
        }
        this->update();
    }
}

void FrameViewer::mousePressEvent(QGraphicsSceneMouseEvent* mevt)
{
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        std::cout << "mpos: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << " @bsz " << annotation_brushsz << std::endl;
        const int rowpos_click = static_cast<int>(std::round(mevt->scenePos().x()));
        const int colpos_click = static_cast<int>(std::round(mevt->scenePos().y()));
        utils::add_segbrush_pixels(current_mask, rowpos_click, colpos_click, annotation_brushsz);
    } else {
        auto mdata_item = itemAt(mevt->pos(), QTransform());
        if (mdata_item) {
            std::cout << "clicked on item " << mdata_item << std::endl;
        } else {
            std::cout << "did not click on item " << std::endl;
        }
        static const QSize default_bbox_sz {0, 0};
        current_bbox = QRect(QPoint(mevt->scenePos().x(), mevt->scenePos().y()), default_bbox_sz);
    }

    drawing_annotations = true;
    this->update();
}

void FrameViewer::mouseReleaseEvent(QGraphicsSceneMouseEvent* mevt)
{
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        std::cout << "mpos: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << " @bsz " << annotation_brushsz << std::endl;
        const int rowpos_click = static_cast<int>(std::round(mevt->scenePos().x()));
        const int colpos_click = static_cast<int>(std::round(mevt->scenePos().y()));
        utils::add_segbrush_pixels(current_mask, rowpos_click, colpos_click, annotation_brushsz);
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
        //TODO: if the brushsz changes from when the point was originally drawn, this will not be correct
        const int num_pts_remove = annotation_brushsz*annotation_brushsz;
        utils::point_un_redo(current_mask, limbo_points, num_pts_remove);

        //TODO: do we need to propogate this to the annotation_locations as well? --> if limbo pts is empty, might make sense to continue the removeal into 
        //the ending of annotation_locations as well, so that we can undo for longer
    } else {
        utils::point_un_redo(boundingbox_locations, limbo_bboxes);
    }
    this->update();
}

void FrameViewer::redo_label()
{
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        //TODO: need th brush size as well?
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
