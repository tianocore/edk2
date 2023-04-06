/** @file
*  Xenio FDT client protocol driver for xen,xen DT node
*
*  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;
  FDT_CLIENT_PROTOCOL  *FdtClient;
  CONST UINT64         *Reg;
  UINT32               RegSize;
  UINTN                AddressCells, SizeCells;
  EFI_HANDLE           Handle;
  UINT64               RegBase;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  Status = FdtClient->FindCompatibleNodeReg (
                        FdtClient,
                        "xen,xen",
                        (CONST VOID **)&Reg,
                        &AddressCells,
                        &SizeCells,
                        &RegSize
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: No 'xen,xen' compatible DT node found\n",
      __func__
      ));
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
  Handle  = NULL;
  Status  = XenIoMmioInstall (&Handle, RegBase);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: XenIoMmioInstall () failed on a new handle "
      "(Status == %r)\n",
      __func__,
      Status
      ));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Found Xen node with Grant table @ 0x%Lx\n", RegBase));

  return EFI_SUCCESS;
}
