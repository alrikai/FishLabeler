#include "VideoLogger.hpp"

#include <fstream>
#include <opencv2/opencv.hpp>

void VideoLogger::write_annotations(const std::string& framenum, std::vector<QPointF>&& annotations, const int ptsz, const int height, const int width)
{
    //TODO: need to make sure the input 'framenum' string has no extension already
    auto output_fpath = annotation_logdir;
    output_fpath /= framenum;
    std::string out_fname = output_fpath.string() + ".png"; 

    cv::Mat log_annotation = cv::Mat::zeros(height, width, CV_8UC1);
    for (auto mpt : annotations) {
        auto col = mpt.x();
        auto row = mpt.y();
        //TODO: how does the point size work here? i.e if it's 8... what does that mean? ... This is the pen width,
        //so if I just click a point, does that mean 8 pixels on either side (but not top/bottom), or is it +8 on all sides, or 
        //+4 on either side, etc. (I culd just zoom in really close and check I guess...)
        // ....
        // TODO: for now, just write out the single pixel (ignore brush size)
        log_annotation.at<uint8_t>(int(row), int(col)) = 255; 
    }
    cv::imwrite(out_fname, log_annotation);
}

void VideoLogger::write_textmetadata(const std::string& framenum, std::string&& text_meta)
{
    //TODO: need to make sure the input 'framenum' string has no extension already
    auto output_fpath = text_logdir;
    output_fpath /= framenum;
    std::string out_fname = output_fpath.string() + ".txt"; 
    std::ofstream fout(out_fname);
    fout << text_meta;
    fout.close();
}
