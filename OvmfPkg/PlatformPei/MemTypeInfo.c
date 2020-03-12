/** @file
  Produce a default memory type information HOB unless we can determine, from
  the existence of the "MemoryTypeInformation" variable, that the DXE IPL PEIM
  will produce the HOB.

  Copyright (C) 2017-2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/MemoryTypeInformation.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Uefi/UefiMultiPhase.h>

#include "Platform.h"

//
// The NumberOfPages values below are ad-hoc. They are updated sporadically at
// best (please refer to git-blame for past updates). The values capture a set
// of BIN hints that made sense at a particular time, for some (now likely
// unknown) workloads / boot paths.
//
STATIC EFI_MEMORY_TYPE_INFORMATION mDefaultMemoryTypeInformation[] = {
  { EfiACPIMemoryNVS,       0x004 },
  { EfiACPIReclaimMemory,   0x008 },
  { EfiReservedMemoryType,  0x004 },
  { EfiRuntimeServicesData, 0x024 },
  { EfiRuntimeServicesCode, 0x030 },
  { EfiBootServicesCode,    0x180 },
  { EfiBootServicesData,    0xF00 },
  { EfiMaxMemoryType,       0x000 }
};

STATIC
VOID
BuildMemTypeInfoHob (
  VOID
  )
{
  BuildGuidDataHob (
    &gEfiMemoryTypeInformationGuid,
    mDefaultMemoryTypeInformation,
    sizeof mDefaultMemoryTypeInformation
    );
  DEBUG ((DEBUG_INFO, "%a: default memory type information HOB built\n",
    __FUNCTION__));
}

/**
  Notification function called when EFI_PEI_READ_ONLY_VARIABLE2_PPI becomes
  available.

  @param[in] PeiServices      Indirect reference to the PEI Services Table.
  @param[in] NotifyDescriptor Address of the notification descriptor data
                              structure.
  @param[in] Ppi              Address of the PPI that was installed.

  @return  Status of the notification. The status code returned from this
           function is ignored.
**/
STATIC
EFI_STATUS
EFIAPI
OnReadOnlyVariable2Available (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_PEI_READ_ONLY_VARIABLE2_PPI *ReadOnlyVariable2;
  UINTN                           DataSize;
  EFI_STATUS                      Status;

  DEBUG ((DEBUG_VERBOSE, "%a\n", __FUNCTION__));

  //
  // Check if the "MemoryTypeInformation" variable exists, in the
  // gEfiMemoryTypeInformationGuid namespace.
  //
  ReadOnlyVariable2 = Ppi;
  DataSize = 0;
  Status = ReadOnlyVariable2->GetVariable (
                                ReadOnlyVariable2,
                                EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
                                &gEfiMemoryTypeInformationGuid,
                                NULL,
                                &DataSize,
                                NULL
                                );
  switch (Status) {
  case EFI_BUFFER_TOO_SMALL:
    //
    // The variable exists; the DXE IPL PEIM will build the HOB from it.
    //
    break;
  case EFI_NOT_FOUND:
    //
    // The variable does not exist; install the default memory type information
    // HOB.
    //
    BuildMemTypeInfoHob ();
    break;
  default:
    DEBUG ((DEBUG_ERROR, "%a: unexpected: GetVariable(): %r\n", __FUNCTION__,
      Status));
    ASSERT (FALSE);
    CpuDeadLoop ();
    break;
  }

  return EFI_SUCCESS;
}

//
// Notification object for registering the callback, for when
// EFI_PEI_READ_ONLY_VARIABLE2_PPI becomes available.
//
STATIC CONST EFI_PEI_NOTIFY_DESCRIPTOR mReadOnlyVariable2Notify = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH |
   EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),  // Flags
  &gEfiPeiReadOnlyVariable2PpiGuid,         // Guid
  OnReadOnlyVariable2Available              // Notify
};

VOID
MemTypeInfoInitialization (
  VOID
  )
{
  EFI_STATUS Status;

  if (!FeaturePcdGet (PcdSmmSmramRequire)) {
    //
    // EFI_PEI_READ_ONLY_VARIABLE2_PPI will never be available; install
    // the default memory type information HOB right away.
    //
    BuildMemTypeInfoHob ();
    return;
  }

  Status = PeiServicesNotifyPpi (&mReadOnlyVariable2Notify);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to set up R/O Variable 2 callback: %r\n",
      __FUNCTION__, Status));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
}
