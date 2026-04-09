/** @file
  The default version of EFI_PEI_PCI_CFG2_PPI support published by PeiServices in
  PeiCore initialization phase.

  EFI_PEI_PCI_CFG2_PPI is installed by the PEIM which supports a PCI root bridge.
  When PeiCore is started, the default version of EFI_PEI_PCI_CFG2_PPI will be assigned
  to PeiServices table.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiMain.h"

///
/// This default instance of EFI_PEI_PCI_CFG2_PPI install assigned to EFI_PEI_SERVICE.PciCfg
/// when PeiCore's initialization.
///
EFI_PEI_PCI_CFG2_PPI  gPeiDefaultPciCfg2Ppi = {
  PeiDefaultPciCfg2Read,
  PeiDefaultPciCfg2Write,
  PeiDefaultPciCfg2Modify
};

/**
  Reads from a given location in the PCI configuration space.

  If the EFI_PEI_PCI_CFG2_PPI is not installed by platform/chipset PEIM, then
  return EFI_NOT_YET_AVAILABLE.

  @param  PeiServices     An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This            Pointer to local data for the interface.
  @param  Width           The width of the access. Enumerated in bytes.
                          See EFI_PEI_PCI_CFG_PPI_WIDTH above.
  @param  Address         The physical address of the access. The format of
                          the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.
  @param  Buffer          A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER The invalid access width.
  @retval EFI_NOT_YET_AVAILABLE If the EFI_PEI_PCI_CFG2_PPI is not installed by platform/chipset PEIM.

**/
EFI_STATUS
EFIAPI
PeiDefaultPciCfg2Read (
  IN CONST  EFI_PEI_SERVICES           **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI       *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH  Width,
  IN        UINT64                     Address,
  IN OUT    VOID                       *Buffer
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Write to a given location in the PCI configuration space.

  If the EFI_PEI_PCI_CFG2_PPI is not installed by platform/chipset PEIM, then
  return EFI_NOT_YET_AVAILABLE.

  @param  PeiServices     An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This            Pointer to local data for the interface.
  @param  Width           The width of the access. Enumerated in bytes.
                          See EFI_PEI_PCI_CFG_PPI_WIDTH above.
  @param  Address         The physical address of the access. The format of
                          the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.
  @param  Buffer          A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER The invalid access width.
  @retval EFI_NOT_YET_AVAILABLE If the EFI_PEI_PCI_CFG2_PPI is not installed by platform/chipset PEIM.
**/
EFI_STATUS
EFIAPI
PeiDefaultPciCfg2Write (
  IN CONST  EFI_PEI_SERVICES           **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI       *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH  Width,
  IN        UINT64                     Address,
  IN OUT    VOID                       *Buffer
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  This function performs a read-modify-write operation on the contents from a given
  location in the PCI configuration space.
  If the EFI_PEI_PCI_CFG2_PPI is not installed by platform/chipset PEIM, then
  return EFI_NOT_YET_AVAILABLE.

  @param  PeiServices     An indirect pointer to the PEI Services Table
                          published by the PEI Foundation.
  @param  This            Pointer to local data for the interface.
  @param  Width           The width of the access. Enumerated in bytes. Type
                          EFI_PEI_PCI_CFG_PPI_WIDTH is defined in Read().
  @param  Address         The physical address of the access.
  @param  SetBits         Points to value to bitwise-OR with the read configuration value.
                          The size of the value is determined by Width.
  @param  ClearBits       Points to the value to negate and bitwise-AND with the read configuration value.
                          The size of the value is determined by Width.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER The invalid access width.
  @retval EFI_NOT_YET_AVAILABLE If the EFI_PEI_PCI_CFG2_PPI is not installed by platform/chipset PEIM.
**/
EFI_STATUS
EFIAPI
PeiDefaultPciCfg2Modify (
  IN CONST  EFI_PEI_SERVICES           **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI       *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH  Width,
  IN        UINT64                     Address,
  IN        VOID                       *SetBits,
  IN        VOID                       *ClearBits
  )
{
  return EFI_NOT_AVAILABLE_YET;
}
