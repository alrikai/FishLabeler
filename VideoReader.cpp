#include "VideoReader.hpp"

#include <stdexcept>
#include <iostream>

#include <boost/algorithm/string.hpp>  

VideoFrame<VideoReader::PixelT> VideoReader::get_prev_frame()
{

}

VideoFrame<VideoReader::PixelT> VideoReader::get_next_frame()
{

}

VideoFrame<VideoReader::PixelT> VideoReader::get_frame(const int houroffset, const int minoffset, const int secoffset)
{

}

VideoFrame<VideoReader::PixelT> VideoReader::get_frame(const int index)
{

}


void VideoReader::parse_video_frames()
{
    if (!boost::filesystem::is_directory(fpath)) {
        std::string err_msg {"ERROR: directory " + fpath + " doesn't exist or isn't a directory"}; 
        throw std::runtime_error(err_msg);
    }

    for (boost::filesystem::directory_iterator fit(fpath); fit != boost::filesystem::directory_iterator(); fit++) {
        //check if it's a file
        if (boost::filesystem::is_regular_file(fit->status())) {
            //... and if the file extension matches our target extension(s)
            auto file_fext = fit->path().extension().string();
            auto fext = boost::algorithm::to_lower_copy(file_fext);
            bool valid_file = std::find(valid_ext.begin(), valid_ext.end(), fext) != valid_ext.end(); 
            if (valid_file) {
                files.emplace_back(fit->path().filename().string());
            }
        }
    }

    std::cout << "Got " << files.size() << " #frames" << std::endl;
    if (files.size() == 0) {
        std::string err_msg {"ERROR: 0 valid frames in directory " + fpath};
        throw std::runtime_error(err_msg);
    }
}

const std::array<std::string, VideoReader::NUM_FEXTS> VideoReader::valid_ext = {{
    ".png", ".jpg", ".jpeg", ".bmp"
}};
