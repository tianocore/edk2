/** @file
  Header file for NV data structure definition.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RAM_DISK_NVDATA_H_
#define _RAM_DISK_NVDATA_H_

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/RamDiskHii.h>

#define MAIN_FORM_ID                     0x1000
#define MAIN_GOTO_FILE_EXPLORER_ID       0x1001
#define MAIN_REMOVE_RD_QUESTION_ID       0x1002
#define MAIN_LABEL_LIST_START            0x1003
#define MAIN_LABEL_LIST_END              0x1004
#define MAIN_CHECKBOX_QUESTION_ID_START  0x1100

#define CREATE_RAW_RAM_DISK_FORM_ID         0x2000
#define CREATE_RAW_SIZE_QUESTION_ID         0x2001
#define CREATE_RAW_SUBMIT_QUESTION_ID       0x2002
#define CREATE_RAW_DISCARD_QUESTION_ID      0x2003
#define CREATE_RAW_MEMORY_TYPE_QUESTION_ID  0x2004

#define RAM_DISK_BOOT_SERVICE_DATA_MEMORY  0x00
#define RAM_DISK_RESERVED_MEMORY           0x01
#define RAM_DISK_MEMORY_TYPE_MAX           0x02

typedef struct {
  //
  // The size of the RAM disk to be created.
  //
  UINT64    Size;
  //
  // Selected RAM Disk Memory Type
  //
  UINT8     MemType;
} RAM_DISK_CONFIGURATION;

#endif
