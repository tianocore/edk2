/** @file
  RISC-V SEC phase module for Qemu Virt.

  Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecMain.h"

STATIC
EFI_STATUS
EFIAPI
SecInitializePlatform (
  VOID
  )
{
  EFI_STATUS  Status;

  MemoryPeimInitialization ();

  CpuPeimInitialization ();

  // Set the Boot Mode
  SetBootMode (BOOT_WITH_FULL_CONFIGURATION);

  Status = PlatformPeimInitialization ();
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.


  @param[in]  BootHartId         Hardware thread ID of boot hart.
  @param[in]  DeviceTreeAddress  Pointer to Device Tree (DTB)
**/
VOID
NORETURN
EFIAPI
SecStartup (
  IN  UINTN  BootHartId,
  IN  VOID   *DeviceTreeAddress
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *HobList;
  EFI_RISCV_FIRMWARE_CONTEXT  FirmwareContext;
  EFI_STATUS                  Status;
  UINT64                      UefiMemoryBase;
  UINT64                      StackBase;
  UINT32                      StackSize;

  //
  // Report Status Code to indicate entering SEC core
  //
  DEBUG ((
    DEBUG_INFO,
    "%a() BootHartId: 0x%x, DeviceTreeAddress=0x%x\n",
    __FUNCTION__,
    BootHartId,
    DeviceTreeAddress
    ));

  FirmwareContext.BootHartId          = BootHartId;
  FirmwareContext.FlattenedDeviceTree = (UINT64)DeviceTreeAddress;
  SetFirmwareContextPointer (&FirmwareContext);

  StackBase      = (UINT64)FixedPcdGet32 (PcdOvmfSecPeiTempRamBase);
  StackSize      = FixedPcdGet32 (PcdOvmfSecPeiTempRamSize);
  UefiMemoryBase = StackBase + StackSize - SIZE_32MB;

  // Declare the PI/UEFI memory region
  HobList = HobConstructor (
              (VOID *)UefiMemoryBase,
              SIZE_32MB,
              (VOID *)UefiMemoryBase,
              (VOID *)StackBase // The top of the UEFI Memory is reserved for the stacks
              );
  PrePeiSetHobList (HobList);

  SecInitializePlatform ();

  BuildStackHob (StackBase, StackSize);

  //
  // Process all libraries constructor function linked to SecMain.
  //
  ProcessLibraryConstructorList ();

  // Assume the FV that contains the SEC (our code) also contains a compressed FV.
  Status = DecompressFirstFv ();
  ASSERT_EFI_ERROR (Status);

  // Load the DXE Core and transfer control to it
  Status = LoadDxeCoreFromFv (NULL, 0);
  ASSERT_EFI_ERROR (Status);
  //
  // Should not come here.
  //
  UNREACHABLE ();
}
