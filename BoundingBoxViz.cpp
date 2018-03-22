#include "BoundingBoxViz.hpp"

#include <iostream>
#include <cmath>

void BoundingBoxViz::mouseMoveEvent(QGraphicsSceneMouseEvent* mevt) 
{
    if (is_selected) {
        std::cout << "dragged bbox " << std::endl;
    }
}

void BoundingBoxViz::mousePressEvent(QGraphicsSceneMouseEvent* mevt) 
{
    is_selected = true;
    std::cout << "selsected bbox ID " << bbox_id << std::endl;
}
void BoundingBoxViz::mouseReleaseEvent(QGraphicsSceneMouseEvent* mevt) 
{
    is_selected = false;
    std::cout << "UN selsected bbox ID " << bbox_id << std::endl;
}


bool BoundingBoxViz::intersects_bbox_frame(const QPointF click, const int brushsz) const
{
    //NOTE: we need the bounding box oordinates and the brush size used to paint it
    //Need to implement this to have the ability to drag bounding boxes areound. 
    int col, row, width, height;
    bbox.getRect(&col, &row, &width, &height);
    const float brush_hsz = static_cast<float>(brushsz) / 2.f;
    QPoint click_pos {std::rint(click.x()), std::rint(click.y())};

    QRect top_rect {col - brush_hsz, row - brush_hsz, width + brushsz, brushsz};
    QRect lhs_rect {col - brush_hsz, row - brush_hsz, brushsz, height + brushsz};
    QRect rhs_rect {col + width - brush_hsz, row - brush_hsz, brushsz, height + brushsz};
    QRect bot_rect {col - brush_hsz, row + height - brush_hsz, width + brushsz, brushsz};

    bool has_pt_v1 = top_rect.contains(click_pos) || lhs_rect.contains(click_pos) || rhs_rect.contains(click_pos) || bot_rect.contains(click_pos);
    return has_pt_v1;

    /*
    //NOTE: proper contains --> just the interior of bbox, not the frame.
    //improper contains --> interior and outer frame of the bounding box
    //so, if one is true and the other isn't, then the click is along the bbox frame
    bool proper_contains = bbox.contains(click_pos, true);
    bool improper_contains = bbox.contains(click_pos, false);
    bool has_pt_v2 =  proper_contains != improper_contains;

    if (has_pt_v1 != has_pt_v2) {
        //NOTE: this is an interesting case, as it means one of the approaches is wrong
        std::cout << "consider putting a breakpt here" << std::endl;
    } 
    return has_pt_v2;
    */
}
