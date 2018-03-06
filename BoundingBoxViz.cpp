#include "BoundingBoxViz.hpp"

#include <iostream>

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
