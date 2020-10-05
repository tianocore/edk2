/** @file
  This file defines the hob structure for Tcg Physical Presence Interface.

  Copyright (c) 2020, 9elements Agency GmbH<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __TCG_PHYSICAL_PRESENCE_GUID_H__
#define __TCG_PHYSICAL_PRESENCE_GUID_H__

///
/// TCG Physical Presence Information GUID
///
extern EFI_GUID gEfiTcgPhysicalPresenceInfoHobGuid;

typedef struct {
  UINT32 PpiAddress;
  UINT8  TpmVersion;
  UINT8  PpiVersion;
} TCG_PHYSICAL_PRESENCE_INFO;

#define UEFIPAYLOAD_TPM_VERSION_UNSPEC    0
#define UEFIPAYLOAD_TPM_VERSION_1_2       1
#define UEFIPAYLOAD_TPM_VERSION_2         2

#define UEFIPAYLOAD_TPM_PPI_VERSION_NONE  0
#define UEFIPAYLOAD_TPM_PPI_VERSION_1_30  1

#endif
