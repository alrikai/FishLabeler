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
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "VideoFrame.hpp"

class VideoReader
{
public:
    using PixelT = uint8_t;
    static constexpr int VFRAME_CACHE_SIZE = 10;

    VideoReader(const std::string& video_path, const int FPS = 30)
        : vpath(video_path), FPS(FPS), current_frame_idx(0)
    {
        /*
        //-1 indicates it's not mapped to anything
        for (int i = 0; i < VFRAME_CACHE_SIZE; i++) {
            frame_cache_idxmap[i] = -1;
        }
        */

        intialize_ffmpeg();
    }

    VideoFrame<PixelT> get_frame(const int index) const {
        return frames[index];
    }

    VideoFrame<PixelT> get_next_frame() {
        auto frame = frames[current_frame_idx];
        current_frame_idx++;
        return frame;
    }

    int get_num_frames() const {
        return av_params.video_frame_count;
    }

    void parse_video();
private:

    void intialize_ffmpeg();
    template <typename T>
    T* decode_packet(bool cached);


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
            frame = nullptr;
	        BGR_frame = nullptr;
        }

        //will have to double check what exactly needs to be freed here
        ~AVParams()
        {
            if(video_dec_ctx) {
                avcodec_close(video_dec_ctx);
            }

            avformat_close_input(&fmt_ctx);              
            av_free(frame);                  
	        av_free(BGR_frame);

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
        AVFrame *BGR_frame;        
        AVPacket pkt;

        int video_frame_count;
    };


    const std::string vpath;
    const int FPS;
    int current_frame_idx;
    AVParams av_params;

    std::vector<VideoFrame<PixelT>> frames;

    //std::array<VideoFrame<PixelT>, VFRAME_CACHE_SIZE> frame_cache;
    //std::array<int, VFRAME_CACHE_SIZE> frame_cache_idxmap;
};


template <typename T>
T* VideoReader::decode_packet(bool cached) {
    static constexpr bool DEBUG = false;
    int ret = 0;
    if(av_params.pkt.stream_index == av_params.video_stream_idx) {
        int got_frame = 0;
        //decode the video frame stored in the packet
        ret = avcodec_decode_video2(av_params.video_dec_ctx, av_params.frame, &got_frame, &av_params.pkt);
        if(ret < 0) {
            std::cout << "ERROR decoding frame" << std::endl;
            return nullptr;
        }

        if(got_frame) {
            if(DEBUG) {
                //print frame debug info
                printf("video_frame%s n:%d coded_n:%d \n",
                        cached ? "(cached)" : "",
                        av_params.video_frame_count++, av_params.frame->coded_picture_number);
            }

            //convert input to BGR24
            sws_scale(av_params.img_transform, av_params.frame->data, av_params.frame->linesize, 0, av_params.video_dec_ctx->height, av_params.BGR_frame->data, av_params.BGR_frame->linesize); 

            //make a copy of the frame data to be buffered (I think ffmpeg reclaims its AVFrame buffers) 
            T* data_frame;
            data_frame = reinterpret_cast<T*>(malloc(av_params.BGR_frame->linesize[0] * av_params.video_dec_ctx->height));

            //just to make sure -- NOTE: linesize is in bytes
            assert((av_params.BGR_frame->linesize[0]/sizeof(uint8_t))* av_params.video_dec_ctx->height ==  av_params.video_dec_ctx->height * av_params.video_dec_ctx->width * 3);

            T* src_data = reinterpret_cast<T*>(av_params.BGR_frame->data[0]);
            const int n_pixel_elems = av_params.video_dec_ctx->height * av_params.video_dec_ctx->width * 3;
            std::copy(src_data, src_data + n_pixel_elems, data_frame);
    
            //add the decoded frame to the frames buffer
            return data_frame; 
        }
    }
    return nullptr;
}

#endif
