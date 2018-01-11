#include "FrameViewer.hpp"

#include <iostream>

void FrameView::wheelEvent(QWheelEvent *evt)
{
    if (evt->modifiers() & Qt::ControlModifier) {
        const QGraphicsView::ViewportAnchor prev_anchor = transformationAnchor();
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        //TODO: this is zooming proportionately to how much scrolling is done, but it is a 
        //bit unreliable and ends up with large, sudden jumps sometimes... which is annoying.
        //might want to move to having it just scroll by a constant factor (i.e. 1.2 up/down each time) 
        float zfactor = evt->angleDelta().y() / 100.f;
        if (zfactor < 0) {
            zfactor = std::abs(zfactor) - 1.f; 
        }
        std::cout << "zooming " << (zfactor > 1.f ? "IN ":"OUT ") << "by " << zfactor << std::endl;
        scale(zfactor, zfactor);
        setTransformationAnchor(prev_anchor);
    } else {
        QGraphicsView::wheelEvent(evt);
    }
}
