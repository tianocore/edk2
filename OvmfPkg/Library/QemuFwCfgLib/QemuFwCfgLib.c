/** @file

  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Uefi.h"
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "QemuFwCfgLibInternal.h"


/**
  Selects a firmware configuration item for reading.
  
  Following this call, any data read from this item will start from
  the beginning of the configuration item's data.

  @param[in] QemuFwCfgItem - Firmware Configuration item to read

**/
VOID
EFIAPI
QemuFwCfgSelectItem (
  IN FIRMWARE_CONFIG_ITEM   QemuFwCfgItem
  )
{
  DEBUG ((EFI_D_INFO, "Select Item: 0x%x\n", (UINT16)(UINTN) QemuFwCfgItem));
  IoWrite16 (FW_CFG_IO_SELECTOR, (UINT16)(UINTN) QemuFwCfgItem);
}


/**
  Transfer an array of bytes, or skip a number of bytes, using the DMA
  interface.

  @param[in]     Size     Size in bytes to transfer or skip.

  @param[in,out] Buffer   Buffer to read data into or write data from. Ignored,
                          and may be NULL, if Size is zero, or Control is
                          FW_CFG_DMA_CTL_SKIP.

  @param[in]     Control  One of the following:
                          FW_CFG_DMA_CTL_WRITE - write to fw_cfg from Buffer.
                          FW_CFG_DMA_CTL_READ  - read from fw_cfg into Buffer.
                          FW_CFG_DMA_CTL_SKIP  - skip bytes in fw_cfg.
**/
VOID
InternalQemuFwCfgDmaBytes (
  IN     UINT32   Size,
  IN OUT VOID     *Buffer OPTIONAL,
  IN     UINT32   Control
  )
{
  volatile FW_CFG_DMA_ACCESS Access;
  UINT32                     AccessHigh, AccessLow;
  UINT32                     Status;

  ASSERT (Control == FW_CFG_DMA_CTL_WRITE || Control == FW_CFG_DMA_CTL_READ ||
    Control == FW_CFG_DMA_CTL_SKIP);

  if (Size == 0) {
    return;
  }

  Access.Control = SwapBytes32 (Control);
  Access.Length  = SwapBytes32 (Size);
  Access.Address = SwapBytes64 ((UINTN)Buffer);

  //
  // Delimit the transfer from (a) modifications to Access, (b) in case of a
  // write, from writes to Buffer by the caller.
  //
  MemoryFence ();

  //
  // Start the transfer.
  //
  AccessHigh = (UINT32)RShiftU64 ((UINTN)&Access, 32);
  AccessLow  = (UINT32)(UINTN)&Access;
  IoWrite32 (FW_CFG_IO_DMA_ADDRESS,     SwapBytes32 (AccessHigh));
  IoWrite32 (FW_CFG_IO_DMA_ADDRESS + 4, SwapBytes32 (AccessLow));

  //
  // Don't look at Access.Control before starting the transfer.
  //
  MemoryFence ();

  //
  // Wait for the transfer to complete.
  //
  do {
    Status = SwapBytes32 (Access.Control);
    ASSERT ((Status & FW_CFG_DMA_CTL_ERROR) == 0);
  } while (Status != 0);

  //
  // After a read, the caller will want to use Buffer.
  //
  MemoryFence ();
}


/**
  Reads firmware configuration bytes into a buffer

  @param[in] Size - Size in bytes to read
  @param[in] Buffer - Buffer to store data into  (OPTIONAL if Size is 0)

**/
VOID
EFIAPI
InternalQemuFwCfgReadBytes (
  IN UINTN                  Size,
  IN VOID                   *Buffer  OPTIONAL
  )
{
  if (InternalQemuFwCfgDmaIsAvailable () && Size <= MAX_UINT32) {
    InternalQemuFwCfgDmaBytes ((UINT32)Size, Buffer, FW_CFG_DMA_CTL_READ);
    return;
  }
  IoReadFifo8 (FW_CFG_IO_DATA, Size, Buffer);
}


/**
  Reads firmware configuration bytes into a buffer

  If called multiple times, then the data read will
  continue at the offset of the firmware configuration
  item where the previous read ended.

  @param[in] Size - Size in bytes to read
  @param[in] Buffer - Buffer to store data into

**/
VOID
EFIAPI
QemuFwCfgReadBytes (
  IN UINTN                  Size,
  IN VOID                   *Buffer
  )
{
  if (InternalQemuFwCfgIsAvailable ()) {
    InternalQemuFwCfgReadBytes (Size, Buffer);
  } else {
    ZeroMem (Buffer, Size);
  }
}

/**
  Write firmware configuration bytes from a buffer

  If called multiple times, then the data written will
  continue at the offset of the firmware configuration
  item where the previous write ended.

  @param[in] Size - Size in bytes to write
  @param[in] Buffer - Buffer to read data from

**/
VOID
EFIAPI
QemuFwCfgWriteBytes (
  IN UINTN                  Size,
  IN VOID                   *Buffer
  )
{
  if (InternalQemuFwCfgIsAvailable ()) {
    if (InternalQemuFwCfgDmaIsAvailable () && Size <= MAX_UINT32) {
      InternalQemuFwCfgDmaBytes ((UINT32)Size, Buffer, FW_CFG_DMA_CTL_WRITE);
      return;
    }
    IoWriteFifo8 (FW_CFG_IO_DATA, Size, Buffer);
  }
}


/**
  Skip bytes in the firmware configuration item.

  Increase the offset of the firmware configuration item without transferring
  bytes between the item and a caller-provided buffer. Subsequent read, write
  or skip operations will commence at the increased offset.

  @param[in] Size  Number of bytes to skip.
**/
VOID
EFIAPI
QemuFwCfgSkipBytes (
  IN UINTN                  Size
  )
{
  UINTN ChunkSize;
  UINT8 SkipBuffer[256];

  if (!InternalQemuFwCfgIsAvailable ()) {
    return;
  }

  if (InternalQemuFwCfgDmaIsAvailable () && Size <= MAX_UINT32) {
    InternalQemuFwCfgDmaBytes ((UINT32)Size, NULL, FW_CFG_DMA_CTL_SKIP);
    return;
  }

  //
  // Emulate the skip by reading data in chunks, and throwing it away. The
  // implementation below is suitable even for phases where RAM or dynamic
  // allocation is not available or appropriate. It also doesn't affect the
  // static data footprint for client modules. Large skips are not expected,
  // therefore this fallback is not performance critical. The size of
  // SkipBuffer is thought not to exert a large pressure on the stack in any
  // phase.
  //
  while (Size > 0) {
    ChunkSize = MIN (Size, sizeof SkipBuffer);
    IoReadFifo8 (FW_CFG_IO_DATA, ChunkSize, SkipBuffer);
    Size -= ChunkSize;
  }
}


/**
  Reads a UINT8 firmware configuration value

  @return    Value of Firmware Configuration item read

**/
UINT8
EFIAPI
QemuFwCfgRead8 (
  VOID
  )
{
  UINT8 Result;

  QemuFwCfgReadBytes (sizeof (Result), &Result);

  return Result;
}


/**
  Reads a UINT16 firmware configuration value

  @return    Value of Firmware Configuration item read

**/
UINT16
EFIAPI
QemuFwCfgRead16 (
  VOID
  )
{
  UINT16 Result;

  QemuFwCfgReadBytes (sizeof (Result), &Result);

  return Result;
}


/**
  Reads a UINT32 firmware configuration value

  @return    Value of Firmware Configuration item read

**/
UINT32
EFIAPI
QemuFwCfgRead32 (
  VOID
  )
{
  UINT32 Result;

  QemuFwCfgReadBytes (sizeof (Result), &Result);

  return Result;
}


/**
  Reads a UINT64 firmware configuration value

  @return    Value of Firmware Configuration item read

**/
UINT64
EFIAPI
QemuFwCfgRead64 (
  VOID
  )
{
  UINT64 Result;

  QemuFwCfgReadBytes (sizeof (Result), &Result);

  return Result;
}


/**
  Find the configuration item corresponding to the firmware configuration file.

  @param[in]  Name - Name of file to look up.
  @param[out] Item - Configuration item corresponding to the file, to be passed
                     to QemuFwCfgSelectItem ().
  @param[out] Size - Number of bytes in the file.

  @return    RETURN_SUCCESS       If file is found.
             RETURN_NOT_FOUND     If file is not found.
             RETURN_UNSUPPORTED   If firmware configuration is unavailable.

**/
RETURN_STATUS
EFIAPI
QemuFwCfgFindFile (
  IN   CONST CHAR8           *Name,
  OUT  FIRMWARE_CONFIG_ITEM  *Item,
  OUT  UINTN                 *Size
  )
{
  UINT32 Count;
  UINT32 Idx;

  if (!InternalQemuFwCfgIsAvailable ()) {
    return RETURN_UNSUPPORTED;
  }

  QemuFwCfgSelectItem (QemuFwCfgItemFileDir);
  Count = SwapBytes32 (QemuFwCfgRead32 ());

  for (Idx = 0; Idx < Count; ++Idx) {
    UINT32 FileSize;
    UINT16 FileSelect;
    UINT16 FileReserved;
    CHAR8  FName[QEMU_FW_CFG_FNAME_SIZE];

    FileSize     = QemuFwCfgRead32 ();
    FileSelect   = QemuFwCfgRead16 ();
    FileReserved = QemuFwCfgRead16 ();
    (VOID) FileReserved; /* Force a do-nothing reference. */
    InternalQemuFwCfgReadBytes (sizeof (FName), FName);

    if (AsciiStrCmp (Name, FName) == 0) {
      *Item = SwapBytes16 (FileSelect);
      *Size = SwapBytes32 (FileSize);
      return RETURN_SUCCESS;
    }
  }

  return RETURN_NOT_FOUND;
}
