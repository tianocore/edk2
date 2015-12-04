/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

  @file
  PchPlatformLib.c

  @brief
  PCH Platform Lib implementation.

**/

#include "PchPlatformLibrary.h"

//
// Silicon Steppings
//
/**
  Return Pch stepping type

  @param[in] None

  @retval PCH_STEPPING            Pch stepping type

**/
PCH_STEPPING
EFIAPI
PchStepping (
  VOID
  )
{
  UINT8 RevId;

  RevId = MmioRead8 (
          MmPciAddress (0,
            DEFAULT_PCI_BUS_NUMBER_PCH,
            PCI_DEVICE_NUMBER_PCH_LPC,
            PCI_FUNCTION_NUMBER_PCH_LPC,
            R_PCH_LPC_RID_CC)
          );

  switch (RevId) {
    case V_PCH_LPC_RID_0:
    case V_PCH_LPC_RID_1:
      return PchA0;
      break;

    case V_PCH_LPC_RID_2:
    case V_PCH_LPC_RID_3:
      return PchA1;
      break;

    case V_PCH_LPC_RID_4:
    case V_PCH_LPC_RID_5:
      return PchB0;
      break;

    case V_PCH_LPC_RID_6:
    case V_PCH_LPC_RID_7:
      return PchB1;
      break;

    case V_PCH_LPC_RID_8:
    case V_PCH_LPC_RID_9:
      return PchB2;
      break;

    case V_PCH_LPC_RID_A:
    case V_PCH_LPC_RID_B:
      return PchB3;
      break;

    case V_PCH_LPC_RID_C:
    case V_PCH_LPC_RID_D:
      return PchC0;
      break;
    
    case V_PCH_LPC_RID_E:
    case V_PCH_LPC_RID_F:
      return PchD0;
      break;
        
    default:
      return PchSteppingMax;
      break;

  }
}

/**
  Determine if PCH is supported

  @param[in] None

  @retval TRUE                    PCH is supported
  @retval FALSE                   PCH is not supported

**/
BOOLEAN
IsPchSupported (
  VOID
  )
{
  UINT32  Identifiers;
  UINT16  PcuVendorId;
  UINT16  PcuDeviceId;

  Identifiers = MmioRead32 (
                  MmPciAddress (0,
                  DEFAULT_PCI_BUS_NUMBER_PCH,
                  PCI_DEVICE_NUMBER_PCH_LPC,
                  PCI_FUNCTION_NUMBER_PCH_LPC,
                  R_PCH_LPC_REG_ID)
                );

  PcuDeviceId = (UINT16) ((Identifiers & B_PCH_LPC_DEVICE_ID) >> 16);
  PcuVendorId = (UINT16) (Identifiers & B_PCH_LPC_VENDOR_ID);

  //
  // Verify that this is a supported chipset
  //
  if (PcuVendorId != (UINT16) V_PCH_LPC_VENDOR_ID || !IS_PCH_VLV_LPC_DEVICE_ID (PcuDeviceId)) {
    DEBUG ((EFI_D_ERROR, "VLV SC code doesn't support the PcuDeviceId: 0x%04x!\n", PcuDeviceId));
    return FALSE;
  }
  return TRUE;
}

/**
  Detect Turbot board
  
  @param   None

  @retval  0    Not Turbot board
  @retval  1    Turbot board 

**/
UINT32 
DetectTurbotBoard (
  void
  )
{
  UINTN PciD31F0RegBase = 0;
  UINT32 GpioValue = 0;
  UINT32 TmpVal = 0;
  UINT32 MmioConf0 = 0;
  UINT32 MmioPadval = 0;
  UINT32 PConf0Offset = 0x200; //GPIO_S5_4 pad_conf0 register offset
  UINT32 PValueOffset = 0x208; //GPIO_S5_4 pad_value register offset
  UINT32 SSUSOffset = 0x2000;
  UINT32 IoBase = 0;

  DEBUG ((EFI_D_ERROR, "DetermineTurbotBoard() Entry\n"));
  PciD31F0RegBase = MmPciAddress (0,
                      0,
                      PCI_DEVICE_NUMBER_PCH_LPC,
                      PCI_FUNCTION_NUMBER_PCH_LPC,
                      0
                    );
  IoBase = MmioRead32 (PciD31F0RegBase + R_PCH_LPC_IO_BASE) & B_PCH_LPC_IO_BASE_BAR;
  
  MmioConf0 = IoBase + SSUSOffset + PConf0Offset;
  MmioPadval = IoBase + SSUSOffset + PValueOffset;
  //0xFED0E200/0xFED0E208 is pad_Conf/pad_val register address of GPIO_S5_4
  DEBUG ((EFI_D_ERROR, "MmioConf0[0x%x], MmioPadval[0x%x]\n", MmioConf0, MmioPadval));
  
  MmioWrite32 (MmioConf0, 0x2003CC00);  

  TmpVal = MmioRead32 (MmioPadval);
  TmpVal &= ~0x6; //Clear bit 1:2
  TmpVal |= 0x2; // Set the pin as GPI
  MmioWrite32 (MmioPadval, TmpVal); 

  GpioValue = MmioRead32 (MmioPadval);

  DEBUG ((EFI_D_ERROR, "Gpio_S5_4 value is 0x%x\n", GpioValue));
  return (GpioValue & 0x1);
}

/**
  Detect if "Reset BIOS Setup" jumper is plugged. 
  Only for MinnowBoard Turbot.

  @param   None

  @retval  0    Jumper is present.
  @retval  1    Jumper is not present.

**/

UINT32
DetectGpioPinValue (
VOID
  )
{
  UINTN                            PciD31F0RegBase = 0;
  UINT32                           GpioValue;
  UINT32                           TmpVal = 0;
  UINT32                           SSUSOffset = 0x2000;
  UINT32                           IoBase = 0;
  UINT32                           MmioConf0 = 0;
  UINT32                           MmioPadval = 0;
  UINT32                           PConf0Offset = 0xA0; //GPIO_S5_17 pad_conf0 register offset
  UINT32                           PValueOffset = 0xA8; //GPIO_S5_17 pad_value register offset
  
  if (DetectTurbotBoard() == 0) {
    return 1;
  }
  
  PciD31F0RegBase = MmPciAddress (0,
                      0,
                      PCI_DEVICE_NUMBER_PCH_LPC,
                      PCI_FUNCTION_NUMBER_PCH_LPC,
                      0
                    );
  IoBase = MmioRead32 (PciD31F0RegBase + R_PCH_LPC_IO_BASE) & B_PCH_LPC_IO_BASE_BAR;

  //
  // 0xFED0E0A0/0xFED0E0A8 is pad_Conf/pad_val register address of GPIO_S5_17
  //
  MmioConf0 = IoBase + SSUSOffset + PConf0Offset;
  MmioPadval = IoBase + SSUSOffset + PValueOffset;
  
  MmioWrite32 (MmioConf0, 0x2003CC01);

  TmpVal = MmioRead32 (MmioPadval);
  TmpVal &= ~0x6; //Clear bit 1:2
  TmpVal |= 0x2; // Set the pin as GPI
  MmioWrite32 (MmioPadval, TmpVal);

  GpioValue = MmioRead32 (MmioPadval);
  DEBUG ((EFI_D_INFO, "Gpio_S5_17 value is 0x%x\n", GpioValue));

  return (GpioValue & 0x1);
}
