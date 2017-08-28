/************************************************************************/
/* 使用规则：                                                             */
/* 需要log4cplus动态/静态链接库及头文件                                      */
/* log4cplus.lib + log4cplus.dll 为动态链接库。log4cplusS.lib为静态链接库    */
/************************************************************************/
#include "VLog.h"  

void InitLog4cplus(const wchar_t* logfile, const bool console, const bool bebug,
	LogLevel lv, const wchar_t* format, const bool immediateFlush)
	/*
	NOT_SET_LOG_LEVEL               (   -1) ：接受缺省的LogLevel，如果有父logger则继承它的
	LogLevelALL_LOG_LEVEL           (    0) ：开放所有log信息输出
	TRACE_LOG_LEVEL                 (    0) ：开放trace信息输出(即ALL_LOG_LEVEL)
	DEBUG_LOG_LEVEL                 (10000) ：开放debug信息输出
	INFO_LOG_LEVEL                  (20000) ：开放info信息输出
	WARN_LOG_LEVEL                  (30000) ：开放warning信息输出
	ERROR_LOG_LEVEL                 (40000) ：开放error信息输出
	FATAL_LOG_LEVEL                 (50000) ：开放fatal信息输出
	OFF_LOG_LEVEL                   (60000) ：关闭所有log信息输出
	*/ {
	// 0.日志系统配置：设置显示debug信息  
	LogLog::getLogLog()->setInternalDebugging(bebug);


	// 创建屏幕输出Appender，不采用stderror流，不采用立刻写入模式  
	SharedAppenderPtr pappender1(new RollingFileAppender(logfile, 1024 * 1024 * 5, 50, immediateFlush));
	// 生成的日志文件名称，文件最大值(最小1 * 200 M)，扩展50文件个数，不采用立刻写入模式  

	// 2.实例化一个layout对象  
	// 2.1创建layout布局格式  
	std::auto_ptr<Layout> playout1(new PatternLayout(format));

	// 3.将layout对象绑定(attach)到appender对象  
	// pappender.setLayout(std::auto_ptr<Layout> layout);  
	pappender1->setLayout(playout1);

	// 4.Logger ：记录器，保存并跟踪对象日志信息变更的实体，当你需要对一个对象进行记录时，就需要生成一个logger。  
	Logger rootLogger = Logger::getRoot();

	// 5.将appender对象绑定(attach)到logger对象，如省略此步骤，标准输出（屏幕）appender对象会绑定到logger  
	rootLogger.addAppender(pappender1);

	if (console) {
		// 1.Appenders ：挂接器，与布局器紧密配合，将特定格式的消息输出到所挂接的设备终端 （如屏幕，文件等等)。  
		SharedAppenderPtr pappender2(new ConsoleAppender(false, false));
		std::auto_ptr<Layout> playout2(new PatternLayout(format));
		pappender2->setLayout(playout2);
		rootLogger.addAppender(pappender2);
	}

	// 6.设置logger的优先级，如省略此步骤，各种有限级的消息都将被记录  
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
