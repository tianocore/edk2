/** @file
  EFI MP information protocol provides a lightweight MP_SERVICES_PROTOCOL.

  MP information protocol only provides static information of MP processor.

  If SwitchBSP or Enable/DisableAP in MP service is called between the HOB
  production and HOB consumption, EFI_PROCESSOR_INFORMATION.StatusFlag field
  in this HOB may be invalidated.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MP_INFORMATION2_H_
#define MP_INFORMATION2_H_

#include <Protocol/MpService.h>
#include <PiPei.h>
#include <Ppi/SecPlatformInformation.h>

#define MP_INFORMATION2_HOB_REVISION  1

#define MP_INFORMATION2_GUID \
  { \
    0x417a7f64, 0xf4e9, 0x4b32, {0x84, 0x6a, 0x5c, 0xc4, 0xd8, 0x62, 0x18, 0x79}  \
  }

typedef struct {
  EFI_PROCESSOR_INFORMATION    ProcessorInfo;
  UINT8                        CoreType;
  UINT8                        Reserved[7];
  //
  // Add more fields in future
  //
} MP_INFORMATION2_ENTRY;

typedef struct {
  UINT16                   NumberOfProcessors;
  UINT16                   EntrySize;
  UINT8                    Version;
  UINT8                    Reserved[3];
  UINT64                   ProcessorIndex;
  MP_INFORMATION2_ENTRY    Entry[0];
} MP_INFORMATION2_HOB_DATA;

//
// Producer of MP_INFORMATION2_HOB_DATA should assign sizeof (MP_INFORMATION2_ENTRY) to MP_INFORMATION2_HOB_DATA.EntrySize
// Consumer of MP_INFORMATION2_HOB_DATA should use below macro or similar logic to get the individual entry
// as the entry structure might be updated to include more information.
//
#define GET_MP_INFORMATION_ENTRY(MpInfoHobData, Index) \
    (MP_INFORMATION2_ENTRY *)((UINTN)&((MP_INFORMATION2_HOB_DATA *)(MpInfoHobData))->Entry + (MpInfoHobData)->EntrySize * Index)

extern EFI_GUID  gMpInformation2HobGuid;

#endif
