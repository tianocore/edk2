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
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/QemuFwCfgLib.h>

STATIC UINTN mFwCfgSelectorAddress;
STATIC UINTN mFwCfgDataAddress;


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
    if (Signature != SIGNATURE_32 ('Q', 'E', 'M', 'U')) {
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
  Reads firmware configuration bytes into a buffer

  @param[in] Size    Size in bytes to read
  @param[in] Buffer  Buffer to store data into  (OPTIONAL if Size is 0)

**/
STATIC
VOID
EFIAPI
InternalQemuFwCfgReadBytes (
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
