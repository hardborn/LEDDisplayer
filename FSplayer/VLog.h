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
* ʹ��log4cplus
* @param logfile ��¼���ļ�·�����硰main.log��
* @param format ��ʽ��ǰ׺��Ĭ��%d [%l] %-5p : %m %n
* @param bebug �Ƿ��ӡbebug��Ϣ��Ĭ��true
* @param lv �����ռǼ���
* @param lv ����д��ģʽ 
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
* ��ø���־
* @return
*/
Logger GetRootLogger(void);

/**
* �������־
* @param child ����־�����磬sub��sub.sub1
* @return
*/
Logger GetSubLogger(const wchar_t* sub);

/**
* �ر���־ϵͳ
*/
void ShutdownLogger(void);

#endif  /* VLOG_H */  

