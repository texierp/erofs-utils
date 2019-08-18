// SPDX-License-Identifier: GPL-2.0+
/*
 * erofs-fuse\read.c
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#include "read.h"
#include <errno.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include <string.h>

#include "erofs/defs.h"
#include "erofs/internal.h"
#include "logging.h"
#include "namei.h"
#include "disk_io.h"
#include "init.h"

size_t erofs_read_data(struct erofs_vnode *vnode, char *buffer,
		       size_t size, off_t offset)
{
	int ret;
	size_t sum, rdsz = 0;
	uint32_t addr = blknr_to_addr(vnode->raw_blkaddr) + offset;

	sum = (offset + size) > vnode->i_size ?
		(size_t)(vnode->i_size - offset) : size;
	while (rdsz < sum) {
		size_t count = min(EROFS_BLKSIZ, (uint32_t)(sum - rdsz));

		ret = dev_read(buffer + rdsz, count, addr + rdsz);
		if (ret < 0 || (size_t)ret != count)
			return -EIO;
		rdsz += count;
	}

	logi("nid:%u size=%zd offset=%llu realsize=%zd done",
	     vnode->nid, size, (long long)offset, rdsz);
	return rdsz;

}

size_t erofs_read_data_inline(struct erofs_vnode *vnode, char *buffer,
			      size_t size, off_t offset)
{
	int ret;
	size_t sum, suminline, rdsz = 0;
	uint32_t addr = blknr_to_addr(vnode->raw_blkaddr) + offset;
	uint32_t szblk = vnode->i_size - erofs_blkoff(vnode->i_size);

	sum = (offset + size) > szblk ? (size_t)(szblk - offset) : size;
	suminline = size - sum;

	while (rdsz < sum) {
		size_t count = min(EROFS_BLKSIZ, (uint32_t)(sum - rdsz));

		ret = dev_read(buffer + rdsz, count, addr + rdsz);
		if (ret < 0 || (uint32_t)ret != count)
			return -EIO;
		rdsz += count;
	}

	if (!suminline)
		goto finished;

	addr = nid2addr(vnode->nid) + sizeof(struct erofs_inode_v1)
		+ vnode->xattr_isize;
	ret = dev_read(buffer + rdsz, suminline, addr);
	if (ret < 0 || (size_t)ret != suminline)
		return -EIO;
	rdsz += suminline;

finished:
	logi("nid:%u size=%zd suminline=%u offset=%llu realsize=%zd done",
	     vnode->nid, size, suminline, (long long)offset, rdsz);
	return rdsz;

}


int erofs_read(const char *path, char *buffer, size_t size, off_t offset,
	       struct fuse_file_info *fi)
{
	int ret;
	erofs_nid_t nid;
	struct erofs_vnode v;

	UNUSED(fi);
	logi("path:%s size=%zd offset=%llu", path, size, (long long)offset);

	ret = walk_path(path, &nid);
	if (ret)
		return ret;

	ret = erofs_iget_by_nid(nid, &v);
	if (ret)
		return ret;

	logi("path:%s nid=%llu mode=%u", path, nid, v.data_mapping_mode);
	switch (v.data_mapping_mode) {
	case EROFS_INODE_FLAT_PLAIN:
		return erofs_read_data(&v, buffer, size, offset);

	case EROFS_INODE_FLAT_INLINE:
		return erofs_read_data_inline(&v, buffer, size, offset);

	case EROFS_INODE_FLAT_COMPRESSION_LEGACY:
	case EROFS_INODE_FLAT_COMPRESSION:
		/* Fixme: */
	default:
		return -EINVAL;
	}
}
