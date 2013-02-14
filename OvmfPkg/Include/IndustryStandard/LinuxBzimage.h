/** @file

  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __LINUX_BZIMAGE_H__
#define __LINUX_BZIMAGE_H__

#define BOOTSIG			0x1FE
#define SETUP_HDR		0x53726448	/* 0x53726448 == "HdrS" */

#define E820_RAM		1
#define E820_RESERVED		2
#define E820_ACPI		3
#define E820_NVS		4
#define E820_UNUSABLE		5

#pragma pack(1)

struct setup_header {
	UINT8 setup_secs;	/* Sectors for setup code */
	UINT16 root_flags;
	UINT32 sys_size;
	UINT16 ram_size;
	UINT16 video_mode;
	UINT16 root_dev;
	UINT16 signature;	/* Boot signature */
	UINT16 jump;
	UINT32 header;
	UINT16 version;
	UINT16 su_switch;
	UINT16 setup_seg;
	UINT16 start_sys;
	UINT16 kernel_ver;
	UINT8 loader_id;
	UINT8 load_flags;
	UINT16 movesize;
	UINT32 code32_start;	/* Start of code loaded high */
	UINT32 ramdisk_start;	/* Start of initial ramdisk */
	UINT32 ramdisk_len;	/* Lenght of initial ramdisk */
	UINT32 bootsect_kludge;
	UINT16 heap_end;
	UINT8 ext_loader_ver;  /* Extended boot loader version */
	UINT8 ext_loader_type; /* Extended boot loader ID */
	UINT32 cmd_line_ptr;   /* 32-bit pointer to the kernel command line */
	UINT32 ramdisk_max;    /* Highest legal initrd address */
	UINT32 kernel_alignment; /* Physical addr alignment required for kernel */
	UINT8 relocatable_kernel; /* Whether kernel is relocatable or not */
	UINT8 min_alignment;
	UINT16 xloadflags;
	UINT32 cmdline_size;
	UINT32 hardware_subarch;
	UINT64 hardware_subarch_data;
	UINT32 payload_offset;
	UINT32 payload_length;
	UINT64 setup_data;
	UINT64 pref_address;
	UINT32 init_size;
	UINT32 handover_offset;
};

struct efi_info {
	UINT32 efi_loader_signature;
	UINT32 efi_systab;
	UINT32 efi_memdesc_size;
	UINT32 efi_memdesc_version;
	UINT32 efi_memmap;
	UINT32 efi_memmap_size;
	UINT32 efi_systab_hi;
	UINT32 efi_memmap_hi;
};

struct e820_entry {
	UINT64 addr;		/* start of memory segment */
	UINT64 size;		/* size of memory segment */
	UINT32 type;		/* type of memory segment */
};

struct screen_info {
        UINT8  orig_x;           /* 0x00 */
        UINT8  orig_y;           /* 0x01 */
        UINT16 ext_mem_k;        /* 0x02 */
        UINT16 orig_video_page;  /* 0x04 */
        UINT8  orig_video_mode;  /* 0x06 */
        UINT8  orig_video_cols;  /* 0x07 */
        UINT8  flags;            /* 0x08 */
        UINT8  unused2;          /* 0x09 */
        UINT16 orig_video_ega_bx;/* 0x0a */
        UINT16 unused3;          /* 0x0c */
        UINT8  orig_video_lines; /* 0x0e */
        UINT8  orig_video_isVGA; /* 0x0f */
        UINT16 orig_video_points;/* 0x10 */

	/* VESA graphic mode -- linear frame buffer */
        UINT16 lfb_width;        /* 0x12 */
        UINT16 lfb_height;       /* 0x14 */
        UINT16 lfb_depth;        /* 0x16 */
        UINT32 lfb_base;         /* 0x18 */
        UINT32 lfb_size;         /* 0x1c */
        UINT16 cl_magic, cl_offset; /* 0x20 */
	UINT16 lfb_linelength;   /* 0x24 */
        UINT8  red_size;         /* 0x26 */
        UINT8  red_pos;          /* 0x27 */
        UINT8  green_size;       /* 0x28 */
	UINT8  green_pos;        /* 0x29 */
        UINT8  blue_size;        /* 0x2a */
	UINT8  blue_pos;         /* 0x2b */
        UINT8  rsvd_size;        /* 0x2c */
        UINT8  rsvd_pos;         /* 0x2d */
        UINT16 vesapm_seg;       /* 0x2e */
	UINT16 vesapm_off;       /* 0x30 */
        UINT16 pages;            /* 0x32 */
        UINT16 vesa_attributes;  /* 0x34 */
        UINT32 capabilities;     /* 0x36 */
        UINT8  _reserved[6];     /* 0x3a */
};

struct boot_params {
	struct screen_info screen_info;
	UINT8 apm_bios_info[0x14];
	UINT8 _pad2[4];
	UINT64 tboot_addr;
	UINT8 ist_info[0x10];
	UINT8 _pad3[16];
	UINT8 hd0_info[16];
	UINT8 hd1_info[16];
	UINT8 sys_desc_table[0x10];
	UINT8 olpc_ofw_header[0x10];
	UINT8 _pad4[128];
	UINT8 edid_info[0x80];
	struct efi_info efi_info;
	UINT32 alt_mem_k;
	UINT32 scratch;
	UINT8 e820_entries;
	UINT8 eddbuf_entries;
	UINT8 edd_mbr_sig_buf_entries;
	UINT8 _pad6[6];
	struct setup_header hdr;
	UINT8 _pad7[0x290-0x1f1-sizeof(struct setup_header)];
	UINT32 edd_mbr_sig_buffer[16];
	struct e820_entry e820_map[128];
	UINT8 _pad8[48];
	UINT8 eddbuf[0x1ec];
	UINT8 _pad9[276];
};

typedef struct {
	UINT16 limit;
	UINT64 *base;
} dt_addr_t;

#pragma pack()

extern EFI_STATUS setup_graphics(struct boot_params *buf);

#endif /* __LINUX_BZIMAGE_H__ */
