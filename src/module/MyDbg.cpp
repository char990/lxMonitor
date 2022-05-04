#include <cstdarg>
#include <stdexcept>
#include <cstdio>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <module/MyDbg.h>
#include <module/Utils.h>
#include <module/BootTimer.h>

using namespace Utils;


std::string FmtException(const char * fmt, ...)
{
	char buf[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, 255, fmt, args);
	va_end(args);
	return std::string(buf);
}


#define MyDbgBuf_SIZE 1024
static char MyDbgBuf[MyDbgBuf_SIZE];

void Log(int);

int PrintDbg(DBG_LEVEL level, const char *fmt, ...)
{
	static DBG_LEVEL lastlvl;
	int len = 0;
	if (level >= DBG_HB)
	{
		if (lastlvl == DBG_HB)
		{
			putchar(level == DBG_HB ? '\r' : '\n');
		}
		lastlvl = level;
		struct timeval t;
		gettimeofday(&t, nullptr);
		MyDbgBuf[0] = '[';
		char *p = Time::ParseTimeToLocalStr(&t, MyDbgBuf + 1);
		*p++ = ']';
		len = p - MyDbgBuf;
		va_list args;
		va_start(args, fmt);
		len += vsnprintf(p, MyDbgBuf_SIZE - 1 - len, fmt, args);
		va_end(args);
		if (level > DBG_HB)
		{
			MyDbgBuf[len++] = '\n';
			MyDbgBuf[len] = '\0';
		}
		printf("%s", MyDbgBuf);
	}
	if (level >= DBG_LOG)
	{
		Log(len);
	}
	return len;
}

//BootTimer printTmr;
extern const char *mainpath;
int days = 0;
void Log(int len)
{
	char filename[256];
	int d, m, y;
	if (sscanf(MyDbgBuf, "[%d/%d/%d", &d, &m, &y) != 3)
	{
		return;
	}
	snprintf(filename, 255, "%s/log/%d_%02d_%02d", mainpath, y, m, d);
	int today = ((y * 0x100) + m) * 0x100 + d;
	if (days != 0 && days != today)
	{
		Exec::Shell("rm %s/log/*_%02d > /dev/null 2>&1", mainpath, d);
	}
	days = today;
	int log_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (log_fd < 0)
	{
		throw std::runtime_error(FmtException("Open log file failed:%s", filename));
	}
	write(log_fd, MyDbgBuf, len);
	close(log_fd);
}

void PrintDash(char c)
{
#define DASH_LEN 80
	char buf[DASH_LEN + 1];
	memset(buf, c, DASH_LEN);
	buf[DASH_LEN] = '\0';
	puts(buf);
}
