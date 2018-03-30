#ifndef FISHLABELER_DETECTION_INTERPOLATE_HPP
#define FISHLABELER_DETECTION_INTERPOLATE_HPP

#include <vector>
#include <cmath>

#include <QRectF>
#include <QRect>
#include <QPointF>

#include "AnnotationTypes.hpp"

template <typename PtT>
struct LinearInterpolation
{
    static std::vector<PtT> interpolate(PtT lhs, PtT rhs, const int num_frames)
    {
        std::vector<PtT> bbox_interpolation;
        //bbox_interpolation.reserve(num_frames);
        for (int i = 0; i < num_frames; i++) {
            //bbox_interpolation[i] = lhs.predict(rhs, num_frames, i+1); 
            bbox_interpolation.emplace_back(lhs.predict(rhs, num_frames, i+1)); 
        }

        return bbox_interpolation;
    }
};

//TODO: add other interpolation types

//TODO: would be interesting to try to integrate:
//- motion model information
//- tracker(s) prediction
//- ???
template <typename PtT, typename InterpType>
std::vector<PtT> interpolate_sequence(PtT lhs_pt, PtT rhs_pt, const int num_frames)
{
    return InterpType::interpolate(lhs_pt, rhs_pt, num_frames); 
}

#endif


