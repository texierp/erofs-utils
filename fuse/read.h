/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * erofs-fuse\read.h
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#ifndef __EROFS_READ_H
#define __EROFS_READ_H

#include <fuse.h>
#include <fuse_opt.h>

int erofs_read(const char *path, char *buffer, size_t size, off_t offset,
	    struct fuse_file_info *fi);

#endif
