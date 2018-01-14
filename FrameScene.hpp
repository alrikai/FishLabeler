#ifndef FISHLABELER_FRAMESCENE_HPP
#define FISHLABELER_FRAMESCENE_HPP

#include <QWidget>
#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QRect>
#include <QPoint>

#include "AnnotationTypes.hpp"

class FrameViewer : public QGraphicsScene
{
public:
    using PixelT = uint8_t;
    FrameViewer(const QImage& initial_frame, QObject *parent = 0);

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
        return boundingbox_locations;
    }

    std::vector<PixelLabelMB> get_frame_annotations() const {
        return annotation_locations;
    }
    
    void set_metadata(FrameAnnotations&& metadata) {
        boundingbox_locations.insert(boundingbox_locations.end(), metadata.bboxes.begin(), metadata.bboxes.end());
        annotation_locations.insert(annotation_locations.end(), metadata.segm_points.begin(), metadata.segm_points.end());
    }

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
    QImage current_frame;

    //the (float) coords of the mouse position as the user draws things
    //in segmentation mode
    std::vector<PixelLabelMB> annotation_locations;
    std::vector<QPoint> limbo_points;
    std::vector<QPoint> current_mask;

    //the bounding box coordinates when the user is drawing in bounding box mode
    std::vector<BoundingBoxMD> boundingbox_locations;
    std::vector<BoundingBoxMD> limbo_bboxes;
    QRect current_bbox;

    QGraphicsTextItem cursor;
    int annotation_brushsz;
    bool drawing_annotations;
    int current_id;

    ANNOTATION_MODE mode;
};

#endif


