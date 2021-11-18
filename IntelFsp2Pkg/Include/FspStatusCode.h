/** @file
  Intel FSP status code definition

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_STATUS_CODE_H_
#define _FSP_STATUS_CODE_H_

//
// FSP API - 4 BITS
//
#define FSP_STATUS_CODE_TEMP_RAM_INIT                 0xF000
#define FSP_STATUS_CODE_MEMORY_INIT                   0xD000
#define FSP_STATUS_CODE_TEMP_RAM_EXIT                 0xB000
#define FSP_STATUS_CODE_SILICON_INIT                  0x9000
#define FSP_STATUS_CODE_POST_PCIE_ENUM_NOTIFICATION   0x6000
#define FSP_STATUS_CODE_READY_TO_BOOT_NOTIFICATION    0x4000
#define FSP_STATUS_CODE_END_OF_FIRMWARE_NOTIFICATION  0x2000

//
// MODULE - 4 BITS
//
#define FSP_STATUS_CODE_GFX_PEIM             0x0700
#define FSP_STATUS_CODE_COMMON_CODE          0x0800
#define FSP_STATUS_CODE_SILICON_COMMON_CODE  0x0900
#define FSP_STATUS_CODE_SYSTEM_AGENT         0x0A00
#define FSP_STATUS_CODE_PCH                  0x0B00
#define FSP_STATUS_CODE_CPU                  0x0C00
#define FSP_STATUS_CODE_MRC                  0x0D00
#define FSP_STATUS_CODE_ME_BIOS              0x0E00
//
// Individual Codes - 1 BYTE
//
#define FSP_STATUS_CODE_API_ENTRY  0x0000
#define FSP_STATUS_CODE_API_EXIT   0x007F

#endif
