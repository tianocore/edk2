/** @file
  FDT client library for ARM's PL031 RTC driver

  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/FdtClient.h>

RETURN_STATUS
EFIAPI
ArmVirtPL031FdtClientLibConstructor (
  VOID
  )
{
  EFI_STATUS           Status;
  FDT_CLIENT_PROTOCOL  *FdtClient;
  INT32                Node;
  CONST UINT64         *Reg;
  UINT32               RegSize;
  UINT64               RegBase;
  RETURN_STATUS        PcdStatus;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  Status = FdtClient->FindCompatibleNode (FdtClient, "arm,pl031", &Node);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: No 'arm,pl031' compatible DT node found\n",
      __func__
      ));
    return EFI_SUCCESS;
  }

  Status = FdtClient->GetNodeProperty (
                        FdtClient,
                        Node,
                        "reg",
                        (CONST VOID **)&Reg,
                        &RegSize
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: No 'reg' property found in 'arm,pl031' compatible DT node\n",
      __func__
      ));
    return EFI_SUCCESS;
  }

  ASSERT (RegSize == 16);

  RegBase = SwapBytes64 (Reg[0]);
  ASSERT (RegBase < MAX_UINT32);

  PcdStatus = PcdSet32S (PcdPL031RtcBase, (UINT32)RegBase);
  ASSERT_RETURN_ERROR (PcdStatus);

  DEBUG ((DEBUG_INFO, "Found PL031 RTC @ 0x%Lx\n", RegBase));

  //
  // UEFI takes ownership of the RTC hardware, and exposes its functionality
  // through the UEFI Runtime Services GetTime, SetTime, etc. This means we
  // need to disable it in the device tree to prevent the OS from attaching
  // its device driver as well.
  //
  Status = FdtClient->SetNodeProperty (
                        FdtClient,
                        Node,
                        "status",
                        "disabled",
                        sizeof ("disabled")
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to set PL031 status to 'disabled'\n"));
  }

  return EFI_SUCCESS;
}
