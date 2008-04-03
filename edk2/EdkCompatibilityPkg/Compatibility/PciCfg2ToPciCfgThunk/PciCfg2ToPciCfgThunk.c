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
#include <Ppi/EcpPciCfg.h>
#include <Library/DebugLib.h>

//
// Function Prototypes - Callbacks
//
EFI_STATUS
EFIAPI
EcpPciCfgPpiNotifyCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  );

//
// Function Prototypes
//
EFI_STATUS
EFIAPI
PciCfg2Read (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN OUT    VOID                      *Buffer
  );

EFI_STATUS
EFIAPI
PciCfg2Write (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN OUT    VOID                      *Buffer
  );

EFI_STATUS
EFIAPI
PciCfg2Modify (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN CONST  VOID                      *SetBits,
  IN CONST  VOID                      *ClearBits
  );

//
// Module globals
//
EFI_PEI_NOTIFY_DESCRIPTOR mNotifyOnEcpPciCfgList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEcpPeiPciCfgPpiGuid,
  EcpPciCfgPpiNotifyCallback 
};

EFI_PEI_PCI_CFG2_PPI mPciCfg2Ppi = {
  PciCfg2Read,
  PciCfg2Write,
  PciCfg2Modify,
  0
};

EFI_PEI_PPI_DESCRIPTOR mPpiListPciCfg2 = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPciCfg2PpiGuid,
  &mPciCfg2Ppi
};


EFI_STATUS
EFIAPI
PeimInitializePciCfg2 (
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
  EFI_STATUS  Status;

  //
  // Register a notification for ECP PCI CFG PPI
  //
  Status = (*PeiServices)->NotifyPpi (PeiServices, &mNotifyOnEcpPciCfgList);
  ASSERT_EFI_ERROR (Status);
  return Status;
}

EFI_STATUS
EFIAPI
EcpPciCfgPpiNotifyCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  )
{
  //
  // When ECP PCI CFG PPI is installed, publish the PCI CFG2 PPI in the 
  // PEI Services Table and the PPI database
  //
  (*PeiServices)->PciCfg = &mPciCfg2Ppi;
  return (*PeiServices)->InstallPpi ((CONST EFI_PEI_SERVICES **)PeiServices, &mPpiListPciCfg2);
}

EFI_STATUS
EFIAPI
PciCfg2Read (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN OUT    VOID                      *Buffer
  )
{
  EFI_STATUS           Status;
  EFI_PEI_PCI_CFG_PPI  *PciCfg;

  Status = (*PeiServices)->LocatePpi (
             PeiServices,
             &gEcpPeiPciCfgPpiGuid,
             0,
             NULL,
             (VOID **)&PciCfg
             );
  ASSERT_EFI_ERROR (Status);

  return PciCfg->Read ((EFI_PEI_SERVICES **)PeiServices, PciCfg, Width, Address, Buffer);
}

EFI_STATUS
EFIAPI
PciCfg2Write (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN OUT    VOID                      *Buffer
  )
{
  EFI_STATUS           Status;
  EFI_PEI_PCI_CFG_PPI  *PciCfg;

  Status = (*PeiServices)->LocatePpi (
             PeiServices,
             &gEcpPeiPciCfgPpiGuid,
             0,
             NULL,
             (VOID **)&PciCfg
             );
  ASSERT_EFI_ERROR (Status);

  return PciCfg->Write ((EFI_PEI_SERVICES **)PeiServices, PciCfg, Width, Address, Buffer);
}

EFI_STATUS
EFIAPI
PciCfg2Modify (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN CONST  VOID                      *SetBits,
  IN CONST  VOID                      *ClearBits
  )
{
  EFI_STATUS           Status;
  EFI_PEI_PCI_CFG_PPI  *PciCfg;

  Status = (*PeiServices)->LocatePpi (
             PeiServices,
             &gEcpPeiPciCfgPpiGuid,
             0,
             NULL,
             (VOID **)&PciCfg
             );
  ASSERT_EFI_ERROR (Status);

  return PciCfg->Modify ((EFI_PEI_SERVICES **)PeiServices, PciCfg, Width, Address, *(UINTN *)SetBits, *(UINTN *)ClearBits);
}
