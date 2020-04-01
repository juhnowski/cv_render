#include <iostream>
#include <vector>
// FFmpeg
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}
// OpenCV
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

using namespace std;
using namespace cv;

static void logging(const char *fmt, ...);
static void logging(const char *fmt, ...)
{
    va_list args;
    fprintf( stderr, "LOG: " );
    va_start( args, fmt );
    vfprintf( stderr, fmt, args );
    va_end( args );
    fprintf( stderr, "\n" );
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << "Usage: cv2ff <outfile>" << std::endl;
        return 1;
    }
    const char* outfile = argv[3];

    setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "protocol_whitelist;file,rtp,udp", 1);

    // initialize FFmpeg library
    av_register_all();
//  av_log_set_level(AV_LOG_DEBUG);
    int ret;

    const int dst_width = 1280;
    const int dst_height = 720;
    const AVRational dst_fps = {30, 1};

    // initialize audio
//-----------------------------------------------------------------------------------------------
    AVFormatContext *input_format_context = nullptr;
    AVPacket packet;

    AVCodec *pLocalCodec = NULL;
    AVCodecParameters *pLocalCodecParameters =  NULL;

    AVDictionary *d = NULL;
    av_dict_set(&d, "protocol_whitelist", "file,udp,rtp", 0);

    if ((ret = avformat_open_input(&input_format_context, argv[1], NULL, &d)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", argv[1]);
        return 2;
    }

    if ((ret = avformat_find_stream_info(input_format_context, NULL)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        return 2;
    }


    for (int i = 0; i < input_format_context->nb_streams; i++)
    {
        pLocalCodecParameters = input_format_context->streams[i]->codecpar;
        logging("AVStream->time_base before open coded %d/%d", input_format_context->streams[i]->time_base.num, input_format_context->streams[i]->time_base.den);
        logging("AVStream->r_frame_rate before open coded %d/%d", input_format_context->streams[i]->r_frame_rate.num, input_format_context->streams[i]->r_frame_rate.den);
        logging("AVStream->start_time %" PRId64, input_format_context->streams[i]->start_time);
        logging("AVStream->duration %" PRId64, input_format_context->streams[i]->duration);

        logging("finding the proper decoder (CODEC)");

        pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

        if (pLocalCodec==NULL) {
            logging("ERROR unsupported codec!");
            return -1;
        }

        logging("Audio Codec: %d channels, sample rate %d", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);

        // print its name, id and bitrate
        logging("\tCodec %s ID %d bit_rate %lld", pLocalCodec->name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
    }

    AVCodecContext *pCodecContext = avcodec_alloc_context3(pLocalCodec);
    if (!pCodecContext)
    {
        logging("failed to allocated memory for AVCodecContext");
        return -1;
    }

    if (avcodec_parameters_to_context(pCodecContext, pLocalCodecParameters) < 0)
    {
        logging("failed to copy codec params to codec context");
        return -1;
    }

    if (avcodec_open2(pCodecContext, pLocalCodec, NULL) < 0)
    {
        logging("failed to open codec through avcodec_open2");
        return -1;
    }

    AVFrame *pFrame = av_frame_alloc();
    if (!pFrame)
    {
        logging("failed to allocated memory for AVFrame");
        return -1;
    }

    AVPacket *pPacket = av_packet_alloc();
    if (!pPacket)
    {
        logging("failed to allocated memory for AVPacket");
        return -1;
    }

//------------------------------------------------------------------------------------------------
    // initialize OpenCV capture as input frame generator

    cv::VideoCapture cvcap(argv[2], cv::CAP_FFMPEG);
    //cv::VideoCapture cvcap(argv[1]);
    if (!cvcap.isOpened()) {
        std::cerr << "fail to open cv::VideoCapture";
        return 2;
    }
    cvcap.set(cv::CAP_PROP_FRAME_WIDTH, dst_width);
    cvcap.set(cv::CAP_PROP_FRAME_HEIGHT, dst_height);

    // allocate cv::Mat with extra bytes (required by AVFrame::data)
    std::vector<uint8_t> imgbuf(dst_height * dst_width * 3 + 16);
    cv::Mat image(dst_height, dst_width, CV_8UC3, imgbuf.data(), dst_width * 3);

    // open output format context
    AVFormatContext* outctx = nullptr;
    ret = avformat_alloc_output_context2(&outctx, nullptr, "flv", nullptr);
    outctx->protocol_whitelist = "file,tcp,rtmp,udp,rtp";
    if (ret < 0) {
        std::cerr << "fail to avformat_alloc_output_context2(" << outfile << "): ret=" << ret;
        return 2;
    }

    // open output IO context
    ret = avio_open2(&outctx->pb, outfile, AVIO_FLAG_WRITE, nullptr, nullptr);
    if (ret < 0) {
        std::cerr << "fail to avio_open2: ret=" << ret;
        return 2;
    }

    // create new video stream
    AVCodec* vcodec = avcodec_find_encoder(outctx->oformat->video_codec);
    AVStream* vstrm = avformat_new_stream(outctx, vcodec);
    if (!vstrm) {
        std::cerr << "fail to avformat_new_stream";
        return 2;
    }
    avcodec_get_context_defaults3(vstrm->codec, vcodec);
    vstrm->codec->width = dst_width;
    vstrm->codec->height = dst_height;
    vstrm->codec->pix_fmt = vcodec->pix_fmts[0];
    vstrm->codec->time_base = vstrm->time_base = av_inv_q(dst_fps);
    vstrm->r_frame_rate = vstrm->avg_frame_rate = dst_fps;
    if (outctx->oformat->flags & AVFMT_GLOBALHEADER)
        vstrm->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // open video encoder
    ret = avcodec_open2(vstrm->codec, vcodec, nullptr);
    if (ret < 0) {
        std::cerr << "fail to avcodec_open2: ret=" << ret;
        return 2;
    }

    std::cout
            << "outfile: " << outfile << "\n"
            << "format:  " << outctx->oformat->name << "\n"
            << "vcodec:  " << vcodec->name << "\n"
            << "size:    " << dst_width << 'x' << dst_height << "\n"
            << "fps:     " << av_q2d(dst_fps) << "\n"
            << "pixfmt:  " << av_get_pix_fmt_name(vstrm->codec->pix_fmt) << "\n"
            << std::flush;

    // initialize sample scaler
    SwsContext* swsctx = sws_getCachedContext(
            nullptr, dst_width, dst_height, AV_PIX_FMT_BGR24,
            dst_width, dst_height, vstrm->codec->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!swsctx) {
        std::cerr << "fail to sws_getCachedContext";
        return 2;
    }

    // allocate frame buffer for encoding
    AVFrame* frame = av_frame_alloc();
    std::vector<uint8_t> framebuf(avpicture_get_size(vstrm->codec->pix_fmt, dst_width, dst_height));
    avpicture_fill(reinterpret_cast<AVPicture*>(frame), framebuf.data(), vstrm->codec->pix_fmt, dst_width, dst_height);
    frame->width = dst_width;
    frame->height = dst_height;
    frame->format = static_cast<int>(vstrm->codec->pix_fmt);

    // encoding loop
    avformat_write_header(outctx, nullptr);
    int64_t frame_pts = 0;
    unsigned nb_frames = 0;
    bool end_of_stream = false;
    int got_pkt = 0;
    do {
        if (!end_of_stream) {
            // retrieve source image
            cvcap >> image;
            cv::imshow("press ESC to exit", image);
            if (cv::waitKey(33) == 0x1b)
                end_of_stream = true;
        }
        if (!end_of_stream) {
            // convert cv::Mat(OpenCV) to AVFrame(FFmpeg)
            const int stride[] = { static_cast<int>(image.step[0]) };
            sws_scale(swsctx, &image.data, stride, 0, image.rows, frame->data, frame->linesize);
            frame->pts = frame_pts++;
        }
        // encode video frame
        AVPacket pkt;
        pkt.data = nullptr;
        pkt.size = 0;
        av_init_packet(&pkt);
        ret = avcodec_encode_video2(vstrm->codec, &pkt, end_of_stream ? nullptr : frame, &got_pkt);
        if (ret < 0) {
            std::cerr << "fail to avcodec_encode_video2: ret=" << ret << "\n";
            break;
        }
        if (got_pkt) {
            // rescale packet timestamp
            pkt.duration = 1;
            av_packet_rescale_ts(&pkt, vstrm->codec->time_base, vstrm->time_base);
            // write packet
            av_write_frame(outctx, &pkt);
            std::cout << nb_frames << '\r' << std::flush;  // dump progress
            ++nb_frames;
        }
        av_free_packet(&pkt);

//---------------------------------------------------------------------------------------------------
//Audio
        if (!end_of_stream) {
            if (av_read_frame(input_format_context, pPacket) >= 0) {
                logging("AVPacket->pts %" PRId64, pPacket->pts);
            }
        }
//---------------------------------------------------------------------------------------------------
    } while (!end_of_stream || got_pkt);
    av_write_trailer(outctx);
    std::cout << nb_frames << " frames encoded" << std::endl;

    av_frame_free(&frame);
    avcodec_close(vstrm->codec);
    avio_close(outctx->pb);
    avformat_free_context(outctx);
    return 0;
}

