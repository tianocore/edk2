/** @file

 Copyright (c) 2014-2018, Linaro Ltd. All rights reserved.<BR>
 Copyright (c) 2023, Ventana Micro Systems Inc. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/VirtNorFlashPlatformLib.h>

#include <Protocol/FdtClient.h>

#define QEMU_NOR_BLOCK_SIZE  SIZE_256KB

#define MAX_FLASH_BANKS  4

EFI_STATUS
VirtNorFlashPlatformInitialization (
  VOID
  )
{
  return EFI_SUCCESS;
}

STATIC VIRT_NOR_FLASH_DESCRIPTION  mNorFlashDevices[MAX_FLASH_BANKS];

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
  UINT32               Num;
  UINT64               Base;
  UINT64               Size;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  Num = 0;
  for (FindNodeStatus = FdtClient->FindCompatibleNode (
                                     FdtClient,
                                     "cfi-flash",
                                     &Node
                                     );
       !EFI_ERROR (FindNodeStatus) && Num < MAX_FLASH_BANKS;
       FindNodeStatus = FdtClient->FindNextCompatibleNode (
                                     FdtClient,
                                     "cfi-flash",
                                     Node,
                                     &Node
                                     ))
  {
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
      Num++;
    }

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
  }

  *NorFlashDescriptions = mNorFlashDevices;
  *Count                = Num;

  return EFI_SUCCESS;
}
