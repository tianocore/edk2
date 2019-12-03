/** @file
  FDT client library for ARM's TimerDxe

  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/FdtClient.h>

#pragma pack (1)
typedef struct {
  UINT32  Type;
  UINT32  Number;
  UINT32  Flags;
} INTERRUPT_PROPERTY;
#pragma pack ()

RETURN_STATUS
EFIAPI
ArmVirtTimerFdtClientLibConstructor (
  VOID
  )
{
  EFI_STATUS                    Status;
  FDT_CLIENT_PROTOCOL           *FdtClient;
  CONST INTERRUPT_PROPERTY      *InterruptProp;
  UINT32                        PropSize;
  INT32                         SecIntrNum, IntrNum, VirtIntrNum, HypIntrNum;
  RETURN_STATUS                 PcdStatus;

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL,
                  (VOID **)&FdtClient);
  ASSERT_EFI_ERROR (Status);

  Status = FdtClient->FindCompatibleNodeProperty (FdtClient, "arm,armv7-timer",
                        "interrupts", (CONST VOID **)&InterruptProp,
                        &PropSize);
  if (Status == EFI_NOT_FOUND) {
    Status = FdtClient->FindCompatibleNodeProperty (FdtClient,
                          "arm,armv8-timer", "interrupts",
                          (CONST VOID **)&InterruptProp,
                          &PropSize);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // - interrupts : Interrupt list for secure, non-secure, virtual and
  //  hypervisor timers, in that order.
  //
  ASSERT (PropSize == 36 || PropSize == 48);

  SecIntrNum = SwapBytes32 (InterruptProp[0].Number)
               + (InterruptProp[0].Type ? 16 : 0);
  IntrNum = SwapBytes32 (InterruptProp[1].Number)
            + (InterruptProp[1].Type ? 16 : 0);
  VirtIntrNum = SwapBytes32 (InterruptProp[2].Number)
                + (InterruptProp[2].Type ? 16 : 0);
  HypIntrNum = PropSize < 48 ? 0 : SwapBytes32 (InterruptProp[3].Number)
                                   + (InterruptProp[3].Type ? 16 : 0);

  DEBUG ((EFI_D_INFO, "Found Timer interrupts %d, %d, %d, %d\n",
    SecIntrNum, IntrNum, VirtIntrNum, HypIntrNum));

  PcdStatus = PcdSet32S (PcdArmArchTimerSecIntrNum, SecIntrNum);
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet32S (PcdArmArchTimerIntrNum, IntrNum);
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet32S (PcdArmArchTimerVirtIntrNum, VirtIntrNum);
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet32S (PcdArmArchTimerHypIntrNum, HypIntrNum);
  ASSERT_RETURN_ERROR (PcdStatus);

  return EFI_SUCCESS;
}
