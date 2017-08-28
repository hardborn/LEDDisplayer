

extern "C" {

#include <libavcodec\avcodec.h>
#include <libavformat\avformat.h>
#include <libswscale\swscale.h>
#include <libswresample\swresample.h>
#include <libavutil/time.h> 
}

#include "ScreenState.h"

#include <SDL.h>
#include <SDL_thread.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/pointer.h"



#include <iostream>

#include "PacketQueue.h"
#include "Audio.h"
#include "Media.h"
#include "VideoDisplay.h"
#include "ScreenState.h"
#include "Utils.h"

#include <msclr\marshal_cppstd.h>

#include <log4cplus/fileappender.h>  
#include <log4cplus/layout.h>  
#include <log4cplus/ndc.h>  
#include <log4cplus/helpers/loglog.h>  
#include <log4cplus/loggingmacros.h>  

#include "VLog.h"
//#include <vld.h>

using namespace System;
using namespace Runtime::InteropServices;
using namespace rapidjson;

bool quit = false;

bool is_initialized = false;

SDL_Thread *dispose_thread;
SDL_Thread *decode_Media_thread;
SDL_Thread *decode_thread;
SDL_Thread *top_show_thread;
int schedule_loop_count = 0;
int schedule_media_count = 0;
int count = 0;



int ANSIToUTF8(const char *pszCode, char *UTF8code)
{
	WCHAR Unicode[100] = { 0, };
	char utf8[100] = { 0, };

	//read charLenth
	int nUnicodeSize = MultiByteToWideChar(CP_ACP, 0, pszCode, strlen(pszCode), Unicode, sizeof(Unicode));
	memset(UTF8code, 0, nUnicodeSize + 1);
	//read UTF-8Lenth
	int nUTF8codeSize = WideCharToMultiByte(CP_UTF8, 0, Unicode, nUnicodeSize, UTF8code, sizeof(Unicode), NULL, NULL);

	//converttoUTF-8
	MultiByteToWideChar(CP_UTF8, 0, utf8, nUTF8codeSize, Unicode, sizeof(Unicode));
	UTF8code[nUTF8codeSize + 1] = '\0';
	return nUTF8codeSize;
}

time_t GetCurrentDateTime()
{
	time_t now_time;
	now_time = time(NULL);
	return now_time;
}


//int dispose(void *arg)
//{
//	std::queue<MediaState*>* medialist = (std::queue<MediaState*>*)arg;
//	while (!medialist->empty())
//	{
//		auto media = medialist->front();
//		medialist->pop();
//	}
//	std::queue<MediaState*>().swap(*medialist);
//	return 0;
//}
int top_show(void *data) {
	while (true)
	{
		if (quit)
		{
			break;
		}
		SDL_RaiseWindow((SDL_Window *)data);
		SDL_Delay(200);
	}
	return 0;
}


MediaState *PlayNext(int window_width, int window_height, ScreenState& screenState)
{
	log4cplus::Logger logger = GetSubLogger(L"ScreenState");
	MediaState	*media = nullptr;
	//std::vector<PlayInfo *> playInfoList = screenState.get_play_info_list();
	if (screenState.get_play_info_list().empty())
	{
		return nullptr;
	}

	auto playInfo = screenState.get_play_info_list().front();
	std::cout << "playInfo: " << playInfo->file_path() << " \r\n列表数量为：" << screenState.get_play_info_list().size() << std::endl;
	media = new MediaState(playInfo->file_path(), playInfo->file_md5(), playInfo->play_count(), playInfo->display_x(), playInfo->display_y(), playInfo->display_width(), playInfo->display_heigh(), screenState.get_window(), screenState.get_renderer());
	screenState.get_play_info_list().erase(screenState.get_play_info_list().begin());
	std::cout << "剩余列表数量为：" << screenState.get_play_info_list().size() << std::endl;

	quit = false;
	if (media->openInput())
	{
		LOG4CPLUS_DEBUG(logger, "打开媒体文件成功" << media->filename);
		decode_Media_thread = SDL_CreateThread(decode_Media, "decode", media); // 创建解码线程，读取packet到队列中缓存
	}
	else
	{
		LOG4CPLUS_DEBUG(logger, "打开媒体文件失败" << media->filename);
		//std::cout << "Open File error 3: " << media->filename << std::endl;
		return nullptr;
	}



	media->audio->audio_play(); // create audio thread

	media->video->video_play(media->display_x, media->display_y, media->display_width, media->display_heigh, window_width, window_height, media); // create video thread

	media->start_play_time = GetCurrentDateTime();

	//std::cout << "media start time: " << media->start_play_time << std::endl;
	media->timeBase = (int64_t(media->video->video_ctx->time_base.num) * AV_TIME_BASE) / int64_t(media->video->video_ctx->time_base.den);

	/*AVStream *audio_stream = media->pFormatCtx->streams[media->audio->stream_index];
	AVStream *video_stream = media->pFormatCtx->streams[media->video->stream_index];

	double audio_duration = audio_stream->duration * av_q2d(audio_stream->time_base);
	double video_duration = video_stream->duration * av_q2d(video_stream->time_base);

	std::cout << "文件名：" << media->filename << "audio时长：" << audio_duration << std::endl;
	std::cout << "文件名：" << media->filename << "video时长：" << video_duration << std::endl;*/

	return media;
}

void ScreenState::Play(ScheduleInfo* scheduleInfo)
{
	log4cplus::Logger logger = GetSubLogger(L"ScreenState");
	while (!is_initialized)
	{
		SDL_Delay(400);
	}

	if (is_playing)
	{
		ResetScreen();
	}
	schedule_info = scheduleInfo;
	auto playInfoList = schedule_info->play_info_list();
	schedule_media_count = playInfoList.size();
	schedule_loop_count = schedule_info->loop_count();

	LOG4CPLUS_DEBUG(logger, "(播放媒体列表数：" << schedule_media_count<<") (播放循环数："<< schedule_loop_count);
	//for (auto item : playInfoList)
	//{
	//	MediaState* newMedia = new MediaState(item->file_path(), item->file_md5(), item->play_count(), item->display_x(), item->display_y(), item->display_width(), item->display_heigh(), this->window_, this->renderer_);
	//	media_list.push(newMedia);
	//}

	is_playing = true;


	current_media = PlayNext(width_, height_, *this);

	//OnCompletedPlay("123456789");

	if (current_media == nullptr)
	{
		delete current_media;
		return;
	}


}

void ScreenState::Initialize()
{
	InitLog4cplus(L"main.log");
	log4cplus::Logger logger = GetSubLogger(L"ScreenState");
	//long long start = Utils::milliseconds_now();

	av_register_all();

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

	LOG4CPLUS_DEBUG(logger, "SDL环境初始化完成！");

	window_ = SDL_CreateWindow("", x_, y_, width_, height_, SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);

	renderer_ = SDL_CreateRenderer(window_, -1, 0);
	default_texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STATIC, width_, height_);

	//media_list = std::vector<MediaState*>();

	SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
	SDL_RenderClear(renderer_);
	SDL_RenderPresent(renderer_);

	SDL_RaiseWindow(window_);
	//top_show_thread = SDL_CreateThread(top_show, "top_show", window_);

	//long long elapsed = Utils::milliseconds_now() - start;

	//std::cout << "Init time: " << elapsed << std::endl;
	is_initialized = true;

	LOG4CPLUS_DEBUG(logger, "播放窗口初始化完成！");

	SDL_Event event;
	while (true)
	{
		SDL_WaitEvent(&event);
		switch (event.type)
		{
		case FF_QUIT_EVENT:
		case SDL_QUIT:
			quit = 1;

			SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
			SDL_RenderClear(renderer_);
			SDL_RenderPresent(renderer_);
			LOG4CPLUS_DEBUG(logger, "SDL_QUIT");
			//SDL_Quit();
			break;

		case FF_REFRESH_EVENT:
			video_refresh_timer(current_media);
			break;

		case FF_LOOPPLAY_EVENT:
			if (av_seek_frame(current_media->pFormatCtx, -1, current_media->timeBase, AVSEEK_FLAG_ANY) < 0)
			{
				LOG4CPLUS_DEBUG(logger, "av_seek_frame失败！");
			}
			int threadReturnValue;
			SDL_WaitThread(decode_Media_thread, &threadReturnValue);
			decode_Media_thread = SDL_CreateThread(decode_Media, "decode_loop", current_media);
			break;
		case MEDIA_COMPLETED_EVENT:
		{
			MediaState *temp = nullptr;
			MediaState* in_mediaState = nullptr;


			quit = true;
			count++;

			current_media->end_play_time = GetCurrentDateTime();
			//std::cout << "media end time: " << current_media->end_play_time << std::endl;
			////completedInfos = 
			GetCompletedPlayInfo(*current_media);

			LOG4CPLUS_DEBUG(logger, "媒体" << current_media->filename << "播放完成");
			//OnCompletedPlay(completedInfos);

			if (schedule_loop_count > 1)
			{
				in_mediaState = new MediaState(current_media->filename, current_media->md5, current_media->loopCount, current_media->display_x, current_media->display_y, current_media->display_width, current_media->display_heigh, this->window_, this->renderer_);
				media_list.push(in_mediaState);
			}

			delete current_media;
			current_media = nullptr;
			if (schedule_media_count == count)
			{
				count = 0;
				schedule_loop_count--;
			}

			if (schedule_loop_count > 0)
			{
				if ((temp = PlayNext(width_, height_, *this)) != nullptr) {
					current_media = temp;
				}
				else {
					LOG4CPLUS_DEBUG(logger, "");
					delete temp;
				}
			}
			else {
				/*SDL_Quit();
				return;*/
				LOG4CPLUS_DEBUG(logger, "无媒体播放，黑屏等待！");
				SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
				SDL_RenderClear(renderer_);
				SDL_RenderPresent(renderer_);
				/*SDL_Quit();*/
			}
			break;
		}
		default:
			break;
		}
	}

}



void ScreenState::GetCompletedPlayInfo(MediaState &media)
{
	Document d;
	Pointer("/MediaName").Set(d, media.filename);
	Pointer("/MediaMD5").Set(d, media.md5);
	Pointer("/StartTime").Set(d, media.start_play_time);
	Pointer("/EndTime").Set(d, media.end_play_time);

	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	d.Accept(writer);

	OnCompletedPlay(buffer.GetString());
}

void ScreenState::ResetScreen() {
	quit = true;
	count = 0;
	SDL_Delay(40);
	schedule_info->play_info_list().swap(std::vector<PlayInfo *>());
	/*while (!media_list.empty())
	{
		auto media_item = media_list.front();
		media_list.pop();
		delete media_item;
	}*/
	if (current_media != nullptr)
		delete current_media;
	quit = false;
}



void ScreenState::Stop()
{
	log4cplus::Logger logger = GetSubLogger(L"ScreenState");
	LOG4CPLUS_DEBUG(logger, "媒体停止播放");
	quit = true;
	SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
	SDL_RenderClear(renderer_);
	SDL_RenderPresent(renderer_);
}

void ScreenState::UpdateScreenSize(int x, int y, int width, int height)
{
	log4cplus::Logger logger = GetSubLogger(L"ScreenState");
	LOG4CPLUS_DEBUG(logger, "更新显示窗口大小位置" << "(" << x << "," << y << "," << width << "," << height << ")");
	this->x_ = x;
	this->y_ = y;
	this->width_ = width;// width <= 0 ? 400 : width;
	this->height_ = height;//height <= 0 ? 400 : height;

	SDL_SetWindowPosition(window_, x, y);
	SDL_SetWindowSize(window_, width, height);

	//SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
	//SDL_RenderClear(renderer_);
	SDL_RenderPresent(renderer_);

}

ScreenState::~ScreenState()
{
	/*if (schedule_info)
		delete schedule_info;

	if (current_media)
		delete current_media;

	while (!media_list.empty())
	{
		auto media = media_list.front();
		media_list.pop();
		delete media;
	}

	SDL_Quit();
	if (window_)
		SDL_DestroyWindow(window_);
	if (renderer_)
		SDL_DestroyRenderer(renderer_);
	if (default_texture_)
		SDL_DestroyTexture(default_texture_);*/
}

PlayInfoWrapper::PlayInfoWrapper(System::String^ filePath, System::String^ md5, int playCount, int x, int y, int width, int heigh)
{
	this->file_path_ = filePath;
	this->file_md5_ = md5;
	this->play_count_ = playCount;
	this->display_x_ = x;
	this->display_y_ = y;
	this->display_width_ = width;
	this->display_heigh_ = width;
}



ScreenPlayer::ScreenPlayer(int x, int y, int width, int height)
{
	this->_x = x;
	this->_y = y;
	this->_width = width <= 0 ? 400 : width;
	this->_height = height <= 0 ? 400 : height;
	this->screenState = new ScreenState(x, y, width, height);

	//OnCompletedPlayHandler = gcnew OnCompletedPlayDelegate(this, &ScreenPlayer::OnCompletedPlay);

}

ScreenPlayer::~ScreenPlayer()
{
	delete screenState;
}


void ScreenPlayer::Play(ScheduleInfoWrapper^ scheduleInfo)
{
	auto playInfos = scheduleInfo->get_playInfos();
	std::vector<PlayInfo*> *result = new std::vector<PlayInfo*>();
	for (size_t i = 0; i < playInfos->Count; i++)
	{
		const char* name_chars = (const char*)(Marshal::StringToHGlobalAnsi(playInfos[i]->get_filePath())).ToPointer();
		const char* md5_chars = (const char*)(Marshal::StringToHGlobalAnsi(playInfos[i]->get_fileMD5())).ToPointer();
		//ANSIToUTF8(chars, filePath);
		//const std::string file_name(chars);
		PlayInfo *region = new PlayInfo(name_chars, md5_chars, playInfos[i]->get_playCount(), playInfos[i]->get_display_x(), playInfos[i]->get_display_y(), playInfos[i]->get_display_width(), playInfos[i]->get_display_heigh());
		result->push_back(region);
	}
	ScheduleInfo* info = new ScheduleInfo(*result, scheduleInfo->get_loopCount());
	long long start = Utils::milliseconds_now();
	this->screenState->Play(info);
	long long elapsed = Utils::milliseconds_now() - start;
	delete result;
	//std::cout << "Play time: " << elapsed << std::endl;
}

void ScreenPlayer::Stop()
{
	this->screenState->Stop();
}

void ScreenPlayer::UpdateScreenSize(int x, int y, int width, int height)
{
	this->screenState->UpdateScreenSize(x, y, width, height);
}

void ScreenPlayer::Initialize()
{
	IntPtr stub_ptr = Marshal::GetFunctionPointerForDelegate(OnCompletedPlayHandler);
	callback = static_cast<CompletedPlayCallback>(stub_ptr.ToPointer());
	GC::KeepAlive(stub_ptr);

	this->screenState->SetOnCompletedPlayCallback(callback);

	this->screenState->Initialize();
}

PlayInfo::~PlayInfo()
{
	if (file_path_)
		delete[] file_path_;
}

ScheduleInfo::~ScheduleInfo()
{
	while (!play_info_list_.empty())
	{
		auto info = play_info_list_.front();
		play_info_list_.pop_back();
		delete info;
	}
}
