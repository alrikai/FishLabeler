#include "VideoReader.hpp"

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/algorithm/string.hpp>  
#include <boost/lexical_cast.hpp>
#include <boost/sort/spreadsort/string_sort.hpp>

QImage VideoReader::get_prev_frame()
{
    const int target_frame = frame_index - 1;
	auto qframe = get_frame(target_frame);
	frame_index = target_frame;
	return qframe;
}

QImage VideoReader::get_next_frame()
{
    const int target_frame = frame_index + 1;
	auto qframe = get_frame(target_frame);
	frame_index = target_frame;
	return qframe;
}

QImage VideoReader::get_frame(const int houroffset, const int minoffset, const int secoffset)
{
    //convert the timestamp to a frame index
	const float time_offset = 60*60*houroffset + 60*minoffset + secoffset;
	const int offset_index = static_cast<int>(std::round(time_offset * video_fps));
    frame_index = offset_index;
	return get_frame(frame_index);
}

QImage VideoReader::get_frame(const int index)
{
	if (index < 0 || index >= files.size()) {
		std::string err_msg {"ERROR: " + std::to_string(index) + " out of bounds"};
        throw std::runtime_error(err_msg);
	}

	std::cout << "index " << index << " --> " << files[index] << std::endl;
	QImage qframe(files[index].c_str());
	//TODO: will this get deallocated? what does the copy ctor do?
	return qframe;
}


void VideoReader::parse_video_frames()
{
    if (!boost::filesystem::is_directory(fpath)) {
        std::string err_msg {"ERROR: directory " + fpath + " doesn't exist or isn't a directory"}; 
        throw std::runtime_error(err_msg);
    }

	//get the video info.txt file
	boost::filesystem::path info_metadata_fpath {fpath};
    info_metadata_fpath /= "info.txt";
    if (!boost::filesystem::exists(info_metadata_fpath)) {
        std::string err_msg {"ERROR: video info.txt " + info_metadata_fpath.string() + " doesn't exist"}; 
        throw std::runtime_error(err_msg);
	}

    std::ifstream info_ifstream(info_metadata_fpath.string());
    std::stringstream metadata_buffer;
    metadata_buffer << info_ifstream.rdbuf();
	std::string video_metadata = metadata_buffer.str();

	std::vector<std::string> metadata_tokens;
    boost::split(metadata_tokens, video_metadata, boost::is_any_of(","));

	//for now, I think we just need the FPS
	for (auto& mtoken : metadata_tokens) {
        if (boost::algorithm::contains(mtoken, "fps")) {
			std::cout << "deriving fps from string " << mtoken << std::endl;
			std::vector<std::string> mdata_fps;
            boost::split(mdata_fps, mtoken, boost::is_any_of(" "));
			video_fps = boost::lexical_cast<double>(mdata_fps[1]);
		}
	}

    for (boost::filesystem::directory_iterator fit(fpath); fit != boost::filesystem::directory_iterator(); fit++) {
        //check if it's a file
        if (boost::filesystem::is_regular_file(fit->status())) {
            //... and if the file extension matches our target extension(s)
            auto file_fext = fit->path().extension().string();
            auto fext = boost::algorithm::to_lower_copy(file_fext);
            bool valid_file = std::find(valid_ext.begin(), valid_ext.end(), fext) != valid_ext.end(); 
            if (valid_file) {
				auto fpath_str = fit->path().string();
                files.emplace_back(fpath_str);
            }
        }
    }
	boost::sort::spreadsort::string_sort(files.begin(), files.end());

    std::cout << "Got " << files.size() << " #frames" << std::endl;
    if (files.size() == 0) {
        std::string err_msg {"ERROR: 0 valid frames in directory " + fpath};
        throw std::runtime_error(err_msg);
    }
}

const std::array<std::string, VideoReader::NUM_FEXTS> VideoReader::valid_ext = {{
    ".png", ".jpg", ".jpeg", ".bmp"
}};
