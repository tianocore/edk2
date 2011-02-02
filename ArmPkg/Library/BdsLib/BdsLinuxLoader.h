/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*  
*  This program and the accompanying materials                          
*  are licensed and made available under the terms and conditions of the BSD License         
*  which accompanies this distribution.  The full text of the license may be found at        
*  http://opensource.org/licenses/bsd-license.php                                            
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
*
**/

#ifndef __BDSLINUXLOADER_H
#define __BDSLINUXLOADER_H

#define ATAG_MAX_SIZE       0x4000
//PcdKernelParamsMaxMemorySize

/* ATAG : list of possible tags */
#define ATAG_NONE            0x00000000
#define ATAG_CORE            0x54410001
#define ATAG_MEM             0x54410002
#define ATAG_VIDEOTEXT       0x54410003
#define ATAG_RAMDISK         0x54410004
#define ATAG_INITRD2         0x54420005
#define ATAG_SERIAL          0x54410006
#define ATAG_REVISION        0x54410007
#define ATAG_VIDEOLFB        0x54410008
#define ATAG_CMDLINE         0x54410009
#define ATAG_ARM_MP_CORE     0x5441000A

// Some system addresses
// These should probably come from the platform header file or from pcd values
#define DRAM_BASE            0x10000000
#define ZIMAGE_LOAD_ADDRESS  (DRAM_BASE + 0x8000)
#define INITRD_LOAD_ADDRESS  (DRAM_BASE + 0x800000)

#define SIZE_1B              0x00000001
#define SIZE_2B              0x00000002
#define SIZE_4B              0x00000004
#define SIZE_8B              0x00000008
#define SIZE_16B             0x00000010
#define SIZE_32B             0x00000020
#define SIZE_64B             0x00000040
#define SIZE_128B            0x00000080
#define SIZE_256B            0x00000100
#define SIZE_512B            0x00000200
#define SIZE_1KB             0x00000400
#define SIZE_2KB             0x00000800
#define SIZE_4KB             0x00001000
#define SIZE_8KB             0x00002000
#define SIZE_16KB            0x00004000
#define SIZE_32KB            0x00008000
#define SIZE_64KB            0x00010000
#define SIZE_128KB           0x00020000
#define SIZE_256KB           0x00040000
#define SIZE_512KB           0x00080000
#define SIZE_1MB             0x00100000
#define SIZE_2MB             0x00200000
#define SIZE_4MB             0x00400000
#define SIZE_8MB             0x00800000
#define SIZE_16MB            0x01000000
#define SIZE_32MB            0x02000000
#define SIZE_64MB            0x04000000
#define SIZE_100MB           0x06400000
#define SIZE_128MB           0x08000000
#define SIZE_256MB           0x10000000
#define SIZE_512MB           0x20000000
#define SIZE_1GB             0x40000000
#define SIZE_2GB             0x80000000

/* structures for each atag */
struct atag_header {
  UINT32  size; /* length of tag in words including this header */
  UINT32  type;  /* tag type */
};

struct atag_core {
  UINT32  flags;
  UINT32  pagesize;
  UINT32  rootdev;
};

struct atag_mem {
  UINT32  size;
  UINTN  start;
};

struct atag_videotext {
  UINT8   x;
  UINT8   y;
  UINT16  video_page;
  UINT8   video_mode;
  UINT8   video_cols;
  UINT16  video_ega_bx;
  UINT8   video_lines;
  UINT8   video_isvga;
  UINT16  video_points;
};

struct atag_ramdisk {
  UINT32  flags;
  UINT32  size;
  UINTN  start;
};

struct atag_initrd2 {
  UINT32  start;
  UINT32  size;
};

struct atag_serialnr {
  UINT32  low;
  UINT32  high;
};

struct atag_revision {
  UINT32  rev;
};

struct atag_videolfb {
  UINT16  lfb_width;
  UINT16  lfb_height;
  UINT16  lfb_depth;
  UINT16  lfb_linelength;
  UINT32  lfb_base;
  UINT32  lfb_size;
  UINT8   red_size;
  UINT8   red_pos;
  UINT8   green_size;
  UINT8   green_pos;
  UINT8   blue_size;
  UINT8   blue_pos;
  UINT8   rsvd_size;
  UINT8   rsvd_pos;
};

struct atag_cmdline {
  CHAR8   cmdline[1];
};

struct atag {
  struct atag_header header;
  union {
    struct atag_core         core_tag;
    struct atag_mem          mem_tag;
    struct atag_videotext    videotext_tag;
    struct atag_ramdisk      ramdisk_tag;
    struct atag_initrd2      initrd2_tag;
    struct atag_serialnr     serialnr_tag;
    struct atag_revision     revision_tag;
    struct atag_videolfb     videolfb_tag;
    struct atag_cmdline      cmdline_tag;
  } body;
};

#define next_tag_address(t)     ((struct atag *)((UINT32)(t) + (((t)->header.size) << 2) ))
#define tag_size(type)  ((UINT32)((sizeof(struct atag_header) + sizeof(struct type)) >> 2))

STATIC struct atag *Params; /* used to point at the current tag */

#endif

