
#ifndef VIDEO_DISPLAY_H
#define VIDEO_DISPLAY_H

#include "Video.h"





// ÑÓ³Ùdelay msºóË¢ÐÂvideoÖ¡
void schedule_refresh(MediaState *media, int delay);

uint32_t sdl_refresh_timer_cb(uint32_t interval, void *opaque);

uint32_t sdl_loop_timer_cb(uint32_t interval, void *opaque);

uint32_t sdl_completed_timer_cb(uint32_t interval, void *opaque);

void schedule_completed(void *userdata);

void video_refresh_timer(void *userdata);


//void video_display(VideoState *video);


#endif