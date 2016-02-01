/*
 * cgilog.h
 *
 *  Created on: 2014-10-4
 *      Author: fenngwang
 */

#ifndef LOG_H_
#define LOG_H_

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

namespace CGI_LOG
{

using namespace std;

enum CGI_LOG_PRIORITY
{
	LM_SHUTDOWN = 01,
	LM_TRACE = 02,
	LM_DEBUG = 04,
	LM_INFO = 010,
	LM_NOTICE = 020,
	LM_WARNING = 040,
	LM_STARTUP = 0100,
	LM_ERROR = 0200,
	LM_CRITICAL = 0400,
	LM_ALERT = 01000,
	LM_EMERGENCY = 02000,
	LM_MAX = LM_EMERGENCY,
	LM_ENSURE_32_BITS = 0x7FFFFFFF
};

class CLogFile {
public:
	CLogFile() {};

	int SetLogPath (const string & sLogFilePath)
	{
		m_sLogFilePath = sLogFilePath + ".log";
		return 0;
	};

	int ShiftFiles(long lMaxLogSize, int iMaxLogNum);

	int WriteLog (const char *sFile, const int iLine, const char *sFunc, int i32Level, const char*pszFormat, va_list ap);

	const char * priority_name(int errCode);

private:
	string m_sLogFilePath;
};


/**
@desc    初始化日志
*/
void CGI_Log_Init(const string & sLogFilePath);

void CGI_Error_Log_Orig (const char *sFile, const int iLine, const char *sFunc, int i32Level, const char * pszFormat, ...) __attribute((format(printf, 5, 6)));
#define API_LOG_DEBUG(level, args...) CGI_Error_Log_Orig(__FILE__, __LINE__, __FUNCTION__, level, args)


}

#endif /* LOG_H_ */
