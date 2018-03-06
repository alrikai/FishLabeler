#ifndef BOUNDING_BOX_VIZ_H
#define BOUNDING_BOX_VIZ_H

#include <iostream>

#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QRect>
#include <QPainter>

#include "AnnotationTypes.hpp"

class BoundingBoxViz : public QGraphicsRectItem
{
public:
    BoundingBoxViz(QRect bbox, int inst_id)
        : bbox(bbox), bbox_id(inst_id)
    {
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        is_selected = false;
		std::cout << "Making bbox @ID " << inst_id << std::endl; 
    }

    BoundingBoxViz(BoundingBoxMD bbox)
        : BoundingBoxViz(bbox.bbox, bbox.instance_id)
    {}
/* 
    QRectF boundingRect() const override
    {
        int tl_x, tl_y, br_x, br_y;
        bbox.getCoords(&tl_x, &tl_y, &br_x, &br_y);
        float penWidth = 1;
        return QRectF(std::max(0.f, tl_x - penWidth / 2.f), std::max(0.f, tl_y - penWidth / 2.f),
                      br_x + penWidth, br_y + penWidth);
    }
*/
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
    {
        painter->drawRect(bbox);
    }


    QRect& get_bounding_box()  
    {
        return bbox;
    }

    BoundingBoxMD get_bbox_metadata() const
    {
        BoundingBoxMD bbox_meta (bbox, bbox_id);
        return bbox_meta;
    }

    int get_id() const {
        return bbox_id;
    }


    void set_id(const int id) {
        bbox_id = id;
    }

protected slots:
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;

private:
    QRect bbox;
    int bbox_id;
    bool is_selected;
};

#endif
