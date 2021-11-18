/** @file
  SMM STM support

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_STM_H_
#define _SMM_STM_H_

#include <Protocol/SmMonitorInit.h>

/**

  Create 4G page table for STM.
  2M PAE page table in X64 version.

  @param PageTableBase        The page table base in MSEG

**/
VOID
StmGen4GPageTable (
  IN UINTN  PageTableBase
  );

/**
  This is SMM exception handle.
  Consumed by STM when exception happen.

  @param Context  STM protection exception stack frame

  @return the EBX value for STM reference.
          EBX = 0: resume SMM guest using register state found on exception stack.
          EBX = 1 to 0x0F: EBX contains a BIOS error code which the STM must record in the
                           TXT.ERRORCODE register and subsequently reset the system via
                           TXT.CMD.SYS_RESET. The value of the TXT.ERRORCODE register is calculated as
                           follows: TXT.ERRORCODE = (EBX & 0x0F) | STM_CRASH_BIOS_PANIC
          EBX = 0x10 to 0xFFFFFFFF - reserved, do not use.

**/
UINT32
EFIAPI
SmmStmExceptionHandler (
  IN OUT STM_PROTECTION_EXCEPTION_STACK_FRAME  Context
  );

/**

  Get STM state.

  @return STM state

**/
EFI_SM_MONITOR_STATE
EFIAPI
GetMonitorState (
  VOID
  );

/**

  Load STM image to MSEG.

  @param StmImage      STM image
  @param StmImageSize  STM image size

  @retval EFI_SUCCESS            Load STM to MSEG successfully
  @retval EFI_BUFFER_TOO_SMALL   MSEG is smaller than minimal requirement of STM image

**/
EFI_STATUS
EFIAPI
LoadMonitor (
  IN EFI_PHYSICAL_ADDRESS  StmImage,
  IN UINTN                 StmImageSize
  );

/**

  Add resources in list to database. Allocate new memory areas as needed.

  @param ResourceList  A pointer to resource list to be added
  @param NumEntries    Optional number of entries.
                       If 0, list must be terminated by END_OF_RESOURCES.

  @retval EFI_SUCCESS            If resources are added
  @retval EFI_INVALID_PARAMETER  If nested procedure detected resource failer
  @retval EFI_OUT_OF_RESOURCES   If nested procedure returned it and we cannot allocate more areas.

**/
EFI_STATUS
EFIAPI
AddPiResource (
  IN  STM_RSC  *ResourceList,
  IN  UINT32   NumEntries OPTIONAL
  );

/**

  Delete resources in list to database.

  @param ResourceList  A pointer to resource list to be deleted
                       NULL means delete all resources.
  @param NumEntries    Optional number of entries.
                       If 0, list must be terminated by END_OF_RESOURCES.

  @retval EFI_SUCCESS            If resources are deleted
  @retval EFI_INVALID_PARAMETER  If nested procedure detected resource failer

**/
EFI_STATUS
EFIAPI
DeletePiResource (
  IN  STM_RSC  *ResourceList,
  IN  UINT32   NumEntries OPTIONAL
  );

/**

  Get BIOS resources.

  @param ResourceList  A pointer to resource list to be filled
  @param ResourceSize  On input it means size of resource list input.
                       On output it means size of resource list filled,
                       or the size of resource list to be filled if size of too small.

  @retval EFI_SUCCESS            If resources are returned.
  @retval EFI_BUFFER_TOO_SMALL   If resource list buffer is too small to hold the whole resources.

**/
EFI_STATUS
EFIAPI
GetPiResource (
  OUT    STM_RSC  *ResourceList,
  IN OUT UINT32   *ResourceSize
  );

/**
  This function initialize STM configuration table.
**/
VOID
StmSmmConfigurationTableInit (
  VOID
  );

/**
  This function notify STM resource change.

  @param StmResource BIOS STM resource

**/
VOID
NotifyStmResourceChange (
  IN VOID  *StmResource
  );

/**
  This function return BIOS STM resource.

  @return BIOS STM resource

**/
VOID *
GetStmResource (
  VOID
  );

/**
  This function fixes up the address of the global variable or function
  referred in SmiEntry assembly files to be the absolute address.
**/
VOID
EFIAPI
SmmCpuFeaturesLibStmSmiEntryFixupAddress (
  );

#endif
