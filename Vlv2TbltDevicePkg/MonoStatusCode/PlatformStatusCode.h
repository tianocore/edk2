/*++

  Copyright (c) 2004  - 2017, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent
                                                                                   

Module Name:

  PlatformStatusCode.h

Abstract:

  Contains Platform specific implementations required to use status codes.

--*/

#ifndef _PLATFORM_STATUS_CODE_H_
#define _PLATFORM_STATUS_CODE_H_


#define CONFIG_PORT0    0x4E
#define INDEX_PORT0     0x4E
#define DATA_PORT0      0x4F
#define PCI_IDX        0xCF8
#define PCI_DAT        0xCFC

#include "MonoStatusCode.h"
#ifndef _PEI_PORT_80_STATUS_CODE_H_
#define _PEI_PORT_80_STATUS_CODE_H_



//
// Status code reporting function
//
EFI_STATUS
Port80ReportStatusCode (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE           CodeType,
  IN EFI_STATUS_CODE_VALUE          Value,
  IN UINT32                         Instance,
  IN CONST EFI_GUID                 * CallerId,
  IN CONST EFI_STATUS_CODE_DATA     * Data OPTIONAL
  );

#endif

#ifndef _PEI_SERIAL_STATUS_CODE_LIB_H_
#define _PEI_SERIAL_STATUS_CODE_LIB_H_


#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/StatusCodeDataTypeDebug.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>

//
// Initialization function
//
VOID
SerialInitializeStatusCode (
  VOID
  );

//
// Status code reporting function
//
EFI_STATUS
SerialReportStatusCode (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN CONST EFI_GUID                 * CallerId,
  IN CONST EFI_STATUS_CODE_DATA     * Data OPTIONAL
  );

#endif

extern EFI_PEI_PROGRESS_CODE_PPI    mStatusCodePpi;
extern EFI_PEI_PPI_DESCRIPTOR mPpiListStatusCode;
#define EFI_SIGNATURE_16(A, B)        ((A) | (B << 8))
#define EFI_SIGNATURE_32(A, B, C, D)  (EFI_SIGNATURE_16 (A, B) | (EFI_SIGNATURE_16 (C, D) << 16))
#define STATUSCODE_PEIM_SIGNATURE EFI_SIGNATURE_32 ('p', 's', 't', 'c')

typedef struct {
  UINT32                    Signature;
  EFI_FFS_FILE_HEADER       *FfsHeader;
  EFI_PEI_NOTIFY_DESCRIPTOR StatusCodeNotify;
} STATUSCODE_CALLBACK_STATE_INFORMATION;

#pragma pack(1)
typedef struct {
  UINT16  Limit;
  UINT32  Base;
} GDT_DSCRIPTOR;
#pragma pack()

#define STATUSCODE_PEIM_FROM_THIS(a) \
  BASE_CR ( \
  a, \
  STATUSCODE_CALLBACK_STATE_INFORMATION, \
  StatusCodeNotify \
  )

VOID
EFIAPI
PlatformInitializeStatusCode (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  );


//
// Function declarations
//
/**
  Install Firmware Volume Hob's once there is main memory

  @param PeiServices        General purpose services available to every PEIM.
  @param NotifyDescriptor   Not Used
  @param Ppi                Not Used

  @retval Status            EFI_SUCCESS if the interface could be successfully
                            installed

**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

#endif
