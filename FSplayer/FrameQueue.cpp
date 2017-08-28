#include "FrameQueue.h"


#include <log4cplus/fileappender.h>  
#include <log4cplus/layout.h>  
#include <log4cplus/ndc.h>  
#include <log4cplus/helpers/loglog.h>  
#include <log4cplus/loggingmacros.h>  

#include "VLog.h"

FrameQueue::FrameQueue()
{
	nb_frames = 0;

	mutex = SDL_CreateMutex();
	cond = SDL_CreateCond();
}

FrameQueue::~FrameQueue()
{
	while (!queue.empty())
	{
		auto item = queue.front();
		queue.pop();
		av_frame_free(&item);
	}
	std::queue<AVFrame*>().swap(queue);
	SDL_DestroyCond(cond);
	SDL_DestroyMutex(mutex);

}

bool FrameQueue::enQueue(const AVFrame* frame)
{
	SDL_LockMutex(mutex);

	AVFrame* p = av_frame_alloc();

	int ret = av_frame_ref(p, frame);
	if (ret < 0) {
		av_frame_free(&p);
		return false;
	}


	p->opaque = (void *)new double(*(double*)p->opaque); //上一个指向的是一个局部的变量，这里重新分配pts空间


	queue.push(p);

	nb_frames++;

	SDL_CondSignal(cond);
	SDL_UnlockMutex(mutex);

	return true;
}

bool FrameQueue::deQueue(AVFrame *frame)
{
	bool ret = true;

	SDL_LockMutex(mutex);
	while (true)
	{
		if (!queue.empty())
		{
			if (av_frame_ref(frame, queue.front()) < 0)
			{
				ret = false;
				break;
			}

			auto tmp = queue.front();
			queue.pop();

			av_frame_unref(tmp);
			av_frame_free(&tmp);

			nb_frames--;

			ret = true;
			break;
		}
		else
		{
			try
			{
				SDL_CondWaitTimeout(cond, mutex, 40);
			}
			catch (const std::exception& ex)
			{
				log4cplus::Logger logger = GetSubLogger(L"Audio");
				LOG4CPLUS_DEBUG(logger, "异常：" << ex.what());
			}
		}
	}

	SDL_UnlockMutex(mutex);
	return ret;
}