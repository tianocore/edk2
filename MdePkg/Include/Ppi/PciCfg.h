/** @file
  This file declares PciCfg PPI used to access PCI configuration space in PEI

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  PciCfg.h

  @par Revision Reference:
  This PPI is defined in PEI CIS
  Version 0.91.

**/

#ifndef __PEI_PCI_CFG_H__
#define __PEI_PCI_CFG_H__

#define EFI_PEI_PCI_CFG_PPI_INSTALLED_GUID \
  { \
    0xe1f2eba0, 0xf7b9, 0x4a26, {0x86, 0x20, 0x13, 0x12, 0x21, 0x64, 0x2a, 0x90 } \
  }

typedef struct _EFI_PEI_PCI_CFG_PPI   EFI_PEI_PCI_CFG_PPI;

#define PEI_PCI_CFG_ADDRESS(bus, dev, func, reg)  ( \
      (UINT64) ((((UINTN) bus) << 24) + (((UINTN) dev) << 16) + (((UINTN) func) << 8) + ((UINTN) reg)) \
    ) & 0x00000000ffffffff

typedef enum {
  EfiPeiPciCfgWidthUint8   = 0,
  EfiPeiPciCfgWidthUint16  = 1,
  EfiPeiPciCfgWidthUint32  = 2,
  EfiPeiPciCfgWidthUint64  = 3,
  EfiPeiPciCfgWidthMaximum
} EFI_PEI_PCI_CFG_PPI_WIDTH;

typedef struct {
  UINT8 Register;
  UINT8 Function;
  UINT8 Device;
  UINT8 Bus;
  UINT8 Reserved[4];
} EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS;

/**
  PCI read and write operation.

  @param  PeiServices    An indirect pointer to the PEI Services Table
                         published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Width          The width of the access. Enumerated in bytes.
  @param  Address        The physical address of the access.
  @param  Buffer         A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_YET_AVAILABLE The service has not been installed.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_PCI_CFG_PPI_IO) (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_PCI_CFG_PPI          *This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                       Address,
  IN OUT VOID                     *Buffer
  );

/**
  PCI read-modify-write operation.

  @param  PeiServices    An indirect pointer to the PEI Services Table
                         published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Width          The width of the access. Enumerated in bytes.
  @param  Address        The physical address of the access.
  @param  SetBits        Value of the bits to set.
  @param  ClearBits      Value of the bits to clear.

  @retval EFI_SUCCESS           The function completed successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_PCI_CFG_PPI_RW) (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_PCI_CFG_PPI          *This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                       Address,
  IN UINTN                        SetBits,
  IN UINTN                        ClearBits
  );

/**
  @par Ppi Description:
  The EFI_PEI_PCI_CFG_PPI interfaces are used to abstract accesses to PCI 
  controllers behind a PCI root bridge controller.

  @param Read
  PCI read services.  See the Read() function description.

  @param Write
  PCI write services.  See the Write() function description.

  @param Modify
  PCI read-modify-write services.  See the Modify() function description.

**/
struct _EFI_PEI_PCI_CFG_PPI {
  EFI_PEI_PCI_CFG_PPI_IO  Read;
  EFI_PEI_PCI_CFG_PPI_IO  Write;
  EFI_PEI_PCI_CFG_PPI_RW  Modify;
};

extern EFI_GUID gEfiPciCfgPpiInServiceTableGuid;

#endif
