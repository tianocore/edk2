/** @file
Helper routines with common PEI / DXE implementation.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CommonHeader.h"

CHAR16 *mPlatTypeNameTable[]  = { EFI_PLATFORM_TYPE_NAME_TABLE_DEFINITION };
UINTN mPlatTypeNameTableLen  = ((sizeof(mPlatTypeNameTable)) / sizeof (CHAR16 *));

//
// Routines defined in other source modules of this component.
//

//
// Routines local to this source module.
//

//
// Routines shared with other souce modules in this component.
//

EFI_STATUS
WriteFirstFreeSpiProtect (
  IN CONST UINT32                         PchRootComplexBar,
  IN CONST UINT32                         DirectValue,
  IN CONST UINT32                         BaseAddress,
  IN CONST UINT32                         Length,
  OUT UINT32                              *OffsetPtr
  )
{
  UINT32                            RegVal;
  UINT32                            Offset;
  UINT32                            StepLen;

  ASSERT (PchRootComplexBar > 0);

  Offset = 0;
  if (OffsetPtr != NULL) {
    *OffsetPtr = Offset;
  }
  if (MmioRead32 (PchRootComplexBar + R_QNC_RCRB_SPIPBR0) == 0) {
    Offset = R_QNC_RCRB_SPIPBR0;
  } else {
    if (MmioRead32 (PchRootComplexBar + R_QNC_RCRB_SPIPBR1) == 0) {
      Offset = R_QNC_RCRB_SPIPBR1;
    } else {
      if (MmioRead32 (PchRootComplexBar + R_QNC_RCRB_SPIPBR2) == 0) {
        Offset = R_QNC_RCRB_SPIPBR2;
      }
    }
  }
  if (Offset != 0) {
    if (DirectValue == 0) {
      StepLen = ALIGN_VALUE (Length,SIZE_4KB);   // Bring up to 4K boundary.
      RegVal = BaseAddress + StepLen - 1;
      RegVal &= 0x00FFF000;                     // Set EDS Protected Range Limit (PRL).
      RegVal |= ((BaseAddress >> 12) & 0xfff);  // or in EDS Protected Range Base (PRB).
    } else {
      RegVal = DirectValue;
    }
    //
    // Enable protection.
    //
    RegVal |= B_QNC_RCRB_SPIPBRn_WPE;
    MmioWrite32 (PchRootComplexBar + Offset, RegVal);
    if (RegVal == MmioRead32 (PchRootComplexBar + Offset)) {
      if (OffsetPtr != NULL) {
        *OffsetPtr = Offset;
      }
      return EFI_SUCCESS;
    }
    return EFI_DEVICE_ERROR;
  }
  return EFI_NOT_FOUND;
}

//
// Routines exported by this component.
//

/**
  Read 8bit character from debug stream.

  Block until character is read.

  @return 8bit character read from debug stream.

**/
CHAR8
EFIAPI
PlatformDebugPortGetChar8 (
  VOID
  )
{
  CHAR8                             Got;

  do {
    if (SerialPortPoll ()) {
      if (SerialPortRead ((UINT8 *) &Got, 1) == 1) {
        break;
      }
    }
  } while (TRUE);

  return Got;
}

/**
  Clear SPI Protect registers.

  @retval EFI_SUCCESS        SPI protect registers cleared.
  @retval EFI_ACCESS_DENIED  Unable to clear SPI protect registers.
**/

EFI_STATUS
EFIAPI
PlatformClearSpiProtect (
  VOID
  )
{
  UINT32                            PchRootComplexBar;

  PchRootComplexBar = QNC_RCRB_BASE;
  //
  // Check if the SPI interface has been locked-down.
  //
  if ((MmioRead16 (PchRootComplexBar + R_QNC_RCRB_SPIS) & B_QNC_RCRB_SPIS_SCL) != 0) {
    return EFI_ACCESS_DENIED;
  }
  MmioWrite32 (PchRootComplexBar + R_QNC_RCRB_SPIPBR0, 0);
  if (MmioRead32 (PchRootComplexBar + R_QNC_RCRB_SPIPBR0) != 0) {
    return EFI_ACCESS_DENIED;
  }
  MmioWrite32 (PchRootComplexBar + R_QNC_RCRB_SPIPBR1, 0);
  if (MmioRead32 (PchRootComplexBar + R_QNC_RCRB_SPIPBR0) != 0) {
    return EFI_ACCESS_DENIED;
  }
  MmioWrite32 (PchRootComplexBar + R_QNC_RCRB_SPIPBR2, 0);
  if (MmioRead32 (PchRootComplexBar + R_QNC_RCRB_SPIPBR0) != 0) {
    return EFI_ACCESS_DENIED;
  }
  return EFI_SUCCESS;
}

/**
  Determine if an SPI address range is protected.

  @param  SpiBaseAddress  Base of SPI range.
  @param  Length          Length of SPI range.

  @retval TRUE       Range is protected.
  @retval FALSE      Range is not protected.
**/
BOOLEAN
EFIAPI
PlatformIsSpiRangeProtected (
  IN CONST UINT32                         SpiBaseAddress,
  IN CONST UINT32                         Length
  )
{
  UINT32                            RegVal;
  UINT32                            Offset;
  UINT32                            Limit;
  UINT32                            ProtectedBase;
  UINT32                            ProtectedLimit;
  UINT32                            PchRootComplexBar;

  PchRootComplexBar = QNC_RCRB_BASE;

  if (Length > 0) {
    Offset = R_QNC_RCRB_SPIPBR0;
    Limit = SpiBaseAddress + (Length - 1);
    do {
      RegVal = MmioRead32 (PchRootComplexBar + Offset);
      if ((RegVal & B_QNC_RCRB_SPIPBRn_WPE) != 0) {
        ProtectedBase = (RegVal & 0xfff) << 12;
        ProtectedLimit = (RegVal & 0x00fff000) + 0xfff;
        if (SpiBaseAddress >= ProtectedBase && Limit <= ProtectedLimit) {
          return TRUE;
        }
      }
      if (Offset == R_QNC_RCRB_SPIPBR0) {
        Offset = R_QNC_RCRB_SPIPBR1;
      } else if (Offset == R_QNC_RCRB_SPIPBR1) {
        Offset = R_QNC_RCRB_SPIPBR2;
      } else {
        break;
      }
    } while (TRUE);
  }
  return FALSE;
}

/**
  Set Legacy GPIO Level

  @param  LevelRegOffset      GPIO level register Offset from GPIO Base Address.
  @param  GpioNum             GPIO bit to change.
  @param  HighLevel           If TRUE set GPIO High else Set GPIO low.

**/
VOID
EFIAPI
PlatformLegacyGpioSetLevel (
  IN CONST UINT32       LevelRegOffset,
  IN CONST UINT32       GpioNum,
  IN CONST BOOLEAN      HighLevel
  )
{
  UINT32  RegValue;
  UINT32  GpioBaseAddress;
  UINT32  GpioNumMask;

  GpioBaseAddress =  LpcPciCfg32 (R_QNC_LPC_GBA_BASE) & B_QNC_LPC_GPA_BASE_MASK;
  ASSERT (GpioBaseAddress > 0);

  RegValue = IoRead32 (GpioBaseAddress + LevelRegOffset);
  GpioNumMask = (1 << GpioNum);
  if (HighLevel) {
    RegValue |= (GpioNumMask);
  } else {
    RegValue &= ~(GpioNumMask);
  }
  IoWrite32 (GpioBaseAddress + LevelRegOffset, RegValue);
}

/**
  Get Legacy GPIO Level

  @param  LevelRegOffset      GPIO level register Offset from GPIO Base Address.
  @param  GpioNum             GPIO bit to check.

  @retval TRUE       If bit is SET.
  @retval FALSE      If bit is CLEAR.

**/
BOOLEAN
EFIAPI
PlatformLegacyGpioGetLevel (
  IN CONST UINT32       LevelRegOffset,
  IN CONST UINT32       GpioNum
  )
{
  UINT32  RegValue;
  UINT32  GpioBaseAddress;
  UINT32  GpioNumMask;

  GpioBaseAddress =  LpcPciCfg32 (R_QNC_LPC_GBA_BASE) & B_QNC_LPC_GPA_BASE_MASK;
  RegValue = IoRead32 (GpioBaseAddress + LevelRegOffset);
  GpioNumMask = (1 << GpioNum);
  return ((RegValue & GpioNumMask) != 0);
}
