/** @file
  Produce the memory type information HOB.

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

#define MEMORY_TYPE_INFO_DEFAULT(Type) \
  { Type, FixedPcdGet32 (PcdMemoryType ## Type) }

STATIC EFI_MEMORY_TYPE_INFORMATION  mMemoryTypeInformation[] = {
  MEMORY_TYPE_INFO_DEFAULT (EfiACPIMemoryNVS),
  MEMORY_TYPE_INFO_DEFAULT (EfiACPIReclaimMemory),
  MEMORY_TYPE_INFO_DEFAULT (EfiReservedMemoryType),
  MEMORY_TYPE_INFO_DEFAULT (EfiRuntimeServicesCode),
  MEMORY_TYPE_INFO_DEFAULT (EfiRuntimeServicesData),
  { EfiMaxMemoryType,                               0}
};

STATIC
VOID
BuildMemTypeInfoHob (
  VOID
  )
{
  BuildGuidDataHob (
    &gEfiMemoryTypeInformationGuid,
    mMemoryTypeInformation,
    sizeof mMemoryTypeInformation
    );
}

/**
  Refresh the mMemoryTypeInformation array (which we'll turn into the
  MemoryTypeInformation HOB) from the MemoryTypeInformation UEFI variable.

  Normally, the DXE IPL PEIM builds the HOB from the UEFI variable. But it does
  so *transparently*. Instead, we consider the UEFI variable as a list of
  hints, for updating our HOB defaults:

  - Record types not covered in mMemoryTypeInformation are ignored. In
    particular, this hides record types from the UEFI variable that may lead to
    reboots without benefiting SMM security, such as EfiBootServicesData.

  - Records that would lower the defaults in mMemoryTypeInformation are also
    ignored.

  @param[in] ReadOnlyVariable2  The EFI_PEI_READ_ONLY_VARIABLE2_PPI used for
                                retrieving the MemoryTypeInformation UEFI
                                variable.
**/
STATIC
VOID
RefreshMemTypeInfo (
  IN EFI_PEI_READ_ONLY_VARIABLE2_PPI  *ReadOnlyVariable2
  )
{
  UINTN                        DataSize;
  EFI_MEMORY_TYPE_INFORMATION  Entries[EfiMaxMemoryType + 1];
  EFI_STATUS                   Status;
  UINTN                        NumEntries;
  UINTN                        HobRecordIdx;

  //
  // Read the MemoryTypeInformation UEFI variable from the
  // gEfiMemoryTypeInformationGuid namespace.
  //
  DataSize = sizeof Entries;
  Status   = ReadOnlyVariable2->GetVariable (
                                  ReadOnlyVariable2,
                                  EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
                                  &gEfiMemoryTypeInformationGuid,
                                  NULL,
                                  &DataSize,
                                  Entries
                                  );
  if (EFI_ERROR (Status)) {
    //
    // If the UEFI variable does not exist (EFI_NOT_FOUND), we can't use it for
    // udpating mMemoryTypeInformation.
    //
    // If the UEFI variable exists but Entries is too small to hold it
    // (EFI_BUFFER_TOO_SMALL), then the variable contents are arguably invalid.
    // That's because Entries has room for every distinct EFI_MEMORY_TYPE,
    // including the terminator record with EfiMaxMemoryType. Thus, we can't
    // use the UEFI variable for updating mMemoryTypeInformation.
    //
    // If the UEFI variable couldn't be read for some other reason, we
    // similarly can't use it for udpating mMemoryTypeInformation.
    //
    DEBUG ((DEBUG_ERROR, "%a: GetVariable(): %r\n", __func__, Status));
    return;
  }

  //
  // Sanity-check the UEFI variable size against the record size.
  //
  if (DataSize % sizeof Entries[0] != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid UEFI variable size %Lu\n",
      __func__,
      (UINT64)DataSize
      ));
    return;
  }

  NumEntries = DataSize / sizeof Entries[0];

  //
  // For each record in mMemoryTypeInformation, except the terminator record,
  // look up the first match (if any) in the UEFI variable, based on the memory
  // type.
  //
  for (HobRecordIdx = 0;
       HobRecordIdx < ARRAY_SIZE (mMemoryTypeInformation) - 1;
       HobRecordIdx++)
  {
    EFI_MEMORY_TYPE_INFORMATION  *HobRecord;
    UINTN                        Idx;
    EFI_MEMORY_TYPE_INFORMATION  *VariableRecord;

    HobRecord = &mMemoryTypeInformation[HobRecordIdx];

    for (Idx = 0; Idx < NumEntries; Idx++) {
      VariableRecord = &Entries[Idx];

      if (VariableRecord->Type == HobRecord->Type) {
        break;
      }
    }

    //
    // If there is a match, allow the UEFI variable to increase NumberOfPages.
    //
    if ((Idx < NumEntries) &&
        (HobRecord->NumberOfPages < VariableRecord->NumberOfPages))
    {
      DEBUG ((
        DEBUG_VERBOSE,
        "%a: Type 0x%x: NumberOfPages 0x%x -> 0x%x\n",
        __func__,
        HobRecord->Type,
        HobRecord->NumberOfPages,
        VariableRecord->NumberOfPages
        ));

      HobRecord->NumberOfPages = VariableRecord->NumberOfPages;
    }
  }
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
  DEBUG ((DEBUG_VERBOSE, "%a\n", __func__));

  RefreshMemTypeInfo (Ppi);
  BuildMemTypeInfoHob ();
  return EFI_SUCCESS;
}

//
// Notification object for registering the callback, for when
// EFI_PEI_READ_ONLY_VARIABLE2_PPI becomes available.
//
STATIC CONST EFI_PEI_NOTIFY_DESCRIPTOR  mReadOnlyVariable2Notify = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH |
    EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST), // Flags
  &gEfiPeiReadOnlyVariable2PpiGuid,         // Guid
  OnReadOnlyVariable2Available              // Notify
};

VOID
MemTypeInfoInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  EFI_STATUS  Status;

  if (!PlatformInfoHob->SmmSmramRequire) {
    //
    // EFI_PEI_READ_ONLY_VARIABLE2_PPI will never be available; install
    // the default memory type information HOB right away.
    //
    BuildMemTypeInfoHob ();
    return;
  }

  Status = PeiServicesNotifyPpi (&mReadOnlyVariable2Notify);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: failed to set up R/O Variable 2 callback: %r\n",
      __func__,
      Status
      ));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
}
