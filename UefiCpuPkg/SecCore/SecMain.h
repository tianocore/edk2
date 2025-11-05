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
#include <Ppi/RepublishSecPpi.h>

#include <Guid/FirmwarePerformance.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PlatformSecLib.h>
#include <Library/CpuLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>

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
  IN UINT32  SizeOfRam,
  IN UINT32  TempRamBase,
  IN VOID    *BootFirmwareVolume
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
  IN  EFI_FIRMWARE_VOLUME_HEADER  *SecCoreFirmwareVolumePtr,
  IN  EFI_FIRMWARE_VOLUME_HEADER  *PeiCoreFirmwareVolumePtr,
  OUT EFI_PEI_CORE_ENTRY_POINT    *PeiCoreEntryPoint
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
