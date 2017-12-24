#ifndef FISHLABELER_VIDEOREADER_HPP
#define FISHLABELER_VIDEOREADER_HPP

#include <array>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "VideoFrame.hpp"

class VideoReader
{
public:
    using PixelT = uint8_t;
    static constexpr int VFRAME_CACHE_SIZE = 16;
    static constexpr int NUM_CACHES = 2;
    static_assert(NUM_CACHES == 2, "ERROR: Code implicitly assumes 2 caches"); 

    explicit VideoReader(const std::string& video_path)
        : vpath(video_path), current_frame_idx(0)
    {
        //-1 indicates it's not mapped to anything

        for (int j = 0; j < NUM_CACHES; j++) {
            for (int i = 0; i < VFRAME_CACHE_SIZE; i++) {
                frame_cache_idxmap[j][i] = -1;
            }
        }
        fwd_cache_idx = 0;

        intialize_ffmpeg();
        //populate the frame cache
        read_video_frames(0, true);

        num_req = 0;
        num_misses = 0;
    }

    VideoFrame<PixelT> get_frame(const int index) {
        num_req++;
        //TODO: should I be updating the current_frame_index for this type of access?
        for (int cidx = 0; cidx < NUM_CACHES; cidx++) { 
            for (int frame_cache_idx = 0; frame_cache_idx < VFRAME_CACHE_SIZE; frame_cache_idx++) {
                if (index == frame_cache_idxmap[cidx][frame_cache_idx]) {
                    return frame_cache[cidx][frame_cache_idx];
                }
            }
        }

        //TODO: just out of interest -- keep track of the cache miss rates
        num_misses++;

        bool read_fwd = index > current_frame_idx;
        //read the target frame, and frames around it. Return the target frame, read the surrounding frames into the frame cache
        return read_video_frames(index, read_fwd);
    }

    VideoFrame<PixelT> get_next_frame() {
        num_req++;
        //TODO: this assumes that the cache frames are in monotonically increasing order, 
        //doesn't work for seeking / jumping around
        current_frame_idx += 1;
        for (int cidx = 0; cidx < NUM_CACHES; cidx++) { 
            //NOTE: we want the forward cache, then the backwards cache. Assumes 2 caches only
            int target_cacheidx = cidx == 0 ? fwd_cache_idx : 1 - fwd_cache_idx;
            for (int i = 0; i < VFRAME_CACHE_SIZE; i++) {
                if (current_frame_idx == frame_cache_idxmap[target_cacheidx][i]) {
                    return frame_cache[target_cacheidx][i];
                }
            }
        }
        //TODO: just out of interest -- keep track of the cache miss rates
        num_misses++;
        return read_video_frames(current_frame_idx, true);
    }

    VideoFrame<PixelT> get_prev_frame() {
        num_req++;
        //TODO: this assumes that the cache frames are in monotonically increasing order, 
        //doesn't work for seeking / jumping around
        current_frame_idx -= 1;
        for (int cidx = 0; cidx < NUM_CACHES; cidx++) { 
            //NOTE: we want the backwards cache, then the forward cache. Assumes 2 caches only
            int target_cacheidx = cidx == 0 ? 1 - fwd_cache_idx : fwd_cache_idx;
            for (int i = VFRAME_CACHE_SIZE-1; i >= 0; i--) {
                if (current_frame_idx == frame_cache_idxmap[target_cacheidx][i]) {
                    return frame_cache[target_cacheidx][i];
                }
            }
        }
        //TODO: just out of interest -- keep track of the cache miss rates
        num_misses++;

        return read_video_frames(current_frame_idx, false);
    }

    int get_num_frames() const {
        return av_params.video_frame_count;
    }

    float get_video_fps() const {
        return av_params.video_stream->r_frame_rate.num / (double)av_params.video_stream->r_frame_rate.den;
    }

    float get_cache_stats() const {
        float cache_miss_rate = static_cast<float>(num_misses) / num_req;
        std::cout << "VideoReader: " << cache_miss_rate << "\% miss rate" << std::endl;
        return cache_miss_rate;
    }

private:

    void intialize_ffmpeg();
    template <typename T>
    T* decode_frame();
    bool decode_packet(int decode_call_index = -1);

    int parse_video(const int base_frame_index, const int num_read_frames, const int cache_index);
    VideoFrame<PixelT> read_video_frames(const int frame_index, const bool fwd);

    //parameters for the video demuxing and decoding
    struct AVParams
    {
        AVParams()
        {
            fmt_ctx = nullptr;
            video_dec_ctx = nullptr;
            video_stream = nullptr;
            video_codec = nullptr;
            img_transform = nullptr;

            video_stream_idx = -1;
            video_frame_count = 0;
            video_current_pts = 0; 
            video_current_pts_time = 0;
            last_seek_pos = 0;
            need_to_seek = false;

            frame = nullptr;
            RGB_frame = nullptr;
        }

        //will have to double check what exactly needs to be freed here
        ~AVParams()
        {
            if(video_dec_ctx) {
                avcodec_close(video_dec_ctx);
            }

            avformat_close_input(&fmt_ctx);              
            av_free(frame);                  
            av_free(RGB_frame);

            if(img_transform) {
                sws_freeContext(img_transform);
            }
        }

        AVFormatContext* fmt_ctx;
        AVCodecContext* video_dec_ctx;
        AVStream* video_stream;
        AVCodec* video_codec;

        //use this to re-size and/or convert between pixel formats
        SwsContext *img_transform;

        int video_stream_idx;
        AVFrame *frame;
        AVFrame *RGB_frame;        
        AVPacket pkt;

        int video_frame_count;
        AVRational video_time_base;

        double video_current_pts; 
        int64_t video_current_pts_time;
        int64_t last_seek_pos;
        bool need_to_seek;
    };

    const std::string vpath;
    int current_frame_idx;
    AVParams av_params;

    //NOTE: can't just read the entire video, as it will take too much memory
    //std::vector<VideoFrame<PixelT>> frames;
    //Have 2 caches, one for forward and 1 for backward. Can just swap them as you are iterating forward or backward
    int fwd_cache_idx;
    std::array<std::array<VideoFrame<PixelT>, VFRAME_CACHE_SIZE>, NUM_CACHES> frame_cache;
    std::array<std::array<int, VFRAME_CACHE_SIZE>, NUM_CACHES> frame_cache_idxmap;

    mutable uint64_t num_req;
    mutable uint64_t num_misses;
};


template <typename T>
T* VideoReader::decode_frame() {
    static constexpr bool DEBUG = true;
    if(av_params.pkt.stream_index == av_params.video_stream_idx) {
        //decode the video frame stored in the packet
        bool got_frame = decode_packet();
        if(got_frame) {
            if(DEBUG) {
                //print frame debug info
                printf("video_frame n:%d coded_n:%d display_n: %d\n",
                        av_params.video_frame_count++, av_params.frame->coded_picture_number, av_params.frame->display_picture_number);
            }

            if (av_params.need_to_seek) {
                while (av_params.frame->pts > av_params.last_seek_pos || !got_frame) {
                    std::cout << "REDOING decode_packet: " << av_params.frame->pts << " against " << av_params.last_seek_pos << std::endl;
                    got_frame = decode_packet();
                }
                av_params.need_to_seek = false;
            }

            //convert input to RGB24
            sws_scale(av_params.img_transform, av_params.frame->data, av_params.frame->linesize, 0, av_params.video_dec_ctx->height, av_params.RGB_frame->data, av_params.RGB_frame->linesize); 

            //make a copy of the frame data to be buffered (I think ffmpeg reclaims its AVFrame buffers) 
            T* data_frame;
            data_frame = reinterpret_cast<T*>(malloc(av_params.RGB_frame->linesize[0] * av_params.video_dec_ctx->height));

            //just to make sure -- NOTE: linesize is in bytes
            assert((av_params.RGB_frame->linesize[0]/sizeof(uint8_t))*av_params.video_dec_ctx->height==av_params.video_dec_ctx->height*av_params.video_dec_ctx->width*3);

            T* src_data = reinterpret_cast<T*>(av_params.RGB_frame->data[0]);
            const int n_pixel_elems = av_params.video_dec_ctx->height * av_params.video_dec_ctx->width * 3;
            std::copy(src_data, src_data + n_pixel_elems, data_frame);
            //av_frame_free(av_params.RGB_frame);
    
            //add the decoded frame to the frames buffer
            return data_frame; 
        } else {
            std::cout << "ERROR decoding frame" << std::endl;
            return nullptr;
        }
    }
    return nullptr;
}




#endif
