#pragma once  
#ifndef VLOG_H  
#define VLOG_H  

#include <log4cplus/logger.h>  
#include <log4cplus/layout.h>  
#include <log4cplus/loglevel.h>  
#include <log4cplus/fileappender.h>  
#include <log4cplus/consoleappender.h>  

#include <log4cplus/helpers/loglog.h>  

using namespace log4cplus;
using namespace helpers;

/**
* 使用log4cplus
* @param logfile 记录的文件路径，如“main.log”
* @param format 格式化前缀，默认%d [%l] %-5p : %m %n
* @param bebug 是否打印bebug信息，默认true
* @param lv 设置日记级别
* @param lv 立刻写入模式 
* @return
*/
void InitLog4cplus(
	const wchar_t* logfile,
	const bool console = true,
	const bool bebug = true,
	LogLevel lv = DEBUG_LOG_LEVEL,
	const wchar_t* format = L"%-d{%y-%m-%d %H:%M:%S} [%5p][%t][%c{1}]-[%M] %m%n",// "[%-5p %d{%y-%m-%d %H:%M:%S}] [%l]%n%m%n%n", /*%d %-5p [%c < %l] : %m %n*/
	const bool immediateFlush = true
);

/**
* 获得根日志
* @return
*/
Logger GetRootLogger(void);

/**
* 获得子日志
* @param child 子日志名，如，sub；sub.sub1
* @return
*/
Logger GetSubLogger(const wchar_t* sub);

/**
* 关闭日志系统
*/
void ShutdownLogger(void);

#endif  /* VLOG_H */  

