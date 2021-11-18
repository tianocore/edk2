/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EXTRA_DATA_H_
#define EXTRA_DATA_H_

extern GUID  gUniversalPayloadExtraDataGuid;

#pragma pack(1)

typedef struct {
  CHAR8                   Identifier[16];
  EFI_PHYSICAL_ADDRESS    Base;
  UINT64                  Size;
} UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY;

typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER      Header;
  UINT32                                Count;
  UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY    Entry[0];
} UNIVERSAL_PAYLOAD_EXTRA_DATA;

#pragma pack()

#define UNIVERSAL_PAYLOAD_EXTRA_DATA_REVISION  1

#endif
