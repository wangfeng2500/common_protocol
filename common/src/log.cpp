/*
 * log.cpp
 *
 *  Created on: 2014-10-4
 *      Author: fenngwang
 */


#include "log.h"
#include "singleton.h"
#include <math.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdarg.h>

namespace CGI_LOG
{

#define LOG_CONFIG_PATH "./log/"

const char * CLogFile::priority_name(int errCode)
{
	static const string arrPriority[] = {
			"LM_SHUTDOWN",
			"LM_TRACE",
			"LM_DEBUG",
			"LM_INFO",
			"LM_NOTICE",
			"LM_WARNING",
			"LM_STARTUP",
			"LM_ERROR",
			"LM_CRITICAL",
			"LM_ALERT",
			"LM_EMERGENCY"
			"LM_UNKNOW"
	};
	float err = log((double)errCode)/log(2.0);
	if (err > 10 || errCode < 0)
		return arrPriority[11].c_str();
	return arrPriority[(int)err].c_str();
}

int CLogFile::WriteLog(const char *sFile, const int iLine, const char *sFunc, int i32Level, const char * pszFormat, va_list ap)
{
	FILE * m_pLogFile;

	if (m_sLogFilePath.empty()) return -2; //not initialize
	m_pLogFile = fopen(m_sLogFilePath.c_str(), "a+");
	if (m_pLogFile == NULL)
	{
		return -1;
	}

	struct tm _tm;
	time_t nowtime = time(NULL);
	localtime_r(&nowtime, &_tm);

	char _pstr[20];
	snprintf(_pstr, sizeof(_pstr), "%04d-%02d-%02d %02d:%02d:%02d",
					_tm.tm_year+1900, _tm.tm_mon+1, _tm.tm_mday,
					_tm.tm_hour, _tm.tm_min, _tm.tm_sec);
	_pstr[20] = '\0';

	fprintf(m_pLogFile, "[%d,%d--%s:%d:%s] <%s> [%s]", getppid(), getpid(), sFile, iLine, sFunc, priority_name(i32Level), _pstr);
	vfprintf(m_pLogFile, pszFormat, ap);
	fprintf(m_pLogFile, "\n");
	fclose(m_pLogFile);

	ShiftFiles(10000000, 5);
	return 0;
}

int CLogFile::ShiftFiles(long lMaxLogSize, int iMaxLogNum)
{
	struct stat stStat;
	char sOldLogFileName[300];
	char sNewLogFileName[300];
	int i;

	if (stat(m_sLogFilePath.c_str(), &stStat) < 0)
	{
		return -1;
	}

	if (stStat.st_size < lMaxLogSize)
	{
		return 0;
	}

	snprintf(sNewLogFileName, sizeof(sNewLogFileName) - 1, "%s.%d", m_sLogFilePath.c_str(), iMaxLogNum-1);
	if (access(sNewLogFileName, F_OK) == 0)
	{
		if (remove(sNewLogFileName) < 0 )
		{
			return -1;
		}
	}
	for (i = iMaxLogNum-2; i >= 0; i--)
	{
		if (i != 0)
			snprintf(sOldLogFileName,sizeof(sOldLogFileName)-1, "%s.%d", m_sLogFilePath.c_str(), i);
		else {
			snprintf(sOldLogFileName, sizeof(sOldLogFileName)-1,"%s", m_sLogFilePath.c_str());
		}

		if (access(sOldLogFileName, F_OK) == 0)
		{
			snprintf(sNewLogFileName, sizeof(sNewLogFileName)-1, "%s.%d", m_sLogFilePath.c_str(), i+1);
			if (rename(sOldLogFileName, sNewLogFileName) < 0 )
			{
				return -1;
			}
		}
	}
	return 0;
}

void CGI_Log_Init(const string & strLogName)
{
	CLogFile * pLog = CSingleton<CLogFile>::instance();

	string path = LOG_CONFIG_PATH;
	if (path.empty())
	{
		return;
	}

	string filename;
	size_t pos = strLogName.find_last_of('/');
	if (pos == string::npos)
	{
		filename = strLogName;
	}
	else
	{
		filename = strLogName.substr(pos + 1);
	}
	pLog->SetLogPath(path + "/" + filename);
}

void CGI_Error_Log_Orig(const char *sFile, const int iLine, const char *sFunc, int i32Level, const char* pszFormat, ...)
{
//	static int LOG_LEVEL = CSingleton<CGILogConfig>::instance()->GetLogLevel();
//	if (i32Level <= LOG_LEVEL) return;

	va_list ap;
	CLogFile * pLog= CSingleton<CLogFile>::instance();

	va_start (ap, pszFormat);
	pLog->WriteLog (sFile, iLine, sFunc, i32Level, pszFormat, ap);
	va_end (ap);
}

}
