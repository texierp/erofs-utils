// SPDX-License-Identifier: GPL-2.0+
/*
 * erofs-fuse\main.c
 * Created by Li Guifu <blucerlee@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <signal.h>
#include <stddef.h>

#include "logging.h"
#include "init.h"
#include "read.h"
#include "getattr.h"
#include "open.h"
#include "readir.h"
#include "disk_io.h"

enum {
	EROFS_OPT_HELP,
	EROFS_OPT_VER,
};

struct options {
	const char *disk;
	const char *mount;
	const char *logfile;
	unsigned int debug_lvl;
};
static struct options cfg;

#define OPTION(t, p)    { t, offsetof(struct options, p), 1 }

static const struct fuse_opt option_spec[] = {
	OPTION("--log=%s", logfile),
	OPTION("--dbg=%u", debug_lvl),
	FUSE_OPT_KEY("-h", EROFS_OPT_HELP),
	FUSE_OPT_KEY("-v", EROFS_OPT_VER),
	FUSE_OPT_END
};

static void usage(void)
{
	fprintf(stderr, "\terofsfuse [options] <image> <mountpoint>\n");
	fprintf(stderr, "\t    --log=<file>    output log file\n");
	fprintf(stderr, "\t    --dbg=<level>   log debug level 0 ~ 7\n");
	fprintf(stderr, "\t    -h   show help\n");
	fprintf(stderr, "\t    -v   show version\n");
	exit(1);
}

static void dump_cfg(void)
{
	fprintf(stderr, "\tdisk :%s\n", cfg.disk);
	fprintf(stderr, "\tmount:%s\n", cfg.mount);
	fprintf(stderr, "\tdebug_lvl:%u\n", cfg.debug_lvl);
	fprintf(stderr, "\tlogfile  :%s\n", cfg.logfile);
}

static int optional_opt_func(void *data, const char *arg, int key,
			     struct fuse_args *outargs)
{
	UNUSED(data);
	UNUSED(outargs);

	switch (key) {
	case FUSE_OPT_KEY_OPT:
		return 1;

	case FUSE_OPT_KEY_NONOPT:
		if (!cfg.disk) {
			cfg.disk = strdup(arg);
			return 0;
		} else if (!cfg.mount)
			cfg.mount = strdup(arg);

		return 1;
	case EROFS_OPT_HELP:
		usage();
		break;

	case EROFS_OPT_VER:
		fprintf(stderr, "EROFS FUSE VERSION v 1.0.0\n");
		exit(0);
	}

	return 1;
}

static void signal_handle_sigsegv(int signal)
{
	void *array[10];
	size_t nptrs;
	char **strings;
	size_t i;

	UNUSED(signal);
	logd("========================================");
	logd("Segmentation Fault.  Starting backtrace:");
	nptrs = backtrace(array, 10);
	strings = backtrace_symbols(array, nptrs);
	if (strings) {
		for (i = 0; i < nptrs; i++)
			logd("%s", strings[i]);
		free(strings);
	}
	logd("========================================");

	abort();
}

static struct fuse_operations erofs_ops = {
	.getattr = erofs_getattr,
	.readdir = erofs_readdir,
	.open = erofs_open,
	.read = erofs_read,
	.init = erofs_init,
};

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	if (signal(SIGSEGV, signal_handle_sigsegv) == SIG_ERR) {
		fprintf(stderr, "Failed to initialize signals\n");
		return EXIT_FAILURE;
	}

	/* Parse options */
	if (fuse_opt_parse(&args, &cfg, option_spec, optional_opt_func) < 0)
		return 1;

	dump_cfg();

	if (logging_open(cfg.logfile) < 0) {
		fprintf(stderr, "Failed to initialize logging\n");
		goto exit;
	}

	logging_setlevel(cfg.debug_lvl);

	if (dev_open(cfg.disk) < 0) {
		fprintf(stderr, "Failed to open disk:%s\n", cfg.disk);
		goto exit_log;
	}

	if (erofs_init_super()) {
		fprintf(stderr, "Failed to read erofs super block\n");
		goto exit_dev;
	}

	logi("fuse start");

	ret = fuse_main(args.argc, args.argv, &erofs_ops, NULL);

	logi("fuse done ret=%d", ret);

exit_dev:
	dev_close();
exit_log:
	logging_close();
exit:
	fuse_opt_free_args(&args);
	return ret;
}

