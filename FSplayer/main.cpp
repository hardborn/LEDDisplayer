#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <processthreadsapi.h>
#include <iostream>
//#include <vld.h>


extern "C" {

#include <libavcodec\avcodec.h>
#include <libavformat\avformat.h>
#include <libswscale\swscale.h>
#include <libswresample\swresample.h>

}

#include <log4cplus/fileappender.h>  
#include <log4cplus/layout.h>  
#include <log4cplus/ndc.h>  
#include <log4cplus/helpers/loglog.h>  
#include <log4cplus/loggingmacros.h>  

#include <SDL.h>
#include <SDL_thread.h>

#include "PacketQueue.h"
#include "Audio.h"
#include "Media.h"
#include "VideoDisplay.h"
#include "ScreenState.h"
#include "VLog.h"

using namespace std;
using namespace log4cplus;


DWORD WINAPI initialize(LPVOID s)
{
	ScreenState* screen = (ScreenState*)s;
	screen->Initialize();
	return 0;
}

int main(int argv, char* argc[])
{

	/*log4cplus::initialize();
	helpers::LogLog::getLogLog()->setInternalDebugging(true);
	SharedAppenderPtr append_1(new RollingFileAppender(LOG4CPLUS_TEXT("Test.log"), 5 * 1024, 5));
	append_1->setName(LOG4CPLUS_TEXT("First"));
	append_1->setLayout(std::auto_ptr<Layout>(new TTCCLayout()));
	Logger::getRoot().addAppender(append_1);

	Logger root = Logger::getRoot();
	Logger test = Logger::getInstance(LOG4CPLUS_TEXT("test"));
	Logger subTest = Logger::getInstance(LOG4CPLUS_TEXT("test.subtest"));

	for (int i = 0; i<20000; ++i) {
		NDCContextCreator _context(LOG4CPLUS_TEXT("loop"));
		LOG4CPLUS_DEBUG(subTest, "Entering loop #" << i);
		LOG4CPLUS_DEBUG(test, "Entering loop #" << i);
	}


	return 0;*/
	InitLog4cplus(L"./main.log");
	log4cplus::Logger test1 = GetSubLogger(L"test1");
	ScreenState *screen = new ScreenState(0, 0, 800, 600);
	LOG4CPLUS_DEBUG(test1, "完成屏体初始化！");
	HANDLE loop_thread = CreateThread(NULL, 0, initialize, (LPVOID)screen, 0, NULL);
	
	std::vector<PlayInfo*> play_info_list;
	PlayInfo* play_info1 = new PlayInfo("F:\\Temp\\广告片\\15秒预告【IMAX3D Ice Age Collision Course 】.mp4","", 1, 0, 0, 800, 600);
	LOG4CPLUS_DEBUG(test1, "添加媒体：" << play_info1->file_path());
	PlayInfo* play_info2 = new PlayInfo("F:\\Temp\\广告片\\15秒预告【IMAX3D Ice Age Collision Course 】.mp4","", 1, 0, 0, 800, 600);
	LOG4CPLUS_DEBUG(test1, "添加媒体：" << play_info2->file_path());
	PlayInfo* play_info3 = new PlayInfo("F:\\Temp\\广告片\\15秒预告【IMAX3D Jason Bourne】.mp4","", 1, 0, 0, 800, 600);
	LOG4CPLUS_DEBUG(test1, "添加媒体：" << play_info3->file_path());
	PlayInfo* play_info4 = new PlayInfo("F:\\Temp\\广告片\\15秒预告【IMAX3D 盗墓笔记】.mp4","", 1, 0, 0, 800, 600);
	LOG4CPLUS_DEBUG(test1, "添加媒体：" << play_info4->file_path());
	play_info_list.push_back(play_info1);
	play_info_list.push_back(play_info2);
	play_info_list.push_back(play_info3);
	play_info_list.push_back(play_info4);
	Sleep(5000);
	ScheduleInfo* schedule_info = new ScheduleInfo(play_info_list, 1);
	LOG4CPLUS_DEBUG(test1, "开始播放.");
	screen->Play(schedule_info);
	WaitForSingleObject(loop_thread, INFINITE);

	delete screen;
	delete play_info1;
	delete play_info2;
	delete play_info3;
	delete play_info4;
	//delete schedule_info;
	ShutdownLogger();
	return 0;
}
