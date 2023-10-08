/** @file
  Define DebugLibFdtPL011UartWrite() for modules that can only run from RAM.

  Copyright (C) Red Hat

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiMultiPhase.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <Library/PL011UartLib.h>
#include <Library/PcdLib.h>
#include <Guid/EarlyPL011BaseAddress.h>

#include "Ram.h"
#include "Write.h"

UINTN          mDebugLibFdtPL011UartAddress;
RETURN_STATUS  mDebugLibFdtPL011UartPermanentStatus = RETURN_SUCCESS;

/**
  Statefully initialize both the library instance and the debug PL011 UART.
**/
STATIC
RETURN_STATUS
Initialize (
  VOID
  )
{
  CONST VOID                      *Hob;
  CONST EARLY_PL011_BASE_ADDRESS  *UartBase;
  RETURN_STATUS                   Status;
  UINTN                           DebugAddress;
  UINT64                          BaudRate;
  UINT32                          ReceiveFifoDepth;
  EFI_PARITY_TYPE                 Parity;
  UINT8                           DataBits;
  EFI_STOP_BITS_TYPE              StopBits;

  if (mDebugLibFdtPL011UartAddress != 0) {
    return RETURN_SUCCESS;
  }

  if (RETURN_ERROR (mDebugLibFdtPL011UartPermanentStatus)) {
    return mDebugLibFdtPL011UartPermanentStatus;
  }

  Hob = GetFirstGuidHob (&gEarlyPL011BaseAddressGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof *UartBase)) {
    Status = RETURN_NOT_FOUND;
    goto Failed;
  }

  UartBase = GET_GUID_HOB_DATA (Hob);

  DebugAddress = (UINTN)UartBase->DebugAddress;
  if (DebugAddress == 0) {
    Status = RETURN_NOT_FOUND;
    goto Failed;
  }

  BaudRate         = (UINTN)PcdGet64 (PcdUartDefaultBaudRate);
  ReceiveFifoDepth = 0; // Use the default value for Fifo depth
  Parity           = (EFI_PARITY_TYPE)PcdGet8 (PcdUartDefaultParity);
  DataBits         = PcdGet8 (PcdUartDefaultDataBits);
  StopBits         = (EFI_STOP_BITS_TYPE)PcdGet8 (PcdUartDefaultStopBits);

  Status = PL011UartInitializePort (
             DebugAddress,
             FixedPcdGet32 (PL011UartClkInHz),
             &BaudRate,
             &ReceiveFifoDepth,
             &Parity,
             &DataBits,
             &StopBits
             );
  if (RETURN_ERROR (Status)) {
    goto Failed;
  }

  mDebugLibFdtPL011UartAddress = DebugAddress;
  return RETURN_SUCCESS;

Failed:
  mDebugLibFdtPL011UartPermanentStatus = Status;
  return Status;
}

/**
  (Copied from SerialPortWrite() in "MdePkg/Include/Library/SerialPortLib.h" at
  commit c4547aefb3d0, with the Buffer non-nullity assertion removed:)

  Write data from buffer to serial device.

  Writes NumberOfBytes data bytes from Buffer to the serial device.
  The number of bytes actually written to the serial device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.
  If NumberOfBytes is zero, then return 0.

  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the serial device.
                           If this value is less than NumberOfBytes, then the write operation failed.
**/
UINTN
DebugLibFdtPL011UartWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  RETURN_STATUS  Status;

  Status = Initialize ();
  if (RETURN_ERROR (Status)) {
    return 0;
  }

  return PL011UartWrite (mDebugLibFdtPL011UartAddress, Buffer, NumberOfBytes);
}
