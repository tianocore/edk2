/** @file  ArmVExpressSysConfig.c

  Copyright (c) 2011-2012, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>

#include <Library/ArmPlatformSysConfigLib.h>
#include <ArmPlatform.h>

//
// SYS_CFGCTRL Bits
//
#define SYS_CFGCTRL_START                 BIT31
#define SYS_CFGCTRL_READ                  (0 << 30)
#define SYS_CFGCTRL_WRITE                 (1 << 30)
#define SYS_CFGCTRL_FUNCTION(fun)         (((fun ) &  0x3F) << 20)
#define SYS_CFGCTRL_SITE(site)            (((site) &   0x3) << 16)
#define SYS_CFGCTRL_POSITION(pos)         (((pos ) &   0xF) << 12)
#define SYS_CFGCTRL_DEVICE(dev)            ((dev ) & 0xFFF)

//
// SYS_CFGSTAT Bits
//
#define SYS_CFGSTAT_ERROR                 BIT1
#define SYS_CFGSTAT_COMPLETE              BIT0

/****************************************************************************
 *
 *  This file makes it easier to access the System Configuration Registers
 *  in the ARM Versatile Express motherboard.
 *
 ****************************************************************************/

RETURN_STATUS
ArmPlatformSysConfigInitialize (
  VOID
  )
{
  return RETURN_SUCCESS;
}

/***************************************
 * GENERAL FUNCTION: AccessSysCfgRegister
 * Interacts with
 *    SYS_CFGSTAT
 *    SYS_CFGDATA
 *    SYS_CFGCTRL
 * for setting and for reading out values
 ***************************************/

RETURN_STATUS
AccessSysCfgRegister (
  IN     UINT32   ReadWrite,
  IN     UINT32   Function,
  IN     UINT32   Site,
  IN     UINT32   Position,
  IN     UINT32   Device,
  IN OUT UINT32*  Data
  )
{
  UINT32          SysCfgCtrl;

  // Clear the COMPLETE bit
  MmioAnd32(ARM_VE_SYS_CFGSTAT_REG, ~SYS_CFGSTAT_COMPLETE);

  // If writing, then set the data value
  if(ReadWrite == SYS_CFGCTRL_WRITE) {
    MmioWrite32(ARM_VE_SYS_CFGDATA_REG, *Data);
  }

  // Set the control value
  SysCfgCtrl = SYS_CFGCTRL_START | ReadWrite | SYS_CFGCTRL_FUNCTION(Function) | SYS_CFGCTRL_SITE(Site) |
      SYS_CFGCTRL_POSITION(Position) | SYS_CFGCTRL_DEVICE(Device);
  MmioWrite32(ARM_VE_SYS_CFGCTRL_REG, SysCfgCtrl);

  // Wait until the COMPLETE bit is set
  while ((MmioRead32(ARM_VE_SYS_CFGSTAT_REG) & SYS_CFGSTAT_COMPLETE) == 0);

  // Check for errors
  if(MmioRead32(ARM_VE_SYS_CFGSTAT_REG) & SYS_CFGSTAT_ERROR) {
    return RETURN_DEVICE_ERROR;
  }

  // If reading then get the data value
  if(ReadWrite == SYS_CFGCTRL_READ) {
    *Data = MmioRead32(ARM_VE_SYS_CFGDATA_REG);
  }

  return RETURN_SUCCESS;
}

RETURN_STATUS
ArmPlatformSysConfigGet (
  IN  SYS_CONFIG_FUNCTION   Function,
  OUT UINT32*               Value
  )
{
  UINT32          Site;
  UINT32          Position;
  UINT32          Device;

  Position = 0;
  Device = 0;

  // Intercept some functions
  switch(Function) {

  case SYS_CFG_OSC_SITE1:
    Function = SYS_CFG_OSC;
    Site = ARM_VE_DAUGHTERBOARD_1_SITE;
    break;

  case SYS_CFG_OSC_SITE2:
    Function = SYS_CFG_OSC;
    Site = ARM_VE_DAUGHTERBOARD_2_SITE;
    break;

  case SYS_CFG_MUXFPGA:
    Site = *Value;
    break;

  case SYS_CFG_OSC:
  case SYS_CFG_VOLT:
  case SYS_CFG_AMP:
  case SYS_CFG_TEMP:
  case SYS_CFG_RESET:
  case SYS_CFG_SCC:
  case SYS_CFG_DVIMODE:
  case SYS_CFG_POWER:
    Site = ARM_VE_MOTHERBOARD_SITE;
    break;

  case SYS_CFG_SHUTDOWN:
  case SYS_CFG_REBOOT:
  case SYS_CFG_RTC:
  default:
    return RETURN_UNSUPPORTED;
  }

  return AccessSysCfgRegister (SYS_CFGCTRL_READ, Function, Site, Position, Device, Value);
}

RETURN_STATUS
ArmPlatformSysConfigGetValues (
  IN  SYS_CONFIG_FUNCTION   Function,
  IN  UINTN                 Size,
  OUT UINT32*               Values
  )
{
  return RETURN_UNSUPPORTED;
}

RETURN_STATUS
ArmPlatformSysConfigSet (
  IN  SYS_CONFIG_FUNCTION   Function,
  IN  UINT32                Value
  )
{
  UINT32          Site;
  UINT32          Position;
  UINT32          Device;

  Position = 0;
  Device = 0;

  // Intercept some functions
  switch(Function) {

  case SYS_CFG_OSC_SITE1:
    Function = SYS_CFG_OSC;
    Site = ARM_VE_DAUGHTERBOARD_1_SITE;
    break;

  case SYS_CFG_OSC_SITE2:
    Function = SYS_CFG_OSC;
    Site = ARM_VE_DAUGHTERBOARD_2_SITE;
    break;

  case SYS_CFG_MUXFPGA:
    Site = Value;
    break;

  case SYS_CFG_RESET:
  case SYS_CFG_SCC:
  case SYS_CFG_SHUTDOWN:
  case SYS_CFG_REBOOT:
  case SYS_CFG_DVIMODE:
  case SYS_CFG_POWER:
    Site = ARM_VE_MOTHERBOARD_SITE;
    break;

  case SYS_CFG_OSC:
  case SYS_CFG_VOLT:
  case SYS_CFG_AMP:
  case SYS_CFG_TEMP:
  case SYS_CFG_RTC:
  default:
    return RETURN_UNSUPPORTED;
  }

  return AccessSysCfgRegister (SYS_CFGCTRL_WRITE, Function, Site, Position, Device, &Value);
}

RETURN_STATUS
ArmPlatformSysConfigSetDevice (
  IN  SYS_CONFIG_FUNCTION   Function,
  IN  UINT32                Device,
  IN  UINT32                Value
  )
{
  UINT32          Site;
  UINT32          Position;

  Position = 0;

  // Intercept some functions
  switch(Function) {
  case SYS_CFG_SCC:
#ifdef ARM_VE_SCC_BASE
    MmioWrite32 ((ARM_VE_SCC_BASE + (Device * 4)),Value);
    return RETURN_SUCCESS;
#else
    // There is no System Configuration Controller on the Model
    return RETURN_UNSUPPORTED;
#endif

  case SYS_CFG_OSC_SITE1:
    Function = SYS_CFG_OSC;
    Site = ARM_VE_DAUGHTERBOARD_1_SITE;
    break;

  case SYS_CFG_OSC_SITE2:
    Function = SYS_CFG_OSC;
    Site = ARM_VE_DAUGHTERBOARD_2_SITE;
    break;

  case SYS_CFG_MUXFPGA:
    Site = Value;
    break;

  case SYS_CFG_RTC:
    return RETURN_UNSUPPORTED;
    //break;

  case SYS_CFG_OSC:
  case SYS_CFG_VOLT:
  case SYS_CFG_AMP:
  case SYS_CFG_TEMP:
  case SYS_CFG_RESET:
  case SYS_CFG_SHUTDOWN:
  case SYS_CFG_REBOOT:
  case SYS_CFG_DVIMODE:
  case SYS_CFG_POWER:
    Site = ARM_VE_MOTHERBOARD_SITE;
    break;
  default:
    return RETURN_UNSUPPORTED;
  }

  return AccessSysCfgRegister (SYS_CFGCTRL_WRITE, Function, Site, Position, Device, &Value);
}
