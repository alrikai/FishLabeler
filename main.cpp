#include <iostream>
#include <opencv2/opencv.hpp>

#include "VideoReader.hpp"

int main() {
    const std::string vpath {"/home/alrik/Data/NRTFish/20130117144639.mts"};
    VideoReader vreader {vpath};

    std::cout << "video has " << vreader.get_num_frames() << " #frames" << std::endl;
    for (int i = 0; i < 10; i++) {
        auto vframe = vreader.get_next_frame();
        std::cout << "vf " << i << ": [" << vframe.height << " x " << vframe.width << "]" << std::endl;
        cv::Mat cv_frame (vframe.height, vframe.width, CV_8UC3, vframe.data.get());
        std::string fname {"vframe_" + std::to_string(i) + ".png"};
        cv::imwrite(fname, cv_frame);
    }

    vreader.get_cache_stats();
}
