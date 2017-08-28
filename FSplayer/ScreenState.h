#pragma once

#include "Media.h"

#include <queue>

#include <log4cplus/fileappender.h>  
#include <log4cplus/layout.h>  
#include <log4cplus/ndc.h>  
#include <log4cplus/helpers/loglog.h>  
#include <log4cplus/loggingmacros.h>  

#include "VLog.h"

class PlayInfo
{
public:
	PlayInfo(const char* filePath, const char* md5, int playCount, int x, int y, int width, int heigh)
		:play_count_(playCount), display_x_(x), display_y_(y), display_width_(width), display_heigh_(heigh)
	{
		if (filePath)
		{
			file_path_ = new char[strlen(filePath) + 1];
			strcpy(file_path_, filePath);
		}
		if (md5)
		{
			file_md5_ = new char[strlen(md5) + 1];
			strcpy(file_md5_, md5);
		}
	};
	~PlayInfo();
	char* file_path() const { return file_path_; }
	char* file_md5() const { return file_md5_; }
	int play_count() const { return play_count_; }
	int display_x() const { return display_x_; }
	int display_y() const { return display_y_; }
	int display_width() const { return display_width_; }
	int display_heigh() const { return display_heigh_; }
private:
	char* file_path_;
	char* file_md5_;
	int play_count_;
	int display_x_;
	int display_y_;
	int display_width_;
	int display_heigh_;
};

class ScheduleInfo
{
public:
	ScheduleInfo() {};
	ScheduleInfo(std::vector<PlayInfo*>& playInfoList, int loopCount)
		:play_info_list_(playInfoList), loop_count_(loopCount)
	{};
	~ScheduleInfo();
	std::vector<PlayInfo*> &play_info_list() { return play_info_list_; }
	int loop_count() const { return loop_count_; }
private:
	std::vector<PlayInfo*> play_info_list_;
	int loop_count_;
};

typedef void(__stdcall *CompletedPlayCallback)(const char*);

class ScreenState
{
public:
	ScreenState()
		:x_(0), y_(0), width_(200), height_(200)
	{
		//InitLog4cplus(L"main.log");
		current_media = nullptr;
	};
	ScreenState(int x, int y, int width, int height)
		:x_(x), y_(y), width_(width), height_(height)
	{
		//InitLog4cplus(L"main.log");
		current_media = nullptr;
	};
	~ScreenState();
	void Initialize();
	void Play(ScheduleInfo* playInfoList);
	void Stop();
	void UpdateScreenSize(int x, int y, int width, int height);
	void ResetScreen();
	void OnCompletedPlay(const char* filename) {
		if (_completed_play_callback != nullptr) {
			char* file_path = new char[strlen(filename) + 1];
			strcpy(file_path, filename);
			_completed_play_callback(file_path);
		}
	};
	void GetCompletedPlayInfo(MediaState &media);
	void SetOnCompletedPlayCallback(CompletedPlayCallback callback)
	{
		_completed_play_callback = callback;
	};

	SDL_Window *get_window() { return window_; }
	SDL_Renderer *get_renderer() { return renderer_; }
	std::vector<PlayInfo *> &get_play_info_list() { return schedule_info->play_info_list(); }

private:
	int x_;
	int y_;
	int width_;
	int height_;
	int display_region_x_;
	int display_region_y_;
	int display_region_width_;
	int display_region_height_;

	bool is_playing = false;

	SDL_Window *window_;
	SDL_Renderer *renderer_;
	SDL_Texture *default_texture_;
	std::queue<MediaState*> media_list;
	MediaState *current_media;
	ScheduleInfo *schedule_info;
	CompletedPlayCallback _completed_play_callback = nullptr;
};

public ref class PlayInfoWrapper
{
public:
	PlayInfoWrapper(System::String^ filePath, System::String^ md5, int playCount, int x, int y, int width, int heigh);
	System::String^ get_filePath() { return file_path_; }
	System::String^ get_fileMD5() { return file_md5_; }
	int get_playCount() { return play_count_; }
	int get_display_x() { return display_x_; }
	int get_display_y() { return display_y_; }
	int get_display_width() { return display_width_; }
	int get_display_heigh() { return display_heigh_; }
private:
	System::String^ file_path_;
	System::String^ file_md5_;
	int play_count_;
	int display_x_;
	int display_y_;
	int display_width_;
	int display_heigh_;
};

public ref class ScheduleInfoWrapper
{
public:
	ScheduleInfoWrapper(System::Collections::Generic::List<PlayInfoWrapper^> ^playInfos, int loopCount)
		:_playInfos(playInfos), _loopCount(loopCount)
	{};
	System::Collections::Generic::List<PlayInfoWrapper^> ^get_playInfos() { return _playInfos; }
	int get_loopCount() { return _loopCount; }
private:
	System::Collections::Generic::List<PlayInfoWrapper^> ^_playInfos;
	int _loopCount;
};

public delegate void OnCompletedPlayDelegate(System::String^);

public ref class ScreenPlayer
{
public:
	OnCompletedPlayDelegate^ OnCompletedPlayHandler;

	ScreenPlayer(int x, int y, int width, int height);
	~ScreenPlayer();
	void Initialize();
	void Play(ScheduleInfoWrapper^ scheduleInfo);
	void Stop();
	void UpdateScreenSize(int x, int y, int width, int height);
private:
	int _x;
	int _y;
	int _width;
	int _height;
	ScreenState *screenState;
	CompletedPlayCallback callback;
};

