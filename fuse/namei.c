// SPDX-License-Identifier: GPL-2.0+
/*
 * erofs-fuse\inode.c
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#include "namei.h"
#include <linux/kdev_t.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#include "erofs/defs.h"
#include "logging.h"
#include "disk_io.h"
#include "dentry.h"
#include "init.h"

#define IS_PATH_SEPARATOR(__c)      ((__c) == '/')
#define MINORBITS	20
#define MINORMASK	((1U << MINORBITS) - 1)
#define DT_UNKNOWN	0

static const char *skip_trailing_backslash(const char *path)
{
	while (IS_PATH_SEPARATOR(*path))
		path++;
	return path;
}

static uint8_t get_path_token_len(const char *path)
{
	uint8_t len = 0;

	while (path[len] != '/' && path[len])
		len++;
	return len;
}

int erofs_iget_by_nid(erofs_nid_t nid, struct erofs_vnode *vi)
{
	int ret;
	char buf[EROFS_BLKSIZ];
	struct erofs_inode_v1 *v1;
	const erofs_off_t addr = nid2addr(nid);
	const size_t size = EROFS_BLKSIZ - erofs_blkoff(addr);

	ret = dev_read(buf, size, addr);
	if (ret != (int)size)
		return -EIO;

	v1 = (struct erofs_inode_v1 *)buf;
	vi->data_mapping_mode = __inode_data_mapping(le16_to_cpu(v1->i_advise));
	vi->inode_isize = sizeof(struct erofs_inode_v1);
	vi->xattr_isize = ondisk_xattr_ibody_size(v1->i_xattr_icount);
	vi->i_size = le32_to_cpu(v1->i_size);
	vi->i_mode = le16_to_cpu(v1->i_mode);
	vi->i_uid = le16_to_cpu(v1->i_uid);
	vi->i_gid = le16_to_cpu(v1->i_gid);
	vi->i_nlink = le16_to_cpu(v1->i_nlink);
	vi->nid = nid;

	switch (vi->i_mode & S_IFMT) {
	case S_IFBLK:
	case S_IFCHR:
		/* fixme: add special devices support
		 * vi->i_rdev = new_decode_dev(le32_to_cpu(v1->i_u.rdev));
		 */
		break;
	case S_IFIFO:
	case S_IFSOCK:
		/*fixme: vi->i_rdev = 0; */
		break;
	case S_IFREG:
	case S_IFLNK:
	case S_IFDIR:
		vi->raw_blkaddr = le32_to_cpu(v1->i_u.raw_blkaddr);
		break;
	default:
		return -EIO;
	}

	return 0;
}

/* dirent + name string */
struct dcache_entry *list_name(const char *buf, struct dcache_entry *parent,
				const char *name, unsigned int len)
{
	struct dcache_entry *entry = NULL;
	struct erofs_dirent *ds, *de;

	ds = (struct erofs_dirent *)buf;
	de = (struct erofs_dirent *)(buf + le16_to_cpu(ds->nameoff));

	while (ds < de) {
		erofs_nid_t nid = le64_to_cpu(ds->nid);
		uint16_t nameoff = le16_to_cpu(ds->nameoff);
		char *d_name = (char *)(buf + nameoff);
		uint16_t name_len = (ds + 1 >= de) ? (uint16_t)strlen(d_name) :
			le16_to_cpu(ds[1].nameoff) - nameoff;

		#if defined(EROFS_DEBUG_ENTRY)
		{
			char debug[EROFS_BLKSIZ];

			memcpy(debug, d_name, name_len);
			debug[name_len] = '\0';
			logi("list entry: %s nid=%u", debug, nid);
		}
		#endif

		entry = dcache_try_insert(parent, d_name, name_len, nid);
		if (len == name_len && !memcmp(name, d_name, name_len))
			return entry;

		entry = NULL;
		++ds;
	}

	return entry;
}

struct dcache_entry *disk_lookup(struct dcache_entry *parent, const char *name,
		unsigned int name_len)
{
	int ret;
	char buf[EROFS_BLKSIZ];
	struct dcache_entry *entry = NULL;
	struct erofs_vnode v;
	uint32_t nr_cnt, dir_nr, dirsize, blkno;

	ret = erofs_iget_by_nid(parent->nid, &v);
	if (ret)
		return NULL;

	/* to check whether dirent is in the inline dirs */
	blkno = v.raw_blkaddr;
	dirsize = v.i_size;
	dir_nr = erofs_blknr(dirsize);

	nr_cnt = 0;
	while (nr_cnt < dir_nr) {
		if (dev_read_blk(buf, blkno + nr_cnt) != EROFS_BLKSIZ)
			return NULL;

		entry = list_name(buf, parent, name, name_len);
		if (entry)
			goto next;

		++nr_cnt;
	}

	if (v.data_mapping_mode == EROFS_INODE_FLAT_INLINE) {
		uint32_t dir_off = erofs_blkoff(dirsize);
		off_t dir_addr = nid2addr(dcache_get_nid(parent))
			+ sizeof(struct erofs_inode_v1);

		memset(buf, 0, sizeof(buf));
		ret = dev_read(buf, dir_off, dir_addr);
		if (ret < 0 && (uint32_t)ret != dir_off)
			return NULL;

		entry = list_name(buf, parent, name, name_len);
	}
next:
	return entry;
}

extern struct dcache_entry root_entry;
int walk_path(const char *_path, erofs_nid_t *out_nid)
{
	struct dcache_entry *next, *ret;
	const char *path = _path;

	ret = next = &root_entry;
	for (;;) {
		uint8_t path_len;

		path = skip_trailing_backslash(path);
		path_len = get_path_token_len(path);
		ret = next;
		if (path_len == 0)
			break;

		next = dcache_lookup(ret, path, path_len);
		if (!next) {
			next = disk_lookup(ret, path, path_len);
			if (!next)
				return -ENOENT;
		}

		path += path_len;
	}

	if (!ret)
		return -ENOENT;
	logd("find path = %s nid=%u", _path, ret->nid);

	*out_nid = ret->nid;
	return 0;

}

int erofs_iget_by_path(const char *path, struct erofs_vnode *v)
{
	int ret;
	erofs_nid_t nid;

	ret = walk_path(path, &nid);
	if (ret)
		return ret;

	return erofs_iget_by_nid(nid, v);
}

int inode_init(erofs_nid_t root)
{
	dcache_init_root(root);

	return 0;
}

