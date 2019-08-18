/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * erofs-fuse\init.h
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#ifndef __EROFS_INIT_H
#define __EROFS_INIT_H

#include <fuse.h>
#include <fuse_opt.h>
#include "erofs/internal.h"

#define BOOT_SECTOR_SIZE	0x400

int erofs_init_super(void);
erofs_nid_t erofs_get_root_nid(void);
erofs_off_t nid2addr(erofs_nid_t nid);
erofs_nid_t addr2nid(erofs_off_t addr);
void *erofs_init(struct fuse_conn_info *info);

#endif
