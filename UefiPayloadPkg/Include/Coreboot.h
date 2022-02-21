/** @file
  Coreboot PEI module include file.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
 * This file is part of the libpayload project.
 *
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _COREBOOT_PEI_H_INCLUDED_
#define _COREBOOT_PEI_H_INCLUDED_

#if defined (_MSC_VER)
  #pragma warning( disable : 4200 )
#endif

#define DYN_CBMEM_ALIGN_SIZE  (4096)

#define IMD_ENTRY_MAGIC    (~0xC0389481)
#define CBMEM_ENTRY_MAGIC  (~0xC0389479)

struct imd_root_pointer {
  UINT32    magic;
  INT32     root_offset;
};

struct cbmem_entry {
  UINT32    magic;
  UINT32    start;
  UINT32    size;
  UINT32    id;
};

struct cbmem_root {
  UINT32                max_entries;
  UINT32                num_entries;
  UINT32                locked;
  UINT32                size;
  struct cbmem_entry    entries[0];
};

struct imd_entry {
  UINT32    magic;
  UINT32    start_offset;
  UINT32    size;
  UINT32    id;
};

struct imd_root {
  UINT32              max_entries;
  UINT32              num_entries;
  UINT32              flags;
  UINT32              entry_align;
  UINT32              max_offset;
  struct imd_entry    entries[0];
};

struct cbuint64 {
  UINT32    lo;
  UINT32    hi;
};

#define CB_HEADER_SIGNATURE  0x4F49424C

struct cb_header {
  UINT32    signature;
  UINT32    header_bytes;
  UINT32    header_checksum;
  UINT32    table_bytes;
  UINT32    table_checksum;
  UINT32    table_entries;
};

struct cb_record {
  UINT32    tag;
  UINT32    size;
};

#define CB_TAG_UNUSED  0x0000
#define CB_TAG_MEMORY  0x0001

struct cb_memory_range {
  struct cbuint64    start;
  struct cbuint64    size;
  UINT32             type;
};

#define CB_MEM_RAM          1
#define CB_MEM_RESERVED     2
#define CB_MEM_ACPI         3
#define CB_MEM_NVS          4
#define CB_MEM_UNUSABLE     5
#define CB_MEM_VENDOR_RSVD  6
#define CB_MEM_TABLE        16

struct cb_memory {
  UINT32                    tag;
  UINT32                    size;
  struct cb_memory_range    map[0];
};

#define CB_TAG_MAINBOARD  0x0003

struct cb_mainboard {
  UINT32    tag;
  UINT32    size;
  UINT8     vendor_idx;
  UINT8     part_number_idx;
  UINT8     strings[0];
};

#define CB_TAG_VERSION         0x0004
#define CB_TAG_EXTRA_VERSION   0x0005
#define CB_TAG_BUILD           0x0006
#define CB_TAG_COMPILE_TIME    0x0007
#define CB_TAG_COMPILE_BY      0x0008
#define CB_TAG_COMPILE_HOST    0x0009
#define CB_TAG_COMPILE_DOMAIN  0x000a
#define CB_TAG_COMPILER        0x000b
#define CB_TAG_LINKER          0x000c
#define CB_TAG_ASSEMBLER       0x000d

struct cb_string {
  UINT32    tag;
  UINT32    size;
  UINT8     string[0];
};

#define CB_TAG_SERIAL  0x000f

struct cb_serial {
  UINT32    tag;
  UINT32    size;
  #define CB_SERIAL_TYPE_IO_MAPPED      1
  #define CB_SERIAL_TYPE_MEMORY_MAPPED  2
  UINT32    type;
  UINT32    baseaddr;
  UINT32    baud;
  UINT32    regwidth;

  // Crystal or input frequency to the chip containing the UART.
  // Provide the board specific details to allow the payload to
  // initialize the chip containing the UART and make independent
  // decisions as to which dividers to select and their values
  // to eventually arrive at the desired console baud-rate.
  UINT32    input_hertz;

  // UART PCI address: bus, device, function
  // 1 << 31 - Valid bit, PCI UART in use
  // Bus << 20
  // Device << 15
  // Function << 12
  UINT32    uart_pci_addr;
};

#define CB_TAG_CONSOLE  0x00010

struct cb_console {
  UINT32    tag;
  UINT32    size;
  UINT16    type;
};

#define CB_TAG_CONSOLE_SERIAL8250  0
#define CB_TAG_CONSOLE_VGA         1 // OBSOLETE
#define CB_TAG_CONSOLE_BTEXT       2 // OBSOLETE
#define CB_TAG_CONSOLE_LOGBUF      3
#define CB_TAG_CONSOLE_SROM        4// OBSOLETE
#define CB_TAG_CONSOLE_EHCI        5

#define CB_TAG_FORWARD  0x00011

struct cb_forward {
  UINT32    tag;
  UINT32    size;
  UINT64    forward;
};

#define CB_TAG_FRAMEBUFFER  0x0012
struct cb_framebuffer {
  UINT32    tag;
  UINT32    size;

  UINT64    physical_address;
  UINT32    x_resolution;
  UINT32    y_resolution;
  UINT32    bytes_per_line;
  UINT8     bits_per_pixel;
  UINT8     red_mask_pos;
  UINT8     red_mask_size;
  UINT8     green_mask_pos;
  UINT8     green_mask_size;
  UINT8     blue_mask_pos;
  UINT8     blue_mask_size;
  UINT8     reserved_mask_pos;
  UINT8     reserved_mask_size;
};

#define CB_TAG_VDAT  0x0015
struct cb_vdat {
  UINT32    tag;
  UINT32    size; /* size of the entire entry */
  UINT64    vdat_addr;
  UINT32    vdat_size;
};

#define CB_TAG_TIMESTAMPS     0x0016
#define CB_TAG_CBMEM_CONSOLE  0x0017
#define CB_TAG_MRC_CACHE      0x0018
struct cb_cbmem_tab {
  UINT32    tag;
  UINT32    size;
  UINT64    cbmem_tab;
};

#define FMAP_STRLEN             32
#define CBMEM_ID_IMD_SMALL      0x53a11439
#define CBMEM_ID_FMAP           0x464d4150
#define CBMEM_ID_CBFS_RO_MCACHE 0x524d5346
#define MCACHE_MAGIC_FILE       0x454c4946
#define MCACHE_MAGIC_FULL       0x4c4c5546
#define MCACHE_MAGIC_END        0x444e4524
#define CBFS_METADATA_MAX_SIZE  256
#define CBFS_UNIVERSAL_PAYLOAD  "img/UniversalPayload"
#define CBFS_MCACHE_ALIGNMENT   sizeof (UINT32)

#pragma pack(1)
struct cbfs_file {
  char magic[8];
  UINT32 len;
  UINT32 type;
  UINT32 attributes_offset;
  UINT32 offset;
  char filename[0];
} ;

struct fmap_area {
  UINT32 offset;            /* offset relative to base */
  UINT32 size;              /* size in bytes */
  UINT8  name[FMAP_STRLEN]; /* descriptive name */
  UINT16 flags;             /* flags for this area */
};

struct fmap {
  UINT8  signature[8];      /* "__FMAP__" (0x5F5F464D41505F5F) */
  UINT8  ver_major;         /* major version */
  UINT8  ver_minor;         /* minor version */
  UINT64 base;              /* address of the firmware binary */
  UINT32 size;              /* size of firmware binary in bytes */
  UINT8  name[FMAP_STRLEN]; /* name of this firmware binary */
  UINT16 nareas;            /* number of areas */
  struct fmap_area areas[];
};

struct cbfs_payload_segment {
  UINT32 type;
  UINT32 compression;
  UINT32 offset;
  UINT64 load_addr;
  UINT32 len;
  UINT32 mem_len;
};

struct cbfs_payload {
  struct cbfs_payload_segment segments;
};
#pragma pack()

union cbfs_mdata {
  struct cbfs_file h;
  UINT8 raw[CBFS_METADATA_MAX_SIZE];
};

union mcache_entry {
  union cbfs_mdata file;
  struct {  /* These fields exactly overlap file.h.magic */
    UINT32 magic;
    UINT32 offset;
  };
};

enum cbfs_payload_segment_type {
  PAYLOAD_SEGMENT_CODE   = 0x434F4445,  /* BE: 'CODE' */
  PAYLOAD_SEGMENT_DATA   = 0x44415441,  /* BE: 'DATA' */
  PAYLOAD_SEGMENT_BSS    = 0x42535320,  /* BE: 'BSS ' */
  PAYLOAD_SEGMENT_PARAMS = 0x50415241,  /* BE: 'PARA' */
  PAYLOAD_SEGMENT_ENTRY  = 0x454E5452,  /* BE: 'ENTR' */
};

#define MEM_RANGE_COUNT(_rec) \
  (((_rec)->size - sizeof(*(_rec))) / sizeof((_rec)->map[0]))

#define MEM_RANGE_PTR(_rec, _idx) \
  (void *)(((UINT8 *) (_rec)) + sizeof(*(_rec)) \
    + (sizeof((_rec)->map[0]) * (_idx)))

typedef struct cb_memory CB_MEMORY;

#endif // _COREBOOT_PEI_H_INCLUDED_
