/** @file
Helper routines with common PEI / DXE implementation.

Copyright (c) 2013-2016 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"
#include <Library/I2cLib.h>

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


BOOLEAN
Pcal9555GetPortRegBit (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum,
  IN CONST UINT8                          RegBase
  )
{
  EFI_STATUS                        Status;
  UINTN                             ReadLength;
  UINTN                             WriteLength;
  UINT8                             Data[2];
  EFI_I2C_DEVICE_ADDRESS            I2cDeviceAddr;
  EFI_I2C_ADDR_MODE                 I2cAddrMode;
  UINT8                             *RegValuePtr;
  UINT8                             GpioNumMask;
  UINT8                             SubAddr;

  I2cDeviceAddr.I2CDeviceAddress = (UINTN)Pcal9555SlaveAddr;
  I2cAddrMode = EfiI2CSevenBitAddrMode;

  if (GpioNum < 8) {
    SubAddr = RegBase;
    GpioNumMask = (UINT8)(1 << GpioNum);
  } else {
    SubAddr = RegBase + 1;
    GpioNumMask = (UINT8)(1 << (GpioNum - 8));
  }

  //
  // Output port value always at 2nd byte in Data variable.
  //
  RegValuePtr = &Data[1];

  //
  // On read entry sub address at 2nd byte, on read exit output
  // port value in 2nd byte.
  //
  Data[1] = SubAddr;
  WriteLength = 1;
  ReadLength = 1;
  Status = I2cReadMultipleByte (
    I2cDeviceAddr,
    I2cAddrMode,
    &WriteLength,
    &ReadLength,
    &Data[1]
    );
  ASSERT_EFI_ERROR (Status);

  //
  // Adjust output port bit given callers request.
  //
  return ((*RegValuePtr & GpioNumMask) != 0);
}

VOID
Pcal9555SetPortRegBit (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum,
  IN CONST UINT8                          RegBase,
  IN CONST BOOLEAN                        LogicOne
  )
{
  EFI_STATUS                        Status;
  UINTN                             ReadLength;
  UINTN                             WriteLength;
  UINT8                             Data[2];
  EFI_I2C_DEVICE_ADDRESS            I2cDeviceAddr;
  EFI_I2C_ADDR_MODE                 I2cAddrMode;
  UINT8                             *RegValuePtr;
  UINT8                             GpioNumMask;
  UINT8                             SubAddr;

  I2cDeviceAddr.I2CDeviceAddress = (UINTN)Pcal9555SlaveAddr;
  I2cAddrMode = EfiI2CSevenBitAddrMode;

  if (GpioNum < 8) {
    SubAddr = RegBase;
    GpioNumMask = (UINT8)(1 << GpioNum);
  } else {
    SubAddr = RegBase + 1;
    GpioNumMask = (UINT8)(1 << (GpioNum - 8));
  }

  //
  // Output port value always at 2nd byte in Data variable.
  //
  RegValuePtr = &Data[1];

  //
  // On read entry sub address at 2nd byte, on read exit output
  // port value in 2nd byte.
  //
  Data[1] = SubAddr;
  WriteLength = 1;
  ReadLength = 1;
  Status = I2cReadMultipleByte (
    I2cDeviceAddr,
    I2cAddrMode,
    &WriteLength,
    &ReadLength,
    &Data[1]
    );
  ASSERT_EFI_ERROR (Status);

  //
  // Adjust output port bit given callers request.
  //
  if (LogicOne) {
    *RegValuePtr = *RegValuePtr | GpioNumMask;
  } else {
    *RegValuePtr = *RegValuePtr & ~(GpioNumMask);
  }

  //
  // Update register. Sub address at 1st byte, value at 2nd byte.
  //
  WriteLength = 2;
  Data[0] = SubAddr;
  Status = I2cWriteMultipleByte (
    I2cDeviceAddr,
    I2cAddrMode,
    &WriteLength,
    Data
    );
  ASSERT_EFI_ERROR (Status);
}

/**
Set the direction of Pcal9555 IO Expander GPIO pin.

@param  Pcal9555SlaveAddr  I2c Slave address of Pcal9555 Io Expander.
@param  GpioNum            Gpio direction to configure - values 0-7 for Port0
and 8-15 for Port1.
@param  CfgAsInput         If TRUE set pin direction as input else set as output.

**/
VOID
EFIAPI
PlatformPcal9555GpioSetDir (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum,
  IN CONST BOOLEAN                        CfgAsInput
  )
{
  Pcal9555SetPortRegBit (
    Pcal9555SlaveAddr,
    GpioNum,
    PCAL9555_REG_CFG_PORT0,
    CfgAsInput
    );
}

/**
Set the level of Pcal9555 IO Expander GPIO high or low.

@param  Pcal9555SlaveAddr  I2c Slave address of Pcal9555 Io Expander.
@param  GpioNum            Gpio to change values 0-7 for Port0 and 8-15
for Port1.
@param  HighLevel          If TRUE set pin high else set pin low.

**/
VOID
EFIAPI
PlatformPcal9555GpioSetLevel (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum,
  IN CONST BOOLEAN                        HighLevel
  )
{
  Pcal9555SetPortRegBit (
    Pcal9555SlaveAddr,
    GpioNum,
    PCAL9555_REG_OUT_PORT0,
    HighLevel
    );
}

/**

Enable pull-up/pull-down resistors of Pcal9555 GPIOs.

@param  Pcal9555SlaveAddr  I2c Slave address of Pcal9555 Io Expander.
@param  GpioNum            Gpio to change values 0-7 for Port0 and 8-15
for Port1.

**/
VOID
EFIAPI
PlatformPcal9555GpioEnablePull (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum
  )
{
  Pcal9555SetPortRegBit (
    Pcal9555SlaveAddr,
    GpioNum,
    PCAL9555_REG_PULL_EN_PORT0,
    TRUE
    );
}

/**

Disable pull-up/pull-down resistors of Pcal9555 GPIOs.

@param  Pcal9555SlaveAddr  I2c Slave address of Pcal9555 Io Expander.
@param  GpioNum            Gpio to change values 0-7 for Port0 and 8-15
for Port1.

**/
VOID
EFIAPI
PlatformPcal9555GpioDisablePull (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum
  )
{
  Pcal9555SetPortRegBit (
    Pcal9555SlaveAddr,
    GpioNum,
    PCAL9555_REG_PULL_EN_PORT0,
    FALSE
    );
}

/**

Get state of Pcal9555 GPIOs.

@param  Pcal9555SlaveAddr  I2c Slave address of Pcal9555 Io Expander.
@param  GpioNum            Gpio to change values 0-7 for Port0 and 8-15
for Port1.

@retval TRUE               GPIO pin is high
@retval FALSE              GPIO pin is low
**/
BOOLEAN
EFIAPI
PlatformPcal9555GpioGetState (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum
  )
{
  return Pcal9555GetPortRegBit (Pcal9555SlaveAddr, GpioNum, PCAL9555_REG_IN_PORT0);
}


