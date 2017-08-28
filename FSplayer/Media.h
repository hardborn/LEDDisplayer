
#ifndef MEDIA_H
#define MEDIA_H

#include <string>
#include <time.h>
#include "Audio.h"
#include "Video.h"

extern "C" {

#include <libavformat\avformat.h>

}

#define FF_REFRESH_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT (SDL_USEREVENT + 1)
#define FF_LOOPPLAY_EVENT (SDL_USEREVENT + 2)
#define MEDIA_COMPLETED_EVENT (SDL_USEREVENT + 3)

class VideoState;

class MediaState
{
public:
	AudioState *audio;
	VideoState *video;
	AVFormatContext *pFormatCtx;

	char* filename;
	char* md5;
	int loopCount;
	int timeBase;
	int display_x;
	int display_y;
	int display_width;
	int display_heigh;

	time_t start_play_time;
	time_t end_play_time;
	//bool quit;

	MediaState();
	MediaState(const char* filename);
	MediaState(const char* filename, const char* md5, int count,int x,int y, int width,int heigh, SDL_Window *window, SDL_Renderer *renderer);
	~MediaState();
	bool openInput();
};

int decode_Media(void *data);

#endif