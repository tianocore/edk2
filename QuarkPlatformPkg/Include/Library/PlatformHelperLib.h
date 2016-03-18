/** @file
PlatformHelperLib function prototype definitions.

Copyright (c) 2013 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PLATFORM_HELPER_LIB_H__
#define __PLATFORM_HELPER_LIB_H__

#include "Platform.h"

//
// Function prototypes for routines exported by this library.
//

/**
  Find pointer to RAW data in Firmware volume file.

  @param   FvNameGuid       Firmware volume to search. If == NULL search all.
  @param   FileNameGuid     Firmware volume file to search for.
  @param   SectionData      Pointer to RAW data section of found file.
  @param   SectionDataSize  Pointer to UNITN to get size of RAW data.

  @retval  EFI_SUCCESS            Raw Data found.
  @retval  EFI_INVALID_PARAMETER  FileNameGuid == NULL.
  @retval  EFI_NOT_FOUND          Firmware volume file not found.
  @retval  EFI_UNSUPPORTED        Unsupported in current enviroment (PEI or DXE).

**/
EFI_STATUS
EFIAPI
PlatformFindFvFileRawDataSection (
  IN CONST EFI_GUID                 *FvNameGuid OPTIONAL,
  IN CONST EFI_GUID                 *FileNameGuid,
  OUT VOID                          **SectionData,
  OUT UINTN                         *SectionDataSize
  );

/**
  Read 8bit character from debug stream.

  Block until character is read.

  @return 8bit character read from debug stream.

**/
CHAR8
EFIAPI
PlatformDebugPortGetChar8 (
  VOID
  );

/**
  Find free spi protect register and write to it to protect a flash region.

  @param   DirectValue      Value to directly write to register.
                            if DirectValue == 0 the use Base & Length below.
  @param   BaseAddress      Base address of region in Flash Memory Map.
  @param   Length           Length of region to protect.

  @retval  EFI_SUCCESS      Free spi protect register found & written.
  @retval  EFI_NOT_FOUND    Free Spi protect register not found.
  @retval  EFI_DEVICE_ERROR Unable to write to spi protect register.

**/
EFI_STATUS
EFIAPI
PlatformWriteFirstFreeSpiProtect (
  IN CONST UINT32                         DirectValue,
  IN CONST UINT32                         BaseAddress,
  IN CONST UINT32                         Length
  );

/**
  Lock legacy SPI static configuration information.

  Function will assert if unable to lock config.

**/
VOID
EFIAPI
PlatformFlashLockConfig (
  VOID
  );

/**
  Lock regions and config of SPI flash given the policy for this platform.

  Function will assert if unable to lock regions or config.

  @param   PreBootPolicy    If TRUE do Pre Boot Flash Lock Policy.

**/
VOID
EFIAPI
PlatformFlashLockPolicy (
  IN CONST BOOLEAN                        PreBootPolicy
  );

/**
  Erase and Write to platform flash.

  Routine accesses one flash block at a time, each access consists
  of an erase followed by a write of FLASH_BLOCK_SIZE. One or both
  of DoErase & DoWrite params must be TRUE.

  Limitations:-
    CpuWriteAddress must be aligned to FLASH_BLOCK_SIZE.
    DataSize must be a multiple of FLASH_BLOCK_SIZE.

  @param   Smst                   If != NULL then InSmm and use to locate
                                  SpiProtocol.
  @param   CpuWriteAddress        Address in CPU memory map of flash region.
  @param   Data                   The buffer containing the data to be written.
  @param   DataSize               Amount of data to write.
  @param   DoErase                Earse each block.
  @param   DoWrite                Write to each block.

  @retval  EFI_SUCCESS            Operation successful.
  @retval  EFI_NOT_READY          Required resources not setup.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  Others                 Unexpected error happened.

**/
EFI_STATUS
EFIAPI
PlatformFlashEraseWrite (
  IN  VOID                              *Smst,
  IN  UINTN                             CpuWriteAddress,
  IN  UINT8                             *Data,
  IN  UINTN                             DataSize,
  IN  BOOLEAN                           DoErase,
  IN  BOOLEAN                           DoWrite
  );

/** Check if System booted with recovery Boot Stage1 image.

  @retval  TRUE    If system booted with recovery Boot Stage1 image.
  @retval  FALSE   If system booted with normal stage1 image.

**/
BOOLEAN
EFIAPI
PlatformIsBootWithRecoveryStage1 (
  VOID
  );

/**
  Clear SPI Protect registers.

  @retval EFI_SUCESS         SPI protect registers cleared.
  @retval EFI_ACCESS_DENIED  Unable to clear SPI protect registers.
**/

EFI_STATUS
EFIAPI
PlatformClearSpiProtect (
  VOID
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

BOOLEAN
EFIAPI
PlatformPcal9555GpioGetState (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum
  );

/**
  Init platform LEDs into known state.

  @param   PlatformType     Executing platform type.

  @retval  EFI_SUCCESS      Operation success.

**/
EFI_STATUS
EFIAPI
PlatformLedInit (
  IN CONST EFI_PLATFORM_TYPE              Type
  );

/**
  Turn on or off platform flash update LED.

  @param   PlatformType     Executing platform type.
  @param   TurnOn           If TRUE turn on else turn off.

  @retval  EFI_SUCCESS      Operation success.

**/
EFI_STATUS
EFIAPI
PlatformFlashUpdateLed (
  IN CONST EFI_PLATFORM_TYPE              Type,
  IN CONST BOOLEAN                        TurnOn
  );

#endif // #ifndef __PLATFORM_HELPER_LIB_H__
