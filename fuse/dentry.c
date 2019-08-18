// SPDX-License-Identifier: GPL-2.0+
/*
 * erofs-fuse\dentry.c
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#include "dentry.h"
#include "erofs/internal.h"
#include "logging.h"

#define DCACHE_ENTRY_CALLOC()   calloc(1, sizeof(struct dcache_entry))
#define DCACHE_ENTRY_LIFE       8

struct dcache_entry root_entry;

int dcache_init_root(uint32_t nid)
{
	if (root_entry.nid)
		return -1;

	/* Root entry doesn't need most of the fields. Namely, it only uses the
	 * nid field and the subdirs pointer.
	 */
	logi("Initializing root_entry dcache entry");
	root_entry.nid = nid;
	root_entry.subdirs = NULL;
	root_entry.siblings = NULL;

	return 0;
}

/* Inserts a node as a subdirs of a given parent. The parent is updated to
 * point the newly inserted subdirs as the first subdirs. We return the new
 * entry so that further entries can be inserted.
 *
 *      [0]                  [0]
 *       /        ==>          \
 *      /         ==>           \
 * .->[1]->[2]-.       .->[1]->[3]->[2]-.
 * `-----------麓       `----------------麓
 */
struct dcache_entry *dcache_insert(struct dcache_entry *parent,
				   const char *name, int namelen, uint32_t nid)
{
	struct dcache_entry *new_entry;

	logd("Inserting %s,%d to dcache", name, namelen);

	/* TODO: Deal with names that exceed the allocated size */
	if (namelen + 1 > DCACHE_ENTRY_NAME_LEN)
		return NULL;

	if (parent == NULL)
		parent = &root_entry;

	new_entry = DCACHE_ENTRY_CALLOC();
	if (!new_entry)
		return NULL;

	strncpy(new_entry->name, name, namelen);
	new_entry->name[namelen] = 0;
	new_entry->nid = nid;

	if (!parent->subdirs) {
		new_entry->siblings = new_entry;
		parent->subdirs = new_entry;
	} else {
		new_entry->siblings = parent->subdirs->siblings;
		parent->subdirs->siblings = new_entry;
		parent->subdirs = new_entry;
	}

	return new_entry;
}

/* Lookup a cache entry for a given file name.  Return value is a struct pointer
 * that can be used to both obtain the nid number and insert further child
 * entries.
 * TODO: Prune entries by using the LRU counter
 */
struct dcache_entry *dcache_lookup(struct dcache_entry *parent,
				   const char *name, int namelen)
{
	struct dcache_entry *iter;

	if (parent == NULL)
		parent = &root_entry;

	if (!parent->subdirs)
		return NULL;

	/* Iterate the list of siblings to see if there is any match */
	iter = parent->subdirs;

	do {
		if (strncmp(iter->name, name, namelen) == 0 &&
		    iter->name[namelen] == 0) {
			parent->subdirs = iter;

			return iter;
		}

		iter = iter->siblings;
	} while (iter != parent->subdirs);

	return NULL;
}

struct dcache_entry *dcache_try_insert(struct dcache_entry *parent,
				   const char *name, int namelen, uint32_t nid)
{
	struct dcache_entry *d = dcache_lookup(parent, name, namelen);

	if (d)
		return d;

	return dcache_insert(parent, name, namelen, nid);

}
erofs_nid_t dcache_get_nid(struct dcache_entry *entry)
{
	return entry ? entry->nid : root_entry.nid;
}

struct dcache_entry *dcache_root(void)
{
	return &root_entry;
}

