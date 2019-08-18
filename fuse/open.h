/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * erofs-fuse\open.h
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#ifndef __EROFS_OPEN_H
#define __EROFS_OPEN_H

#include <fuse.h>
#include <fuse_opt.h>

int erofs_open(const char *path, struct fuse_file_info *fi);

#endif
