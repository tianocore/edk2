/** @file
  This file declares PciCfg PPI used to access PCI configuration space in PEI

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This PPI is defined in PI
  Version 1.00.

**/

#ifndef __PEI_PCI_CFG2_H__
#define __PEI_PCI_CFG2_H__


#define EFI_PEI_PCI_CFG2_PPI_GUID \
  { 0x57a449a, 0x1fdc, 0x4c06, { 0xbf, 0xc9, 0xf5, 0x3f, 0x6a, 0x99, 0xbb, 0x92 } }


typedef struct _EFI_PEI_PCI_CFG2_PPI   EFI_PEI_PCI_CFG2_PPI;

#define EFI_PEI_PCI_CFG_ADDRESS(bus,dev,func,reg) \
  (((bus) << 24) | \
  ((dev) << 16) | \
  ((func) << 8) | \
  ((reg) < 256 ? (reg) : ((UINT64) (reg) << 32)));

//
// EFI_PEI_PCI_CFG_PPI_WIDTH
//
typedef enum {
  EfiPeiPciCfgWidthUint8  = 0,
  EfiPeiPciCfgWidthUint16 = 1,
  EfiPeiPciCfgWidthUint32 = 2,
  EfiPeiPciCfgWidthUint64 = 3,
  EfiPeiPciCfgWidthMaximum
} EFI_PEI_PCI_CFG_PPI_WIDTH;

//
// EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS
//
typedef struct {
  UINT8   Register;
  UINT8   Function;
  UINT8   Device;
  UINT8   Bus;
  UINT32  ExtendedRegister;
} EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS;

/**
  Reads from or write to a given location in the PCI configuration space.

  @param  PeiServices     An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @param  This            Pointer to local data for the interface.

  @param  Width           The width of the access. Enumerated in bytes.
                          See EFI_PEI_PCI_CFG_PPI_WIDTH above.

  @param  Address         The physical address of the access. The format of
                          the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.

  @param  Buffer          A pointer to the buffer of data..


  @retval EFI_SUCCESS           The function completed successfully.

  @retval EFI_DEVICE_ERROR      There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting the operation at this
                                time.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_PCI_CFG2_PPI_IO) (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN OUT    VOID                      *Buffer
);


/**
  PCI read-modify-write operation.

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

  @retval EFI_DEVICE_ERROR      There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting
                                the operation at this time.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_PCI_CFG2_PPI_RW) (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN CONST  VOID                      *SetBits,
  IN CONST  VOID                      *ClearBits
);

/**
  @par Ppi Description:
  The EFI_PEI_PCI_CFG_PPI interfaces are used to abstract accesses to PCI
  controllers behind a PCI root bridge controller.

  @param Read     PCI read services.  See the Read() function description.

  @param Write    PCI write services.  See the Write() function description.

  @param Modify   PCI read-modify-write services.  See the Modify() function description.

  @param Segment  The PCI bus segment which the specified functions will access.

**/
struct _EFI_PEI_PCI_CFG2_PPI {
  EFI_PEI_PCI_CFG2_PPI_IO  Read;
  EFI_PEI_PCI_CFG2_PPI_IO  Write;
  EFI_PEI_PCI_CFG2_PPI_RW  Modify;
  UINT16                  Segment;
};


extern EFI_GUID gEfiPciCfg2PpiGuid;

#endif
