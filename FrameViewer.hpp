#ifndef FISHLABELER_FRAMEVIEWER_HPP
#define FISHLABELER_FRAMEVIEWER_HPP

#include <QWidget>
#include <QObject>
#include <QGraphicsView>
#include <QWheelEvent>

#include "FrameScene.hpp"
#include "AnnotationTypes.hpp"

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
        fviewer->set_metadata(std::move(frame_metadata));
        this->update();
    }

protected:
    //for zooming into the scene -- hold down control to zoom (versus just scrolling up and down)
    void wheelEvent(QWheelEvent*) override;

private:
    FrameViewer* fviewer; 
};

#endif
