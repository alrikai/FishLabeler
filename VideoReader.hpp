#ifndef FISHLABELER_VIDEOREADER_HPP
#define FISHLABELER_VIDEOREADER_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <iostream>

#include <boost/filesystem.hpp>
#include <QImage> 

class VideoReader
{
    static constexpr int NUM_FEXTS = 4;
    static const std::array<std::string, NUM_FEXTS> valid_ext;

public: 
    using PixelT = uint8_t;

    //TODO: need some way of getting the accurate frame rate, so we can convert from timestamps to frame indices
    explicit VideoReader(const std::string& filepath) 
        : fpath(filepath), frame_index(0), video_fps(0.0)
    {
        parse_video_frames();
    }

    QImage get_prev_frame();
    QImage get_next_frame();
    QImage get_frame(const int houroffset, const int minoffset, const int secoffset);
    QImage get_frame(const int index);

    int get_num_frames() const {
        return files.size();
    }

    std::string get_frame_name(const int frame_index) {
        if (frame_index < 0 || frame_index >= files.size()) {
            std::string err_msg {"ERROR: index " + std::to_string(frame_index) + " is out of bounds"};     
            throw std::runtime_error(err_msg);
        }
        auto fname = files[frame_index];
        boost::filesystem::path p (fname);
        return p.stem().string();
    }

    //vestige of old class, might be informative if I add caching to this one
    float get_cache_stats() const {
        return 0;
    } 

    int get_current_frame_index() const {
        return frame_index;
    }

    std::tuple<int, int, int> get_current_timestamp() const {
        double foffset = frame_index / video_fps;
        int hour_offset = static_cast<int>(std::floor(foffset / (60*60)));
        foffset -= hour_offset * 60*60;
        int min_offset = static_cast<int>(std::floor(foffset / 60));
        foffset -= min_offset*60;
        int sec_offset = static_cast<int>(std::floor(foffset));
        std::cout << "Frame Offset: " << frame_index << " --> H: " << hour_offset << " M: " << min_offset << " S: " << sec_offset << std::endl; 
        return std::make_tuple(hour_offset, min_offset, sec_offset);
    }

private:
    void parse_video_frames();

    const std::string fpath;
    int frame_index;
    std::vector<std::string> files;
    double video_fps;
};

#endif
