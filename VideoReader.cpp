#include "VideoReader.hpp"

#include <stdexcept>
#include <iostream>


namespace utils {
    void open_codec_context(int *stream_idx, AVFormatContext *fmt_ctx, AVMediaType type)
    {
        int ret;
        AVStream *st;
        AVCodecContext *dec_ctx = nullptr;
        AVCodec *dec = nullptr;
        const std::string media_typestr {av_get_media_type_string(type)};
        ret = av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
        if (ret < 0) {
            std::string err_msg{"ERROR: Could not find " +  media_typestr + " stream in input file"};
            throw std::runtime_error(err_msg);
        } else {
            *stream_idx = ret;
            st = fmt_ctx->streams[*stream_idx];
            // find decoder for the stream 
            dec_ctx = st->codec;
            dec = avcodec_find_decoder(dec_ctx->codec_id);
            if (!dec) {
                std::string err_msg{"ERROR: Failed to find " + media_typestr + " codec"};
                throw std::runtime_error(err_msg);
            }
            
            if (avcodec_open2(dec_ctx, dec, nullptr) < 0) {
                std::string err_msg{"ERROR: couldn't open " + media_typestr + " codec"};
                throw std::runtime_error(err_msg);
            }
        }
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

    utils::open_codec_context(&av_params.video_stream_idx, av_params.fmt_ctx, AVMEDIA_TYPE_VIDEO);
    av_params.video_stream = av_params.fmt_ctx->streams[av_params.video_stream_idx];
    av_params.video_dec_ctx = av_params.video_stream->codec;

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
    std::cout << "output: " << height << " x " << width << " @ " << av_get_pix_fmt_name(pix_format) << std::endl;
    //will write the input information (to stderr)
    av_dump_format(av_params.fmt_ctx, 0, vpath.c_str(), 0);
}

VideoFrame<VideoReader::PixelT> VideoReader::read_video_frames(const int frame_index)
{
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

	//allocate the BGRFrame buffer 
    const int num_bytes = avpicture_get_size(AV_PIX_FMT_BGR24, av_params.video_dec_ctx->width, av_params.video_dec_ctx->height);
    uint8_t* BGR_frame_buffer = (uint8_t *)av_malloc(num_bytes*sizeof(uint8_t));
    avpicture_fill((AVPicture*)av_params.BGR_frame, BGR_frame_buffer, AV_PIX_FMT_BGR24, av_params.video_dec_ctx->width, av_params.video_dec_ctx->height);
    av_params.BGR_frame->width = av_params.video_dec_ctx->width;
    av_params.BGR_frame->height =  av_params.video_dec_ctx->height;
    bool got_frame = false;
    int nframes_read = 0;
    while(av_read_frame(av_params.fmt_ctx, &av_params.pkt) >= 0 && nframes_read < num_read_frames) {
        auto fdata = decode_packet<PixelT>(false);
        got_frame = fdata != nullptr;
        av_free_packet(&av_params.pkt);

        //TODO: I should probably have some more intelligent caching scheme. Can (should?) experiment with this some more
        frame_cache[nframes_read] = VideoFrame<PixelT>(fdata, av_params.BGR_frame->height, av_params.BGR_frame->width);
        frame_cache_idxmap[nframes_read] = base_frame_index + nframes_read;
        nframes_read++;

        //check if the frame-acquisition has been cancelled
        //if(!get_frames.load(std::memory_order_relaxed))
        //    break;
		//get roughly the target framerate
		//auto frame_wait = std::chrono::milliseconds(static_cast<int>(1000/FPS));
		//std::this_thread::sleep_for(frame_wait);
    }

    //also have to flush the cached frames
    av_params.pkt.data = nullptr;
    av_params.pkt.size = 0;
    do {
        auto fdata = decode_packet<PixelT>(true);
        frame_cache[nframes_read] = VideoFrame<PixelT>(fdata, av_params.BGR_frame->height, av_params.BGR_frame->width);
        frame_cache_idxmap[nframes_read] = base_frame_index + nframes_read;
        nframes_read++;
        got_frame = fdata != nullptr;
        std::cout << "Flushing Cached Frames" << std::endl;
    } while (got_frame && nframes_read < num_read_frames);

    //return the actual number of frames read
    return nframes_read;
}
