/** @file
Header file for SMM S3 Handler Driver.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#ifndef _ACPI_SMM_DRIVER_H
#define _ACPI_SMM_DRIVER_H
//
// Include files
//
//
// Driver Consumed Protocol Prototypes
//
#include <Protocol/SmmSxDispatch2.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/FirmwareVolume.h>
#include <Protocol/GlobalNvsArea.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/Spi.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/PcdLib.h>
#include <Library/LockBoxLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/S3IoLib.h>
#include <Library/S3BootScriptLib.h>
#include <Guid/Acpi.h>
#include <Guid/GlobalVariable.h>
#include <Library/SmmServicesTableLib.h>
#include <Guid/SmramMemoryReserve.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/HobLib.h>
#include <QNCAccess.h>
#include <Library/QNCAccessLib.h>
#include <Library/IntelQNCLib.h>
#include <Library/PlatformHelperLib.h>
#include <Library/PlatformPcieHelperLib.h>
#include "Platform.h"
#include <IndustryStandard/Pci22.h>

#define EFI_ACPI_ACPI_ENABLE          0xA0
#define EFI_ACPI_ACPI_DISABLE         0xA1

#define R_IOPORT_CMOS_STANDARD_INDEX            0x70
#define R_IOPORT_CMOS_STANDARD_DATA             0x71
#define RTC_ADDRESS_REGISTER_C    12
#define RTC_ADDRESS_REGISTER_D    13

#define PCI_DEVICE(Bus, Dev, Func)  \
          Bus, Dev, Func

#define PCI_REG_MASK(Byte0, Byte1, Byte2, Byte3, Byte4, Byte5, Byte6, Byte7) \
          Byte0, Byte1, Byte2, Byte3, Byte4, Byte5, Byte6, Byte7

#define PCI_DEVICE_END    0xFF

//
// Related data structures definition
//
typedef struct _EFI_ACPI_SMM_DEV {

  //
  // Parent dispatch driver returned sleep handle
  //
  EFI_HANDLE  S3SleepEntryHandle;

  EFI_HANDLE  S4SleepEntryHandle;

  EFI_HANDLE  S1SleepEntryHandle;

  EFI_HANDLE  S5SoftOffEntryHandle;

  EFI_HANDLE  EnableAcpiHandle;

  EFI_HANDLE  DisableAcpiHandle;

  EFI_HANDLE  PpCallbackHandle;

  EFI_HANDLE  MorCallbackHandle;

  //
  // QNC Power Management I/O register base
  //
  UINT32      QncPmBase;

  //
  // QNC General Purpose Event0 register base
  //
  UINT32      QncGpe0Base;

  UINT32      BootScriptSaved;

} EFI_ACPI_SMM_DEV;

//
// Prototypes
//
EFI_STATUS
InitPlatformAcpiSmm (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable,
  IN  OUT VOID            *CommBuffer,
  IN  OUT UINTN           *CommBufferSize
  );

EFI_STATUS
SxSleepEntryCallBack (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  CONST VOID                    *DispatchContext,

  IN  OUT VOID                      *CommBuffer,
  IN  OUT UINTN                     *CommBufferSize
  );

EFI_STATUS
DisableAcpiCallback (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  CONST VOID                    *DispatchContext,
  IN  OUT VOID                      *CommBuffer,
  IN  OUT UINTN                     *CommBufferSize
  );

EFI_STATUS
EnableAcpiCallback (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  CONST VOID                    *DispatchContext,
  IN  OUT VOID                      *CommBuffer,
  IN  OUT UINTN                     *CommBufferSize
  );

EFI_STATUS
RegisterToDispatchDriver (
  VOID
  );

EFI_STATUS
GetAllQncPmBase (
  IN EFI_SMM_SYSTEM_TABLE2       *Smst
  );

EFI_STATUS
SaveRuntimeScriptTable (
  IN EFI_SMM_SYSTEM_TABLE2       *Smst
  );

EFI_STATUS
RestoreQncS3SwCallback (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  CONST VOID                    *DispatchContext,
  IN  OUT VOID                      *CommBuffer,
  IN  OUT UINTN                     *CommBufferSize
  );

extern EFI_GUID gQncS3CodeInLockBoxGuid;
extern EFI_GUID gQncS3ContextInLockBoxGuid;

#endif
