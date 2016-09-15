/** @file
*  Xenio FDT client protocol driver for xen,xen DT node
*
*  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
*
*  This program and the accompanying materials are
*  licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/XenIoMmioLib.h>

#include <Protocol/FdtClient.h>

EFI_STATUS
EFIAPI
InitializeXenioFdtDxe (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS            Status;
  FDT_CLIENT_PROTOCOL   *FdtClient;
  CONST UINT64          *Reg;
  UINT32                RegSize;
  UINTN                 AddressCells, SizeCells;
  EFI_HANDLE            Handle;
  UINT64                RegBase;

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL,
                  (VOID **)&FdtClient);
  ASSERT_EFI_ERROR (Status);

  Status = FdtClient->FindCompatibleNodeReg (FdtClient, "xen,xen",
                        (CONST VOID **)&Reg, &AddressCells, &SizeCells,
                        &RegSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_WARN, "%a: No 'xen,xen' compatible DT node found\n",
      __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  ASSERT (AddressCells == 2);
  ASSERT (SizeCells == 2);
  ASSERT (RegSize == 2 * sizeof (UINT64));

  //
  // Retrieve the reg base from this node and wire it up to the
  // MMIO flavor of the XenBus root device I/O protocol
  //
  RegBase = SwapBytes64 (Reg[0]);
  Handle = NULL;
  Status = XenIoMmioInstall (&Handle, RegBase);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: XenIoMmioInstall () failed on a new handle "
      "(Status == %r)\n", __FUNCTION__, Status));
    return Status;
  }

  DEBUG ((EFI_D_INFO, "Found Xen node with Grant table @ 0x%Lx\n", RegBase));

  return EFI_SUCCESS;
}
