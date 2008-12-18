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

  if (Width == EfiPeiPciCfgWidthUint8) {
    *((UINT8 *) Buffer) = PciRead8 (PciLibAddress);
  } else if (Width == EfiPeiPciCfgWidthUint16) {
    if ((PciLibAddress & 0x01) == 0) {
      //
      // Aligned Pci address access
      //
      WriteUnaligned16 (((UINT16 *) Buffer), PciRead16 (PciLibAddress));
    } else {
      //
      // Unaligned Pci address access, break up the request into byte by byte.
      //
      *((UINT8 *) Buffer) = PciRead8 (PciLibAddress);
      *((UINT8 *) Buffer + 1) = PciRead8 (PciLibAddress + 1);
    }
  } else if (Width == EfiPeiPciCfgWidthUint32) {
    if ((PciLibAddress & 0x03) == 0) {
      //
      // Aligned Pci address access
      //
      WriteUnaligned32 (((UINT32 *) Buffer), PciRead32 (PciLibAddress));
    } else if ((PciLibAddress & 0x01) == 0) {
      //
      // Unaligned Pci address access, break up the request into word by word.
      //
      WriteUnaligned16 (((UINT16 *) Buffer), PciRead16 (PciLibAddress));
      WriteUnaligned16 (((UINT16 *) Buffer + 1), PciRead16 (PciLibAddress + 2));
    } else {
      //
      // Unaligned Pci address access, break up the request into byte by byte.
      //
      *((UINT8 *) Buffer) = PciRead8 (PciLibAddress);
      *((UINT8 *) Buffer + 1) = PciRead8 (PciLibAddress + 1);
      *((UINT8 *) Buffer + 2) = PciRead8 (PciLibAddress + 2);
      *((UINT8 *) Buffer + 3) = PciRead8 (PciLibAddress + 3);
    }
  } else {
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

  if (Width == EfiPeiPciCfgWidthUint8) {
    PciWrite8 (PciLibAddress, *((UINT8 *) Buffer));
  } else if (Width == EfiPeiPciCfgWidthUint16) {
    if ((PciLibAddress & 0x01) == 0) {
      //
      // Aligned Pci address access
      //
      PciWrite16 (PciLibAddress, ReadUnaligned16 ((UINT16 *) Buffer));
    } else {
      //
      // Unaligned Pci address access, break up the request into byte by byte.
      //
      PciWrite8 (PciLibAddress, *((UINT8 *) Buffer));
      PciWrite8 (PciLibAddress + 1, *((UINT8 *) Buffer + 1)); 
    }
  } else if (Width == EfiPeiPciCfgWidthUint32) {
    if ((PciLibAddress & 0x03) == 0) {
      //
      // Aligned Pci address access
      //
      PciWrite32 (PciLibAddress, ReadUnaligned32 ((UINT32 *) Buffer));
    } else if ((PciLibAddress & 0x01) == 0) {
      //
      // Unaligned Pci address access, break up the request into word by word.
      //
      PciWrite16 (PciLibAddress, ReadUnaligned16 ((UINT16 *) Buffer));
      PciWrite16 (PciLibAddress + 2, ReadUnaligned16 ((UINT16 *) Buffer + 1));
    } else {
      //
      // Unaligned Pci address access, break up the request into byte by byte.
      //
      PciWrite8 (PciLibAddress, *((UINT8 *) Buffer));
      PciWrite8 (PciLibAddress + 1, *((UINT8 *) Buffer + 1)); 
      PciWrite8 (PciLibAddress + 2, *((UINT8 *) Buffer + 2)); 
      PciWrite8 (PciLibAddress + 3, *((UINT8 *) Buffer + 3)); 
    }
  } else {
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
  if (Width == EfiPeiPciCfgWidthUint8) {
    PciAndThenOr8 (PciLibAddress, (UINT8)~ClearBits, (UINT8)SetBits);
  } else if (Width == EfiPeiPciCfgWidthUint16) {
    if ((PciLibAddress & 0x01) == 0) {
      //
      // Aligned Pci address access
      //
      PciAndThenOr16 (PciLibAddress, (UINT16)~ClearBits, (UINT16)SetBits);
    } else {
      //
      // Unaligned Pci address access, break up the request into byte by byte.
      //
      PciAndThenOr8 (PciLibAddress, (UINT8)~ClearBits, (UINT8)SetBits);
      PciAndThenOr8 (PciLibAddress + 1, (UINT8)~(ClearBits >> 8), (UINT8)(SetBits >> 8));
    }
  } else if (Width == EfiPeiPciCfgWidthUint32) {
    if ((PciLibAddress & 0x03) == 0) {
      //
      // Aligned Pci address access
      //
      PciAndThenOr32 (PciLibAddress, (UINT32)~ClearBits, (UINT32)SetBits);
    } else if ((PciLibAddress & 0x01) == 0) {
      //
      // Unaligned Pci address access, break up the request into word by word.
      //
      PciAndThenOr16 (PciLibAddress, (UINT16)~ClearBits, (UINT16)SetBits);
      PciAndThenOr16 (PciLibAddress + 2, (UINT16)~(ClearBits >> 16), (UINT16)(SetBits >> 16));
    } else {
      //
      // Unaligned Pci address access, break up the request into byte by byte.
      //
      PciAndThenOr8 (PciLibAddress, (UINT8)~ClearBits, (UINT8)SetBits);
      PciAndThenOr8 (PciLibAddress + 1, (UINT8)~(ClearBits >> 8), (UINT8)(SetBits >> 8));
      PciAndThenOr8 (PciLibAddress + 2, (UINT8)~(ClearBits >> 16), (UINT8)(SetBits >> 16));
      PciAndThenOr8 (PciLibAddress + 3, (UINT8)~(ClearBits >> 24), (UINT8)(SetBits >> 24));
    }
  } else {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

