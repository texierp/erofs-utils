# SPDX-License-Identifier: GPL-2.0+
# Makefile.am

AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS     = erofsfuse
erofsfuse_SOURCES = main.c dentry.c getattr.c logging.c namei.c read.c disk_io.c init.c open.c readir.c
erofsfuse_CFLAGS = -Wall -Werror -Wextra \
                   -I$(top_srcdir)/include \
                   $(shell pkg-config fuse --cflags) \
                   -DFUSE_USE_VERSION=26 \
                   -std=gnu99
LDFLAGS += $(shell pkg-config fuse --libs)
erofsfuse_LDADD = $(top_builddir)/lib/liberofs.la -ldl

