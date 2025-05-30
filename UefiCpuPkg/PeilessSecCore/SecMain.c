/** @file
  C functions in SEC

  Copyright (c) 2008 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2025 Ventana Micro Systems Inc.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecMain.h"

/**
  Caller provided function to be invoked at the end of InitializeDebugAgent().

  @param[in] Context    The first input parameter of InitializeDebugAgent().

**/
VOID
NORETURN
EFIAPI
SecStartupPhase2 (
  IN VOID  *Context
  );

/**

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param TempRamBase         Base address of temporary ram.
  @param TempRamSize         The size of temporary ram.
**/
VOID
NORETURN
EFIAPI
SecStartup (
  IN UINT32  SizeOfRam,
  IN UINT32  TempRamBase,
  IN VOID    *BootFirmwareVolume
  )
{
  EFI_STATUS                  Status;
  EFI_HOB_HANDOFF_INFO_TABLE  *HobList;
  UINTN                       SecStackBase;
  UINTN                       SecStackSize;

  DEBUG ((
    DEBUG_INFO,
    "%a() TempRAM Base: 0x%x, TempRAM Size: 0x%x\n",
    __func__,
    TempRamBase,
    SizeOfRam
    ));

  SecStackSize = PcdGet32 (PcdPeiTemporaryRamStackSize);
  if (SecStackSize == 0) {
    SecStackSize = (SizeOfRam >> 1);
  }

  ASSERT (SecStackSize < SizeOfRam);
  SecStackBase = TempRamBase + SizeOfRam - SecStackSize;

  //
  // Initialize floating point operating environment
  // to be compliant with UEFI spec.
  //
  InitializeFloatingPointUnits ();

  //
  // Setup the default exception handlers
  //
  Status = InitializeCpuExceptionHandlers (NULL);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((
    DEBUG_INFO,
    "%a() BFV Base: 0x%p,StackBase: 0x%x, StackSize: 0x%x\n",
    __func__,
    BootFirmwareVolume,
    SecStackBase,
    SecStackSize
    ));

  //
  // Declare the temporary memory region
  //
  HobList = HobConstructor (
              (VOID *)(UINTN)TempRamBase,
              SizeOfRam,
              (VOID *)(UINTN)TempRamBase,
              (VOID *)SecStackBase
              );
  PrePeiSetHobList (HobList);

  BuildStackHob (SecStackBase, SecStackSize);

  //
  // Initialize Debug Agent to support source level debug in SEC phases before switching to DXE phase.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, BootFirmwareVolume, SecStartupPhase2);

  //
  // Should not come here.
  //
  UNREACHABLE ();
}

/**
  Caller provided function to be invoked at the end of InitializeDebugAgent().

  @param[in] Context    The first input parameter of InitializeDebugAgent().

**/
VOID
NORETURN
EFIAPI
SecStartupPhase2 (
  IN VOID  *Context
  )
{
  EFI_STATUS           Status;
  EFI_PEI_FILE_HANDLE  FileHandle          = NULL;
  VOID                 *BootFirmwareVolume = Context;

  SecPlatformMain (NULL);

  //
  // Process all libraries constructor function linked to SecMain.
  //
  ProcessLibraryConstructorList ();

  //
  // Decompress BFV first
  //
  Status = FfsFindNextFile (
             EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE,
             (EFI_PEI_FV_HANDLE)BootFirmwareVolume,
             &FileHandle
             );
  ASSERT_EFI_ERROR (Status);

  Status = FfsProcessFvFile (FileHandle);
  ASSERT_EFI_ERROR (Status);

  //
  // Load the DXE Core and transfer control to it
  //
  Status = LoadDxeCoreFromFv (NULL, 0);
  ASSERT_EFI_ERROR (Status);

  //
  // Should not come here.
  //
  UNREACHABLE ();
}
