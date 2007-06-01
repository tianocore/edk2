/** @file
  This file declares Device Recovery Module PPI.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  DeviceRecoveryModule.h

  @par Revision Reference:
  This PPI is defined in Framework of EFI Recovery spec.
  Version 0.9

**/

#ifndef _PEI_DEVICE_RECOVERY_MODULE_PPI_H
#define _PEI_DEVICE_RECOVERY_MODULE_PPI_H

#define EFI_PEI_DEVICE_RECOVERY_MODULE_PPI_GUID \
  { \
    0x0DE2CE25, 0x446A, 0x45a7, {0xBF, 0xC9, 0x37, 0xDA, 0x26, 0x34, 0x4B, 0x37 } \
  }

typedef struct _EFI_PEI_DEVICE_RECOVERY_MODULE_PPI EFI_PEI_DEVICE_RECOVERY_MODULE_PPI;

/**
  This function, by whatever mechanism, searches for DXE capsules from the 
  associated device and returns the number and maximum size in bytes of 
  the capsules discovered. Entry 1 is assumed to be the highest load priority 
  and entry N is assumed to be the lowest priority.

  @param  PeiServices            General-purpose services that are available to every PEIM
  @param  This                   Indicates the EFI_PEI_DEVICE_RECOVERY_MODULE_PPI instance.
  @param  NumberRecoveryCapsules Pointer to a caller-allocated UINTN. On output,
                                 *NumberRecoveryCapsules contains the number of recovery capsule 
                                 images available for retrieval from this PEIM instance.

  @retval EFI_SUCCESS           One or more capsules were discovered.
  @retval EFI_DEVICE_ERROR      A device error occurred.
  @retval EFI_NOT_FOUND         A recovery DXE capsule cannot be found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_DEVICE_GET_NUMBER_RECOVERY_CAPSULE) (
  IN EFI_PEI_SERVICES                               **PeiServices,
  IN EFI_PEI_DEVICE_RECOVERY_MODULE_PPI             *This,
  OUT UINTN                                         *NumberRecoveryCapsules
  );

/**
  This function gets the size and type of the requested recovery capsule.

  @param  PeiServices     General-purpose services that are available to every PEIM
  @param  This            Indicates the EFI_PEI_DEVICE_RECOVERY_MODULE_PPI instance.
  @param  CapsuleInstance Specifies for which capsule instance to retrieve the information.
  @param  Size            A pointer to a caller-allocated UINTN in which the size of
                          the requested recovery module is returned.
  @param  CapsuleType     A pointer to a caller-allocated EFI_GUID in
                          which the type of the requested recovery capsule is returned.

  @retval EFI_SUCCESS           One or more capsules were discovered.
  @retval EFI_DEVICE_ERROR      A device error occurred.
  @retval EFI_NOT_FOUND         A recovery DXE capsule cannot be found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_DEVICE_GET_RECOVERY_CAPSULE_INFO) (
  IN  EFI_PEI_SERVICES                              **PeiServices,
  IN  EFI_PEI_DEVICE_RECOVERY_MODULE_PPI            *This,
  IN  UINTN                                         CapsuleInstance,
  OUT UINTN                                         *Size,
  OUT EFI_GUID                                      *CapsuleType
  );

/**
  This function, by whatever mechanism, retrieves a DXE capsule from some device 
  and loads it into memory. Note that the published interface is device neutral.

  @param  PeiServices     General-purpose services that are available to every PEIM
  @param  This            Indicates the EFI_PEI_DEVICE_RECOVERY_MODULE_PPI instance.
  @param  CapsuleInstance Specifies which capsule instance to retrieve.
  @param  Buffer          Specifies a caller-allocated buffer in which the requested 
                          recovery capsule will be returned.

  @retval EFI_SUCCESS           One or more capsules were discovered.
  @retval EFI_DEVICE_ERROR      A device error occurred.
  @retval EFI_NOT_FOUND         A recovery DXE capsule cannot be found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_DEVICE_LOAD_RECOVERY_CAPSULE) (
  IN OUT EFI_PEI_SERVICES                         **PeiServices,
  IN EFI_PEI_DEVICE_RECOVERY_MODULE_PPI           *This,
  IN UINTN                                        CapsuleInstance,
  OUT VOID                                        *Buffer
  );

/**
  @par Ppi Description:
  Presents a standard interface to EFI_PEI_DEVICE_RECOVERY_MODULE_PPI, 
  regardless of the underlying device(s).

  @param GetNumberRecoveryCapsules
  Returns the number of DXE capsules that were found.

  @param GetRecoveryCapsuleInfo
  Returns the capsule image type and the size of a given image. 

  @param LoadRecoveryCapsule
  Loads a DXE capsule into memory

**/
struct _EFI_PEI_DEVICE_RECOVERY_MODULE_PPI {
  EFI_PEI_DEVICE_GET_NUMBER_RECOVERY_CAPSULE  GetNumberRecoveryCapsules;
  EFI_PEI_DEVICE_GET_RECOVERY_CAPSULE_INFO    GetRecoveryCapsuleInfo;
  EFI_PEI_DEVICE_LOAD_RECOVERY_CAPSULE        LoadRecoveryCapsule;
};

extern EFI_GUID gEfiPeiDeviceRecoveryModulePpiGuid;

#endif
