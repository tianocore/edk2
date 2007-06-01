/** @file
  This file declares Recovery Module PPI.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  RecoveryModule.h

  @par Revision Reference:
  This PPI is defined in Framework of EFI Recovery Spec.
  Version 0.9

**/

#ifndef __PEI_RECOVERY_MODULE_PPI_H__
#define __PEI_RECOVERY_MODULE_PPI_H__

#define EFI_PEI_RECOVERY_MODULE_PPI_GUID \
  { \
    0xFB6D9542, 0x612D, 0x4f45, {0x87, 0x2F, 0x5C, 0xFF, 0x52, 0xE9, 0x3D, 0xCF } \
  }

typedef struct _EFI_PEI_RECOVERY_MODULE_PPI EFI_PEI_RECOVERY_MODULE_PPI;

/**
  Loads a DXE capsule from some media into memory and updates the HOB table 
  with the DXE firmware volume information.

  @param  PeiServices    General-purpose services that are available to every PEIM.
  @param  This           Indicates the EFI_PEI_RECOVERY_MODULE_PPI instance.

  @retval EFI_SUCCESS           The capsule was loaded correctly.
  @retval EFI_DEVICE_ERROR      A device error occurred.
  @retval EFI_NOT_FOUND         A recovery DXE capsule cannot be found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_LOAD_RECOVERY_CAPSULE) (
  IN EFI_PEI_SERVICES                     **PeiServices,
  IN EFI_PEI_RECOVERY_MODULE_PPI          *This
  );

/**
  @par Ppi Description:
  Finds and loads the recovery files.

  @param LoadRecoveryCapsule
  Loads a DXE binary capsule into memory.

**/
struct _EFI_PEI_RECOVERY_MODULE_PPI {
  EFI_PEI_LOAD_RECOVERY_CAPSULE LoadRecoveryCapsule;
};

extern EFI_GUID gEfiPeiRecoveryModulePpiGuid;

#endif
