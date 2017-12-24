#ifndef FISHLABELER_VIDEOREADER_HPP
#define FISHLABELER_VIDEOREADER_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include <boost/filesystem.hpp>

#include "VideoFrame.hpp"

class VideoReader
{
	static constexpr int NUM_FEXTS = 4;
	static const std::array<std::string, NUM_FEXTS> valid_ext;

public:	
	using PixelT = uint8_t;

	//TODO: need some way of getting the accurate frame rate, so we can convert from timestamps to frame indices
    explicit VideoReader(const std::string& filepath) 
		: fpath(filepath), frame_index(0)
	{
	    parse_video_frames();
	}

    VideoFrame<PixelT> get_prev_frame();
    VideoFrame<PixelT> get_next_frame();
    VideoFrame<PixelT> get_frame(const int houroffset, const int minoffset, const int secoffset);
    VideoFrame<PixelT> get_frame(const int index);
    int get_num_frames() const {
        return files.size();
	}

	//TODO: need to parse the video info.txt file to get the FPS / other metadata
	float get_video_fps() const {
        return 0;
	}

	//vestige of old class, might be informative if I add caching to this one
	float get_cache_stats() const {
        return 0;
	} 

private:
	void parse_video_frames();

	const std::string fpath;
	int frame_index;
    std::vector<std::string> files;
};

#endif
