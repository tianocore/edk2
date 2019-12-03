/** @file

 Copyright (c) 2014-2018, Linaro Ltd. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/NorFlashPlatformLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/FdtClient.h>

#define QEMU_NOR_BLOCK_SIZE    SIZE_256KB

#define MAX_FLASH_BANKS        4

EFI_STATUS
NorFlashPlatformInitialization (
  VOID
  )
{
  return EFI_SUCCESS;
}

NOR_FLASH_DESCRIPTION mNorFlashDevices[MAX_FLASH_BANKS];

EFI_STATUS
NorFlashPlatformGetDevices (
  OUT NOR_FLASH_DESCRIPTION   **NorFlashDescriptions,
  OUT UINT32                  *Count
  )
{
  FDT_CLIENT_PROTOCOL         *FdtClient;
  INT32                       Node;
  EFI_STATUS                  Status;
  EFI_STATUS                  FindNodeStatus;
  CONST UINT32                *Reg;
  UINT32                      PropSize;
  UINT32                      Num;
  UINT64                      Base;
  UINT64                      Size;

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL,
                  (VOID **)&FdtClient);
  ASSERT_EFI_ERROR (Status);

  Num = 0;
  for (FindNodeStatus = FdtClient->FindCompatibleNode (FdtClient,
                                     "cfi-flash", &Node);
       !EFI_ERROR (FindNodeStatus) && Num < MAX_FLASH_BANKS;
       FindNodeStatus = FdtClient->FindNextCompatibleNode (FdtClient,
                                     "cfi-flash", Node, &Node)) {

    Status = FdtClient->GetNodeProperty (FdtClient, Node, "reg",
                          (CONST VOID **)&Reg, &PropSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: GetNodeProperty () failed (Status == %r)\n",
        __FUNCTION__, Status));
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
      if ((PcdGet64 (PcdFvBaseAddress) + PcdGet32 (PcdFvSize) > Base) &&
          (Base + Size) > PcdGet64 (PcdFvBaseAddress)) {
        continue;
      }

      mNorFlashDevices[Num].DeviceBaseAddress = (UINTN)Base;
      mNorFlashDevices[Num].RegionBaseAddress = (UINTN)Base;
      mNorFlashDevices[Num].Size              = (UINTN)Size;
      mNorFlashDevices[Num].BlockSize         = QEMU_NOR_BLOCK_SIZE;
      Num++;
    }
  }

  *NorFlashDescriptions = mNorFlashDevices;
  *Count = Num;

  return EFI_SUCCESS;
}
