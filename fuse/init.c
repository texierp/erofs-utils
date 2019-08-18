// SPDX-License-Identifier: GPL-2.0+
/*
 * erofs-fuse\init.c
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#include "init.h"
#include <string.h>
#include <asm-generic/errno-base.h>

#include "namei.h"
#include "disk_io.h"
#include "logging.h"

#define STR(_X) (#_X)
#define SUPER_MEM(_X) (super._X)


struct erofs_super_block super;
static struct erofs_super_block *sbk = &super;

int erofs_init_super(void)
{
	int ret;
	char buf[EROFS_BLKSIZ];
	struct erofs_super_block *sb;

	memset(buf, 0, sizeof(buf));
	ret = dev_read_blk(buf, 0);
	if (ret != EROFS_BLKSIZ) {
		logi("Failed to read super block ret=%d", ret);
		return -EINVAL;
	}

	sb = (struct erofs_super_block *) (buf + BOOT_SECTOR_SIZE);
	sbk->magic = le32_to_cpu(sb->magic);
	if (sbk->magic != EROFS_SUPER_MAGIC_V1) {
		logi("EROFS magic[0x%X] NOT matched to [0x%X] ",
		     super.magic, EROFS_SUPER_MAGIC_V1);
		return -EINVAL;
	}

	sbk->checksum = le32_to_cpu(sb->checksum);
	sbk->features = le32_to_cpu(sb->features);
	sbk->blkszbits = sb->blkszbits;
	ASSERT(sbk->blkszbits != 32);

	sbk->inos = le64_to_cpu(sb->inos);
	sbk->build_time = le64_to_cpu(sb->build_time);
	sbk->build_time_nsec = le32_to_cpu(sb->build_time_nsec);
	sbk->blocks = le32_to_cpu(sb->blocks);
	sbk->meta_blkaddr = le32_to_cpu(sb->meta_blkaddr);
	sbk->xattr_blkaddr = le32_to_cpu(sb->xattr_blkaddr);
	memcpy(sbk->uuid, sb->uuid, 16);
	memcpy(sbk->volume_name, sb->volume_name, 16);
	sbk->root_nid = le16_to_cpu(sb->root_nid);

	logp("%-15s:0x%X", STR(magic), SUPER_MEM(magic));
	logp("%-15s:0x%X", STR(features), SUPER_MEM(features));
	logp("%-15s:%u",   STR(blkszbits), SUPER_MEM(blkszbits));
	logp("%-15s:%u",   STR(root_nid), SUPER_MEM(root_nid));
	logp("%-15s:%ul",  STR(inos), SUPER_MEM(inos));
	logp("%-15s:%d",   STR(meta_blkaddr), SUPER_MEM(meta_blkaddr));
	logp("%-15s:%d",   STR(xattr_blkaddr), SUPER_MEM(xattr_blkaddr));

	return 0;
}

erofs_nid_t erofs_get_root_nid(void)
{
	return sbk->root_nid;
}

erofs_nid_t addr2nid(erofs_off_t addr)
{
	erofs_nid_t offset = (erofs_nid_t)sbk->meta_blkaddr * EROFS_BLKSIZ;

	ASSERT(IS_SLOT_ALIGN(addr));
	return (addr - offset) >> EROFS_ISLOTBITS;
}

erofs_off_t nid2addr(erofs_nid_t nid)
{
	erofs_off_t offset = (erofs_off_t)sbk->meta_blkaddr * EROFS_BLKSIZ;

	return (nid <<  EROFS_ISLOTBITS) + offset;
}

void *erofs_init(struct fuse_conn_info *info)
{
	logi("Using FUSE protocol %d.%d", info->proto_major, info->proto_minor);

	if (inode_init(erofs_get_root_nid()) != 0) {
		loge("inode initialization failed")
		ABORT();
	}
	return NULL;
}
