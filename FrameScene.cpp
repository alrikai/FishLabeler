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

FrameScene::FrameScene(const QImage& initial_frame, QObject* parent)
    : QGraphicsScene(parent) 
{
    drawing_annotations = false;
    annotation_brushsz = 8;
    display_frame(initial_frame);
    mode = ANNOTATION_MODE::BOUNDINGBOX;
    current_pixframe = nullptr;
    selected_bbox = -1;
    emit_bbox = false;
    current_id = 0;

    //set the allowable thresholds for any annotations based on the image dimensions
    allowable_height = initial_frame.height();
    allowable_width = initial_frame.width();
}

void FrameScene::display_frame(const QImage& frame) 
{
    //if we are doing segmentation, write out whatever the current mask is as well
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        set_instance_id(current_id);
    }

    current_frame = QPixmap::fromImage(frame); 

    //moving to the next frame, so clear out the current frame's annotations
    annotation_locations.clear();
    limbo_points.clear();
    for (auto& bbox_item : boundingbox_locations) {
        this->removeItem(bbox_item.get());
    }
    boundingbox_locations.clear();
    if (current_bbox) {
        this->removeItem(current_bbox.get());
    }
    limbo_bboxes.clear();
    this->update();
}

void FrameScene::drawBackground(QPainter* painter, const QRectF& rect)
{
    if (current_pixframe == nullptr) { 
        current_pixframe = this->addPixmap(current_frame);
    } else { 
        current_pixframe->setPixmap(current_frame);
    }
}

void FrameScene::drawForeground(QPainter* painter, const QRectF& rect)
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
        for (int i = 0; i < boundingbox_locations.size(); i++) {
            //adjust pen color based on instance ID
            pen.setBrush(utils::get_qt_color(boundingbox_locations[i]->get_id()));
            painter->setPen(pen);   
            boundingbox_locations[i]->paint(painter, nullptr, nullptr);
            //painter->drawRect(bbox_md.bbox);
        }

        pen.setBrush(Qt::lightGray);
        painter->setPen(pen);   
        //draw the current_bbox only if it is currently being drawn (and thus not in the boundingbox_locations vector)
        if (drawing_annotations && current_bbox) {
            current_bbox->paint(painter, nullptr, nullptr);
        }
    }
}

void FrameScene::mouseMoveEvent(QGraphicsSceneMouseEvent* mevt)
{
    QGraphicsScene::mouseMoveEvent(mevt);
    if (drawing_annotations) {
        if (mode == ANNOTATION_MODE::SEGMENTATION) {
            std::cout << "mpos: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << " @bsz " << annotation_brushsz << std::endl;
            const int rowpos_click = static_cast<int>(std::round(mevt->scenePos().x()));
            const int colpos_click = static_cast<int>(std::round(mevt->scenePos().y()));
            utils::add_segbrush_pixels(current_mask, rowpos_click, colpos_click, annotation_brushsz);
        } else {
            //TODO: consider if we want to re-draw the bounding box as the mouse moves
            if (selected_bbox < 0 && current_bbox) {
                current_bbox->get_bounding_box().setBottomRight(QPoint(mevt->scenePos().x(), mevt->scenePos().y()));
            }
        }
        this->update();
    }
}

void FrameScene::mousePressEvent(QGraphicsSceneMouseEvent* mevt)
{
    QGraphicsScene::mousePressEvent(mevt);
    if(!mevt->isAccepted()) {
        std::cout << "Mouse Click !Accepted" << std::endl;
    } else {
        std::cout << "Mouse Click Accepted" << std::endl;
    }
    std::cout << "mpos: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << " @bsz " << annotation_brushsz << std::endl;

    const int rowpos_click = static_cast<int>(std::round(mevt->scenePos().y()));
    const int colpos_click = static_cast<int>(std::round(mevt->scenePos().x()));
    const int selected_rowpos = std::max(0, std::min(rowpos_click, allowable_height));
    const int selected_colpos = std::max(0, std::min(colpos_click, allowable_width));

    if (mode == ANNOTATION_MODE::SEGMENTATION) {

        utils::add_segbrush_pixels(current_mask, selected_colpos, selected_rowpos, annotation_brushsz);
    } else {
        //reset the current selection
        selected_bbox = -1;
        for (int bbox_idx = 0; bbox_idx < boundingbox_locations.size(); bbox_idx++) {
            if (boundingbox_locations[bbox_idx]->intersects_bbox_frame(mevt->scenePos(), annotation_brushsz)) {
                selected_bbox = bbox_idx;
                break;
            }
        }

        //if we clicked on a bounding box, modify that bounding box. If not, then add a new one
        if (selected_bbox >= 0) {
            std::cout << "Clicked on BBOX @ID " << boundingbox_locations[selected_bbox]->get_id() << std::endl;
            current_bbox = nullptr;
        } else {
            static const QSize default_bbox_sz {0, 0};
            //clip the click to the allowable boundaries
            auto current_bbox_rect = QRect(QPoint(selected_colpos, selected_rowpos), default_bbox_sz);
            current_bbox = std::make_shared<BoundingBoxViz>(current_bbox_rect, current_id);
            this->addItem(current_bbox.get());
            selected_bbox = -1;
        }
    }

    drawing_annotations = true;
    this->update();
}

void FrameScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* mevt)
{
    QGraphicsScene::mouseReleaseEvent(mevt);
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        std::cout << "mpos: " << mevt->scenePos().x() << ", " << mevt->scenePos().y() << " @bsz " << annotation_brushsz << std::endl;
        const int rowpos_click = static_cast<int>(std::round(mevt->scenePos().x()));
        const int colpos_click = static_cast<int>(std::round(mevt->scenePos().y()));
        utils::add_segbrush_pixels(current_mask, rowpos_click, colpos_click, annotation_brushsz);
    } else {
        if (selected_bbox >= 0) {
            auto bbox_moveamt = mevt->scenePos() - selected_bbox_pt;
            QPoint bbox_movement {std::rint(bbox_moveamt.x()), std::rint(bbox_moveamt.y())};
            std::cout << "Moving bbox by " << bbox_movement.x() << ", " <<  bbox_movement.y() << std::endl;

            boundingbox_locations[selected_bbox]->get_bounding_box().translate(bbox_movement); 
        } else {
            if (current_bbox) {
                current_bbox->get_bounding_box().setBottomRight(QPoint(mevt->scenePos().x(), mevt->scenePos().y()));
                current_bbox->set_id(current_id);
                //NOTE: current_bbox will have already been added to the scene
                boundingbox_locations.emplace_back(current_bbox);
                if (emit_bbox) {
                    emit bounding_box_created(current_bbox->get_bounding_box(), current_id);
                    emit_bbox = false;
                }
            }
        }
    }
    drawing_annotations = false;
    this->update();
}

void FrameScene::undo_label()
{
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        //TODO: if the brushsz changes from when the point was originally drawn, this will not be correct
        const int num_pts_remove = annotation_brushsz*annotation_brushsz;
        utils::point_un_redo(current_mask, limbo_points, num_pts_remove);

        //TODO: do we need to propogate this to the annotation_locations as well? --> if limbo pts is empty, might make sense to continue the removeal into 
        //the ending of annotation_locations as well, so that we can undo for longer
    } else {
        utils::point_un_redo(boundingbox_locations, limbo_bboxes);
        if (limbo_bboxes.size() > 0) {
            auto rm_bbox = limbo_bboxes.back();
            this->removeItem(rm_bbox.get());
        }
    }
    this->update();
}

void FrameScene::redo_label()
{
    if (mode == ANNOTATION_MODE::SEGMENTATION) {
        //TODO: need th brush size as well?
        utils::point_un_redo(limbo_points, current_mask);
        //TODO: do we need to propogate this to the annotation_locations as well?
    } else {
        utils::point_un_redo(limbo_bboxes, boundingbox_locations);
        if (boundingbox_locations.size() > 0) {
            this->addItem(boundingbox_locations.back().get());
        }
    }
    this->update();
}

void FrameScene::keyPressEvent(QKeyEvent *evt)
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
                /*
                 * NOTE: this got moved to the main window, I dont think I want a bi-directional 
                 * communication between the viewer and the main UI for this. Plus, the hotkeys
                 * only worked as intended when the window had focus, but one usully wants to set
                 * the annotation mode before clicking on the image
            case Qt::Key_B:
                std::cout << "BOUNDING_BOX key" << std::endl;
                mode = ANNOTATION_MODE::BOUNDINGBOX;
                break;
            case Qt::Key_S:
                std::cout << "SEGMENTATION key" << std::endl;
                mode = ANNOTATION_MODE::SEGMENTATION;
                break;
                */
            default:
                std::cout << "key: " << evt->key() << std::endl;
        }
    } else {
        QGraphicsScene::keyPressEvent(evt);
    }
}
