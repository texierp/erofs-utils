/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * erofs-fuse\dentry.h
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#ifndef _EROFS_DENTRY_H
#define _EROFS_DENTRY_H

#include <stdint.h>
#include "erofs/internal.h"

#ifdef __64BITS
#define DCACHE_ENTRY_NAME_LEN       40
#else
#define DCACHE_ENTRY_NAME_LEN       48
#endif

/* This struct declares a node of a k-tree.  Every node has a pointer to one of
 * the subdirs and a pointer (in a circular list fashion) to its siblings.
 */

struct dcache_entry {
	struct dcache_entry *subdirs;
	struct dcache_entry *siblings;
	uint32_t nid;
	uint16_t lru_count;
	uint8_t user_count;
	char name[DCACHE_ENTRY_NAME_LEN];
};

struct dcache_entry *dcache_insert(struct dcache_entry *parent,
				   const char *name, int namelen, uint32_t n);
struct dcache_entry *dcache_lookup(struct dcache_entry *parent,
				   const char *name, int namelen);
struct dcache_entry *dcache_try_insert(struct dcache_entry *parent,
				       const char *name, int namelen,
				       uint32_t nid);

erofs_nid_t dcache_get_nid(struct dcache_entry *entry);
int dcache_init_root(uint32_t n);
#endif
