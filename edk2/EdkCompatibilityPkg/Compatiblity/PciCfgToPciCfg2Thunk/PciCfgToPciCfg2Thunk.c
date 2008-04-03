/*++

Copyright (c) 2006 - 2008 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
Module Name:

  Variable.c

Abstract:

  PEIM to provide the Variable functionality

--*/

#include <PiPei.h>
#include <Ppi/PciCfg.h>
#include <Ppi/PciCfg2.h>
#include <Library/DebugLib.h>

//
// Function Prototypes
//
EFI_STATUS
EFIAPI
PciCfgRead (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_PCI_CFG_PPI          *This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                       Address,
  IN OUT VOID                     *Buffer
  );

EFI_STATUS
EFIAPI
PciCfgWrite (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_PCI_CFG_PPI          *This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                       Address,
  IN OUT VOID                     *Buffer
  );

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

EFI_STATUS
EFIAPI
PeimInitializePciCfg (
  IN EFI_FFS_FILE_HEADER     *FfsHeader,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
/*++

Routine Description:

  Provide the functionality of the variable services.

Arguments:

  FfsHeadher  - The FFS file header
  PeiServices - General purpose services available to every PEIM.

Returns:

  Status -  EFI_SUCCESS if the interface could be successfully
            installed

--*/
{
  //
  // Publish the variable capability to other modules
  //
  return (*PeiServices)->InstallPpi (PeiServices, &mPpiListPciCfg);
}

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
