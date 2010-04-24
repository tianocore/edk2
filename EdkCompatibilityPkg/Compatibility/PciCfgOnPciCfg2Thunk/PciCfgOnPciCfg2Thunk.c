/** @file
Module produce PciCfgPpi on top of PciCfgPpi2.

PIWG's PI specification replaces Inte's EFI Specification 1.10.
EFI_PEI_PCI_CFG_PPI defined in Inte's EFI Specification 1.10 is replaced by
EFI_PEI_PCI_CFG2_PPI in PI 1.0.
This module produces PciCfgPpi on top of PciCfgPpi2. This module is used on platform when both of
these two conditions are true:
1) Framework module is present that consumes PCI CFG  AND
2) PI module is present that produces PCI CFG2 but not PCI CFG

The Usage of this module is rare since EDK II module IntelFrameworkModulePkg\Universal\PcatSingleSegmentPciCfgPei\PcatSingleSegmentPciCfgPei.inf
that produce PCI CFG2 can also produce PCI CFG by setting Pcd Feature Flag gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdPciCfgDisable
to FALSE.


Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
Module Name:
**/

#include <PiPei.h>
#include <Ppi/PciCfg.h>
#include <Ppi/PciCfg2.h>
#include <Library/DebugLib.h>

//
// Function Prototypes
//

/**
  Reads from a given location in the PCI configuration space.

  @param  PeiServices                   An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @param  This                              Pointer to local data for the interface.

  @param  Width                           The width of the access. Enumerated in bytes.
                                                   See EFI_PEI_PCI_CFG_PPI_WIDTH above.

  @param  Address                       The physical address of the access. The format of
                                                  the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.

  @param  Buffer                           A pointer to the buffer of data..


  @retval EFI_SUCCESS                 The function completed successfully.

  @retval EFI_DEVICE_ERROR        There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting the operation at this
                                                    time.

**/
EFI_STATUS
EFIAPI
PciCfgRead (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_PCI_CFG_PPI          *This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                       Address,
  IN OUT VOID                     *Buffer
  );

/**
  Write to a given location in the PCI configuration space.

  @param  PeiServices                   An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @param  This                              Pointer to local data for the interface.

  @param  Width                            The width of the access. Enumerated in bytes.
                                                    See EFI_PEI_PCI_CFG_PPI_WIDTH above.

  @param  Address                         The physical address of the access. The format of
                                                    the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.

  @param  Buffer                            A pointer to the buffer of data..


  @retval EFI_SUCCESS                   The function completed successfully.

  @retval EFI_DEVICE_ERROR          There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting the operation at this
                                                     time.

**/
EFI_STATUS
EFIAPI
PciCfgWrite (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_PCI_CFG_PPI          *This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                       Address,
  IN OUT VOID                     *Buffer
  );

/**
  PCI read-modify-write operation.

  @param  PeiServices                     An indirect pointer to the PEI Services Table
                                                      published by the PEI Foundation.

  @param  This                                Pointer to local data for the interface.

  @param  Width                             The width of the access. Enumerated in bytes. Type
                                                      EFI_PEI_PCI_CFG_PPI_WIDTH is defined in Read().

  @param  Address                           The physical address of the access.

  @param  SetBits                            Points to value to bitwise-OR with the read configuration value.
                                                      The size of the value is determined by Width.

  @param  ClearBits                         Points to the value to negate and bitwise-AND with the read configuration value.
                                                      The size of the value is determined by Width.


  @retval EFI_SUCCESS                   The function completed successfully.

  @retval EFI_DEVICE_ERROR          There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting
                                                    the operation at this time.

**/
EFI_STATUS
EFIAPI
PciCfgModify (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_PCI_CFG_PPI          *This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                       Address,
  IN UINTN                        SetBits,
  IN UINTN                        ClearBits
  );

//
// Module globals
//
EFI_PEI_PCI_CFG_PPI mPciCfgPpi = {
  PciCfgRead,
  PciCfgWrite,
  PciCfgModify,
};

EFI_PEI_PPI_DESCRIPTOR     mPpiListPciCfg = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPciCfgPpiInServiceTableGuid,
  &mPciCfgPpi
};

/**

  Standard PEIM entry point.

  @param FileHandle   Handle of the file being invoked.
  @param PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS The interface could be successfully installed.

**/
EFI_STATUS
EFIAPI
PeimInitializePciCfg (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  //
  // Publish the PciCfgToPciCfg2 Thunk capability to other modules
  //
  return (*PeiServices)->InstallPpi (PeiServices, &mPpiListPciCfg);
}

/**
  Reads from a given location in the PCI configuration space.

  @param  PeiServices                   An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @param  This                              Pointer to local data for the interface.

  @param  Width                           The width of the access. Enumerated in bytes.
                                                   See EFI_PEI_PCI_CFG_PPI_WIDTH above.

  @param  Address                       The physical address of the access. The format of
                                                  the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.

  @param  Buffer                           A pointer to the buffer of data..


  @retval EFI_SUCCESS                 The function completed successfully.

  @retval EFI_DEVICE_ERROR        There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting the operation at this
                                                    time.

**/
EFI_STATUS
EFIAPI
PciCfgRead (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_PCI_CFG_PPI          *This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                       Address,
  IN OUT VOID                     *Buffer
  )
{
  EFI_PEI_PCI_CFG2_PPI  *PciCfg2;

  PciCfg2 = (*PeiServices)->PciCfg;

  return PciCfg2->Read ((CONST EFI_PEI_SERVICES **)PeiServices, PciCfg2, Width, Address, Buffer);
}


/**
  Write to a given location in the PCI configuration space.

  @param  PeiServices                   An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @param  This                              Pointer to local data for the interface.

  @param  Width                            The width of the access. Enumerated in bytes.
                                                    See EFI_PEI_PCI_CFG_PPI_WIDTH above.

  @param  Address                         The physical address of the access. The format of
                                                    the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.

  @param  Buffer                            A pointer to the buffer of data..


  @retval EFI_SUCCESS                   The function completed successfully.

  @retval EFI_DEVICE_ERROR          There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting the operation at this
                                                     time.

**/
EFI_STATUS
EFIAPI
PciCfgWrite (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_PCI_CFG_PPI          *This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                       Address,
  IN OUT VOID                     *Buffer
  )
{
  EFI_PEI_PCI_CFG2_PPI  *PciCfg2;

  PciCfg2 = (*PeiServices)->PciCfg;

  return PciCfg2->Write ((CONST EFI_PEI_SERVICES **)PeiServices, PciCfg2, Width, Address, Buffer);
}

/**
  PCI read-modify-write operation.

  @param  PeiServices                     An indirect pointer to the PEI Services Table
                                                      published by the PEI Foundation.

  @param  This                                Pointer to local data for the interface.

  @param  Width                             The width of the access. Enumerated in bytes. Type
                                                      EFI_PEI_PCI_CFG_PPI_WIDTH is defined in Read().

  @param  Address                           The physical address of the access.

  @param  SetBits                            Points to value to bitwise-OR with the read configuration value.
                                                      The size of the value is determined by Width.

  @param  ClearBits                         Points to the value to negate and bitwise-AND with the read configuration value.
                                                      The size of the value is determined by Width.


  @retval EFI_SUCCESS                   The function completed successfully.

  @retval EFI_DEVICE_ERROR          There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting
                                                    the operation at this time.

**/
EFI_STATUS
EFIAPI
PciCfgModify (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_PCI_CFG_PPI          *This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                       Address,
  IN UINTN                        SetBits,
  IN UINTN                        ClearBits
  )
{
  EFI_PEI_PCI_CFG2_PPI  *PciCfg2;

  PciCfg2 = (*PeiServices)->PciCfg;

  return PciCfg2->Modify ((CONST EFI_PEI_SERVICES **)PeiServices, PciCfg2, Width, Address, &SetBits, &ClearBits);
}
