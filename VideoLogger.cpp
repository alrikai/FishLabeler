#include "VideoLogger.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>

#include <opencv2/opencv.hpp>
#include <boost/algorithm/string.hpp>  
#include <boost/lexical_cast.hpp>

namespace utils {
    template <typename T>
    T clip(const T n, const T low, const T high) {
        return std::max(low, std::min(n, high));
    }
}

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
void VideoLogger::write_annotations(const std::string& framenum, std::vector<PixelLabelMB>&& annotations, const int height, const int width)
{
    auto fpath = make_filepath(annotation_logdir, framenum, ".png");
    const std::string out_fname = fpath.string(); 

    //write out the annotation (if it exists), and if there now is no annotation, but one exists on disk, delete it
    if (annotations.size() > 0) {
        cv::Mat log_annotation = cv::Mat::zeros(height, width, CV_8UC1);
        for (auto mmask : annotations) {
            //each of these will be a different instance
            for (auto mpt : mmask.smask) {
                auto col = mpt.x();
                auto row = mpt.y();
                //NOTE: since we 0-index in the instances, we need to +1 (since 0 is reserved for background)
                log_annotation.at<uint8_t>(int(row), int(col)) = mmask.instance_id + 1; 
            }
        }
        std::cout << "logging mask frame to " << out_fname << std::endl;
        cv::imwrite(out_fname, log_annotation);
    } else {
        if (boost::filesystem::exists(out_fname)) {
            //NOTE: we want to just delete the file, since that frame's annotations have been removed
            //TODO: delete the file
            boost::filesystem::path rmpath {out_fname};
            boost::filesystem::remove(rmpath);
            std::cout << "NOTE: removing mask file " << out_fname << std::endl;
        }
    }
}


//bounding boxes --> logged in a text file
void VideoLogger::write_bboxes(const std::string& framenum, std::vector<BoundingBoxMD>&& bbox_rects, const int height, const int width)
{
    auto fpath = make_filepath(bbox_logdir, framenum, ".txt");
    const std::string out_fname = fpath.string(); 

    //NOTE: if we want to undo previously written boxes, we need to do this check 
    bool has_annotation = bbox_rects.size() > 0;
    if (has_annotation) {
        std::ofstream fout(out_fname);
        //top left and bottom right coordinates
        int tl_x, tl_y, br_x, br_y;
        for (auto bbox_md : bbox_rects) {
            auto id = bbox_md.instance_id;
            bbox_md.bbox.getCoords(&tl_x, &tl_y, &br_x, &br_y);

            //have the bbox coordinates arranged as [low col, low row, high col, high row]
            if (tl_x > br_x) {
                std::swap(tl_x, br_x);
            }
            if (tl_y > br_y) {
                std::swap(tl_y, br_y);
            }

            //clip the bounding boxes to valid ranges too
            tl_x = utils::clip(tl_x, 0, width);
            br_x = utils::clip(br_x, 0, width);
            tl_y = utils::clip(tl_y, 0, height);
            br_y = utils::clip(br_y, 0, height);

            fout << id << ", " << tl_x << ", " << tl_y << ", " << br_x << ", " << br_y << "\n";
        }
        fout.close();
    } else {
        if (boost::filesystem::exists(out_fname)) {
            //NOTE: we want to just delete the file, since that frame's annotations have been removed
            //TODO: delete the file
            std::cout << "NOTE: removing detection file " << out_fname << std::endl;
            boost::filesystem::path rmpath {out_fname};
            boost::filesystem::remove(rmpath);
        }
    }
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
        auto mask_img = cv::imread(frame_fpath, CV_LOAD_IMAGE_GRAYSCALE);

        std::map<int, std::vector<QPoint>> instance_segs;
        for (int row = 0; row < mask_img.rows; row++) {
            for (int col = 0; col < mask_img.cols; col++) {
                auto pxlabel = mask_img.at<uint8_t>(row,col);
                if (pxlabel > 0) {
                    //NOTE: since we 0-index in the instances, we need to -1 
                    //(doing it above could overflow on bg labels, unless we also cast the label type, which is annoying...)
                    pxlabel -= 1;
                    auto pxkey_it = instance_segs.find(pxlabel); 
                    if (pxkey_it != instance_segs.end()) {
                        pxkey_it->second.emplace_back(col, row);
                    } else {
                        std::vector<QPoint> new_instancepts {QPoint(col, row)};
                        instance_segs.emplace(std::make_pair(pxlabel, std::move(new_instancepts)));
                    }
                }
            }
        }

        for (auto instance_it : instance_segs) {
            std::cout << "Loading seg instance " << instance_it.first << " --> " << instance_it.second.size() << " #pts" << std::endl; 
            frame_annotations.emplace_back(std::move(instance_it.second), instance_it.first);
        } 
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
            boost::trim_right(bbox_str);
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
