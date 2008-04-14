/** @file

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

    PciCfg.c

Abstract:

    Single Segment Pci Configuration PPI

Revision History

**/

#include "PciCfgInternal.h"


/**
  PCI read operation.

  @param  PeiServices    An indirect pointer to the PEI Services Table
                         published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Width          The width of the access. Enumerated in bytes.
  @param  Address        The physical address of the access.
  @param  Buffer         A pointer to the buffer of data.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Unsupported width
                                  enumeration.

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
  UINTN  PciLibAddress;

  PciLibAddress = PciCfgAddressConvert ((EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS *) &Address);
  switch (Width) {
    case EfiPeiPciCfgWidthUint8:
      * (UINT8 *) Buffer = PciRead8 (PciLibAddress);
      break;

    case EfiPeiPciCfgWidthUint16:
      * (UINT16 *) Buffer = PciRead16 (PciLibAddress);
      break;

    case EfiPeiPciCfgWidthUint32:
      * (UINT32 *) Buffer = PciRead32 (PciLibAddress);
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}


/**
  PCI write operation.

  @param  PeiServices    An indirect pointer to the PEI Services Table
                         published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Width          The width of the access. Enumerated in bytes.
  @param  Address        The physical address of the access.
  @param  Buffer         A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  
  
  @retval EFI_INVALID_PARAMETER   Unsupported width
                                  enumeration.
  
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
  UINTN  PciLibAddress;

  PciLibAddress = PciCfgAddressConvert ((EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS *) &Address);
  switch (Width) {
    case EfiPeiPciCfgWidthUint8:
      PciWrite8 (PciLibAddress, *(UINT8 *) Buffer);
      break;

    case EfiPeiPciCfgWidthUint16:
      PciWrite16 (PciLibAddress, *(UINT16 *) Buffer);
      break;

    case EfiPeiPciCfgWidthUint32:
      PciWrite32 (PciLibAddress, *(UINT32 *) Buffer);
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}


/**
  PCI read-modify-write operation.

  @param  PeiServices    An indirect pointer to the PEI Services Table
                         published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Width          The width of the access. Enumerated in bytes.
  @param  Address        The physical address of the access.
  @param  SetBits        Value of the bits to set.
  @param  ClearBits      Value of the bits to clear.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Unsupported width
                                  enumeration.

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
  UINTN  PciLibAddress;

  PciLibAddress = PciCfgAddressConvert ((EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS *) &Address);
  switch (Width) {
    case EfiPeiPciCfgWidthUint8:
      PciAndThenOr8 (PciLibAddress, (UINT8)~ClearBits, (UINT8)SetBits);
      break;

    case EfiPeiPciCfgWidthUint16:
      PciAndThenOr16 (PciLibAddress, (UINT16)~ClearBits, (UINT16)SetBits);
      break;

    case EfiPeiPciCfgWidthUint32:
      PciAndThenOr32 (PciLibAddress, (UINT32)~ClearBits, (UINT32)SetBits);
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}

