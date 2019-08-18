/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * erofs-fuse\logging.h
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#ifndef __LOGGING_H
#define __LOGGING_H

#include <assert.h>
#include <stdlib.h>

#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_DUMP    2
#define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7

#define logem(...)	__LOG(LOG_EMERG, __func__, __LINE__, ##__VA_ARGS__)
#define loga(...)	__LOG(LOG_ALERT, __func__, __LINE__, ##__VA_ARGS__)
#define loge(...)	__LOG(LOG_ERR,   __func__, __LINE__, ##__VA_ARGS__)
#define logw(...)	__LOG(LOG_WARNING, __func__, __LINE__, ##__VA_ARGS__)
#define logn(...)	__LOG(LOG_NOTICE,  __func__, __LINE__, ##__VA_ARGS__)
#define logi(...)	__LOG(LOG_INFO,  __func__, __LINE__, ##__VA_ARGS__)
#define logp(...)	__LOG(LOG_DUMP,  "", -1, ##__VA_ARGS__)
#define logd(...)	__LOG(LOG_DEBUG, __func__, __LINE__, ##__VA_ARGS__)

#define DEFAULT_LOG_FILE	"fuse.log"

#ifdef _DEBUG
#define DEFAULT_LOG_LEVEL LOG_DEBUG

#define ASSERT(assertion) ({                            \
	if (!(assertion)) {                             \
		logw("ASSERT FAIL: " #assertion);       \
		assert(assertion);                      \
	}                                               \
})
#define ABORT(_X) abort(_X)
#else
#define DEFAULT_LOG_LEVEL LOG_ERR
#define ASSERT(assertion)
#define ABORT(_X)
#endif

void __LOG(int level, const char *func, int line, const char *format, ...);
void logging_setlevel(int new_level);
int logging_open(const char *path);
void logging_close(void);

#endif

