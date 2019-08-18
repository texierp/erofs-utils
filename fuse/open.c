// SPDX-License-Identifier: GPL-2.0+
/*
 * erofs-fuse\open.c
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#include "open.h"
#include <asm-generic/errno-base.h>
#include <fuse.h>
#include <fuse_opt.h>
#include "logging.h"

int erofs_open(const char *path, struct fuse_file_info *fi)
{
	logi("open path=%s", path);

	if ((fi->flags & O_ACCMODE) != O_RDONLY)
		return -EACCES;

	return 0;
}

