#include "VideoLogger.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>

#include <opencv2/opencv.hpp>
#include <boost/algorithm/string.hpp>  
#include <boost/lexical_cast.hpp>

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
void VideoLogger::write_annotations(const std::string& framenum, std::vector<PixelLabelMB>&& annotations, const int ptsz, const int height, const int width)
{
    auto fpath = make_filepath(annotation_logdir, framenum, ".png");
    const std::string out_fname = fpath.string(); 
    cv::Mat log_annotation = cv::Mat::zeros(height, width, CV_8UC1);
    for (auto mmask : annotations) {
        //each of these will be a different instance
        for (auto mpt : mmask.smask) {
            auto col = mpt.x();
            auto row = mpt.y();
            //TODO: the brushsz poses a problem here -- if we want to be able to re-load the points unambiguously, we need to 
            //store the points as well as the brushsize (as otherwise if there are 2 points which overlap due to brushsize, it 
            //will be impossible to recover all the original points unambiguously). But if we just store the datapoints independent
            //of brushsz, then we cannot easily store the brushsize.
            //The other alternative would be to use textfiles for the annotation points (e.g. brushsz, then  point per line), and have
            //a script offline that goes through and reads the points and generates masks from them. Or, we could just have it output both,
            //or we could have it just be afunction that runs on the GUI close event. 
            //But basically, we could get perfect reconstruction if we use a text file, but not if we use images. 
            log_annotation.at<uint8_t>(int(row), int(col)) = mmask.instance_id; 
        }
    }
    cv::imwrite(out_fname, log_annotation);
}


//bounding boxes --> logged in a text file
void VideoLogger::write_bboxes(const std::string& framenum, std::vector<BoundingBoxMD>&& bbox_rects, const int ptsz, const int height, const int width)
{
    auto fpath = make_filepath(bbox_logdir, framenum, ".txt");
    const std::string out_fname = fpath.string(); 

    std::ofstream fout(out_fname);
    //top left and bottom right coordinates
    int tl_x, tl_y, br_x, br_y;
    for (auto bbox_md : bbox_rects) {
        auto id = bbox_md.instance_id;
        bbox_md.bbox.getCoords(&tl_x, &tl_y, &br_x, &br_y);
        fout << id << ", " << tl_x << ", " << tl_y << ", " << br_x << ", " << br_y << "\n";
    }
    fout.close();
}

void VideoLogger::write_textmetadata(const std::string& framenum, std::string&& text_meta)
{
    auto fpath = make_filepath(text_logdir, framenum, ".txt");
    const std::string out_fname = fpath.string(); 

    std::ofstream fout(out_fname);
    fout << text_meta;
    fout.close();
}

std::vector<PixelLabelMB> VideoLogger::get_annotations (const std::string& framenum) const
{
    //TODO: need to implement this (which requires deciding how best to log the pixel-wise annotations)
    std::vector<PixelLabelMB> frame_annotations;
    auto fpath = make_filepath(annotation_logdir, framenum, ".png");
    if (boost::filesystem::exists(fpath)) {
        const std::string frame_fpath = fpath.string();


    }
    return frame_annotations;
}

std::vector<BoundingBoxMD> VideoLogger::get_boundingboxes (const std::string& framenum) const 
{
    std::vector<BoundingBoxMD> frame_bboxes;
    auto fpath = make_filepath(bbox_logdir, framenum, ".txt");
    if (boost::filesystem::exists(fpath)) {
        std::string bbox_str;
        std::ifstream bbox_ifstream(fpath.string());
        while(std::getline(bbox_ifstream, bbox_str)) {
            bbox_str.erase (std::remove (bbox_str.begin(), bbox_str.end(), ' '), bbox_str.end());
            std::vector<std::string> bbox_tokens;
            boost::split(bbox_tokens, bbox_str, boost::is_any_of(","));
            assert(bbox_tokens.size() % 5 == 0);
            auto id = boost::lexical_cast<int>(bbox_tokens[0]);
            auto tl_x = boost::lexical_cast<int>(bbox_tokens[1]);
            auto tl_y = boost::lexical_cast<int>(bbox_tokens[2]);
            auto br_x = boost::lexical_cast<int>(bbox_tokens[3]);
            auto br_y = boost::lexical_cast<int>(bbox_tokens[4]);
            QRect bbox_rect (QPoint(tl_x, tl_y), QPoint(br_x, br_y));
            frame_bboxes.emplace_back(bbox_rect, id);
        }         
    }
    return frame_bboxes;
}

std::string VideoLogger::get_textmetadata (const std::string& framenum) const
{
    std::string metadata;
    auto fpath = make_filepath(text_logdir, framenum, ".txt");
    if (boost::filesystem::exists(fpath)) {
        std::ifstream mdata_ifstream(fpath.string());
        std::stringstream metadata_buffer;
        metadata_buffer << mdata_ifstream.rdbuf();
        metadata = metadata_buffer.str();
    }
    return metadata;
}
