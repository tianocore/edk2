/** @file
  Master header file for SecCore.

  Copyright (c) 2008 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SEC_CORE_H_
#define _SEC_CORE_H_

#include <PiPei.h>

#include <Ppi/SecPlatformInformation2.h>
#include <Ppi/TemporaryRamDone.h>
#include <Ppi/SecPerformance.h>
#include <Ppi/PeiCoreFvLocation.h>

#include <Guid/FirmwarePerformance.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PlatformSecLib.h>
#include <Library/UefiCpuLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>

#define SEC_IDT_ENTRY_COUNT  34

typedef struct _SEC_IDT_TABLE {
  //
  // Reserved 8 bytes preceding IDT to store EFI_PEI_SERVICES**, since IDT base
  // address should be 8-byte alignment.
  // Note: For IA32, only the 4 bytes immediately preceding IDT is used to store
  // EFI_PEI_SERVICES**
  //
  UINT64            PeiService;
  UINT64            IdtTable[SEC_IDT_ENTRY_COUNT];
} SEC_IDT_TABLE;

/**
  TemporaryRamDone() disables the use of Temporary RAM. If present, this service is invoked
  by the PEI Foundation after the EFI_PEI_PERMANANT_MEMORY_INSTALLED_PPI is installed.

  @retval EFI_SUCCESS           Use of Temporary RAM was disabled.
  @retval EFI_INVALID_PARAMETER Temporary RAM could not be disabled.

**/
EFI_STATUS
EFIAPI
SecTemporaryRamDone (
  VOID
  );

/**
  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param SizeOfRam           Size of the temporary memory available for use.
  @param TempRamBase         Base address of temporary ram
  @param BootFirmwareVolume  Base address of the Boot Firmware Volume.
**/
VOID
NORETURN
EFIAPI
SecStartup (
  IN UINT32                   SizeOfRam,
  IN UINT32                   TempRamBase,
  IN VOID                     *BootFirmwareVolume
  );

/**
  Find and return Pei Core entry point.

  It also find SEC and PEI Core file debug information. It will report them if
  remote debug is enabled.

  @param   SecCoreFirmwareVolumePtr Point to the firmware volume for finding SecCore.
  @param   PeiCoreFirmwareVolumePtr Point to the firmware volume for finding PeiCore.
  @param   PeiCoreEntryPoint        The entry point of the PEI core.

**/
VOID
EFIAPI
FindAndReportEntryPoints (
  IN  EFI_FIRMWARE_VOLUME_HEADER       *SecCoreFirmwareVolumePtr,
  IN  EFI_FIRMWARE_VOLUME_HEADER       *PeiCoreFirmwareVolumePtr,
  OUT EFI_PEI_CORE_ENTRY_POINT         *PeiCoreEntryPoint
  );

/**
  Auto-generated function that calls the library constructors for all of the module's
  dependent libraries.  This function must be called by the SEC Core once a stack has
  been established.

**/
VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
  );

/**
  Implementation of the PlatformInformation service in EFI_SEC_PLATFORM_INFORMATION_PPI.

  @param  PeiServices                Pointer to the PEI Services Table.
  @param  StructureSize              Pointer to the variable describing size of the input buffer.
  @param  PlatformInformationRecord  Pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD.

  @retval EFI_SUCCESS                The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL       The buffer was too small.

**/
EFI_STATUS
EFIAPI
SecPlatformInformationBist (
  IN CONST EFI_PEI_SERVICES                  **PeiServices,
  IN OUT UINT64                              *StructureSize,
     OUT EFI_SEC_PLATFORM_INFORMATION_RECORD *PlatformInformationRecord
  );

/**
  Implementation of the PlatformInformation2 service in EFI_SEC_PLATFORM_INFORMATION2_PPI.

  @param  PeiServices                The pointer to the PEI Services Table.
  @param  StructureSize              The pointer to the variable describing size of the input buffer.
  @param  PlatformInformationRecord2 The pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD2.

  @retval EFI_SUCCESS                The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL       The buffer was too small. The current buffer size needed to
                                     hold the record is returned in StructureSize.

**/
EFI_STATUS
EFIAPI
SecPlatformInformation2Bist (
  IN CONST EFI_PEI_SERVICES                   **PeiServices,
  IN OUT UINT64                               *StructureSize,
     OUT EFI_SEC_PLATFORM_INFORMATION_RECORD2 *PlatformInformationRecord2
  );

/**
  Republish SecPlatformInformationPpi/SecPlatformInformation2Ppi.

**/
VOID
RepublishSecPlatformInformationPpi (
  VOID
  );

/**
  Entry point of the notification callback function itself within the PEIM.
  It is to get SEC performance data and build HOB to convey the SEC performance
  data to DXE phase.

  @param  PeiServices      Indirect reference to the PEI Services Table.
  @param  NotifyDescriptor Address of the notification descriptor data structure.
  @param  Ppi              Address of the PPI that was installed.

  @return Status of the notification.
          The status code returned from this function is ignored.
**/
EFI_STATUS
EFIAPI
SecPerformancePpiCallBack (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

#endif
