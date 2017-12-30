#ifndef FISHLABELER_FRAMEVIEWER_HPP
#define FISHLABELER_FRAMEVIEWER_HPP

#include <QWidget>
#include <QObject>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QWheelEvent>
#include <QRect>

enum class ANNOTATION_MODE {
    SEGMENTATION,
    BOUNDINGBOX
};

//something to encapulate all of the user-supplied information for a given frame
struct FrameAnnotations {
    FrameAnnotations(std::vector<QRect>&& fvboxes, std::vector<QPoint>&& fvpoints)
        : bboxes(std::move(fvboxes)), segm_points(std::move(fvpoints))
    {}

    std::vector<QRect> bboxes;
    std::vector<QPoint> segm_points;
};

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

    std::vector<QRect> get_bounding_boxes() const {
        return boundingbox_locations;
    }

    std::vector<QPoint> get_frame_annotations() const {
        return annotation_locations;
    }

    void set_boundingboxes(std::vector<QRect>&& bboxes) {
        boundingbox_locations.insert(boundingbox_locations.end(), bboxes.begin(), bboxes.end());
    }

    void set_pixelannotations(std::vector<QPoint>&& pannotations) {
        annotation_locations.insert(annotation_locations.end(), pannotations.begin(), pannotations.end());
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
    std::vector<QPoint> annotation_locations;
    std::vector<QPoint> limbo_points;

    //the bounding box coordinates when the user is drawing in bounding box mode
    std::vector<QRect> boundingbox_locations;
    std::vector<QRect> limbo_bboxes;
    QRect current_bbox;

    QGraphicsTextItem cursor;
    int annotation_brushsz;
    bool drawing_annotations;

    ANNOTATION_MODE mode;
};


class FrameView : public QGraphicsView
{
public:
    FrameView(QGraphicsScene* fview, QWidget* parent = 0)
        : QGraphicsView(fview, parent)
    {
        fviewer = reinterpret_cast<FrameViewer*>(fview);
        this->update();
        this->setMouseTracking(true);
    }

    QSize sizeHint() const override {
        return fviewer->get_size_hint(); 
    }

    void update_frame(const QImage& frame) {
        //re-set any viewing transformations
        resetMatrix();
        fviewer->display_frame(frame);
    }

    FrameAnnotations get_frame_annotations() const {
        auto bboxes = fviewer->get_bounding_boxes();   
        auto segmpts = fviewer->get_frame_annotations();   
        FrameAnnotations metadata (std::move(bboxes), std::move(segmpts));
        return metadata;
    }

    void set_frame_annotations(FrameAnnotations&& frame_metadata) {
        if (frame_metadata.bboxes.size() > 0) {
            fviewer->set_boundingboxes(std::move(frame_metadata.bboxes));
        }

        if (frame_metadata.segm_points.size() > 0) {
            fviewer->set_pixelannotations(std::move(frame_metadata.segm_points));
        }
        this->update();
    }

protected:
    //for zooming into the scene -- hold down control to zoom (versus just scrolling up and down)
    void wheelEvent(QWheelEvent*) override;

private:
    FrameViewer* fviewer; 
};

#endif
