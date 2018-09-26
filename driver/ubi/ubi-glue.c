#include "common.h"
#include "mtd.h"
#include "nand.h"
#include "nandflash.h"
#include "ubi.h"
#include "memalign.h"
#include "autoconf.h"

#ifdef CONFIG_DEBUG
extern int dbg_printf(const char *fmt_str, ...);
extern void dbg_hexdump(const unsigned char *buf,
			unsigned int size, unsigned int width);
#else
#define dbg_printf(...)
#define dbg_hexdump(...)
#endif

extern struct ubi_device *ubi_devices[];

static int
glue_block_is_bad(struct mtd_info *mtd, loff_t ofs);

static struct mtd_info info = {
	.size = CONFIG_UBI_PARTITION_END - CONFIG_UBI_PARTITION_START,

	.writesize = 1 << 11,
	.writesize_shift = 11,
	.writesize_mask = (1 << 11) - 1,

	.writebufsize = 2048,

	.erasesize = 1 << 17,
	.erasesize_shift = 17,	// 128K
	.erasesize_mask = (1 << 17) - 1,

	._block_isbad = glue_block_is_bad,
};


static struct nand_info *nand_ptr;
static unsigned char *page_buf;

void
ubi_glue_init(struct nand_info *nand) {
	nand_ptr = nand;
	if (page_buf == 0) {
		page_buf = malloc_cache_aligned(nand->pagesize + nand->oobsize);
	}
}		
	
struct mtd_info *get_mtd_device(struct mtd_info *mtd, int num) { return &info; }
struct mtd_info *
get_mtd_device_nm(const char *name) {
#if 0
	dbg_printf("%s\n", __func__);
	dbg_printf("%s, info = {\n  .size = %x,\n  .writesize = %d,\n  .writebufsize = %d,\n  .erasesize = %x,\n  .erasesize_shift = %d,\n  .index = %d,\n};\n",
		   __func__,
		   info.size,
		   info.writesize,
		   info.writebufsize,
		   info.erasesize,
		   info.erasesize_shift,
		   info.index);
#endif
	return &info;
}

int mtd_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	return glue_block_is_bad(mtd, ofs + CONFIG_UBI_PARTITION_START);
}

uint64_t mtd_get_device_size(const struct mtd_info *mtd)
{
	return CONFIG_UBI_FLASH_SIZE;
}

static int
glue_block_is_bad(struct mtd_info *mtd, loff_t ofs)
{
	return nand_check_badblock(nand_ptr, mtd_div_by_eb((int)ofs, &info), page_buf) != 0;
}

int
mtd_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
	unsigned length = len;
	unsigned char *buffer = buf;
	unsigned flash_offset = from + CONFIG_UBI_PARTITION_START;
	unsigned block_remaining = info.erasesize - mtd_mod_by_eb(flash_offset, &info);
	unsigned readsize;
	unsigned block;
	unsigned page;
	unsigned start_page, end_page;
	unsigned numpages;
	unsigned offsetpage;
	int ret;
	
	block = mtd_div_by_eb(flash_offset, &info);
	start_page = mtd_mod_by_eb(flash_offset, &info);
	start_page = mtd_div_by_ws(start_page, &info);

	if (from + len > CONFIG_UBI_PARTITION_END) {
		return -1;
	}
	
	while (length > 0) {
		/* read a buffer corresponding to a block */
		if (length < block_remaining)
			readsize = length;
		else
			readsize = block_remaining;

		/* adjust the number of pages to read */
		numpages = mtd_div_by_ws(readsize, &info);
		offsetpage = mtd_mod_by_ws(readsize, &info);
		if (offsetpage)
			numpages++;
		end_page = start_page + numpages;

		for (page = start_page; page < end_page; page++) {
			ret = nand_read_page(nand_ptr, block, page,
						ZONE_DATA, page_buf);
			if (ret)
				return -1;

			memcpy(buffer, page_buf, info.writesize);
			buffer += info.writesize;
		}
		length -= readsize;

		block++;
		start_page = 0;
		block_remaining = nand_ptr->blocksize;
	}

	if (*retlen) {
		*retlen = len;
	}
	return 0;
}

void panic(const char *fmt, ...) {dbg_printf("%s\n", __func__); while (1); }
void print_hex_dump(const char *prefix_str, int prefix_type, int rowsize,
		    int groupsize, const void *buf, size_t len, bool ascii)
{
	dbg_hexdump(buf, (int)len, 1);
}
void put_mtd_device(struct mtd_info *mtd) {}
void __div0(void) { while(1) ;}
