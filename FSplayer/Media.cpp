
#include "Media.h"
#include "Utils.h"
#include <iostream>

extern "C" {
#include <libavutil/time.h>
}


#include <log4cplus/fileappender.h>  
#include <log4cplus/layout.h>  
#include <log4cplus/ndc.h>  
#include <log4cplus/helpers/loglog.h>  
#include <log4cplus/loggingmacros.h>  

#include "VLog.h"


extern bool quit;

static void close_input_file(AVFormatContext **ctx_ptr) {
	int i;
	AVFormatContext *fmt_ctx = *ctx_ptr;

	for (i = 0; i < fmt_ctx->nb_streams; i++) {
		AVStream *stream = fmt_ctx->streams[i];
		avcodec_close(stream->codec);
	}
	avformat_close_input(ctx_ptr);
}

MediaState::MediaState()
{
	pFormatCtx = nullptr;
	//audio = new AudioState();

	video = new VideoState();
	//quit = false;
}

MediaState::MediaState(const char* input_file)
{
	if (input_file)
	{
		filename = new char[strlen(input_file) + 1];
		strcpy(filename, input_file);
	}
	pFormatCtx = avformat_alloc_context();

	video = new VideoState();
}

MediaState::MediaState(const char* input_file, const char* file_md5, int count, int x, int y, int width, int heigh, SDL_Window *window, SDL_Renderer *renderer)
{
	if (input_file)
	{
		filename = new char[strlen(input_file) + 1];
		strcpy(filename, input_file);
	}
	if (file_md5)
	{
		md5 = new char[strlen(file_md5) + 1];
		strcpy(md5, file_md5);
	}
	loopCount = count;
	display_x = x;
	display_y = y;
	display_width = width;
	display_heigh = heigh;
	pFormatCtx = avformat_alloc_context();
	audio = new AudioState(input_file);
	video = new VideoState(input_file, window, renderer);
}

MediaState::~MediaState()
{
	//SDL_LockMutex(mutex);
	log4cplus::Logger logger = GetSubLogger(L"Media");
	LOG4CPLUS_DEBUG(logger, "开始回收媒体资源" << filename);
	if (audio) {
		delete audio;
		audio = nullptr;
	}
	if (video) {
		delete video;
		video = nullptr;
	}
	if (pFormatCtx) {
		close_input_file(&pFormatCtx);
	}

	if (filename) {
		delete[] filename;
		filename = nullptr;
	}
	if (md5) {
		delete[] md5;
		md5 = nullptr;
	}
	LOG4CPLUS_DEBUG(logger, "结束回收媒体资源");
	/*if (start_play_time != nullptr){
		delete start_play_time;
		start_play_time = nullptr;
	}

	if (end_play_time != nullptr) {
		delete end_play_time;
		end_play_time = nullptr;
	}*/
	//SDL_UnlockMutex(mutex);
}
//void MediaState::reset()
//{
//	audio->reset();
//	video->reset();
//	avformat_free_context(pFormatCtx);
//	pFormatCtx = avformat_alloc_context();;
//}

bool MediaState::openInput()
{
	log4cplus::Logger logger = GetSubLogger(L"Media");
	//long long start = Utils::milliseconds_now();

	// Open input file
	if (avformat_open_input(&pFormatCtx, filename, nullptr, nullptr) < 0)
	{
		LOG4CPLUS_DEBUG(logger, "打开文件异常：" << filename);
		return false;
	}

	if (avformat_find_stream_info(pFormatCtx, nullptr) < 0)
	{
		LOG4CPLUS_DEBUG(logger, "打开文件异常：" << filename);
		return false;
	}

	// Output the stream info to standard 
	//av_dump_format(pFormatCtx, 0, filename, 0);

	//long long elapsed = Utils::milliseconds_now() - start;

	//std::cout << "openFile time: " << elapsed << std::endl;

	for (uint32_t i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audio->stream_index < 0)
			audio->stream_index = i;

		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && video->stream_index < 0)
			video->stream_index = i;
	}

	if (audio->stream_index < 0 && video->stream_index < 0)
	{
		LOG4CPLUS_DEBUG(logger, "视频流异常：" << filename);
		return false;
	}
	// Fill audio state
	if (audio->stream_index >= 0)
	{
		AVCodec *pCodec = avcodec_find_decoder(pFormatCtx->streams[audio->stream_index]->codecpar->codec_id);
		if (!pCodec){
			LOG4CPLUS_DEBUG(logger, "AVCodec解码器未找到：" << filename);
			return false;
		}
		audio->stream = pFormatCtx->streams[audio->stream_index];

		audio->audio_ctx = avcodec_alloc_context3(pCodec);
		if (avcodec_parameters_to_context(audio->audio_ctx, audio->stream->codecpar) < 0) {
			LOG4CPLUS_DEBUG(logger, "avcodec_parameters_to_context失败：" << filename);
			return false;
		}
		avcodec_open2(audio->audio_ctx, pCodec, nullptr);
	}

	if (video->stream_index >= 0)
	{
		// Fill video state
		AVCodec *pVCodec = avcodec_find_decoder(pFormatCtx->streams[video->stream_index]->codec->codec_id);
		if (!pVCodec) {
			LOG4CPLUS_DEBUG(logger, "pVCodec解码器未找到：" << filename);
			return false;
		}

		video->stream = pFormatCtx->streams[video->stream_index];

		video->video_ctx = avcodec_alloc_context3(pVCodec);
		if (avcodec_parameters_to_context(video->video_ctx, video->stream->codecpar) < 0) {
			LOG4CPLUS_DEBUG(logger, "avcodec_parameters_to_context失败：" << filename);
			return false;
		}
		avcodec_open2(video->video_ctx, pVCodec, nullptr);

		video->frame_timer = static_cast<double>(av_gettime()) / 1000000.0;
		video->frame_last_delay = 40e-3;
	}
	return true;
}

int stream_has_enough_packets(AVStream *st, PacketQueue *queue) {
	return st != nullptr && queue->nb_packets > 25 && (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
}

int decode_Media(void *data)
{
	MediaState *media = (MediaState*)data;

	AVPacket *packet = av_packet_alloc();


	//SDL_Event event;
	while (true)
	{
		if (quit) {
			log4cplus::Logger logger = GetSubLogger(L"Media");
			LOG4CPLUS_DEBUG(logger, "结束视频帧编码");
			break;
		}
		if (stream_has_enough_packets(media->video->stream, media->video->videoq) && stream_has_enough_packets(media->audio->stream, media->audio->audioq))
		{
			SDL_Delay(10);
			continue;
		}
		int ret = av_read_frame(media->pFormatCtx, packet);
		if (ret < 0)
		{
			if (ret == AVERROR_EOF)
			{
				av_packet_unref(packet);
				break;
			}
			if (media->pFormatCtx->pb->error == 0) // No error,wait for user input
			{
				av_packet_unref(packet);
				SDL_Delay(100);
				continue;
			}
			else
			{
				av_packet_unref(packet);
				break;
			}
		}

		if (packet->stream_index == media->audio->stream_index) // audio stream
		{
			media->audio->audioq->enQueue(packet);
			av_packet_unref(packet);
		}

		else if (packet->stream_index == media->video->stream_index) // video stream
		{
			media->video->videoq->enQueue(packet);
			av_packet_unref(packet);
		}
		else
			av_packet_unref(packet);
	}

	media->loopCount--;
	av_packet_free(&packet);
	return 0;
}