#include <iostream>
#include <vector>

#include "opencv2/opencv.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/video.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

using namespace std;
using namespace cv;

int width = 0;
int height = 0;
int fps = 30;
int bitrate = 300000;

VideoCapture get_input_stream() {

    setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "protocol_whitelist;file,rtp,udp", 1);

    VideoCapture cap("test.sdp", cv::CAP_FFMPEG);

    if(!cap.isOpened()){
        cout << "Error opening rtp://127.0.0.1:1234";
        exit(1);
    }

    width = cap.get(CAP_PROP_FRAME_WIDTH);
    height = cap.get(CAP_PROP_FRAME_HEIGHT);

    cout << "Incoming width:" << width << endl;
    cout << "Incoming height:" << height << endl;

    return cap;
}

void initialize_avformat_context(AVFormatContext *&fctx, const char *format_name)
{
    int ret = avformat_alloc_output_context2(&fctx, nullptr, format_name, nullptr);
    if (ret < 0)
    {
        std::cout << "Could not allocate output format context!" << std::endl;
        exit(1);
    }
}

void initialize_io_context(AVFormatContext *&fctx, const char *output)
{
    if (!(fctx->oformat->flags & AVFMT_NOFILE))
    {
        int ret = avio_open2(&fctx->pb, output, AVIO_FLAG_WRITE, nullptr, nullptr);
        if (ret < 0)
        {
            std::cout << "Could not open output IO context!" << std::endl;
            exit(1);
        }
    }
}


void set_codec_params(AVFormatContext *&fctx, AVCodecContext *&codec_ctx)
{
    const AVRational dst_fps = {fps, 1};

    codec_ctx->codec_tag = 0;
    codec_ctx->codec_id = AV_CODEC_ID_H264;
    codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->gop_size = 12;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->framerate = dst_fps;
    codec_ctx->time_base = av_inv_q(dst_fps);
    codec_ctx->bit_rate = bitrate;
    if (fctx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
}

void initialize_codec_stream(AVStream *&stream, AVCodecContext *&codec_ctx, AVCodec *&codec, std::string codec_profile)
{
    int ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
    if (ret < 0)
    {
        std::cout << "Could not initialize stream codec parameters!" << std::endl;
        exit(1);
    }

    AVDictionary *codec_options = nullptr;
    av_dict_set(&codec_options, "profile", codec_profile.c_str(), 0);
    av_dict_set(&codec_options, "preset", "superfast", 0);
    av_dict_set(&codec_options, "tune", "zerolatency", 0);

    // open video encoder
    ret = avcodec_open2(codec_ctx, codec, &codec_options);
    if (ret < 0)
    {
        std::cout << "Could not open video encoder!" << std::endl;
        exit(1);
    }
}

SwsContext *initialize_sample_scaler(AVCodecContext *codec_ctx, double width, double height)
{
    SwsContext *swsctx = sws_getContext(width, height, AV_PIX_FMT_BGR24, width, height, codec_ctx->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!swsctx)
    {
        std::cout << "Could not initialize sample scaler!" << std::endl;
        exit(1);
    }

    return swsctx;
}

AVFrame *allocate_frame_buffer(AVCodecContext *codec_ctx, double width, double height)
{
    AVFrame *frame = av_frame_alloc();

    std::vector<uint8_t> framebuf(av_image_get_buffer_size(codec_ctx->pix_fmt, width, height, 1));
    av_image_fill_arrays(frame->data, frame->linesize, framebuf.data(), codec_ctx->pix_fmt, width, height, 1);
    frame->width = width;
    frame->height = height;
    frame->format = static_cast<int>(codec_ctx->pix_fmt);

    return frame;
}

void write_frame(AVCodecContext *codec_ctx, AVFormatContext *fmt_ctx, AVFrame *frame)
{
    AVPacket pkt = {0};
    av_init_packet(&pkt);

    int ret = avcodec_send_frame(codec_ctx, frame);
    if (ret < 0)
    {
        std::cout << "Error sending frame to codec context!" << std::endl;
        exit(1);
    }

    ret = avcodec_receive_packet(codec_ctx, &pkt);
    if (ret < 0)
    {
        std::cout << "Error receiving packet from codec context!" << std::endl;
        exit(1);
    }

    av_interleaved_write_frame(fmt_ctx, &pkt);
    av_packet_unref(&pkt);
}


int main() {

    std::string h264profile = "high444";
    std::string outputServer = "rtmp://localhost/live/stream";

    av_log_set_level(AV_LOG_DEBUG);

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif
    avformat_network_init();

    const char *output = outputServer.c_str();
    int ret;
    VideoCapture cap = get_input_stream();

    std::vector<uint8_t> imgbuf(height * width * 3 + 16);
    cv::Mat image(height, width, CV_8UC3, imgbuf.data(), width * 3);
    AVFormatContext *ofmt_ctx = nullptr;
    AVCodec *out_codec = nullptr;
    AVStream *out_stream = nullptr;
    AVCodecContext *out_codec_ctx = nullptr;

    initialize_avformat_context(ofmt_ctx, "flv");
    ofmt_ctx->protocol_whitelist = "file,udp,rtp";
    initialize_io_context(ofmt_ctx, output);

    out_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    out_stream = avformat_new_stream(ofmt_ctx, out_codec);
    out_codec_ctx = avcodec_alloc_context3(out_codec);

    set_codec_params(ofmt_ctx, out_codec_ctx);
    initialize_codec_stream(out_stream, out_codec_ctx, out_codec, h264profile);

    out_stream->codecpar->extradata = out_codec_ctx->extradata;
    out_stream->codecpar->extradata_size = out_codec_ctx->extradata_size;

    av_dump_format(ofmt_ctx, 0, output, 1);

    auto *swsctx = initialize_sample_scaler(out_codec_ctx, width, height);
    auto *frame = allocate_frame_buffer(out_codec_ctx, width, height);

    int cur_size;
    uint8_t *cur_ptr;

    ret = avformat_write_header(ofmt_ctx, nullptr);
    if (ret < 0)
    {
        std::cout << "Could not write header!" << std::endl;
        exit(1);
    }

    bool end_of_stream = false;

    do {
        cap >> image;
        const int stride[] = {static_cast<int>(image.step[0])};
        sws_scale(swsctx, &image.data, stride, 0, image.rows, frame->data, frame->linesize);
        frame->pts += av_rescale_q(1, out_codec_ctx->time_base, out_stream->time_base);
        write_frame(out_codec_ctx, ofmt_ctx, frame);
    } while (!end_of_stream);

    av_write_trailer(ofmt_ctx);

    av_frame_free(&frame);
    avcodec_close(out_codec_ctx);
    avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    return 0;
}
