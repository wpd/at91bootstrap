/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2018, iRobot Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * iRobot's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "common.h"
#include "board.h"
#include "nand.h"
#include "pmecc.h"
#include "ubi-api.h"
#include "debug.h"
#include "nandflash.h"
#include "ubi/ubi.h"
#include "string.h"
#include "memalign.h"
#include "fdt.h"
#include "autoconf.h"

//#define DEBUG_UBIFLASH
#ifdef DEBUG_UBIFLASH
#undef printf
#define printf(fmt, ...) dbg_printf(fmt "\n", ##__VA_ARGS__)
#else
#undef printf
#define printf(...)
#endif

extern struct ubi_device *ubi_devices[];
static struct ubi_device *ubi;

#if defined(CONFIG_LOAD_LINUX) || defined(CONFIG_LOAD_ANDROID)
static struct ubi_volume *ubi_find_volume(char *volume)
{
	struct ubi_volume *vol = NULL;
	int i;

	for (i = 0; i < ubi->vtbl_slots; i++) {
		vol = ubi->volumes[i];
		if (vol && !strcmp(vol->name, volume))
			return vol;
	}

	printf("Volume %s not found!\n", volume);
	return NULL;
}

int
ubi_volume_read(char *volume, char *buf, size_t size)
{
	int err, lnum, off, len, tbuf_size;
	void *tbuf;
	unsigned long long tmp;
	struct ubi_volume *vol;
	loff_t offp = 0;

	vol = ubi_find_volume(volume);

	if (vol == NULL)
		return ENODEV;

	if (vol->updating) {
		printf("updating");
		return EBUSY;
	}
	if (vol->upd_marker) {
		printf("damaged volume, update marker is set");
		return EBADF;
	}
	if (offp == vol->used_bytes)
		return 0;

	if (size == 0) {
		printf("No size specified -> Using max size (%lld)\n", vol->used_bytes);
		size = vol->used_bytes;
	}

	printf("Read %d bytes from volume %s to %x\n", size, volume, buf);

	if (vol->corrupted)
		printf("read from corrupted volume %d", vol->vol_id);
	if (offp + size > vol->used_bytes)
		size = vol->used_bytes - offp;

	tbuf_size = vol->usable_leb_size;
	if (size < tbuf_size)
		tbuf_size = ALIGN(size, ubi->min_io_size);
	tbuf = malloc_cache_aligned(tbuf_size);
	if (!tbuf) {
		printf("NO MEM\n");
		return ENOMEM;
	}
	len = size > tbuf_size ? tbuf_size : size;

	tmp = offp;
	off = do_div(tmp, vol->usable_leb_size);
	lnum = tmp;

	do {
		if (off + len >= vol->usable_leb_size)
			len = vol->usable_leb_size - off;

		err = ubi_eba_read_leb(ubi, vol, lnum, tbuf, off, len, 0);
		if (err) {
			printf("read err %x\n", err);
			err = -err;
			break;
		}
		off += len;
		if (off == vol->usable_leb_size) {
			lnum += 1;
			off -= vol->usable_leb_size;
		}

		size -= len;
		offp += len;

		memcpy(buf, tbuf, len);

		buf += len;
		len = size > tbuf_size ? tbuf_size : size;
	} while (size);

	free(tbuf);
	return err;
}

static int update_image_length(char *volume_name,
			       unsigned char *dest,
			       unsigned char flag)
{
	int ret;

	ret = ubi_volume_read(volume_name, dest, 4096);

	if (ret)
		return -1;

	if (flag == KERNEL_IMAGE)
		return kernel_size(dest);
#ifdef CONFIG_OF_LIBFDT
	else {
		ret = check_dt_blob_valid((void *)dest);
		if (!ret)
			return of_get_dt_total_size((void *)dest);
	}
#endif
	return -1;
}
#endif

int load_ubiflash(struct image_info *image)
{
	struct nand_info nand;
	int ret;

	nandflash_hw_init();

	if (nandflash_get_type(&nand))
		return -1;

#ifdef CONFIG_USE_PMECC
	if (init_pmecc(&nand))
		return -1;
#endif
	ubi_glue_init(&nand);

#ifdef CONFIG_ENABLE_SW_ECC
	dbg_info("NAND: Using Software ECC\n");
#endif

	if (ubi_mtd_param_parse("mtd=0" CONFIG_UBI_MTD_PARAM_PARSE_STRING, NULL) != 0) {
		dbg_printf("ubi_mtd_param_parse unsuccessful\n");
		return -1;
	}
	if (ubi_init() != 0) {
	    dbg_printf("ubi_init unsuccessful\n");
#ifdef DEBUG_UBIFLASH
	    // This is helpful during debugging/development
	    dbg_printf("Reseting...\n");
	    *(int *)0xf8048000 = 0xa5000001;
#else
	    return -1;
#endif
	}

	dbg_printf("ubi_init successful\n");
	ubi = ubi_devices[0];
#ifdef DEBUG_UBIFLASH
	display_volume_info();
#endif
	
#if defined(CONFIG_LOAD_LINUX) || defined(CONFIG_LOAD_ANDROID)
	int length = update_image_length(CONFIG_UBI_KERNEL_VOLUME, image->dest, KERNEL_IMAGE);

	if (length == -1)
		return -1;

	image->length = length;
#endif

	dbg_printf("UBI: Image: Copy %x bytes from \"%s\" volume to %x\n",
		   image->length, CONFIG_UBI_KERNEL_VOLUME, image->dest);

	ret = ubi_volume_read(CONFIG_UBI_KERNEL_VOLUME, image->dest, image->length);

#ifdef CONFIG_OF_LIBFDT
	length = update_image_length(CONFIG_UBI_DTB_VOLUME, image->of_dest, DT_BLOB);
	if (length == -1)
		return -1;

	image->of_length = length;

	dbg_printf("UBI: dt blob: Copy %x bytes from \"%s\" volume to %x\n",
		   image->of_length, CONFIG_UBI_DTB_VOLUME, image->of_dest);

	ret = ubi_volume_read(CONFIG_UBI_DTB_VOLUME, image->of_dest, image->of_length);

	if (ret)
		return ret;
#endif

	return 0;
}
