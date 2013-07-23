/*
 * Flash-based transactional key-value store
 *
 * Copyright (C) 2011 Google, Inc.
 * Author: Eugene Surovegin, Terry Heo
 */

#include "common.h"
#include "nflash_drv.h"
#include "config.h"
#include "nand_priv.h"
#include "com_type.h"
#include "debug.h"
#include "errno-base.h"
#include "string.h"
#include "flash_ts.h"

#define printf          lgpl_printf
#define min(a,b)        (((a) < (b)) ? (a) : (b))
#define NAND_INT_PAGE_BUFF_SIZE         ((MV_NAND_MAX_PAGE_SIZE + MV_NAND_MAX_OOB_SIZE) / 4)
#define EMMC_INT_PAGE_BUFF_SIZE         (8448/4 + 32)
#define ALIGNED(val, n)                 ((((unsigned int)val) + (n) - 1) & (~((unsigned int)(n) - 1)))

extern struct mv_nand_data nand_data ;

inline int nand_page_size() {
    return (nand_data.szofpg);

}

inline int nand_block_size() {
    return (nand_data.szofblk);
}

inline int nand_read_page(int page, void *buf, int size) {
#ifdef NAND_BOOT
    static int page_buff[NAND_INT_PAGE_BUFF_SIZE];
#elif defined(EMMC_BOOT)
    static int temp[EMMC_INT_PAGE_BUFF_SIZE];
    int *page_buff;
    page_buff = (int *)ALIGNED(temp, 32);
#else
#error
#endif
    loff_t offset = page * nand_page_size();

    if (size > nand_page_size()) {
        return -1;
    }
    if (NFlash_PageRead(offset, page_buff)) {
        // page may be not erased. return default data.
        memset(buf, 0xff, size);
        return 0;
    }
    UtilMemCpy(buf, page_buff, size);
    return 0;
}

inline int nand_write_page(int page, const void *buf, int size) {
#ifdef NAND_BOOT
    static int page_buff[NAND_INT_PAGE_BUFF_SIZE];
#elif defined(EMMC_BOOT)
    static int temp[EMMC_INT_PAGE_BUFF_SIZE];
    int *page_buff;
    page_buff = (int *)ALIGNED(temp, 32);
#else
#error
#endif
    loff_t offset = page * nand_page_size();

    if (size > nand_page_size()) {
        return -1;
    }
    memset(page_buff, 0xff, sizeof(page_buff));
    UtilMemCpy(page_buff, buf, size);
    if (NFlash_PageWrite(offset, page_buff)) {
        lgpl_printf("Write failed @ 0x%08x\n", (unsigned)offset);
        return -1;
    }
    return 0;
}

/* Compatibility defines to keep code as similar as possible
 * to the Linux version.
 */
#define DRV_NAME		"fts"
static inline int is_power_of_2(unsigned int n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

//typedef unsigned u32;
//typedef long loff_t;
#define likely(x)		__builtin_expect(!!(x), 1)
#define unlikely(x)		__builtin_expect(!!(x), 0)

/* Keep in sync with 'struct flash_ts' */
#define FLASH_TS_HDR_SIZE	(4 * sizeof(u32))
#define FLASH_TS_MAX_SIZE	(16 * 1024)
#define FLASH_TS_MAX_DATA_SIZE	(FLASH_TS_MAX_SIZE - FLASH_TS_HDR_SIZE)

#define FLASH_TS_MAGIC		0x53542a46

/* Physical flash layout */
struct flash_ts {
	u32 magic;		/* "F*TS" */
	u32 crc;		/* doesn't include magic and crc fields */
	u32 len;		/* real size of data */
	u32 version;		/* generation counter, must be positive */

	/* data format is very similar to Unix environment:
	 *   key1=value1\0key2=value2\0\0
	 */
	char data[FLASH_TS_MAX_DATA_SIZE];
};

/* Internal state */
static struct flash_ts_priv {
	/* chunk size, >= sizeof(struct flash_ts) */
	size_t chunk;

	/* current record offset within NAND partition */
	loff_t offset;

	loff_t base;		/* NAND partition base offset */
	size_t size;		/* NAND partition size */
	size_t erasesize;	/* block size */
	size_t writesize;	/* page size */

	/* in-memory copy of flash content */
	struct flash_ts cache;

	/* temporary buffers
	 *  - one backup for failure rollback
	 *  - another for read-after-write verification
	 */
	struct flash_ts cache_tmp_backup;
	struct flash_ts cache_tmp_verify;
} __ts_priv;
static struct flash_ts_priv *__ts = NULL;

static int flash_block_isbad(struct flash_ts_priv *ts, loff_t off)
{
	return is_block_bad(ts->base + off);
}

static int flash_erase(struct flash_ts_priv *ts, loff_t off)
{
	lgpl_printf(DRV_NAME ": flash_erase @ 0x%08x\n", (unsigned)(ts->base + off));
	int res = NFlash_Erase(ts->base + off);
	if (unlikely(res))
		printf(DRV_NAME ": flash_erase(0x%08x) failed, errno %d\n",
		       (unsigned)off, res);
	return res;
}

static int flash_write(struct flash_ts_priv *ts, loff_t off,
		       const void *buf, size_t size)
{
	int page = (ts->base + off) / ts->writesize;
	int res = 0;

	while (size) {
		size_t chunk = min(size, ts->writesize);
		res = nand_write_page(page, buf, chunk);
		if (likely(!res)) {
			++page;
			buf = (const void*)((unsigned long)buf + chunk);
			size -= chunk;
		} else {
			printf(DRV_NAME ": write_page(%u) failed, errno %d\n",
			       page, res);
			break;
		}
	}
	return res;
}

static int flash_read(struct flash_ts_priv *ts, loff_t off, void *buf, size_t size)
{
	int page = (ts->base + off) / ts->writesize;
	int res = 0;

	while (size) {
		size_t chunk = min(size, ts->writesize);
		res = nand_read_page(page, buf, chunk);
		if (likely(!res)) {
			++page;
			buf = (void*)((unsigned long)buf + chunk);
			size -= chunk;
		} else {
			printf(DRV_NAME ": read_page(%u), errno %d\n",
			       page, res);
			break;
		}
	}
	return res;
}

static char *flash_ts_find(struct flash_ts_priv *ts, const char *key,
			   size_t key_len)
{
	char *s = ts->cache.data;
	while (*s) {
		if (!strncmp(s, key, key_len)) {
			if (s[key_len] == '=')
				return s;
		}

		s += strlen(s) + 1;
	}
	return NULL;
}


static inline u32 flash_ts_crc(const struct flash_ts *cache)
{
	const unsigned char *p;
	u32 crc = 0;
	size_t len;

	/* skip magic and crc fields */
	len = cache->len + 2 * sizeof(u32);
	p = (const unsigned char*)&cache->len;

	while (len--) {
		int i;

		crc ^= *p++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
	}
	return crc ^ ~0;
}

/* Verifies cache consistency and locks it */
static struct flash_ts_priv *__flash_ts_get(void)
{
	struct flash_ts_priv *ts = __ts;

	if (likely(ts)) {
		if (unlikely(ts->cache.crc != flash_ts_crc(&ts->cache))) {
			printf(DRV_NAME
			       ": memory corruption detected\n");
			ts = NULL;
		}
	} else {
		printf(DRV_NAME ": not initialized yet\n");
	}

	return ts;
}

static inline void __flash_ts_put(struct flash_ts_priv *ts)
{
}

static int flash_ts_commit(struct flash_ts_priv *ts)
{
	loff_t off = ts->offset + ts->chunk;
	/* we try to make two passes to handle non-erased blocks
	 * this should only matter for the inital pass over the whole device.
	 */
	int max_iterations = (ts->size / ts->erasesize) * 2;
	size_t size = (FLASH_TS_HDR_SIZE + ts->cache.len + ts->chunk - 1)
		       & ~(ts->chunk - 1);

	/* fill unused part of data */
	memset(ts->cache.data + ts->cache.len, 0xff,
	       sizeof(ts->cache.data) - ts->cache.len);

	while (max_iterations--) {
		/* wrap around */
		if (off >= ts->size)
			off = 0;

		/* new block? */
		if (!(off & (ts->erasesize - 1))) {
			if (flash_block_isbad(ts, off)) {
				/* skip this block */
				off += ts->erasesize;
				continue;
			}

			if (unlikely(flash_erase(ts, off))) {
				/* skip this block */
				off += ts->erasesize;
				continue;
			}
		}

		/* write and read back to veryfy */
		if (flash_write(ts, off, &ts->cache, size) ||
		    flash_read(ts, off, &ts->cache_tmp_verify, size)) {
			/* hmm, probably unclean block, skip it for now */
			off = (off + ts->erasesize) & ~(ts->erasesize - 1);
			continue;
		}

		/* compare */
		if (memcmp(&ts->cache, &ts->cache_tmp_verify, size)) {
			printf(DRV_NAME
			       ": record v%u read mismatch @ 0x%08x\n",
				(unsigned)ts->cache.version, (unsigned)off);
			/* skip this block for now */
			off = (off + ts->erasesize) & ~(ts->erasesize - 1);
			continue;
		}

		/* for new block, erase the previous block after write done,
		 * it's to speed up flash_ts_scan
		 */
		if (!(off & (ts->erasesize - 1))) {
			loff_t pre_block_base = ts->offset & ~(ts->erasesize - 1);
			loff_t cur_block_base = off & ~(ts->erasesize - 1);
			if (cur_block_base != pre_block_base)
				flash_erase(ts, pre_block_base);
		}
		ts->offset = off;
		printf(DRV_NAME ": record v%u commited @ 0x%08x\n",
		       (unsigned)ts->cache.version, (unsigned)off);
		return 0;
	}

	printf(DRV_NAME ": commit failure\n");
	return -EIO;
}

int flash_ts_set(const char *key, const char *value)
{
	struct flash_ts_priv *ts;
	size_t klen = strlen(key);
	size_t vlen = strlen(value);
	int res;
	char *p;

	ts = __flash_ts_get();
	if (unlikely(!ts))
		return -EINVAL;

	/* save current cache contents so we can restore it on failure */
	memcpy(&ts->cache_tmp_backup, &ts->cache, sizeof(ts->cache_tmp_backup));

	p = flash_ts_find(ts, key, klen);
	if (p) {
		/* we are replacing existing entry,
		 * empty value (vlen == 0) removes entry completely.
		 */
		size_t cur_len = strlen(p) + 1;
		size_t new_len = vlen ? klen + 1 + vlen + 1 : 0;

		if (cur_len != new_len) {
			/* we need to move stuff around */

			if ((ts->cache.len - cur_len) + new_len >
			     sizeof(ts->cache.data))
				goto no_space;

			memmove(p + new_len, p + cur_len,
				ts->cache.len - (p - ts->cache.data + cur_len));

			ts->cache.len = (ts->cache.len - cur_len) + new_len;
		} else if (!strcmp(p + klen + 1, value)) {
			/* skip update if new value is the same as the old one */
			res = 0;
			goto out;
		}

		if (vlen) {
			p += klen + 1;
			memcpy(p, value, vlen);
			p[vlen] = '\0';
		}
	} else {
		size_t len = klen + 1 + vlen + 1;

		/* don't do anything if value is empty */
		if (!vlen) {
			res = 0;
			goto out;
		}

		if (ts->cache.len + len > sizeof(ts->cache.data))
			goto no_space;

		/* add new entry at the end */
		p = ts->cache.data + ts->cache.len - 1;
		memcpy(p, key, klen);
		p += klen;
		*p++ = '=';
		memcpy(p, value, vlen);
		p += vlen;
		*p++ = '\0';
		*p = '\0';
		ts->cache.len += len;
	}

	++ts->cache.version;
	ts->cache.crc = flash_ts_crc(&ts->cache);
	res = flash_ts_commit(ts);
	if (unlikely(res))
		memcpy(&ts->cache, &ts->cache_tmp_backup, sizeof(ts->cache));
	goto out;

    no_space:
	printf(DRV_NAME ": no space left for '%s=%s'\n", key, value);
	res = -ENOSPC;
    out:
	__flash_ts_put(ts);

	return res;
}

void flash_ts_get(const char *key, char *value, unsigned int size)
{
	size_t klen = strlen(key);
	struct flash_ts_priv *ts;
	const char *p;

	*value = '\0';

	ts = __flash_ts_get();
	if (unlikely(!ts))
		return;

	p = flash_ts_find(ts, key, klen);
	if (p)
		strncpy(value, p + klen + 1, size);

	__flash_ts_put(ts);
}

int flash_ts_get_int(const char *key, int default_value)
{
	char value[16];
	int res;

	flash_ts_get(key, value, sizeof(value));
	if (!value[0])
		return default_value;

	res = (int)simple_strtoul(value, (char**)NULL, 0);
	return res;
}

static inline u32 flash_ts_check_header(const struct flash_ts *cache)
{
	if (cache->magic == FLASH_TS_MAGIC &&
	    cache->version &&
	    cache->len && cache->len <= sizeof(cache->data) &&
	    cache->crc == flash_ts_crc(cache) &&
	    /* check correct null-termination */
	    !cache->data[cache->len - 1] &&
	    (cache->len == 1 || !cache->data[cache->len - 2])) {
	    	/* all is good */
		return cache->version;
	}

	return 0;
}

static int flash_ts_scan(struct flash_ts_priv *ts)
{
	int res, good_blocks = 0;
	loff_t off = 0;

	do {
		/* new block ? */
		if (!(off & (ts->erasesize - 1))) {
			if (flash_block_isbad(ts, off)) {
				printf(DRV_NAME
			       	       ": skipping bad block @ 0x%08x\n",
			       	       (unsigned)off);
				off += ts->erasesize;
				continue;
			} else
				++good_blocks;
		}

		res = flash_read(ts, off, &ts->cache_tmp_verify,
				 sizeof(ts->cache_tmp_verify));
		if (!res) {
			u32 version =
			    flash_ts_check_header(&ts->cache_tmp_verify);
			if (version > ts->cache.version) {
				memcpy(&ts->cache, &ts->cache_tmp_verify,
				       sizeof(ts->cache));
				ts->offset = off;
			}
			if (0 == version &&
				is_blank(&ts->cache_tmp_verify,
					sizeof(ts->cache_tmp_verify))) {
				/* skip the whole block if chunk is blank */
				off = (off + ts->erasesize) & ~(ts->erasesize - 1);
			} else
				off += ts->chunk;
		} else {
			off += ts->chunk;
		}
	} while (off < ts->size);

	if (unlikely(!good_blocks)) {
		printf(DRV_NAME ": no good blocks\n");
		return -ENODEV;
	}

	if (unlikely(good_blocks < 2))
		printf(DRV_NAME ": less than 2 good blocks,"
				" reliability is not guaranteed\n");
	return 0;
}

/* Round-up to the next power-of-2,
 * from "Hacker's Delight" by Henry S. Warren.
 */
static inline u32 clp2(u32 x)
{
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

int flash_ts_init(int start_block, int blocks)
{
	struct flash_ts_priv *ts = &__ts_priv;
	int writesize = nand_page_size();
	int erasesize = nand_block_size();
	int res;

	/* make sure both page and block sizes are power-of-2
	 * (this will make chunk size determination simpler).
	 */
	if (unlikely(writesize < 0 || !is_power_of_2(writesize) ||
		     erasesize < 0 || !is_power_of_2(erasesize))) {
		printf(DRV_NAME ": unsupported flash geometry: erasesize=%d, writesize=%d\n",
			erasesize, writesize);
		return -ENODEV;
	}

	/* determine chunk size so it doesn't cross block boundary,
	 * is multiple of page size and there is no wasted space in a block.
	 * We assume page and block sizes are power-of-2.
	 */
	ts->chunk = clp2((sizeof(struct flash_ts) + writesize - 1) &
			 ~(writesize - 1));
	if (unlikely(ts->chunk > erasesize)) {
		printf(DRV_NAME ": NAND block size is too small\n");
		return -ENODEV;
	}

	if (writesize > MV_NAND_MAX_PAGE_SIZE) {
		printf(DRV_NAME ": NAND page size is too large\n");
		return -ENODEV;
	}

	/* write flash information */
	ts->size = blocks * erasesize;
	ts->base = start_block * erasesize;
	ts->erasesize = erasesize;
	ts->writesize = writesize;

	/* default empty state */
	ts->offset = ts->size - ts->chunk;
	ts->cache.magic = FLASH_TS_MAGIC;
	ts->cache.version = 0;
	ts->cache.len = 1;
	ts->cache.data[0] = '\0';
	ts->cache.crc = flash_ts_crc(&ts->cache);

	/* scan flash partition for the most recent record */
	res = flash_ts_scan(ts);
	if (unlikely(res))
		return res;

	if (ts->cache.version)
		printf(DRV_NAME ": v%u loaded from 0x%08x\n",
		       (unsigned)ts->cache.version, (unsigned)ts->offset);

	/* "enable" it */
	__ts = ts;

	return res;
}
