/** @file

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/VirtNorFlashPlatformLib.h>

#include <Protocol/FdtClient.h>

#define QEMU_NOR_BLOCK_SIZE  SIZE_128KB

EFI_STATUS
VirtNorFlashPlatformInitialization (
  VOID
  )
{
  return EFI_SUCCESS;
}

STATIC VIRT_NOR_FLASH_DESCRIPTION  mNorFlashDevices;

EFI_STATUS
VirtNorFlashPlatformGetDevices (
  OUT VIRT_NOR_FLASH_DESCRIPTION  **NorFlashDescriptions,
  OUT UINT32                      *Count
  )
{
  FDT_CLIENT_PROTOCOL  *FdtClient;
  INT32                Node;
  EFI_STATUS           Status;
  EFI_STATUS           FindNodeStatus;
  CONST UINT32         *Reg;
  UINT32               PropSize;
  UINT64               Base;
  UINT64               Size;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  FindNodeStatus = FdtClient->FindCompatibleNode (
                                FdtClient,
                                "cfi-flash",
                                &Node
                                );
  ASSERT_EFI_ERROR (FindNodeStatus);

  Status = FdtClient->GetNodeProperty (
                        FdtClient,
                        Node,
                        "reg",
                        (CONST VOID **)&Reg,
                        &PropSize
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: GetNodeProperty () failed (Status == %r)\n",
      __func__,
      Status
      ));
    return Status;
  }

  ASSERT ((PropSize % (4 * sizeof (UINT32))) == 0);

  if (PropSize < (4 * sizeof (UINT32))) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: reg node size(%d) is too small \n",
      __func__,
      PropSize
      ));
    return EFI_NOT_FOUND;
  }

  Base = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[0]));
  Size = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[2]));

  mNorFlashDevices.DeviceBaseAddress = (UINTN)Base;
  mNorFlashDevices.RegionBaseAddress = (UINTN)Base;
  mNorFlashDevices.Size              = (UINTN)Size;
  mNorFlashDevices.BlockSize         = QEMU_NOR_BLOCK_SIZE;

  Status = PcdSet32S (PcdFlashNvStorageVariableBase, Base);
  ASSERT_EFI_ERROR (Status);

  /*
   * Base is the value of PcdFlashNvStorageVariableBase,
   * PcdFlashNvStorageFtwWorkingBase can be got by
   *   PcdFlashNvStorageVariableBase + PcdFlashNvStorageVariableSize
   */
  Base  += PcdGet32 (PcdFlashNvStorageVariableSize);
  Status = PcdSet32S (PcdFlashNvStorageFtwWorkingBase, Base);
  ASSERT_EFI_ERROR (Status);

  /*
   * Now,Base is the value of PcdFlashNvStorageFtwWorkingBase,
   * PcdFlashNvStorageFtwSpareBase can be got by
   *   PcdFlashNvStorageFtwWorkingBase + PcdFlashNvStorageFtwWorkingSize.
   */
  Base  += PcdGet32 (PcdFlashNvStorageFtwWorkingSize);
  Status = PcdSet32S (PcdFlashNvStorageFtwSpareBase, Base);
  ASSERT_EFI_ERROR (Status);

  //
  // UEFI takes ownership of the NOR flash, and exposes its functionality
  // through the UEFI Runtime Services GetVariable, SetVariable, etc. This
  // means we need to disable it in the device tree to prevent the OS from
  // attaching its device driver as well.
  // Note that this also hides other flash banks, but the only other flash
  // bank we expect to encounter is the one that carries the UEFI executable
  // code, which is not intended to be guest updatable, and is usually backed
  // in a readonly manner by QEMU anyway.
  //
  Status = FdtClient->SetNodeProperty (
                        FdtClient,
                        Node,
                        "status",
                        "disabled",
                        sizeof ("disabled")
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to set NOR flash status to 'disabled'\n"));
  }

  *NorFlashDescriptions = &mNorFlashDevices;
  *Count                = 1;

  return EFI_SUCCESS;
}
