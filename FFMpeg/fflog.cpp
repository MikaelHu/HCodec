#include "stdafx.h"
#include "fflog.h"


void __ff_log_print(int level, const char *tag, const std::string &str) {
#ifdef ANDROID
	int androidLevel = ANDROID_LOG_DEBUG;
	switch (level) {
	case FF_LOG_DEBUG:
		androidLevel = ANDROID_LOG_DEBUG;
		break;
	case FF_LOG_INFO:
		androidLevel = ANDROID_LOG_INFO;
		break;
	case FF_LOG_WARN:
		androidLevel = ANDROID_LOG_WARN;
		break;
	case FF_LOG_ERROR:
		androidLevel = ANDROID_LOG_ERROR;
		break;
	default:
		return;
	}
	__android_log_write(androidLevel, tag, str.c_str());
#else
	std::string szHead = "";
	switch (level) {
	case FF_LOG_DEBUG:
		szHead = "DEBUG";
		break;
	case FF_LOG_INFO:
		szHead = "INFO";
		break;
	case FF_LOG_WARN:
		szHead = "WARN";
		break;
	case FF_LOG_ERROR:
		szHead = "ERROR";
		break;
	default:
		return;
	}
	fprintf(stdout, "[%s][%s] %s\n", tag, szHead.c_str(), str.c_str());
#endif
}