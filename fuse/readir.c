// SPDX-License-Identifier: GPL-2.0+
/*
 * erofs-fuse\readir.c
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#include "readir.h"
#include <errno.h>
#include <linux/fs.h>
#include <sys/stat.h>

#include "erofs/defs.h"
#include "erofs/internal.h"
#include "erofs_fs.h"
#include "namei.h"
#include "disk_io.h"
#include "logging.h"
#include "init.h"

erofs_nid_t split_entry(char *entry, off_t ofs, char *end, char *name)
{
	struct erofs_dirent *de = (struct erofs_dirent *)(entry + ofs);
	uint16_t nameoff = le16_to_cpu(de->nameoff);
	const char *de_name = entry + nameoff;
	uint32_t de_namelen;

	if ((void *)(de + 1) >= (void *)end)
		de_namelen = strlen(de_name);
	else
		de_namelen = le16_to_cpu(de[1].nameoff) - nameoff;

	memcpy(name, de_name, de_namelen);
	name[de_namelen] = '\0';
	return le64_to_cpu(de->nid);
}

int fill_dir(char *entrybuf, fuse_fill_dir_t filler, void *buf)
{
	uint32_t ofs;
	uint16_t nameoff;
	char *end;
	char name[EROFS_BLKSIZ];

	nameoff = le16_to_cpu(((struct erofs_dirent *)entrybuf)->nameoff);
	end = entrybuf + nameoff;
	ofs = 0;
	while (ofs < nameoff) {
		(void)split_entry(entrybuf, ofs, end, name);
		filler(buf, name, NULL, 0);
		ofs += sizeof(struct erofs_dirent);
	}

	return 0;
}

/** Function to add an entry in a readdir() operation
 *
 * @param buf the buffer passed to the readdir() operation
 * @param name the file name of the directory entry
 * @param stat file attributes, can be NULL
 * @param off offset of the next entry or zero
 * @return 1 if buffer is full, zero otherwise
 */
int erofs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		  off_t offset, struct fuse_file_info *fi)
{
	int ret;
	erofs_nid_t nid;
	struct erofs_vnode v;
	char dirsbuf[EROFS_BLKSIZ];
	uint32_t dir_nr, dir_off, nr_cnt;

	logd("readdir:%s offset=%llu", path, (long long)offset);
	UNUSED(fi);

	ret = walk_path(path, &nid);
	if (ret)
		return ret;

	logd("path=%s nid = %u", path, nid);
	ret = erofs_iget_by_nid(nid, &v);
	if (ret)
		return ret;

	if (!S_ISDIR(v.i_mode))
		return -ENOTDIR;

	if (!v.i_size)
		return 0;

	dir_nr = erofs_blknr(v.i_size);
	dir_off = erofs_blkoff(v.i_size);
	nr_cnt = 0;

	logd("dir_size=%u dir_nr = %u dir_off=%u", v.i_size, dir_nr, dir_off);

	while (nr_cnt < dir_nr) {
		memset(dirsbuf, 0, sizeof(dirsbuf));
		ret = dev_read_blk(dirsbuf, v.raw_blkaddr + nr_cnt);
		if (ret != EROFS_BLKSIZ)
			return -EIO;
		fill_dir(dirsbuf, filler, buf);
		++nr_cnt;
	}

	if (v.data_mapping_mode == EROFS_INODE_FLAT_INLINE) {
		off_t addr;

		addr = nid2addr(nid) + sizeof(struct erofs_inode_v1)
			+ v.xattr_isize;

		memset(dirsbuf, 0, sizeof(dirsbuf));
		ret = dev_read(dirsbuf, dir_off, addr);
		if (ret < 0 || (uint32_t)ret != dir_off)
			return -EIO;
		fill_dir(dirsbuf, filler, buf);
	}

	return 0;
}

