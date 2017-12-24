#include "VideoReader.hpp"

#include <stdexcept>
#include <iostream>

extern "C" {
#include <libavutil/time.h>
}

namespace utils {
    int open_codec_context(AVFormatContext*& fmt_ctx, AVCodecContext*& dec_ctx, const AVMediaType type)
    {
        int stream_idx = -1;
        const std::string media_typestr {av_get_media_type_string(type)};
        stream_idx = av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
        if (stream_idx < 0) {
            std::string err_msg{"ERROR: Could not find " +  media_typestr + " stream in input file"};
            throw std::runtime_error(err_msg);
        } else {
            AVCodec* dec_codec = avcodec_find_decoder(fmt_ctx->streams[stream_idx]->codecpar->codec_id);
            dec_ctx = avcodec_alloc_context3(dec_codec);
            if (avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[stream_idx]->codecpar) < 0) {
                std::string err_msg{"ERROR: Could not find " +  media_typestr + " stream in input file"};
                throw std::runtime_error(err_msg);
            }
            if (avcodec_open2(dec_ctx, dec_codec, nullptr) < 0) {
                std::string err_msg{"ERROR: couldn't open " + media_typestr + " codec"};
                throw std::runtime_error(err_msg);
            }
        }
        return stream_idx;
    }

    inline char* ffav_err2str(int errnum)
    {
        static char str[AV_ERROR_MAX_STRING_SIZE];
        memset(str, 0, sizeof(str));
        return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
    }
}


void VideoReader::intialize_ffmpeg()
{
    av_register_all();
    if (avformat_open_input(&av_params.fmt_ctx, vpath.c_str(), nullptr, nullptr) < 0) {
        std::string err_msg {"ERROR: check if " + vpath + " exists and/or is accessible"};
        throw std::runtime_error(err_msg);
    }

    if(avformat_find_stream_info(av_params.fmt_ctx, nullptr) < 0) {
        std::string err_msg {"ERROR: couldn't find source information for " + vpath};
        throw std::runtime_error(err_msg);
    }

    av_params.video_stream_idx = utils::open_codec_context(av_params.fmt_ctx, av_params.video_dec_ctx, AVMEDIA_TYPE_VIDEO);
    av_params.video_stream = av_params.fmt_ctx->streams[av_params.video_stream_idx];

    //TODO: can change these as needed, if we need a specific output dimension
    const int width = av_params.video_dec_ctx->width;
    const int height = av_params.video_dec_ctx->height;
    constexpr AVPixelFormat pix_format = AV_PIX_FMT_RGB24;
    av_params.img_transform = sws_getCachedContext(
        nullptr, av_params.video_dec_ctx->width, av_params.video_dec_ctx->height, av_params.video_dec_ctx->pix_fmt,
        width, height, pix_format, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!av_params.img_transform) {
        std::string err_msg {"ERROR: couldn't get sws context"};
        throw std::runtime_error(err_msg);
    }

    //TODO: need to compute the time base for the video stream

    av_params.video_time_base = av_params.fmt_ctx->streams[av_params.video_stream_idx]->time_base; 
    std::cout << "output: " << height << " x " << width << " @ " << av_get_pix_fmt_name(pix_format) << std::endl;
    //will write the input information (to stderr)
    av_dump_format(av_params.fmt_ctx, 0, vpath.c_str(), 0);
}

VideoFrame<VideoReader::PixelT> VideoReader::read_video_frames(const int frame_index, const bool read_fwd)
{
    //in theory, if we just access in-order through the video, we won't need to seek at all; also, 
    //if we *do* jump around, but it's all within the frame cache, then we also don't need to seek,
    //but we on;y get to here if there was a cache miss (in which case, we only have to check if the 
    //requested frame is the next frame) 
    av_params.need_to_seek = frame_index != current_frame_idx;
    if (read_fwd) {
        if (av_params.need_to_seek) {
            auto pos = (av_gettime() - av_params.video_current_pts_time) / 1000000.f;
            auto spos = pos + frame_index;
            auto seek_pos = spos * AV_TIME_BASE;
            seek_pos = av_rescale_q (seek_pos, AV_TIME_BASE_Q, av_params.video_time_base);
            av_params.last_seek_pos = seek_pos;

            //get the time-adjusted frame index
            int64_t seek_frame_index = seek_pos; //frame_index * av_params.video_time_base;
            //if(av_seek_frame(av_params.fmt_ctx, -1, seek_frame_index, AVSEEK_FLAG_ANY) < 0) {
            //if(av_seek_frame(av_params.fmt_ctx, av_params.video_stream_idx, seek_frame_index, AVSEEK_FLAG_BACKWARD) < 0) {
            if(avformat_seek_file(av_params.fmt_ctx, av_params.video_stream_idx, INT64_MIN, seek_frame_index, INT64_MAX, AVSEEK_FLAG_ANY) < 0) {
                std::string err_msg {"ERROR: couldn't seek to frame " + std::to_string(seek_frame_index)};
                throw std::runtime_error(err_msg);
            }

            //TODO: the above seeks to the nearest prior keyframe, need to still move it along to the actual target frame 
            //the question is, how do we get the frame number?
        }
    } else {
        //TODO: need to seek back VFRAME_CACHE_SIZE #frames from the requested frame, then read forward until we get to the right frame index,
        //then read forwards by VFRAME_CACHE_SIZE #frames
    }

    //TODO: need to consider different caching approaches -- here we always read the new frames in starting at 
    //the 0th index, but if we have a large cache (i.e. larger than the #frames read each time), then it would 
    //be better to have a more intelligent scheme for reading the frames --> need to experiment some more, and this
    //function is where the different caching policies could be tried out
    static constexpr int NREAD_FRAMES = VFRAME_CACHE_SIZE;

    //NOTE: we need to swap our fwd and backward cache indices
    //TODO: have some check that we are enforcing that the backward cache is indeed filled with lower frame indices than the fwd cache
    fwd_cache_idx = 1 - fwd_cache_idx; 
    //set the cache index according to the direction to read from
    const int cache_idx = read_fwd ? fwd_cache_idx : 1-fwd_cache_idx;
    int num_frames_read = parse_video(frame_index, NREAD_FRAMES, cache_idx);
    assert(num_frames_read > 0);
    if (num_frames_read < NREAD_FRAMES) {
        //NOTE: this should be indicative of us being at the end of the video?
        std::cout << "NOTE: read " << num_frames_read << " out of requested " << NREAD_FRAMES << " #frames" << std::endl;
    }

    //TODO: if we are reading backwards, do we need to return the 1st or last index?
    return frame_cache[cache_idx][0];
}

int VideoReader::parse_video(const int base_frame_index, const int num_read_frames, const int cache_index)
{
    av_params.frame = av_frame_alloc();
    av_params.RGB_frame = av_frame_alloc();
    if(!av_params.frame || !av_params.RGB_frame) {
        std::string err_msg {"ERROR: couldn't allocate the frame"};
        throw std::runtime_error(err_msg);
    }

    //allocate and initialize the AVPacket object
    av_init_packet(&av_params.pkt);
    av_params.pkt.data = nullptr;
    av_params.pkt.size = 0;

    //NOTE: I would have thought 16 to be the right value here, but 1 seems to be the only one that works
    static constexpr int AVFRAME_ALIGN = 1;

    //allocate the RGBFrame buffer 
    const int num_bytes = av_image_get_buffer_size (AV_PIX_FMT_RGB24, av_params.video_dec_ctx->width, av_params.video_dec_ctx->height, AVFRAME_ALIGN);
    uint8_t* RGB_frame_buffer = (uint8_t *)av_malloc(num_bytes*sizeof(uint8_t));
    av_image_fill_arrays (av_params.RGB_frame->data, av_params.RGB_frame->linesize, RGB_frame_buffer, 
            AV_PIX_FMT_RGB24, av_params.video_dec_ctx->width, av_params.video_dec_ctx->height, AVFRAME_ALIGN);

    av_params.RGB_frame->width = av_params.video_dec_ctx->width;
    av_params.RGB_frame->height = av_params.video_dec_ctx->height;
    bool got_frame = false;
    int nframes_read = 0;
    while(av_read_frame(av_params.fmt_ctx, &av_params.pkt) >= 0 && nframes_read < num_read_frames) {
        auto fdata = decode_frame<PixelT>();
        got_frame = fdata != nullptr;
        av_packet_unref(&av_params.pkt);

        if (got_frame) {
            //TODO: I should probably have some more intelligent caching scheme. Can (should?) experiment with this some more
            frame_cache[cache_index][nframes_read] = VideoFrame<PixelT>(fdata, av_params.RGB_frame->height, av_params.RGB_frame->width);
            frame_cache_idxmap[cache_index][nframes_read] = base_frame_index + nframes_read;
            nframes_read++;
        }

        //check if the frame-acquisition has been cancelled
        //if(!get_frames.load(std::memory_order_relaxed))
        //    break;
        //get roughly the target framerate
        //auto frame_wait = std::chrono::milliseconds(static_cast<int>(1000/FPS));
        //std::this_thread::sleep_for(frame_wait);
    }

    //also have to flush the cached frames -- presumably this is only important at the end of the video though?
    av_params.pkt.data = nullptr;
    av_params.pkt.size = 0;
    if (nframes_read < num_read_frames) {
        do {
            auto fdata = decode_frame<PixelT>();
            frame_cache[cache_index][nframes_read] = VideoFrame<PixelT>(fdata, av_params.RGB_frame->height, av_params.RGB_frame->width);
            frame_cache_idxmap[cache_index][nframes_read] = base_frame_index + nframes_read;
            nframes_read++;
            got_frame = fdata != nullptr;
            std::cout << "Flushing Cached Frames" << std::endl;
        } while (got_frame && nframes_read < num_read_frames);
    }

    //return the actual number of frames read
    return nframes_read;
}



bool VideoReader::decode_packet(int decode_call_index)
{
    static constexpr int NUM_DECODE_RETRIES = 10;
    bool at_target_frame = false;
    int ret = avcodec_send_packet(av_params.video_dec_ctx, &av_params.pkt);
    // In particular, we don't expect AVERROR(EAGAIN), because we read all
    // decoded frames with avcodec_receive_frame() until done.
    if (ret < 0) {
        fprintf(stderr, "NOTE: decode_packet1 ret w/ code: %s -- %d\n", utils::ffav_err2str(ret), int(ret == AVERROR_EOF));
        if (decode_call_index < NUM_DECODE_RETRIES) {
            return decode_packet(decode_call_index + 1);
        } else {
            return false;
        }
    }
    ret = avcodec_receive_frame(av_params.video_dec_ctx, av_params.frame);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
        fprintf(stderr, "NOTE: decode_packet1 ret w/ code: %s\n", utils::ffav_err2str(ret));
        return false;
    }
    if (ret >= 0) {
        av_params.video_current_pts = av_params.frame->pts;
        av_params.video_current_pts_time = av_gettime();
        auto seek_diff = av_params.last_seek_pos - av_params.video_current_pts;
        std::cout << "frame pkt seek time delta: " << seek_diff << " @ TS: " << av_params.video_current_pts_time << std::endl;
        return true; 
    }
    fprintf(stderr, "NOTE: decode_packet1 ret w/ code: %s\n", utils::ffav_err2str(ret));
    return false;
}
