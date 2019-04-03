/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

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
