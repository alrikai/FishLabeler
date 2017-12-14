#ifndef FISHLABELER_VIDEOFRAME_HPP
#define FISHLABELER_VIDEOFRAME_HPP

#include <memory>

//really basic image wrapper. Will be converted to a QtImage (or something) for display
template <typename T>
struct VideoFrame
{
    using PixelT = T;

    VideoFrame(T* data, const int height, const int width, const int stride) 
        : data(data), height(height), width(width), stride(stride)
    {}
 

    //assume the VideFrame object takes ownership of the data
    VideoFrame(T* data, const int height, const int width) 
        : VideoFrame(data, height, width, width)
    {}

    VideoFrame()
        : VideoFrame(nullptr, 0, 0)
    {}


    std::shared_ptr<T> data;
    int height;
    int width;
    int stride;
};

#endif


