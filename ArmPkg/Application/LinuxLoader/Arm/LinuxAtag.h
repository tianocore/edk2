/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
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

#ifndef __LINUX_ATAG_H__
#define __LINUX_ATAG_H__

//
// ATAG Definitions
//

#define ATAG_MAX_SIZE        0x3000

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

#define next_tag_address(t)  ((LINUX_ATAG*)((UINT32)(t) + (((t)->header.size) << 2) ))
#define tag_size(type)       ((UINT32)((sizeof(LINUX_ATAG_HEADER) + sizeof(type)) >> 2))

typedef struct {
  UINT32  size; /* length of tag in words including this header */
  UINT32  type;  /* tag type */
} LINUX_ATAG_HEADER;

typedef struct {
  UINT32  flags;
  UINT32  pagesize;
  UINT32  rootdev;
} LINUX_ATAG_CORE;

typedef struct {
  UINT32  size;
  UINTN  start;
} LINUX_ATAG_MEM;

typedef struct {
  UINT8   x;
  UINT8   y;
  UINT16  video_page;
  UINT8   video_mode;
  UINT8   video_cols;
  UINT16  video_ega_bx;
  UINT8   video_lines;
  UINT8   video_isvga;
  UINT16  video_points;
} LINUX_ATAG_VIDEOTEXT;

typedef struct {
  UINT32  flags;
  UINT32  size;
  UINTN  start;
} LINUX_ATAG_RAMDISK;

typedef struct {
  UINT32  start;
  UINT32  size;
} LINUX_ATAG_INITRD2;

typedef struct {
  UINT32  low;
  UINT32  high;
} LINUX_ATAG_SERIALNR;

typedef struct {
  UINT32  rev;
} LINUX_ATAG_REVISION;

typedef struct {
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
} LINUX_ATAG_VIDEOLFB;

typedef struct {
  CHAR8   cmdline[1];
} LINUX_ATAG_CMDLINE;

typedef struct {
  LINUX_ATAG_HEADER header;
  union {
    LINUX_ATAG_CORE         core_tag;
    LINUX_ATAG_MEM          mem_tag;
    LINUX_ATAG_VIDEOTEXT    videotext_tag;
    LINUX_ATAG_RAMDISK      ramdisk_tag;
    LINUX_ATAG_INITRD2      initrd2_tag;
    LINUX_ATAG_SERIALNR     serialnr_tag;
    LINUX_ATAG_REVISION     revision_tag;
    LINUX_ATAG_VIDEOLFB     videolfb_tag;
    LINUX_ATAG_CMDLINE      cmdline_tag;
  } body;
} LINUX_ATAG;

#endif /* __LINUX_ATAG_H__ */
