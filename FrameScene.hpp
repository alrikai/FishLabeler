#ifndef FISHLABELER_FRAMESCENE_HPP
#define FISHLABELER_FRAMESCENE_HPP

#include <memory>

#include <QWidget>
#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QRect>
#include <QPoint>

#include "AnnotationTypes.hpp"
#include "BoundingBoxViz.hpp"

class FrameScene : public QGraphicsScene
{
    Q_OBJECT
public:
    using PixelT = uint8_t;
    FrameScene(const QImage& initial_frame, QObject *parent = 0);

    void display_frame(const QImage& frame);

    QSize get_size_hint() const {
        QSize sz{current_frame.width(), current_frame.height()};
        return sz; 
    }

    void set_instance_id(const int id) { 
        //move the existinig 'current' mask annotation over into the full set for the frame
        annotation_locations.emplace_back(std::move(current_mask), current_id);
        current_id = id;
        this->update();
    }

    void set_annotation_mode(ANNOTATION_MODE annotation_mode) {
        mode = annotation_mode;
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

    std::vector<BoundingBoxMD> get_bounding_boxes() const {
        std::vector<BoundingBoxMD> bbox_md;
        for (auto bbox_it : boundingbox_locations) {
            bbox_md.emplace_back(bbox_it->get_bbox_metadata());
        }
        return bbox_md;
    }

    std::vector<PixelLabelMB> get_frame_annotations() {
        if (current_mask.size() > 0) {
            annotation_locations.emplace_back(std::move(current_mask), current_id);
        }
        return annotation_locations;
    }
    
    void set_metadata(FrameAnnotations&& metadata) {
        for (auto mdata_bbox_it : metadata.bboxes) {
            boundingbox_locations.emplace_back(std::make_shared<BoundingBoxViz>(mdata_bbox_it));
            //boundingbox_locations.insert(boundingbox_locations.end(), metadata.bboxes.begin(), metadata.bboxes.end());
        }
        annotation_locations.insert(annotation_locations.end(), metadata.segm_points.begin(), metadata.segm_points.end());
    }

    void get_next_bounding_box() {
        emit_bbox = true;
    }

    QGraphicsPixmapItem* get_current_pixframe() const {
        return current_pixframe;
    }  
signals:
    void bounding_box_created(const QRect& bbox, const int current_id);

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
    QPixmap current_frame;
    QGraphicsPixmapItem* current_pixframe;
    //the (float) coords of the mouse position as the user draws things
    //in segmentation mode
    std::vector<PixelLabelMB> annotation_locations;
    std::vector<QPoint> limbo_points;
    std::vector<QPoint> current_mask;

    //the bounding box coordinates when the user is drawing in bounding box mode
    using bbox_metadata_t = std::shared_ptr<BoundingBoxViz>; //BoundingBoxMD;
    std::vector<bbox_metadata_t> boundingbox_locations;
    std::vector<bbox_metadata_t> limbo_bboxes;
    bbox_metadata_t current_bbox;

    QGraphicsTextItem cursor;
    int annotation_brushsz;
    bool drawing_annotations;
    int current_id;

    //TODO: figure out a cleaner way to have these -- this is for adjusting the bounding boxes, maybe we should move to using
    //policies for the annotation type being done or something
    int selected_bbox;
    QPointF selected_bbox_pt;

    bool emit_bbox;

    ANNOTATION_MODE mode;
};

#endif


