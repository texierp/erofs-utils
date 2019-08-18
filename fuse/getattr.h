/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * erofs-fuse\getattr.h
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#ifndef __EROFS_GETATTR_H
#define __EROFS_GETATTR_H

#include <fuse.h>
#include <fuse_opt.h>

int erofs_getattr(const char *path, struct stat *st);

#endif
