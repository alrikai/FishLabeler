#include "VideoLogger.hpp"

#include <fstream>
#include <opencv2/opencv.hpp>


void VideoLogger::create_logdirs(boost::filesystem::path& logdir, const std::string& logdir_name) 
{
    logdir /= logdir_name;
    if (!boost::filesystem::exists(logdir)) {
        if(boost::filesystem::create_directory(logdir)) {
            std::cout << "Created " << logdir_name << " directory at " << logdir.string() << std::endl;
        } else {
            std::string err_msg {"ERROR: couldn't create " + logdir_name + " directory at " + logdir.string()};
            throw std::runtime_error(err_msg);
        }
    }
}

//segmentation masks --> logged as an image 
void VideoLogger::write_annotations(const std::string& framenum, std::vector<QPointF>&& annotations, const int ptsz, const int height, const int width)
{
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


//bounding boxes --> logged in a text file
void VideoLogger::write_bboxes(const std::string& framenum, std::vector<QRectF>&& bbox_rects, const int ptsz, const int height, const int width)
{
    auto output_fpath = bbox_logdir;
    output_fpath /= framenum;
    std::string out_fname = output_fpath.string() + ".txt"; 
    std::ofstream fout(out_fname);
    //top left and bottom right coordinates
    qreal tl_x, tl_y, br_x, br_y;
    for (auto bbox : bbox_rects) {
        bbox.getCoords(&tl_x, &tl_y, &br_x, &br_y);
        fout << tl_x << ", " << tl_y << ", " << br_x << ", " << br_y << "\n";
    }
    fout.close();
}

void VideoLogger::write_textmetadata(const std::string& framenum, std::string&& text_meta)
{
    auto output_fpath = text_logdir;
    output_fpath /= framenum;
    std::string out_fname = output_fpath.string() + ".txt"; 
    std::ofstream fout(out_fname);
    fout << text_meta;
    fout.close();
}
