/** @file

  Copyright (c) 2014-2018, Linaro Ltd. All rights reserved.<BR>
  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/VirtNorFlashPlatformLib.h>

#include <Protocol/FdtClient.h>
#include <stdbool.h>

#define QEMU_NOR_BLOCK_SIZE  SIZE_256KB
#define MAX_FLASH_BANKS      4

STATIC VIRT_NOR_FLASH_DESCRIPTION  mNorFlashDevices[MAX_FLASH_BANKS];
STATIC UINTN                       mNorFlashDeviceCount = 0;
STATIC INT32                       mNorFlashNodes[MAX_FLASH_BANKS];
STATIC UINTN                       mNorFlashNodeCount = 0;
FDT_CLIENT_PROTOCOL                *mFdtClient;

EFI_STATUS
EFIAPI
NorFlashQemuLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  INT32         Node;
  EFI_STATUS    Status;
  EFI_STATUS    FindNodeStatus;
  CONST UINT32  *Reg;
  UINT32        PropSize;
  UINT32        Num;
  UINT64        Base;
  UINT64        Size;
  BOOLEAN       Found;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&mFdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  Num   = 0;
  Found = FALSE;
  for (FindNodeStatus = mFdtClient->FindCompatibleNode (
                                      mFdtClient,
                                      "cfi-flash",
                                      &Node
                                      );
       !EFI_ERROR (FindNodeStatus) && Num < MAX_FLASH_BANKS;
       FindNodeStatus = mFdtClient->FindNextCompatibleNode (
                                      mFdtClient,
                                      "cfi-flash",
                                      Node,
                                      &Node
                                      ))
  {
    Status = mFdtClient->GetNodeProperty (
                           mFdtClient,
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
      continue;
    }

    ASSERT ((PropSize % (4 * sizeof (UINT32))) == 0);

    while (PropSize >= (4 * sizeof (UINT32)) && Num < MAX_FLASH_BANKS) {
      Base = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[0]));
      Size = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[2]));
      Reg += 4;

      PropSize -= 4 * sizeof (UINT32);

      //
      // Disregard any flash devices that overlap with the primary FV.
      // The firmware is not updatable from inside the guest anyway.
      //
      if ((PcdGet32 (PcdOvmfFdBaseAddress) + PcdGet32 (PcdOvmfFirmwareFdSize) > Base) &&
          ((Base + Size) > PcdGet32 (PcdOvmfFdBaseAddress)))
      {
        continue;
      }

      mNorFlashDevices[Num].DeviceBaseAddress = (UINTN)Base;
      mNorFlashDevices[Num].RegionBaseAddress = (UINTN)Base;
      mNorFlashDevices[Num].Size              = (UINTN)Size;
      mNorFlashDevices[Num].BlockSize         = QEMU_NOR_BLOCK_SIZE;
      mNorFlashDeviceCount                    = ++Num;
      if (!Found) {
        //
        // By default, the second available flash is stored as a non-volatile variable.
        //
        Status = PcdSet32S (PcdFlashNvStorageVariableBase, Base);
        ASSERT_EFI_ERROR (Status);

        //
        // The Base is the value of PcdFlashNvStorageVariableBase,
        // PcdFlashNvStorageFtwWorkingBase can be got by
        // PcdFlashNvStorageVariableBase + PcdFlashNvStorageVariableSize
        //
        Base  += PcdGet32 (PcdFlashNvStorageVariableSize);
        Status = PcdSet32S (PcdFlashNvStorageFtwWorkingBase, Base);
        ASSERT_EFI_ERROR (Status);

        //
        // Now, the Base is the value of PcdFlashNvStorageFtwWorkingBase,
        // PcdFlashNvStorageFtwSpareBase can be got by
        // PcdFlashNvStorageFtwWorkingBase + PcdFlashNvStorageFtwWorkingSize.
        //
        Base  += PcdGet32 (PcdFlashNvStorageFtwWorkingSize);
        Status = PcdSet32S (PcdFlashNvStorageFtwSpareBase, Base);
        ASSERT_EFI_ERROR (Status);
        Found = TRUE;
      }
    }

    mNorFlashNodes[mNorFlashNodeCount++] = Node;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
VirtNorFlashPlatformInitialization (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

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
  // Note: this library is loaded multiple times, by QemuPlatformDxe and last by
  // NorFlashDxe. Since FindNextCompatibleNode() skips disabled nodes, we only
  // disable the node here, when NorFlashDxe calls
  // VirtNorFlashPlatformInitialization().
  //
  for (Index = 0; Index < mNorFlashNodeCount; Index++) {
    Status = mFdtClient->SetNodeProperty (
                           mFdtClient,
                           mNorFlashNodes[Index],
                           "status",
                           "disabled",
                           sizeof ("disabled")
                           );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Failed to set NOR flash status to 'disabled'\n"));
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
VirtNorFlashPlatformGetDevices (
  OUT VIRT_NOR_FLASH_DESCRIPTION  **NorFlashDescriptions,
  OUT UINT32                      *Count
  )
{
  if (mNorFlashDeviceCount == 0) {
    return EFI_NOT_FOUND;
  }

  *NorFlashDescriptions = mNorFlashDevices;
  *Count                = mNorFlashDeviceCount;

  return EFI_SUCCESS;
}
