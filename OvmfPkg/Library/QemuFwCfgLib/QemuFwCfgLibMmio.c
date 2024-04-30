/** @file

  Copyright (C) 2013 - 2014, Red Hat, Inc.
  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi.h>

#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/FdtClient.h>

#include "QemuFwCfgLibMmioInternal.h"

//
// These correspond to the implementation we detect at runtime.
//
READ_BYTES_FUNCTION   *InternalQemuFwCfgReadBytes  = MmioReadBytes;
WRITE_BYTES_FUNCTION  *InternalQemuFwCfgWriteBytes = MmioWriteBytes;
SKIP_BYTES_FUNCTION   *InternalQemuFwCfgSkipBytes  = MmioSkipBytes;

/**
  Build firmware configure resource HOB.

  @param[in]   FwCfgResource  A pointer to firmware configure resource.

  @retval  VOID
**/
VOID
QemuBuildFwCfgResourceHob (
  IN QEMU_FW_CFG_RESOURCE  *FwCfgResource
  )
{
  BuildGuidDataHob (
    &gQemuFirmwareResourceHobGuid,
    (VOID *)FwCfgResource,
    sizeof (QEMU_FW_CFG_RESOURCE)
    );
}

/**
  Get firmware configure resource in HOB.

  @param VOID

  @retval  non-NULL   The firmware configure resource in HOB.
           NULL       The firmware configure resource not found.
**/
QEMU_FW_CFG_RESOURCE *
QemuGetFwCfgResourceHob (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  GuidHob = NULL;

  GuidHob = GetFirstGuidHob (&gQemuFirmwareResourceHobGuid);
  if (GuidHob == NULL) {
    return NULL;
  }

  return (QEMU_FW_CFG_RESOURCE *)GET_GUID_HOB_DATA (GuidHob);
}

/**
  Returns a boolean indicating if the firmware configuration interface
  is available or not.

  This function may change fw_cfg state.

  @retval TRUE   The interface is available
  @retval FALSE  The interface is not available

**/
BOOLEAN
EFIAPI
QemuFwCfgIsAvailable (
  VOID
  )
{
  return (BOOLEAN)(QemuGetFwCfgSelectorAddress () != 0 && QemuGetFwCfgDataAddress () != 0);
}

/**
  Selects a firmware configuration item for reading.

  Following this call, any data read from this item will start from the
  beginning of the configuration item's data.

  @param[in] QemuFwCfgItem  Firmware Configuration item to read

**/
VOID
EFIAPI
QemuFwCfgSelectItem (
  IN FIRMWARE_CONFIG_ITEM  QemuFwCfgItem
  )
{
  if (QemuFwCfgIsAvailable ()) {
    MmioWrite16 (QemuGetFwCfgSelectorAddress (), SwapBytes16 ((UINT16)QemuFwCfgItem));
  }
}

/**
  Slow READ_BYTES_FUNCTION.
**/
VOID
EFIAPI
MmioReadBytes (
  IN UINTN  Size,
  IN VOID   *Buffer OPTIONAL
  )
{
  UINTN  Left;
  UINT8  *Ptr;
  UINT8  *End;

 #if defined (MDE_CPU_AARCH64) || defined (MDE_CPU_RISCV64) || defined (MDE_CPU_LOONGARCH64)
  Left = Size & 7;
 #else
  Left = Size & 3;
 #endif

  Size -= Left;
  Ptr   = Buffer;
  End   = Ptr + Size;

 #if defined (MDE_CPU_AARCH64) || defined (MDE_CPU_RISCV64) || defined (MDE_CPU_LOONGARCH64)
  while (Ptr < End) {
    *(UINT64 *)Ptr = MmioRead64 (QemuGetFwCfgDataAddress ());
    Ptr           += 8;
  }

  if (Left & 4) {
    *(UINT32 *)Ptr = MmioRead32 (QemuGetFwCfgDataAddress ());
    Ptr           += 4;
  }

 #else
  while (Ptr < End) {
    *(UINT32 *)Ptr = MmioRead32 (QemuGetFwCfgDataAddress ());
    Ptr           += 4;
  }

 #endif

  if (Left & 2) {
    *(UINT16 *)Ptr = MmioRead16 (QemuGetFwCfgDataAddress ());
    Ptr           += 2;
  }

  if (Left & 1) {
    *Ptr = MmioRead8 (QemuGetFwCfgDataAddress ());
  }
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
DmaTransferBytes (
  IN     UINTN   Size,
  IN OUT VOID    *Buffer OPTIONAL,
  IN     UINT32  Control
  )
{
  volatile FW_CFG_DMA_ACCESS  Access;
  UINT32                      Status;

  ASSERT (
    Control == FW_CFG_DMA_CTL_WRITE || Control == FW_CFG_DMA_CTL_READ ||
    Control == FW_CFG_DMA_CTL_SKIP
    );

  if (Size == 0) {
    return;
  }

  ASSERT (Size <= MAX_UINT32);

  Access.Control = SwapBytes32 (Control);
  Access.Length  = SwapBytes32 ((UINT32)Size);
  Access.Address = SwapBytes64 ((UINT64)(UINTN)Buffer);

  //
  // We shouldn't start the transfer before setting up Access.
  //
  MemoryFence ();

  //
  // This will fire off the transfer.
  //
 #if defined (MDE_CPU_AARCH64) || defined (MDE_CPU_RISCV64) || defined (MDE_CPU_LOONGARCH64)
  MmioWrite64 (QemuGetFwCfgDmaAddress (), SwapBytes64 ((UINT64)&Access));
 #else
  MmioWrite32 ((UINT32)(QemuGetFwCfgDmaAddress () + 4), SwapBytes32 ((UINT32)&Access));
 #endif

  //
  // We shouldn't look at Access.Control before starting the transfer.
  //
  MemoryFence ();

  do {
    Status = SwapBytes32 (Access.Control);
    ASSERT ((Status & FW_CFG_DMA_CTL_ERROR) == 0);
  } while (Status != 0);

  //
  // The caller will want to access the transferred data.
  //
  MemoryFence ();
}

/**
  Fast READ_BYTES_FUNCTION.
**/
VOID
EFIAPI
DmaReadBytes (
  IN UINTN  Size,
  IN VOID   *Buffer OPTIONAL
  )
{
  DmaTransferBytes (Size, Buffer, FW_CFG_DMA_CTL_READ);
}

/**
  Reads firmware configuration bytes into a buffer

  If called multiple times, then the data read will continue at the offset of
  the firmware configuration item where the previous read ended.

  @param[in] Size    Size in bytes to read
  @param[in] Buffer  Buffer to store data into

**/
VOID
EFIAPI
QemuFwCfgReadBytes (
  IN UINTN  Size,
  IN VOID   *Buffer
  )
{
  if (QemuFwCfgIsAvailable ()) {
    InternalQemuFwCfgReadBytes (Size, Buffer);
  } else {
    ZeroMem (Buffer, Size);
  }
}

/**
  Slow WRITE_BYTES_FUNCTION.
**/
VOID
EFIAPI
MmioWriteBytes (
  IN UINTN  Size,
  IN VOID   *Buffer OPTIONAL
  )
{
  UINTN  Idx;

  for (Idx = 0; Idx < Size; ++Idx) {
    MmioWrite8 (QemuGetFwCfgDataAddress (), ((UINT8 *)Buffer)[Idx]);
  }
}

/**
  Fast WRITE_BYTES_FUNCTION.
**/
VOID
EFIAPI
DmaWriteBytes (
  IN UINTN  Size,
  IN VOID   *Buffer OPTIONAL
  )
{
  DmaTransferBytes (Size, Buffer, FW_CFG_DMA_CTL_WRITE);
}

/**
  Write firmware configuration bytes from a buffer

  If called multiple times, then the data written will continue at the offset
  of the firmware configuration item where the previous write ended.

  @param[in] Size    Size in bytes to write
  @param[in] Buffer  Buffer to read data from

**/
VOID
EFIAPI
QemuFwCfgWriteBytes (
  IN UINTN  Size,
  IN VOID   *Buffer
  )
{
  if (QemuFwCfgIsAvailable ()) {
    InternalQemuFwCfgWriteBytes (Size, Buffer);
  }
}

/**
  Slow SKIP_BYTES_FUNCTION.
**/
VOID
EFIAPI
MmioSkipBytes (
  IN UINTN  Size
  )
{
  UINTN  ChunkSize;
  UINT8  SkipBuffer[256];

  //
  // Emulate the skip by reading data in chunks, and throwing it away. The
  // implementation below doesn't affect the static data footprint for client
  // modules. Large skips are not expected, therefore this fallback is not
  // performance critical. The size of SkipBuffer is thought not to exert a
  // large pressure on the stack.
  //
  while (Size > 0) {
    ChunkSize = MIN (Size, sizeof SkipBuffer);
    MmioReadBytes (ChunkSize, SkipBuffer);
    Size -= ChunkSize;
  }
}

/**
  Fast SKIP_BYTES_FUNCTION.
**/
VOID
EFIAPI
DmaSkipBytes (
  IN UINTN  Size
  )
{
  DmaTransferBytes (Size, NULL, FW_CFG_DMA_CTL_SKIP);
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
  IN UINTN  Size
  )
{
  if (QemuFwCfgIsAvailable ()) {
    InternalQemuFwCfgSkipBytes (Size);
  }
}

/**
  Reads a UINT8 firmware configuration value

  @return  Value of Firmware Configuration item read

**/
UINT8
EFIAPI
QemuFwCfgRead8 (
  VOID
  )
{
  UINT8  Result;

  QemuFwCfgReadBytes (sizeof Result, &Result);
  return Result;
}

/**
  Reads a UINT16 firmware configuration value

  @return  Value of Firmware Configuration item read

**/
UINT16
EFIAPI
QemuFwCfgRead16 (
  VOID
  )
{
  UINT16  Result;

  QemuFwCfgReadBytes (sizeof Result, &Result);
  return Result;
}

/**
  Reads a UINT32 firmware configuration value

  @return  Value of Firmware Configuration item read

**/
UINT32
EFIAPI
QemuFwCfgRead32 (
  VOID
  )
{
  UINT32  Result;

  QemuFwCfgReadBytes (sizeof Result, &Result);
  return Result;
}

/**
  Reads a UINT64 firmware configuration value

  @return  Value of Firmware Configuration item read

**/
UINT64
EFIAPI
QemuFwCfgRead64 (
  VOID
  )
{
  UINT64  Result;

  QemuFwCfgReadBytes (sizeof Result, &Result);
  return Result;
}

/**
  Find the configuration item corresponding to the firmware configuration file.

  @param[in]  Name  Name of file to look up.
  @param[out] Item  Configuration item corresponding to the file, to be passed
                    to QemuFwCfgSelectItem ().
  @param[out] Size  Number of bytes in the file.

  @retval RETURN_SUCCESS      If file is found.
  @retval RETURN_NOT_FOUND    If file is not found.
  @retval RETURN_UNSUPPORTED  If firmware configuration is unavailable.

**/
RETURN_STATUS
EFIAPI
QemuFwCfgFindFile (
  IN   CONST CHAR8           *Name,
  OUT  FIRMWARE_CONFIG_ITEM  *Item,
  OUT  UINTN                 *Size
  )
{
  UINT32  Count;
  UINT32  Idx;

  if (!QemuFwCfgIsAvailable ()) {
    return RETURN_UNSUPPORTED;
  }

  QemuFwCfgSelectItem (QemuFwCfgItemFileDir);
  Count = SwapBytes32 (QemuFwCfgRead32 ());

  for (Idx = 0; Idx < Count; ++Idx) {
    UINT32  FileSize;
    UINT16  FileSelect;
    CHAR8   FName[QEMU_FW_CFG_FNAME_SIZE];

    FileSize   = QemuFwCfgRead32 ();
    FileSelect = QemuFwCfgRead16 ();
    QemuFwCfgRead16 (); // skip the field called "reserved"
    InternalQemuFwCfgReadBytes (sizeof (FName), FName);

    if (AsciiStrCmp (Name, FName) == 0) {
      *Item = (FIRMWARE_CONFIG_ITEM)SwapBytes16 (FileSelect);
      *Size = SwapBytes32 (FileSize);
      return RETURN_SUCCESS;
    }
  }

  return RETURN_NOT_FOUND;
}
