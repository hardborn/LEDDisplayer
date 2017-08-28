
#include "Video.h"
#include "VideoDisplay.h"
#include <iostream>

extern "C" {

#include <libswscale\swscale.h>
#include <libavutil/imgutils.h>  
#include <libavutil/time.h> 

}


#include <log4cplus/fileappender.h>  
#include <log4cplus/layout.h>  
#include <log4cplus/ndc.h>  
#include <log4cplus/helpers/loglog.h>  
#include <log4cplus/loggingmacros.h>  

#include "VLog.h"

extern bool quit;
extern bool quitflag;
extern SDL_Thread *decode_thread;
VideoState::VideoState()
{
	video_ctx = nullptr;
	stream_index = -1;
	stream = nullptr;

	this->window = nullptr;
	bmp = nullptr;
	this->renderer = nullptr;

	frame = nullptr;
	displayFrame = nullptr;

	videoq = new PacketQueue();
	frameq = new FrameQueue();

	frame_timer = 0.0;
	frame_last_delay = 0.0;
	frame_last_pts = 0.0;
	video_clock = 0.0;
}

VideoState::VideoState(const char* filename, SDL_Window *window, SDL_Renderer *renderer)
{
	if (filename)
	{
		name = new char[strlen(filename) + 1];
		strcpy(name, filename);
	}
	video_ctx = nullptr;
	stream_index = -1;
	stream = nullptr;

	this->window = window;
	bmp = nullptr;
	this->renderer = renderer;

	frame = nullptr;
	displayFrame = nullptr;

	videoq = new PacketQueue();
	frameq = new FrameQueue();

	frame_timer = 0.0;
	frame_last_delay = 0.0;
	frame_last_pts = 0.0;
	video_clock = 0.0;

}

VideoState::~VideoState()
{
	//std::cout << "---析构VideoState" << name << std::endl;

	log4cplus::Logger logger = GetSubLogger(L"Media");
	LOG4CPLUS_DEBUG(logger, "开始回收视频资源" << name);
	if (videoq) {
		delete videoq;
		videoq = nullptr;
	}

	if (frameq) {
		delete frameq;
		frameq = nullptr;
	}

	if (frame) {
		av_frame_free(&frame);
		frame = nullptr;
	}

	if (displayFrame) {
		av_freep(&displayFrame->data[0]);
		av_frame_free(&displayFrame);
		displayFrame = nullptr;
	}
	if (bmp) {
		SDL_DestroyTexture(bmp);
		bmp = nullptr;
	}
	if (video_ctx) {
		avcodec_free_context(&video_ctx);
		video_ctx = nullptr;
	}
	if (name) {
		delete[] name;
		name = nullptr;
	}
	LOG4CPLUS_DEBUG(logger, "结束回收视频资源");
}

//void VideoState::reset()
//{
//	delete videoq;
//	if (frame)
//		av_frame_free(&frame);
//	if (displayFrame)
//	{
//		av_freep(&displayFrame->data[0]);
//		av_freep(&displayFrame->linesize[0]);
//		av_frame_free(&displayFrame);
//	}
//	SDL_DestroyTexture(bmp);
//	avcodec_free_context(&video_ctx);
//
//	videoq = new PacketQueue();
//}

void VideoState::video_play(int x, int y, int width, int height, int window_width, int window_height, MediaState *media)
{
	//int width = 800;
	//int height = 600;
	// 创建sdl窗口
	//this->window = window;// SDL_CreateWindow("FFmpeg Decode", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,width, height, SDL_WINDOW_OPENGL);

	//this->renderer = renderer;// SDL_CreateRenderer(window, -1, 0);
	log4cplus::Logger logger = GetSubLogger(L"Media");
	rect.x = 0;// x;
	rect.y = 0;// y;
	rect.w = window_width;// width;
	rect.h = window_height;// height;;
	//media->video->rect
	bmp = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);

	/*rect.x = 0;
	rect.y = 0;
	rect.w = width;
	rect.h = height;*/

	frame = av_frame_alloc();
	displayFrame = av_frame_alloc();

	displayFrame->format = AV_PIX_FMT_YUV420P;
	displayFrame->width = rect.w;
	displayFrame->height = rect.h;

	//int numBytes = avpicture_get_size((AVPixelFormat)displayFrame->format, displayFrame->width, displayFrame->height);
	/*uint8_t *buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

	avpicture_fill((AVPicture*)displayFrame, buffer, (AVPixelFormat)displayFrame->format, displayFrame->width, displayFrame->height);*/

	uint8_t *frame_buffer_in = (uint8_t *)av_malloc(av_image_get_buffer_size((AVPixelFormat)displayFrame->format, displayFrame->width, displayFrame->height, 1));
	av_image_fill_arrays(displayFrame->data, displayFrame->linesize, frame_buffer_in, (AVPixelFormat)displayFrame->format, displayFrame->width, displayFrame->height, 1);

	decode_thread = SDL_CreateThread(decode, "", this);

	schedule_refresh(media, 40); // start display
	LOG4CPLUS_DEBUG(logger, "创建视频线程成功！");

	//media->loopCount--;
}

double VideoState::synchronize(AVFrame *srcFrame, double pts)
{
	double frame_delay;

	if (pts != 0)
		video_clock = pts; // Get pts,then set video clock to it
	else
		pts = video_clock; // Don't get pts,set it to video clock

	frame_delay = av_q2d(stream->codec->time_base);
	frame_delay += srcFrame->repeat_pict * (frame_delay * 0.5);

	video_clock += frame_delay;

	return pts;
}


int decode(void *arg)
{
	VideoState *video = (VideoState*)arg;

	AVFrame *frame = av_frame_alloc();

	AVPacket *packet = av_packet_alloc();
	double pts;

	while (true)
	{
		if (quit)
		{
			av_packet_unref(packet);
			av_frame_unref(frame);
			break;
		}
		if (!video->videoq->deQueue(packet, false))
		{
			av_packet_unref(packet);
			av_frame_unref(frame);
			break;
		}

		int ret = avcodec_send_packet(video->video_ctx, packet);
		if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
		{
			av_packet_unref(packet);
			av_frame_unref(frame);
			continue;
		}

		ret = avcodec_receive_frame(video->video_ctx, frame);
		if (ret < 0 && ret != AVERROR_EOF)
		{
			av_packet_unref(packet);
			av_frame_unref(frame);
			continue;
		}
		else if (ret == AVERROR_EOF)
		{
			av_packet_unref(packet);
			av_frame_unref(frame);
		}

		if ((pts = av_frame_get_best_effort_timestamp(frame)) == AV_NOPTS_VALUE)
			pts = 0;

		pts *= av_q2d(video->stream->time_base);

		pts = video->synchronize(frame, pts);

		frame->opaque = &pts;

		if (video->frameq->nb_frames >= FrameQueue::capacity)
			SDL_Delay(40);// av_usleep(10);

		video->frameq->enQueue(frame);

		av_packet_unref(packet);
		av_frame_unref(frame);
	}


	av_frame_free(&frame);
	av_packet_free(&packet);
	return 0;
}

