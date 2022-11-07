/** @file
  Private header with declarations and definitions specific to the Standalone
  MM IPL PEI driver

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef STANDALONE_MM_IPL_PEI_H_
#define STANDALONE_MM_IPL_PEI_H_

/**
  Load SMM core to dispatch other Standalone MM drivers.

  @param  Entry                     Entry of Standalone MM Foundation.
  @param  Context1                  A pointer to the context to pass into the EntryPoint
                                    function.
  @retval EFI_SUCCESS               Successfully loaded SMM core.
  @retval Others                    Failed to load SMM core.
**/
EFI_STATUS
LoadSmmCore (
  IN EFI_PHYSICAL_ADDRESS  Entry,
  IN VOID                  *Context1
  );

/**
  Assembly function to transition from long mode to compatibility mode to
  execute 32-bit code and then transit back to long mode.

  @param[in] Function     The 32bit code entry to be executed.
  @param[in] Param1       The first parameter to pass to 32bit code
  @param[in] Param2       The second parameter to pass to 32bit code
  @param[in] InternalGdtr The GDT and GDT descriptor used by this library

  @retval status.
**/
UINT32
AsmExecute64BitCode (
  IN UINT64           Function,
  IN UINT64           Param1,
  IN UINT64           Param2,
  IN IA32_DESCRIPTOR  *InternalGdtr
  );

/**
  This is the callback function on ready to boot.

  Close and lock smram ranges on ready to boot stage.

  @param   PeiServices       General purpose services available to every PEIM.
  @param   NotifyDescriptor  The notification structure this PEIM registered on install.
  @param   Ppi               Pointer to the PPI data associated with this function.
  @retval  EFI_SUCCESS       Close and lock smram ranges successfully.
  @retval  Other             Close and lock smram ranges failed.
**/
EFI_STATUS
EFIAPI
ReadyToBootEvent (
  IN  EFI_PEI_SERVICES           **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN  VOID                       *Ppi
  );

#endif
