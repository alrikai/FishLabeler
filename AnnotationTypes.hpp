#ifndef FISHLABELER_ANNOTATIONTYPES_HPP
#define FISHLABELER_ANNOTATIONTYPES_HPP

#include <vector>
#include <cmath>

#include <QRect>
#include <QPoint>

enum class ANNOTATION_MODE : int{
    SEGMENTATION = 0,
    BOUNDINGBOX
};

template <typename T>
struct BoxPoint
{
    BoxPoint()
        : x(0), y(0)
    {}

    BoxPoint(T x, T y)
        : x(x), y(y)
    {}

    BoxPoint<T>& operator+=(BoxPoint<T> rhs);
    BoxPoint<T>& operator/=(T div);
    BoxPoint<T>& operator*=(T div);

    T x;
    T y;
};

    
template <typename T>
BoxPoint<T>& BoxPoint<T>::operator+=(BoxPoint<T> rhs)
{
    this->x += rhs.x;
    this->y += rhs.y;
    return *this;
}

template <typename T>
BoxPoint<T> operator+(const BoxPoint<T>& lhs, const BoxPoint<T>& rhs)
{
    BoxPoint<T> op = lhs;
    return op += rhs;
}

template <typename T>
BoxPoint<T>& BoxPoint<T>::operator/=(T div)
{
    this->x /= div;
    this->y /= div;
    return *this;
}

template <typename T>
BoxPoint<T> operator/(const BoxPoint<T>& lhs, const T rhs)
{
    BoxPoint<T> op = lhs;
    return op /= rhs;
}

template <typename T>
BoxPoint<T>& BoxPoint<T>::operator*=(T div)
{
    this->x *= div;
    this->y *= div;
    return *this;
}

template <typename T>
BoxPoint<T> operator*(const BoxPoint<T>& lhs, const T rhs)
{
    BoxPoint<T> op = lhs;
    return op *= rhs;
}

template <typename T>
bool operator==(const BoxPoint<T>& lhs, const BoxPoint<T>& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
bool operator!=(const BoxPoint<T>& lhs, const BoxPoint<T>& rhs)
{
    return !(lhs == rhs);
}

template <typename T>
struct BoundingBox
{
    static constexpr T denom = 2;
    BoundingBox() 
    {}

    BoundingBox(T x1, T y1, T x2, T y2) 
    {
        set_coords(x1, y1, x2, y2);
    }

    void set_coords(T x1, T y1, T x2, T y2)
    {
        center = BoxPoint<T>((x1+x2)/denom, (y1+y2)/denom);
        dims = BoxPoint<T>(std::abs(x2-x1), std::abs(y2-y1));
    }

    void average(const BoundingBox<T>& rhs_bbox);
    BoundingBox<T> predict(const BoundingBox<T>& rhs_bbox, const int num_frames, const int pred_frame);
    BoundingBox<T>& operator+=(BoundingBox<T> rhs_bbox);
    BoundingBox<T>& operator*=(BoundingBox<T> rhs_bbox);
    BoundingBox<T>& operator*=(T factor);

    template <typename U=T>
    QRectF get_rect(typename std::enable_if <std::is_floating_point<U>::value, U>::type* = nullptr) const {
        QPointF topleft {center.x - dims.x/denom, center.y - dims.y/denom};
        QPointF botright {center.x + dims.x/denom, center.y + dims.y/denom};
        QRectF bbox (topleft, botright);
        return bbox;
    }

    template <typename U=T>
    QRect get_rect(typename std::enable_if <std::is_integral<U>::value, U>::type* = nullptr) const {
        QPoint topleft {std::rint(center.x - dims.x/denom), std::rint(center.y - dims.y/denom)};
        QPoint botright {std::rint(center.x + dims.x/denom), std::rint(center.y + dims.y/denom)};
        QRect bbox (topleft, botright);
        return bbox;
    }

    BoxPoint<T> center;
    BoxPoint<T> dims; 
};

template<typename T>
BoundingBox<T>& BoundingBox<T>::operator+=(BoundingBox<T> rhs_bbox)
{
    this->center += rhs_bbox.center;
    this->dims += rhs_bbox.dims;
    return *this;
}

template <typename T>
BoundingBox<T> operator+(const BoundingBox<T>& lhs, const BoundingBox<T>& rhs)
{
    BoundingBox<T> op = lhs;
    return op += rhs;
}

template<typename T>
BoundingBox<T>& BoundingBox<T>::operator*=(BoundingBox<T> rhs_bbox)
{
    this->center *= rhs_bbox.center;
    this->dims *= rhs_bbox.dims;
    return *this;
}

template<typename T>
BoundingBox<T>& BoundingBox<T>::operator*=(T factor)
{
    this->center *= factor;
    this->dims *= factor;
    return *this;
}
template<typename T>
BoundingBox<T>& operator*(const BoundingBox<T>& lhs, BoundingBox<T> rhs)
{
    BoundingBox<T> op = lhs;
    return op *= rhs;
}

template<typename T>
BoundingBox<T>& operator*(const BoundingBox<T>& lhs, T factor)
{
    BoundingBox<T> op = lhs;
    return op *= factor;
}

template <typename T>
bool operator==(const BoundingBox<T>& lhs, const BoundingBox<T>& rhs)
{
    return lhs.center == rhs.center && lhs.dims == rhs.dims;
}

template <typename T>
bool operator!=(const BoundingBox<T>& lhs, const BoundingBox<T>& rhs)
{
    return !(lhs==rhs);
}

template<typename T>
void BoundingBox<T>::average(const BoundingBox<T>& rhs_bbox)
{
    static constexpr T demon = 2;
    *this += rhs_bbox;
    this->center /= demon;
    this->dims /= demon;
}

template<typename T>
BoundingBox<T> BoundingBox<T>::predict(const BoundingBox<T>& rhs, const int num_frames, const int pred_frame)
{
    auto lhs_bbox = (*this) * (static_cast<float>(num_frames - pred_frame) / static_cast<float>(num_frames));
    auto rhs_bbox = rhs * (static_cast<float>(pred_frame) / static_cast<float>(num_frames));
    return lhs_bbox + rhs_bbox;
}




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


