#include "VideoReader.hpp"

#include <stdexcept>
#include <iostream>

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
    constexpr AVPixelFormat pix_format = AV_PIX_FMT_BGR24;
    av_params.img_transform = sws_getCachedContext(
        nullptr, av_params.video_dec_ctx->width, av_params.video_dec_ctx->height, av_params.video_dec_ctx->pix_fmt,
        width, height, pix_format, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!av_params.img_transform) {
        std::string err_msg {"ERROR: couldn't get sws context"};
        throw std::runtime_error(err_msg);
    }
    av_params.video_time_base = av_q2d (av_params.video_stream->time_base); //(AV_TIME_BASE * av_params.video_dec_ctx->time_base.num) / av_params.video_dec_ctx->time_base.den;
    std::cout << "output: " << height << " x " << width << " @ " << av_get_pix_fmt_name(pix_format) << std::endl;
    //will write the input information (to stderr)
    av_dump_format(av_params.fmt_ctx, 0, vpath.c_str(), 0);
}

VideoFrame<VideoReader::PixelT> VideoReader::read_video_frames(const int frame_index)
{
    //in theory, if we just access in-order through the video, we won't need to seek at all; also, 
    //if we *do* jump around, but it's all within the frame cache, then we also don't need to seek,
    //but we on;y get to here if there was a cache miss (in which case, we only have to check if the 
    //requested frame is the next frame) 
    bool need_to_seek = frame_index != current_frame_idx;
    if (need_to_seek) {
        std::cout << "seeking in video " << current_frame_idx << " --> " << frame_index << std::endl;
        //get the time-adjusted frame index
        int64_t seek_frame_index = frame_index * av_params.video_time_base;
        //if(av_seek_frame(av_params.fmt_ctx, -1, seek_frame_index, AVSEEK_FLAG_ANY) < 0) {
        if(av_seek_frame(av_params.fmt_ctx, -1, seek_frame_index, AVSEEK_FLAG_BACKWARD) < 0) {
            std::string err_msg {"ERROR: couldn't seek to frame " + std::to_string(seek_frame_index)};
            throw std::runtime_error(err_msg);
        }

        //TODO: the above seeks to the nearest prior keyframe, need to still move it along to the actual target frame 
        //the question is, how do we get the frame number?


    }


    //TODO: need to consider different caching approaches -- here we always read the new frames in starting at 
    //the 0th index, but if we have a large cache (i.e. larger than the #frames read each time), then it would 
    //be better to have a more intelligent scheme for reading the frames --> need to experiment some more, and this
    //function is where the different caching policies could be tried out
    static constexpr int NREAD_FRAMES = 16;
    int num_frames_read = parse_video(frame_index, NREAD_FRAMES);
    assert(num_frames_read > 0);
    if (num_frames_read < NREAD_FRAMES) {
        //NOTE: this should be indicative of us being at the end of the video?
        std::cout << "NOTE: read " << num_frames_read << " out of requested " << NREAD_FRAMES << " #frames" << std::endl;
    }
    return frame_cache[0];
}

int VideoReader::parse_video(const int base_frame_index, const int num_read_frames)
{
    av_params.frame = av_frame_alloc();
    av_params.BGR_frame = av_frame_alloc();
    if(!av_params.frame || !av_params.BGR_frame) {
        std::string err_msg {"ERROR: couldn't allocate the frame"};
        throw std::runtime_error(err_msg);
    }

    //allocate and initialize the AVPacket object
    av_init_packet(&av_params.pkt);
    av_params.pkt.data = nullptr;
    av_params.pkt.size = 0;

    //NOTE: I would have thought 16 to be the right value here, but 1 seems to be the only one that works
    static constexpr int AVFRAME_ALIGN = 1;

    //allocate the BGRFrame buffer 
    const int num_bytes = av_image_get_buffer_size (AV_PIX_FMT_BGR24, av_params.video_dec_ctx->width, av_params.video_dec_ctx->height, AVFRAME_ALIGN);
    uint8_t* BGR_frame_buffer = (uint8_t *)av_malloc(num_bytes*sizeof(uint8_t));
    av_image_fill_arrays (av_params.BGR_frame->data, av_params.BGR_frame->linesize, BGR_frame_buffer, 
            AV_PIX_FMT_BGR24, av_params.video_dec_ctx->width, av_params.video_dec_ctx->height, AVFRAME_ALIGN);

    av_params.BGR_frame->width = av_params.video_dec_ctx->width;
    av_params.BGR_frame->height = av_params.video_dec_ctx->height;
    bool got_frame = false;
    int nframes_read = 0;
    while(av_read_frame(av_params.fmt_ctx, &av_params.pkt) >= 0 && nframes_read < num_read_frames) {
        auto fdata = decode_frame<PixelT>();
        got_frame = fdata != nullptr;
        av_packet_unref(&av_params.pkt);

        if (got_frame) {
            //TODO: I should probably have some more intelligent caching scheme. Can (should?) experiment with this some more
            frame_cache[nframes_read] = VideoFrame<PixelT>(fdata, av_params.BGR_frame->height, av_params.BGR_frame->width);
            frame_cache_idxmap[nframes_read] = base_frame_index + nframes_read;
            nframes_read++;
        }

        //check if the frame-acquisition has been cancelled
        //if(!get_frames.load(std::memory_order_relaxed))
        //    break;
        //get roughly the target framerate
        //auto frame_wait = std::chrono::milliseconds(static_cast<int>(1000/FPS));
        //std::this_thread::sleep_for(frame_wait);
    }

    //also have to flush the cached frames -- presumably this is only important at the end of the video though
    av_params.pkt.data = nullptr;
    av_params.pkt.size = 0;
    if (nframes_read < num_read_frames) {
        do {
            auto fdata = decode_frame<PixelT>();
            frame_cache[nframes_read] = VideoFrame<PixelT>(fdata, av_params.BGR_frame->height, av_params.BGR_frame->width);
            frame_cache_idxmap[nframes_read] = base_frame_index + nframes_read;
            nframes_read++;
            got_frame = fdata != nullptr;
            std::cout << "Flushing Cached Frames" << std::endl;
        } while (got_frame && nframes_read < num_read_frames);
    }

    //return the actual number of frames read
    return nframes_read;
}


namespace ffutils {
inline char* ffav_err2str(int errnum)
{
    static char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}
}



bool VideoReader::decode_packet()
{
    int ret = avcodec_send_packet(av_params.video_dec_ctx, &av_params.pkt);
    // In particular, we don't expect AVERROR(EAGAIN), because we read all
    // decoded frames with avcodec_receive_frame() until done.
    if (ret < 0) {
        fprintf(stderr, "NOTE: decode_packet1 ret w/ code: %s -- %d\n", ffutils::ffav_err2str(ret), int(ret == AVERROR_EOF));
        return decode_packet();
    }
    ret = avcodec_receive_frame(av_params.video_dec_ctx, av_params.frame);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
        fprintf(stderr, "NOTE: decode_packet1 ret w/ code: %s\n", ffutils::ffav_err2str(ret));
        return false;
    }
    if (ret >= 0) {
       return true; 
    }
    fprintf(stderr, "NOTE: decode_packet1 ret w/ code: %s\n", ffutils::ffav_err2str(ret));
    return false;
}
