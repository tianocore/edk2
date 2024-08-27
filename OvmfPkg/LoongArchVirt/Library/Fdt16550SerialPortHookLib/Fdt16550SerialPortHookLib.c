/** @file
  Platform Hook Library instance for 16550 Uart.

  Copyright (c) 2020, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Register/LoongArch64/Csr.h>

/** Platform hook to retrieve the 16550 UART base address from register
    LOONGARCH_CSR_KS1 that caches the UART base address from early boot
    stage and store it in  PcdSerialRegisterBase.

  @retval RETURN_SUCCESS    Success.
  @retval RETURN_NOT_FOUND  Serial Port information not found.

**/
RETURN_STATUS
EFIAPI
PlatformHookSerialPortInitialize (
  VOID
  )
{
  UINT64  UartBase;

  if (PcdGet64 (PcdSerialRegisterBase) != 0) {
    return RETURN_SUCCESS;
  }

  UartBase = CsrRead (LOONGARCH_CSR_KS1);

  return (RETURN_STATUS)PcdSet64S (PcdSerialRegisterBase, (UINTN)UartBase);
}
