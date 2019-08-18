// SPDX-License-Identifier: GPL-2.0+
/*
 * erofs-fuse\logging.c
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#include "logging.h"

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

static int loglevel = DEFAULT_LOG_LEVEL;
static FILE *logfile;
static const char * const loglevel_str[] = {
	[LOG_EMERG]     = "[emerg]",
	[LOG_ALERT]     = "[alert]",
	[LOG_DUMP]      = "[dump] ",
	[LOG_ERR]       = "[err]  ",
	[LOG_WARNING]   = "[warn] ",
	[LOG_NOTICE]    = "[notic]",
	[LOG_INFO]      = "[info] ",
	[LOG_DEBUG]     = "[debug]",
};

void __LOG(int level, const char *func, int line, const char *format, ...)
{
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	va_list ap;
	FILE *fp = logfile ? logfile : stdout;

	if (!fp)
		return;
	if (level < 0 || level > loglevel)
		return;

	/* We have a lock here so different threads don interleave the log
	 * output
	 */
	pthread_mutex_lock(&lock);
	va_start(ap, format);
	fprintf(fp, "%s", loglevel_str[level]);
	if (func)
		fprintf(fp, "%s", func);
	if (line >= 0)
		fprintf(fp, "(%d):", line);
	vfprintf(fp, format, ap);
	fprintf(fp, "\n");
	va_end(ap);
	fflush(fp);
	pthread_mutex_unlock(&lock);
}

inline void logging_setlevel(int new_level)
{
	loglevel = new_level;
}

int logging_open(const char *path)
{
	if (path == NULL)
		return 0;

	logfile = fopen(path, "w");
	if (logfile == NULL) {
		perror("open");
		return -1;
	}

	return 0;
}

void logging_close(void)
{
	if (logfile) {
		fflush(logfile);
		fclose(logfile);
		logfile = NULL;
	}
}

