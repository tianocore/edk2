/** @file
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef __SD_BOOT_CONFIG_H__
#define __SD_BOOT_CONFIG_H__

#define SD_BOOT_CONFIG_FORMSET_GUID \
  {0x68558cfa, 0xaae6, 0x430b, {0xab, 0x24, 0x55, 0xb6, 0x92, 0x59, 0xb7, 0xc6}}

extern EFI_GUID  gSdBootConfigFormSetGuid;

#define ADD_ENTRY_NO   0
#define ADD_ENTRY_YES  1

typedef struct {
  UINT8    AddEntry;
} SD_BOOT_CONFIG_DATA;

#endif
