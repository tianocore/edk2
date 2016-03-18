/** @file

  Stateful and implicitly initialized fw_cfg library implementation.

  Copyright (C) 2013 - 2014, Red Hat, Inc.
  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/QemuFwCfgLib.h>

STATIC UINTN mFwCfgSelectorAddress;
STATIC UINTN mFwCfgDataAddress;
STATIC UINTN mFwCfgDmaAddress;

/**
  Reads firmware configuration bytes into a buffer

  @param[in] Size    Size in bytes to read
  @param[in] Buffer  Buffer to store data into  (OPTIONAL if Size is 0)

**/
typedef
VOID (EFIAPI READ_BYTES_FUNCTION) (
  IN UINTN Size,
  IN VOID  *Buffer OPTIONAL
  );

//
// Forward declaration of the two implementations we have.
//
STATIC READ_BYTES_FUNCTION MmioReadBytes;
STATIC READ_BYTES_FUNCTION DmaReadBytes;

//
// This points to the one we detect at runtime.
//
STATIC READ_BYTES_FUNCTION *InternalQemuFwCfgReadBytes = MmioReadBytes;

//
// Communication structure for DmaReadBytes(). All fields are encoded in big
// endian.
//
#pragma pack (1)
typedef struct {
  UINT32 Control;
  UINT32 Length;
  UINT64 Address;
} FW_CFG_DMA_ACCESS;
#pragma pack ()

//
// Macros for the FW_CFG_DMA_ACCESS.Control bitmap (in native encoding).
//
#define FW_CFG_DMA_CTL_ERROR  BIT0
#define FW_CFG_DMA_CTL_READ   BIT1
#define FW_CFG_DMA_CTL_SKIP   BIT2
#define FW_CFG_DMA_CTL_SELECT BIT3


/**
  Returns a boolean indicating if the firmware configuration interface is
  available for library-internal purposes.

  This function never changes fw_cfg state.

  @retval TRUE   The interface is available internally.
  @retval FALSE  The interface is not available internally.
**/
BOOLEAN
EFIAPI
InternalQemuFwCfgIsAvailable (
  VOID
  )
{
  return (BOOLEAN)(mFwCfgSelectorAddress != 0 && mFwCfgDataAddress != 0);
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
  return InternalQemuFwCfgIsAvailable ();
}


RETURN_STATUS
EFIAPI
QemuFwCfgInitialize (
  VOID
  )
{
  mFwCfgSelectorAddress = (UINTN)PcdGet64 (PcdFwCfgSelectorAddress);
  mFwCfgDataAddress     = (UINTN)PcdGet64 (PcdFwCfgDataAddress);

  if (InternalQemuFwCfgIsAvailable ()) {
    UINT32 Signature;

    QemuFwCfgSelectItem (QemuFwCfgItemSignature);
    Signature = QemuFwCfgRead32 ();
    if (Signature == SIGNATURE_32 ('Q', 'E', 'M', 'U')) {
      //
      // For DMA support, we require the DTB to advertise the register, and the
      // feature bitmap (which we read without DMA) to confirm the feature.
      //
      if (PcdGet64 (PcdFwCfgDmaAddress) != 0) {
        UINT32 Features;

        QemuFwCfgSelectItem (QemuFwCfgItemInterfaceVersion);
        Features = QemuFwCfgRead32 ();
        if ((Features & BIT1) != 0) {
          mFwCfgDmaAddress = PcdGet64 (PcdFwCfgDmaAddress);
          InternalQemuFwCfgReadBytes = DmaReadBytes;
        }
      }
    } else {
      mFwCfgSelectorAddress = 0;
      mFwCfgDataAddress     = 0;
    }
  }
  return RETURN_SUCCESS;
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
  IN FIRMWARE_CONFIG_ITEM QemuFwCfgItem
  )
{
  if (InternalQemuFwCfgIsAvailable ()) {
    MmioWrite16 (mFwCfgSelectorAddress, SwapBytes16 ((UINT16)QemuFwCfgItem));
  }
}


/**
  Slow READ_BYTES_FUNCTION.
**/
STATIC
VOID
EFIAPI
MmioReadBytes (
  IN UINTN Size,
  IN VOID  *Buffer OPTIONAL
  )
{
  UINTN Left;
  UINT8 *Ptr;
  UINT8 *End;

#ifdef MDE_CPU_AARCH64
  Left = Size & 7;
#else
  Left = Size & 3;
#endif

  Size -= Left;
  Ptr = Buffer;
  End = Ptr + Size;

#ifdef MDE_CPU_AARCH64
  while (Ptr < End) {
    *(UINT64 *)Ptr = MmioRead64 (mFwCfgDataAddress);
    Ptr += 8;
  }
  if (Left & 4) {
    *(UINT32 *)Ptr = MmioRead32 (mFwCfgDataAddress);
    Ptr += 4;
  }
#else
  while (Ptr < End) {
    *(UINT32 *)Ptr = MmioRead32 (mFwCfgDataAddress);
    Ptr += 4;
  }
#endif

  if (Left & 2) {
    *(UINT16 *)Ptr = MmioRead16 (mFwCfgDataAddress);
    Ptr += 2;
  }
  if (Left & 1) {
    *Ptr = MmioRead8 (mFwCfgDataAddress);
  }
}


/**
  Fast READ_BYTES_FUNCTION.
**/
STATIC
VOID
EFIAPI
DmaReadBytes (
  IN UINTN Size,
  IN VOID  *Buffer OPTIONAL
  )
{
  volatile FW_CFG_DMA_ACCESS Access;
  UINT32                     Status;

  if (Size == 0) {
    return;
  }

  ASSERT (Size <= MAX_UINT32);

  Access.Control = SwapBytes32 (FW_CFG_DMA_CTL_READ);
  Access.Length  = SwapBytes32 ((UINT32)Size);
  Access.Address = SwapBytes64 ((UINT64)(UINTN)Buffer);

  //
  // We shouldn't start the transfer before setting up Access.
  //
  MemoryFence ();

  //
  // This will fire off the transfer.
  //
#ifdef MDE_CPU_AARCH64
  MmioWrite64 (mFwCfgDmaAddress, SwapBytes64 ((UINT64)&Access));
#else
  MmioWrite32 ((UINT32)(mFwCfgDmaAddress + 4), SwapBytes32 ((UINT32)&Access));
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
  Reads firmware configuration bytes into a buffer

  If called multiple times, then the data read will continue at the offset of
  the firmware configuration item where the previous read ended.

  @param[in] Size    Size in bytes to read
  @param[in] Buffer  Buffer to store data into

**/
VOID
EFIAPI
QemuFwCfgReadBytes (
  IN UINTN Size,
  IN VOID  *Buffer
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

  If called multiple times, then the data written will continue at the offset
  of the firmware configuration item where the previous write ended.

  @param[in] Size    Size in bytes to write
  @param[in] Buffer  Buffer to read data from

**/
VOID
EFIAPI
QemuFwCfgWriteBytes (
  IN UINTN                  Size,
  IN VOID                   *Buffer
  )
{
  if (InternalQemuFwCfgIsAvailable ()) {
    UINTN Idx;

    for (Idx = 0; Idx < Size; ++Idx) {
      MmioWrite8 (mFwCfgDataAddress, ((UINT8 *)Buffer)[Idx]);
    }
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
  UINT8 Result;

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
  UINT16 Result;

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
  UINT32 Result;

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
  UINT64 Result;

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
    CHAR8  FName[QEMU_FW_CFG_FNAME_SIZE];

    FileSize   = QemuFwCfgRead32 ();
    FileSelect = QemuFwCfgRead16 ();
    QemuFwCfgRead16 (); // skip the field called "reserved"
    InternalQemuFwCfgReadBytes (sizeof (FName), FName);

    if (AsciiStrCmp (Name, FName) == 0) {
      *Item = (FIRMWARE_CONFIG_ITEM) SwapBytes16 (FileSelect);
      *Size = SwapBytes32 (FileSize);
      return RETURN_SUCCESS;
    }
  }

  return RETURN_NOT_FOUND;
}


/**
  Determine if S3 support is explicitly enabled.

  @retval TRUE   if S3 support is explicitly enabled.
          FALSE  otherwise. This includes unavailability of the firmware
                 configuration interface.
**/
BOOLEAN
EFIAPI
QemuFwCfgS3Enabled (
  VOID
  )
{
  return FALSE;
}
