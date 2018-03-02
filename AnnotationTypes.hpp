#ifndef FISHLABELER_ANNOTATIONTYPES_HPP
#define FISHLABELER_ANNOTATIONTYPES_HPP

#include <vector>
#include <QRect>
#include <QPoint>

enum class ANNOTATION_MODE : int{
    SEGMENTATION = 0,
    BOUNDINGBOX
};

struct BoundingBoxMD {
    BoundingBoxMD()
        : instance_id(0)
    {}

    BoundingBoxMD(QRect brect, int id)
        : bbox(std::move(brect)), instance_id(id)
    {}
    QRect bbox;
    int instance_id;
};

struct PixelLabelMB {
    PixelLabelMB()
        : instance_id(0)
    {}

    PixelLabelMB(std::vector<QPoint>&& spts, int id)
        : smask(std::move(spts)), instance_id(id)
    {}
    std::vector<QPoint> smask;
    int instance_id;
};

//something to encapulate all of the user-supplied information for a given frame
struct FrameAnnotations {
    FrameAnnotations(std::vector<BoundingBoxMD>&& fvboxes, std::vector<PixelLabelMB>&& fvpoints)
        : bboxes(std::move(fvboxes)), segm_points(std::move(fvpoints))
    {}

    std::vector<BoundingBoxMD> bboxes;
    std::vector<PixelLabelMB> segm_points;
};



#endif


