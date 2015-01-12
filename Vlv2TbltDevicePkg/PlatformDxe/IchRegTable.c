/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  IchRegTable.c

Abstract:

  Register initialization table for Ich.



--*/

#include <Library/EfiRegTableLib.h>
#include "PlatformDxe.h"
extern EFI_PLATFORM_INFO_HOB      mPlatformInfo;

#define R_EFI_PCI_SVID 0x2C

EFI_REG_TABLE mSubsystemIdRegs [] = {

  //
  // Program SVID and SID for PCI devices.
  // Combine two 16 bit PCI_WRITE into one 32 bit PCI_WRITE in order to boost performance
  //
  PCI_WRITE (
      MC_BUS, MC_DEV, MC_FUN, R_EFI_PCI_SVID, EfiPciWidthUint32,
      V_PCH_DEFAULT_SVID_SID, OPCODE_FLAG_S3SAVE
    ),

  PCI_WRITE (
      IGD_BUS, IGD_DEV, IGD_FUN_0, R_EFI_PCI_SVID, EfiPciWidthUint32,
      V_PCH_DEFAULT_SVID_SID, OPCODE_FLAG_S3SAVE
    ),

  PCI_WRITE(
      DEFAULT_PCI_BUS_NUMBER_PCH, 0, 0, R_EFI_PCI_SVID, EfiPciWidthUint32,
      V_PCH_DEFAULT_SVID_SID, OPCODE_FLAG_S3SAVE
    ),
  PCI_WRITE (
      DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPC, PCI_FUNCTION_NUMBER_PCH_LPC, R_PCH_LPC_SS, EfiPciWidthUint32,
      V_PCH_DEFAULT_SVID_SID, OPCODE_FLAG_S3SAVE
    ),
  PCI_WRITE (
      DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_SATA, PCI_FUNCTION_NUMBER_PCH_SATA, R_PCH_SATA_SS, EfiPciWidthUint32,
      V_PCH_DEFAULT_SVID_SID, OPCODE_FLAG_S3SAVE
    ),
  PCI_WRITE (
      DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_SMBUS, PCI_FUNCTION_NUMBER_PCH_SMBUS, R_PCH_SMBUS_SVID, EfiPciWidthUint32,
      V_PCH_DEFAULT_SVID_SID, OPCODE_FLAG_S3SAVE
    ),
  PCI_WRITE (
      DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_USB, PCI_FUNCTION_NUMBER_PCH_EHCI, R_PCH_EHCI_SVID, EfiPciWidthUint32,
      V_PCH_DEFAULT_SVID_SID, OPCODE_FLAG_S3SAVE
    ),
  PCI_WRITE (
      DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_PCIE_ROOT_PORTS, PCI_FUNCTION_NUMBER_PCH_PCIE_ROOT_PORT_1, R_PCH_PCIE_SVID, EfiPciWidthUint32,
      V_PCH_DEFAULT_SVID_SID, OPCODE_FLAG_S3SAVE
    ),
  PCI_WRITE (
      DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_PCIE_ROOT_PORTS, PCI_FUNCTION_NUMBER_PCH_PCIE_ROOT_PORT_2, R_PCH_PCIE_SVID, EfiPciWidthUint32,
      V_PCH_DEFAULT_SVID_SID, OPCODE_FLAG_S3SAVE
    ),
  PCI_WRITE (
      DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_PCIE_ROOT_PORTS, PCI_FUNCTION_NUMBER_PCH_PCIE_ROOT_PORT_3, R_PCH_PCIE_SVID, EfiPciWidthUint32,
      V_PCH_DEFAULT_SVID_SID, OPCODE_FLAG_S3SAVE
    ),
  PCI_WRITE (
      DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_PCIE_ROOT_PORTS, PCI_FUNCTION_NUMBER_PCH_PCIE_ROOT_PORT_4, R_PCH_PCIE_SVID, EfiPciWidthUint32,
      V_PCH_DEFAULT_SVID_SID, OPCODE_FLAG_S3SAVE
    ),
  TERMINATE_TABLE
};

/**
  Updates the mSubsystemIdRegs table, and processes it.  This should program
  the Subsystem Vendor and Device IDs.

  @retval Returns  VOID

**/
VOID
InitializeSubsystemIds (
  )
{

  EFI_REG_TABLE *RegTablePtr;
  UINT32 SubsystemVidDid;
  UINT32 SubsystemAudioVidDid;

  SubsystemVidDid = mPlatformInfo.SsidSvid;
  SubsystemAudioVidDid = mPlatformInfo.SsidSvid;

  RegTablePtr = mSubsystemIdRegs;

  //
  // While we are not at the end of the table
  //
  while (RegTablePtr->Generic.OpCode != OP_TERMINATE_TABLE) {
  	//
    // If the data to write is the original SSID
    //
    if (RegTablePtr->PciWrite.Data ==
          ((V_PCH_DEFAULT_SID << 16) |
           V_PCH_INTEL_VENDOR_ID)
       ) {

    	//
      // Then overwrite it to use the alternate SSID
      //
      RegTablePtr->PciWrite.Data = SubsystemVidDid;
    }

    //
    // Go to next table entry
    //
    RegTablePtr++;
    }

  RegTablePtr = mSubsystemIdRegs;


  //
  // Program the SSVID/SSDID
  //
  ProcessRegTablePci (mSubsystemIdRegs, mPciRootBridgeIo, NULL);

}
