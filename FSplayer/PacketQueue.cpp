
#include "PacketQueue.h"
#include <iostream>


#include <log4cplus/fileappender.h>  
#include <log4cplus/layout.h>  
#include <log4cplus/ndc.h>  
#include <log4cplus/helpers/loglog.h>  
#include <log4cplus/loggingmacros.h>  

#include "VLog.h"


extern bool quit;

PacketQueue::PacketQueue()
{
	nb_packets = 0;
	size = 0;
	duration = 0;

	mutex = SDL_CreateMutex();
	cond = SDL_CreateCond();
}

PacketQueue::~PacketQueue()
{
	while (!queue.empty())
	{
		auto item = queue.front();
		queue.pop();
		av_packet_free(&item);
	}
	std::queue<AVPacket*>().swap(queue);
	SDL_DestroyCond(cond);
	SDL_DestroyMutex(mutex);
}

bool PacketQueue::enQueue(const AVPacket *packet)
{
	SDL_LockMutex(mutex);
	
	AVPacket *pkt = av_packet_alloc();
	if (av_packet_ref(pkt, packet) < 0) {
		av_packet_free(&pkt);
		return false;
	}

	queue.push(pkt);

	size += pkt->size;
	nb_packets++;
	duration += pkt->duration;

	SDL_CondSignal(cond);
	SDL_UnlockMutex(mutex);
	return true;
}

bool PacketQueue::deQueue(AVPacket *packet, bool block)
{
	bool ret = false;

	SDL_LockMutex(mutex);
	while (true)
	{
		if (quit)
		{
			ret = false;
			break;
		}

		if (!queue.empty())
		{
			if (av_packet_ref(packet, queue.front()) < 0)
			{
				ret = false;
				break;
			}
			AVPacket* pkt = queue.front();

			queue.pop();
			av_packet_free(&pkt);
			nb_packets--;
			size -= packet->size;
			duration -= packet->duration;

			ret = true;
			break;
		}
		else if (!block)
		{
			ret = false;
			break;
		}
		else
		{
			try
			{
				SDL_CondWait(cond, mutex);
			}
			catch (const std::exception& ex)
			{
				log4cplus::Logger logger = GetSubLogger(L"PacketQueue");
				LOG4CPLUS_DEBUG(logger, "“Ï≥££∫" << ex.what());
			}
			//SDL_CondWaitTimeout(cond, mutex, 40);
			//SDL_CondWait(cond, mutex);
			//SDL_CondWaitTimeout(cond, mutex, 40);
		}
	}
	SDL_UnlockMutex(mutex);
	return ret;
}