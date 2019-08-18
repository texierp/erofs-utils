// SPDX-License-Identifier: GPL-2.0+
/*
 * erofs-fuse\getattr.c
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#include "getattr.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "erofs/defs.h"
#include "erofs/internal.h"
#include "erofs_fs.h"

#include "logging.h"
#include "namei.h"

extern struct erofs_super_block super;

/* GNU's definitions of the attributes
 * (http://www.gnu.org/software/libc/manual/html_node/Attribute-Meanings.html):
 * st_uid: The user ID of the file鈥檚 owner.
 * st_gid: The group ID of the file.
 * st_atime: This is the last access time for the file.
 * st_mtime: This is the time of the last modification to the contents of the
 *           file.
 * st_mode: Specifies the mode of the file. This includes file type information
 *          (see Testing File Type) and the file permission bits (see Permission
 *          Bits).
 * st_nlink: The number of hard links to the file.This count keeps track of how
 *           many directories have entries for this file. If the count is ever
 *           decremented to zero, then the file itself is discarded as soon as
 *           no process still holds it open. Symbolic links are not counted in
 *           the total.
 * st_size: This specifies the size of a regular file in bytes. For files that
 *         are really devices this field isn鈥檛 usually meaningful.For symbolic
 *         links this specifies the length of the file name the link refers to.
 */
int erofs_getattr(const char *path, struct stat *stbuf)
{
	struct erofs_vnode v;
	int ret;

	logd("getattr(%s)", path);
	memset(&v, 0, sizeof(v));
	ret = erofs_iget_by_path(path, &v);
	if (ret)
		return -ENOENT;

	stbuf->st_mode  = le16_to_cpu(v.i_mode);
	stbuf->st_nlink = le16_to_cpu(v.i_nlink);
	stbuf->st_size  = le32_to_cpu(v.i_size);
	stbuf->st_blocks = stbuf->st_size / EROFS_BLKSIZ;
	stbuf->st_uid = le16_to_cpu(v.i_uid);
	stbuf->st_gid = le16_to_cpu(v.i_gid);
	stbuf->st_atime = super.build_time;
	stbuf->st_mtime = super.build_time;
	stbuf->st_ctime = super.build_time;

	return 0;
}
