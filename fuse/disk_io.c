// SPDX-License-Identifier: GPL-2.0+
/*
 * erofs-fuse\disk_io.c
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#define _XOPEN_SOURCE 500
#include "disk_io.h"

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <errno.h>

#include "logging.h"

#ifdef __FreeBSD__
#include <string.h>
#endif

static const char *erofs_devname;
static int erofs_devfd = -1;
static pthread_mutex_t read_lock = PTHREAD_MUTEX_INITIALIZER;

int dev_open(const char *path)
{
	int fd = open(path, O_RDONLY);

	if (fd < 0)
		return -errno;

	erofs_devfd = fd;
	erofs_devname = path;

	return 0;
}

static inline int pread_wrapper(int fd, void *buf, size_t count, off_t offset)
{
	return pread(fd, buf, count, offset);
}

int dev_read(void *buf, size_t count, off_t offset)
{
	ssize_t pread_ret;
	int lerrno;

	ASSERT(erofs_devfd >= 0);

	pthread_mutex_lock(&read_lock);
	pread_ret = pread_wrapper(erofs_devfd, buf, count, offset);
	lerrno = errno;
	logd("Disk Read: offset[0x%jx] count[%zd] pread_ret=%zd %s",
	     offset, count, pread_ret, strerror(lerrno));
	pthread_mutex_unlock(&read_lock);
	if (count == 0)
		logw("Read operation with 0 size");

	ASSERT((size_t)pread_ret == count);

	return pread_ret;
}

void dev_close(void)
{
	if (erofs_devfd >= 0) {
		close(erofs_devfd);
		erofs_devfd = -1;
	}
}
