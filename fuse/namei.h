/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * erofs-fuse\inode.h
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#ifndef __INODE_H
#define __INODE_H

#include "erofs/internal.h"
#include "erofs_fs.h"

int inode_init(erofs_nid_t root);
struct dcache_entry *get_cached_dentry(struct dcache_entry **parent,
				       const char **path);
int erofs_iget_by_path(const char *path, struct erofs_vnode *v);
int erofs_iget_by_nid(erofs_nid_t nid, struct erofs_vnode *v);
struct dcache_entry *disk_lookup(struct dcache_entry *parent, const char *name,
		unsigned int name_len);
int walk_path(const char *path, erofs_nid_t *out_nid);

#endif
