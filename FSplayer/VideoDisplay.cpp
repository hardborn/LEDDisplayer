
#include "VideoDisplay.h"
#include <iostream>

extern "C" {

#include <libswscale\swscale.h>
#include <libavutil\time.h>

}
#include <log4cplus/fileappender.h>  
#include <log4cplus/layout.h>  
#include <log4cplus/ndc.h>  
#include <log4cplus/helpers/loglog.h>  
#include <log4cplus/loggingmacros.h>  

#include "VLog.h"

extern bool quit;

static const double SYNC_THRESHOLD = 0.01;
static const double NOSYNC_THRESHOLD = 10.0;

// �ӳ�delay ms��ˢ��video֡
void schedule_refresh(MediaState *media, int delay)
{
	SDL_AddTimer(delay, sdl_refresh_timer_cb, media);
}

void schedule_loop(MediaState *media, int delay)
{
	SDL_AddTimer(delay, sdl_loop_timer_cb, media);
}

void schedule_completed(MediaState *media, int delay)
{
	SDL_AddTimer(delay, sdl_completed_timer_cb, media);
}
uint32_t sdl_refresh_timer_cb(uint32_t interval, void *opaque)
{
	SDL_Event event;
	event.type = FF_REFRESH_EVENT;
	event.user.data1 = opaque;
	SDL_PushEvent(&event);
	return 0; /* 0 means stop timer */
}

uint32_t sdl_loop_timer_cb(uint32_t interval, void *opaque)
{
	SDL_Event event;
	event.type = FF_LOOPPLAY_EVENT;
	event.user.data1 = opaque;
	SDL_PushEvent(&event);
	return 0; /* 0 means stop timer */
}

uint32_t sdl_completed_timer_cb(uint32_t interval, void *opaque)
{
	SDL_Event event;
	event.type = MEDIA_COMPLETED_EVENT;
	event.user.data1 = opaque;
	SDL_PushEvent(&event);
	return 0;
}
void video_refresh_timer(void *userdata)
{
	log4cplus::Logger logger = GetSubLogger(L"Video");
	MediaState *media = (MediaState*)userdata;
	if (media == nullptr)
	{
		LOG4CPLUS_DEBUG(logger, "ý�������δ������ɣ�");
		return;
	}
	VideoState *video = media->video;
	if (quit)
	{
		LOG4CPLUS_DEBUG(logger, "�˳���ʱ��Ƶˢ��");
		return;
	}
	if (video->stream_index >= 0)
	{
		if (video->videoq->queue.empty() && video->frameq->queue.empty())
		{
			//schedule_refresh(media, 1);
			if (media->loopCount > 0)
			{
				schedule_loop(media, 1);
			}
			else
			{
				schedule_completed(media, 1);
				return;
			}
			schedule_refresh(media, 40);
		}
		else
		{
			//av_frame_unref(video->frame);
			video->frameq->deQueue(video->frame);

			// ����Ƶͬ������Ƶ�ϣ�������һ֡���ӳ�ʱ��
			double current_pts = *(double*)video->frame->opaque;
			double delay = current_pts - video->frame_last_pts;
			if (delay <= 0 || delay >= 1.0)
				delay = video->frame_last_delay;

			video->frame_last_delay = delay;
			video->frame_last_pts = current_pts;

			// ��ǰ��ʾ֡��PTS��������ʾ��һ֡���ӳ�
			if (media->audio->audio_ctx != nullptr)
			{
				double ref_clock = media->audio->get_audio_clock();

				double diff = current_pts - ref_clock;// diff < 0 => video slow,diff > 0 => video quick

				double threshold = (delay > SYNC_THRESHOLD) ? delay : SYNC_THRESHOLD;

				if (fabs(diff) < NOSYNC_THRESHOLD) // ��ͬ��
				{
					if (diff <= -threshold) // ���ˣ�delay��Ϊ0
						delay = 0;
					else if (diff >= threshold) // ���ˣ��ӱ�delay
						delay *= 2;
				}
			}
			video->frame_timer += delay;
			double actual_delay = video->frame_timer - static_cast<double>(av_gettime()) / 1000000.0;
			if (actual_delay <= 0.010)
				actual_delay = 0.010;

			schedule_refresh(media, static_cast<int>(actual_delay * 1000 + 0.5));

			SwsContext *sws_ctx = sws_getContext(video->video_ctx->width, video->video_ctx->height, video->video_ctx->pix_fmt,
				video->displayFrame->width, video->displayFrame->height, (AVPixelFormat)video->displayFrame->format, SWS_BILINEAR, nullptr, nullptr, nullptr);

			sws_scale(sws_ctx, (uint8_t const * const *)video->frame->data, video->frame->linesize, 0,
				video->video_ctx->height, video->displayFrame->data, video->displayFrame->linesize);

			// Display the image to screen
			//std::cout << video->name << std::endl;
			SDL_UpdateTexture(video->bmp, &(video->rect), video->displayFrame->data[0], video->displayFrame->linesize[0]);
			SDL_RenderClear(video->renderer);
			SDL_RenderCopy(video->renderer, video->bmp, &video->rect, &video->rect);
			SDL_RenderPresent(video->renderer);

			sws_freeContext(sws_ctx);
			av_frame_unref(video->frame);
		}
	}
	else
	{
		schedule_refresh(media, 100);
	}
}