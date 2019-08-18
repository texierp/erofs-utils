/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * erofs-fuse\readir.h
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#ifndef __EROFS_READDIR_H
#define __EROFS_READDIR_H

#include <fuse.h>
#include <fuse_opt.h>

int erofs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler,
	       off_t offset, struct fuse_file_info *fi);


#endif
