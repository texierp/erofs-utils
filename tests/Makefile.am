# SPDX-License-Identifier: GPL-2.0+
# Makefile.am

AUTOMAKE_OPTIONS = serial-tests
TESTS =

TESTS_ENVIRONMENT = \
	if [ -z $$SCRATCH_MNT ]; then \
		SCRATCH_MNT=/tmp/erofstest; export SCRATCH_MNT; \
	fi; \
	if [ -z $$TEST_DIR ]; then \
		TEST_DIR=/tmp/erofstest/mnt export TEST_DIR; \
	fi;

if ENABLE_LZ4
TESTS_ENVIRONMENT += export lz4_on=1;
endif
if ENABLE_LZ4HC
TESTS_ENVIRONMENT += export lz4hc_on=1;
endif

# 011 - test if unknown algorithm is specificed
TESTS += 001

# 002 - mkfs short symlink (fast symlink)
TESTS += 002

# 003 - mkfs long symlink (non-fast symlink)
TESTS += 003

# 004 - mkfs character/block device
TESTS += 004

# 005 - mkfs pipe file
TESTS += 005

# 006 - test short symlink (fast symlink)
TESTS += 006

# 007 - test long symlink (non-fast symlink)
TESTS += 007

# 008 - test character/block device
TESTS += 008

# 009 - test pipe file
TESTS += 009

# 011 - mkfs for uncompressed image
TESTS += 011

# 012 - (legacy image) mkfs for lz4 compressed image
TESTS += 012

# 013 - (legacy image) mkfs for lz4HC compressed image
TESTS += 013

# 014 - verify the uncompressed image
TESTS += 014

# 015 - (legacy image) verify the lz4 compressed image
TESTS += 015

# 016 - (legacy image) verify the lz4HC compressed image
TESTS += 016

# 017 - mkfs for lz4 compressed image
TESTS += 017

# 018 - mkfs for lz4HC compressed image
TESTS += 018

# 019 - verify the lz4 compressed image
TESTS += 019

# 020 - verify the lz4HC compressed image
TESTS += 020

