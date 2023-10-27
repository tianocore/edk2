/** @file

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - FwCfg   - firmWare  Configure
    - CTL   - Control
**/

#include <Base.h>
#include <Uefi.h>
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Uefi/UefiBaseType.h>
#include <Library/HobLib.h>
#include <libfdt.h>
#include "QemuFwCfgLibInternal.h"

EFI_GUID  mFwCfgSelectorAddressGuid = FW_CONFIG_SELECTOR_ADDRESS_HOB_GUID;
EFI_GUID  mFwCfgDataAddressGuid     = FW_CONFIG_DATA_ADDRESS_HOB_GUID;

STATIC UINTN  mFwCfgSelectorAddress;
STATIC UINTN  mFwCfgDataAddress;

/**
  To get firmware configure selector address.

  @param VOID

  @retval  firmware configure selector address
**/
UINTN
EFIAPI
QemuGetFwCfgSelectorAddress (
  VOID
  )
{
  UINTN              FwCfgSelectorAddress;
  EFI_HOB_GUID_TYPE  *GuidHob;
  VOID               *DataInHob;

  FwCfgSelectorAddress = mFwCfgSelectorAddress;
  GuidHob              = NULL;
  DataInHob            = NULL;

  if (FwCfgSelectorAddress == 0) {
    GuidHob              = GetFirstGuidHob (&mFwCfgSelectorAddressGuid);
    DataInHob            = GET_GUID_HOB_DATA (GuidHob);
    FwCfgSelectorAddress = (UINT64)(*(UINTN *)DataInHob);
  }

  return FwCfgSelectorAddress;
}

/**
  To get firmware configure Data address.

  @param VOID

  @retval  firmware configure data address
**/
UINTN
EFIAPI
QemuGetFwCfgDataAddress (
  VOID
  )
{
  UINTN              FwCfgDataAddress;
  EFI_HOB_GUID_TYPE  *GuidHob;
  VOID               *DataInHob;

  FwCfgDataAddress = mFwCfgDataAddress;
  GuidHob          = NULL;
  DataInHob        = NULL;

  if (FwCfgDataAddress == 0) {
    GuidHob          = GetFirstGuidHob (&mFwCfgDataAddressGuid);
    DataInHob        = GET_GUID_HOB_DATA (GuidHob);
    FwCfgDataAddress = (UINT64)(*(UINTN *)DataInHob);
  }

  return FwCfgDataAddress;
}

/**
  Selects a firmware configuration item for reading.

  Following this call, any data read from this item will start from
  the beginning of the configuration item's data.

  @param[in] QemuFwCfgItem - Firmware Configuration item to read
**/
VOID
EFIAPI
QemuFwCfgSelectItem (
  IN FIRMWARE_CONFIG_ITEM  QemuFwCfgItem
  )
{
  UINTN  FwCfgSelectorAddress;

  FwCfgSelectorAddress = QemuGetFwCfgSelectorAddress ();
  MmioWrite16 (FwCfgSelectorAddress, SwapBytes16 ((UINT16)(UINTN)QemuFwCfgItem));
}

/**
  Slow READ_BYTES_FUNCTION.

  @param[in]  The size of the data to be read.
  @param[in]  Buffer    The buffer that stores the readout data.
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
  UINTN  FwCfgDataAddress;

  Left = Size & 7;

  Size -= Left;
  Ptr   = Buffer;
  End   = Ptr + Size;

  FwCfgDataAddress = QemuGetFwCfgDataAddress ();
  while (Ptr < End) {
    *(UINT64 *)Ptr = MmioRead64 (FwCfgDataAddress);
    Ptr           += 8;
  }

  if (Left & 4) {
    *(UINT32 *)Ptr = MmioRead32 (FwCfgDataAddress);
    Ptr           += 4;
  }

  if (Left & 2) {
    *(UINT16 *)Ptr = MmioRead16 (FwCfgDataAddress);
    Ptr           += 2;
  }

  if (Left & 1) {
    *Ptr = MmioRead8 (FwCfgDataAddress);
  }
}

/**
  Slow WRITE_BYTES_FUNCTION.

  @param[in]  The size of the data to be write.
  @param[in]  Buffer    The buffer that stores the writein data.
**/
VOID
EFIAPI
MmioWriteBytes (
  IN UINTN  Size,
  IN VOID   *Buffer OPTIONAL
  )
{
  UINTN  Idx;
  UINTN  FwCfgDataAddress;

  FwCfgDataAddress = QemuGetFwCfgDataAddress ();
  for (Idx = 0; Idx < Size; ++Idx) {
    MmioWrite8 (FwCfgDataAddress, ((UINT8 *)Buffer)[Idx]);
  }
}

/**
  Reads firmware configuration bytes into a buffer

  @param[in] Size - Size in bytes to read
  @param[in] Buffer - Buffer to store data into  (OPTIONAL if Size is 0)
**/
VOID
EFIAPI
InternalQemuFwCfgReadBytes (
  IN UINTN  Size,
  IN VOID   *Buffer  OPTIONAL
  )
{
  if ((InternalQemuFwCfgDmaIsAvailable ()) &&
      (Size <= MAX_UINT32))
  {
    InternalQemuFwCfgDmaBytes ((UINT32)Size, Buffer, FW_CFG_DMA_CTL_READ);
    return;
  }

  MmioReadBytes (Size, Buffer);
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
  IN UINTN  Size,
  IN VOID   *Buffer
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
  IN UINTN  Size,
  IN VOID   *Buffer
  )
{
  if (InternalQemuFwCfgIsAvailable ()) {
    if ((InternalQemuFwCfgDmaIsAvailable ()) &&
        (Size <= MAX_UINT32))
    {
      InternalQemuFwCfgDmaBytes ((UINT32)Size, Buffer, FW_CFG_DMA_CTL_WRITE);
      return;
    }

    MmioWriteBytes (Size, Buffer);
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
  IN UINTN  Size
  )
{
  UINTN  ChunkSize;
  UINT8  SkipBuffer[256];

  if (!InternalQemuFwCfgIsAvailable ()) {
    return;
  }

  if ((InternalQemuFwCfgDmaIsAvailable ()) &&
      (Size <= MAX_UINT32))
  {
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
    MmioReadBytes (ChunkSize, SkipBuffer);
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
  UINT8  Result;

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
  UINT16  Result;

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
  UINT32  Result;

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
  UINT64  Result;

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
  UINT32  Count;
  UINT32  Idx;

  if (!InternalQemuFwCfgIsAvailable ()) {
    return RETURN_UNSUPPORTED;
  }

  QemuFwCfgSelectItem (QemuFwCfgItemFileDir);
  Count = SwapBytes32 (QemuFwCfgRead32 ());

  for (Idx = 0; Idx < Count; ++Idx) {
    UINT32  FileSize;
    UINT16  FileSelect;
    CHAR8   FileName[QEMU_FW_CFG_FNAME_SIZE];

    FileSize   = QemuFwCfgRead32 ();
    FileSelect = QemuFwCfgRead16 ();
    QemuFwCfgRead16 (); // skip the field called "reserved"
    InternalQemuFwCfgReadBytes (sizeof (FileName), FileName);

    if (AsciiStrCmp (Name, FileName) == 0) {
      *Item = SwapBytes16 (FileSelect);
      *Size = SwapBytes32 (FileSize);
      return RETURN_SUCCESS;
    }
  }

  return RETURN_NOT_FOUND;
}

/**
  firmware config initialize.

  @param  VOID

  @return    RETURN_SUCCESS  Initialization succeeded.
**/
RETURN_STATUS
EFIAPI
FdtQemuFwCfgInitialize (
  VOID
  )
{
  VOID          *DeviceTreeBase;
  INT32         Node;
  INT32         Prev;
  CONST CHAR8   *Type;
  INT32         Len;
  CONST UINT64  *RegProp;
  UINT64        FwCfgSelectorAddress;
  UINT64        FwCfgDataAddress;
  UINT64        FwCfgDataSize;

  DeviceTreeBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  ASSERT (DeviceTreeBase != NULL);
  //
  // Make sure we have a valid device tree blob
  //
  ASSERT (fdt_check_header (DeviceTreeBase) == 0);

  for (Prev = 0; ; Prev = Node) {
    Node = fdt_next_node (DeviceTreeBase, Prev, NULL);
    if (Node < 0) {
      break;
    }

    //
    // Check for memory node
    //
    Type = fdt_getprop (DeviceTreeBase, Node, "compatible", &Len);
    if ((Type) &&
        (AsciiStrnCmp (Type, "qemu,fw-cfg-mmio", Len) == 0))
    {
      //
      // Get the 'reg' property of this node. For now, we will assume
      // two 8 byte quantities for base and size, respectively.
      //
      RegProp = fdt_getprop (DeviceTreeBase, Node, "reg", &Len);
      if ((RegProp != 0) &&
          (Len == (2 * sizeof (UINT64))))
      {
        FwCfgDataAddress     = SwapBytes64 (RegProp[0]);
        FwCfgDataSize        = 8;
        FwCfgSelectorAddress = FwCfgDataAddress + FwCfgDataSize;

        mFwCfgSelectorAddress = FwCfgSelectorAddress;
        mFwCfgDataAddress     = FwCfgDataAddress;

        BuildGuidDataHob (
          &mFwCfgSelectorAddressGuid,
          (VOID *)&FwCfgSelectorAddress,
          sizeof (UINT64)
          );

        BuildGuidDataHob (
          &mFwCfgDataAddressGuid,
          (VOID *)&FwCfgDataAddress,
          sizeof (UINT64)
          );
        break;
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to parse FDT QemuCfg node\n",
          __func__
          ));
        break;
      }
    }
  }

  return RETURN_SUCCESS;
}
