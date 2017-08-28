/************************************************************************/
/* ʹ�ù���                                                             */
/* ��Ҫlog4cplus��̬/��̬���ӿ⼰ͷ�ļ�                                      */
/* log4cplus.lib + log4cplus.dll Ϊ��̬���ӿ⡣log4cplusS.libΪ��̬���ӿ�    */
/************************************************************************/
#include "VLog.h"  

void InitLog4cplus(const wchar_t* logfile, const bool console, const bool bebug,
	LogLevel lv, const wchar_t* format, const bool immediateFlush)
	/*
	NOT_SET_LOG_LEVEL               (   -1) ������ȱʡ��LogLevel������и�logger��̳�����
	LogLevelALL_LOG_LEVEL           (    0) ����������log��Ϣ���
	TRACE_LOG_LEVEL                 (    0) ������trace��Ϣ���(��ALL_LOG_LEVEL)
	DEBUG_LOG_LEVEL                 (10000) ������debug��Ϣ���
	INFO_LOG_LEVEL                  (20000) ������info��Ϣ���
	WARN_LOG_LEVEL                  (30000) ������warning��Ϣ���
	ERROR_LOG_LEVEL                 (40000) ������error��Ϣ���
	FATAL_LOG_LEVEL                 (50000) ������fatal��Ϣ���
	OFF_LOG_LEVEL                   (60000) ���ر�����log��Ϣ���
	*/ {
	// 0.��־ϵͳ���ã�������ʾdebug��Ϣ  
	LogLog::getLogLog()->setInternalDebugging(bebug);


	// ������Ļ���Appender��������stderror��������������д��ģʽ  
	SharedAppenderPtr pappender1(new RollingFileAppender(logfile, 1024 * 1024 * 5, 50, immediateFlush));
	// ���ɵ���־�ļ����ƣ��ļ����ֵ(��С1 * 200 M)����չ50�ļ�����������������д��ģʽ  

	// 2.ʵ����һ��layout����  
	// 2.1����layout���ָ�ʽ  
	std::auto_ptr<Layout> playout1(new PatternLayout(format));

	// 3.��layout�����(attach)��appender����  
	// pappender.setLayout(std::auto_ptr<Layout> layout);  
	pappender1->setLayout(playout1);

	// 4.Logger ����¼�������沢���ٶ�����־��Ϣ�����ʵ�壬������Ҫ��һ��������м�¼ʱ������Ҫ����һ��logger��  
	Logger rootLogger = Logger::getRoot();

	// 5.��appender�����(attach)��logger������ʡ�Դ˲��裬��׼�������Ļ��appender�����󶨵�logger  
	rootLogger.addAppender(pappender1);

	if (console) {
		// 1.Appenders ���ҽ������벼����������ϣ����ض���ʽ����Ϣ��������ҽӵ��豸�ն� ������Ļ���ļ��ȵ�)��  
		SharedAppenderPtr pappender2(new ConsoleAppender(false, false));
		std::auto_ptr<Layout> playout2(new PatternLayout(format));
		pappender2->setLayout(playout2);
		rootLogger.addAppender(pappender2);
	}

	// 6.����logger�����ȼ�����ʡ�Դ˲��裬�������޼�����Ϣ��������¼  
	rootLogger.setLogLevel(lv);
}

Logger GetRootLogger(void) {
	return Logger::getRoot();
}

Logger GetSubLogger(const wchar_t* sub) {
	return Logger::getInstance(sub);
}

void ShutdownLogger(void) {
	Logger::getRoot().shutdown();
}
