/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * erofs-fuse\disk_io.h
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#ifndef __DISK_IO_H
#define __DISK_IO_H

#include "erofs/defs.h"
#include "erofs/internal.h"

int dev_open(const char *path);
void dev_close(void);
int dev_read(void *buf, size_t count, off_t offset);

static inline int dev_read_blk(void *buf, uint32_t nr)
{
	return dev_read(buf, EROFS_BLKSIZ, blknr_to_addr(nr));
}
#endif
