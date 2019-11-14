// SPDX-License-Identifier: GPL-2.0+
/*
 * erofs-utils/lib/compressor_xz.c
 *
 * Copyright (C) 2018-2019 HUAWEI, Inc.
 *             http://www.huawei.com/
 * Created by Gao Xiang <gaoxiang25@huawei.com>
 */
#include <lzma.h>
#include "erofs/internal.h"
#include "compressor.h"

/* #define XZDEBUG */
#define TRYSCALE	5
#define EXCESS		128
#define RAWLEN		8192
static u8 trash[RAWLEN];

struct __xz_fitblk_compress {
	lzma_stream *def;
	lzma_filter *filters;
};

static int xz_fitblkprecompress(void *ctx,
				void *in, unsigned int insize,
				void *out, unsigned int *outsize)
{
	struct __xz_fitblk_compress *xc = ctx;
	lzma_stream *def = xc->def;
	lzma_filter *filters = xc->filters;
	lzma_ret ret;
	unsigned int tryinsize;

	/* Initialize the encoder using the custom filter chain. */
	ret = lzma_stream_encoder(def, filters, LZMA_CHECK_NONE);
	if (ret != LZMA_OK)
		return -EFAULT;

	def->next_out = out;
	def->avail_out = *outsize;
	def->next_in = in;

	tryinsize = min_t(unsigned int, *outsize * TRYSCALE, insize);
	def->avail_in = tryinsize;

	ret = lzma_code(def, LZMA_FULL_BARRIER);

	/* all data can be fitted in, feed more then */
	if (ret == LZMA_STREAM_END && tryinsize != insize) {
		def->avail_in = insize - tryinsize;
		ret = lzma_code(def, LZMA_FINISH);
	}

	switch (ret) {
	case LZMA_MEM_ERROR:
		return -ENOMEM;
	case LZMA_DATA_ERROR:
		return -EUCLEAN;
	case LZMA_OK:
	case LZMA_STREAM_END:
		break;
	default:
		return -EFAULT;
	}
	*outsize -= def->avail_out;
	lzma_end(def);
	return tryinsize != insize;
}

static int xz_decompressall(lzma_stream *inf,
			    void *src, unsigned int srcsize,
			    void *dst, unsigned int *dstsize)
{
	lzma_ret ret = lzma_stream_decoder(inf, UINT64_MAX, LZMA_CONCATENATED);

	if (ret != LZMA_OK)
		return -EFAULT;

	inf->next_out = dst;
	inf->avail_out = *dstsize;
	inf->next_in = src;
	inf->avail_in = srcsize;

	ret = lzma_code(inf, LZMA_FINISH);
	*dstsize -= inf->avail_out;
	lzma_end(inf);
	return 0;
}

static int xz_fitblkcompress(void *ctx,
			     void *in, unsigned int insize,
			     void *out, unsigned int *outsize)
{
	struct __xz_fitblk_compress *xc = ctx;
	lzma_stream *def = xc->def;
	lzma_filter *filters = xc->filters;
	lzma_ret ret;
	unsigned int databeyond;

	/* Initialize the encoder using the custom filter chain. */
	ret = lzma_stream_encoder(def, filters, LZMA_CHECK_CRC32);
	/* ret = lzma_stream_encoder(def, filters, LZMA_CHECK_NONE); */
	if (ret != LZMA_OK)
		return -EFAULT;

	def->next_out = out;
	def->avail_out = *outsize;
	def->next_in = in;
	def->avail_in = insize;

	ret = lzma_code(def, LZMA_FINISH);
	*outsize -= def->avail_out;

	/* all data can be fitted in */
	if (ret == LZMA_STREAM_END) {
		lzma_end(def);
		return 0;
	}

	databeyond = 0;
	while (ret == LZMA_OK) {
		def->next_out = trash;
		def->avail_out = RAWLEN;
		ret = lzma_code(def, LZMA_FINISH);
		databeyond += RAWLEN - def->avail_out;
	}

	if (ret != LZMA_STREAM_END) {
		switch (ret) {
		case LZMA_MEM_ERROR:
			return -ENOMEM;
		case LZMA_DATA_ERROR:
			return -EUCLEAN;
		default:
			return -EFAULT;
		}
	}
	lzma_end(def);
	return databeyond;
}

static int xz_decompress(lzma_stream *inf,
			 void *src, unsigned int srcsize,
			 void *dst, unsigned int *dstsize)
{
	lzma_ret ret = lzma_stream_decoder(inf, UINT64_MAX, 0);

	if (ret != LZMA_OK)
		return -EFAULT;

	inf->next_out = dst;
	inf->avail_out = *dstsize;
	inf->next_in = src;
	inf->avail_in = srcsize;

	ret = lzma_code(inf, LZMA_FINISH);
	*dstsize -= inf->avail_out;
	lzma_end(inf);
	return 0;
}

#ifdef XZDEBUG
static char tmp[2*1024*1024];
#endif

static int xz_compress_destsize(struct erofs_compress *c,
				int compression_level,
				void *in, unsigned int *inlen,
				void *out, unsigned int outlen)
{
	lzma_stream defstrm = LZMA_STREAM_INIT;
	lzma_stream infstrm = LZMA_STREAM_INIT;
	lzma_options_lzma opt_lzma2;
	lzma_filter filters[] = {
		{ .id = LZMA_FILTER_X86, .options = NULL },
		{ .id = LZMA_FILTER_LZMA2, .options = &opt_lzma2 },
		{ .id = LZMA_VLI_UNKNOWN, .options = NULL },
	};
	struct __xz_fitblk_compress xc = {
		.def = &defstrm, .filters = filters,
	};
	/* which will greatly affect compression time */
	//bool bisect = true;
	bool bisect = (compression_level == c->alg->best_level);
	int err;
#ifndef XZDEBUG
	char *tmp = in;
#endif
	unsigned int dstsize, left, cur, right, grade;

	if (lzma_lzma_preset(&opt_lzma2,
			     compression_level & LZMA_PRESET_LEVEL_MASK))
		return -EINVAL;

	opt_lzma2.dict_size = 512 * 1024;
	dstsize = outlen + c->paddingdstsz;

	err = xz_fitblkprecompress(&xc, in, *inlen, out, &dstsize);
	if (err < 0)
		return err;

	cur = *inlen;
	/* all data are fitted in when doing pass 0th */
	if (err || dstsize > outlen) {
		err = xz_decompressall(&infstrm, out, dstsize, tmp, &cur);
		if (err)
			return err;
	}
	left = 0;
	right = ~0;
	while (1) {
		unsigned int beyondsize;

		err = xz_fitblkcompress(&xc, in, cur, out, &dstsize);
		if (err < 0)
			return err;
		if (!err && dstsize <= outlen) {
			if (bisect && dstsize < outlen && left < cur) {
				grade = (outlen - dstsize) * cur / dstsize;
				left = cur;
				cur += grade << (cur + 2 * grade < right);

				if (left != cur) {
					dstsize = outlen;
					continue;
				}
			}
			*inlen = cur;
			break;
		}
		beyondsize = err;

		err = xz_decompress(&infstrm, out, dstsize, tmp, &cur);
		if (err)
			return err;
		if (dstsize > outlen) {
			dstsize = outlen;
		} else {
			right = cur;
			cur -= cur * beyondsize / (dstsize + beyondsize);
			if (cur < left)
				cur = left;
		}
	}
#ifdef XZDEBUG
	err = xz_decompress(&infstrm, out, dstsize, tmp, &cur);
	if (err)
		return err;
	if (memcmp(tmp, in, cur) || cur > *inlen)
		return -EBADMSG;
#endif
	memset(out + dstsize, 0, outlen - dstsize);
	*inlen = cur;
	return dstsize;
}

static int compressor_xz_exit(struct erofs_compress *c)
{
	return 0;
}

static int compressor_xz_init(struct erofs_compress *c)
{
	c->alg = &erofs_compressor_xz;
	c->paddingdstsz = EXCESS;
	return 0;
}

struct erofs_compressor erofs_compressor_xz = {
	.name = "xz",
	.default_level = LZMA_PRESET_DEFAULT,
	.best_level = 9,
	.init = compressor_xz_init,
	.exit = compressor_xz_exit,
	.compress_destsize = xz_compress_destsize,
};

