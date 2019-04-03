/** @file
  This file declares the PciCfg PPI used to access the PCI configuration space in PEI

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This PPI is defined in PEI CIS
  Version 0.91.

**/

#ifndef __PEI_PCI_CFG_H__
#define __PEI_PCI_CFG_H__

#include <Ppi/PciCfg2.h>
//
// Get the common definitions for EFI_PEI_PCI_CFG_PPI_WIDTH.
//

#define EFI_PEI_PCI_CFG_PPI_INSTALLED_GUID \
  { \
    0xe1f2eba0, 0xf7b9, 0x4a26, {0x86, 0x20, 0x13, 0x12, 0x21, 0x64, 0x2a, 0x90 } \
  }

typedef struct _EFI_PEI_PCI_CFG_PPI   EFI_PEI_PCI_CFG_PPI;

#define PEI_PCI_CFG_ADDRESS(bus, dev, func, reg)  ( \
      (UINT64) ((((UINTN) bus) << 24) + (((UINTN) dev) << 16) + (((UINTN) func) << 8) + ((UINTN) reg)) \
    ) & 0x00000000ffffffff

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
(EFIAPI *EFI_PEI_PCI_CFG_PPI_IO)(
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
  @param  This           The pointer to local data for the interface.
  @param  Width          The width of the access. Enumerated in bytes.
  @param  Address        The physical address of the access.
  @param  SetBits        Value of the bits to set.
  @param  ClearBits      Value of the bits to clear.

  @retval EFI_SUCCESS           The function completed successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_PCI_CFG_PPI_RW)(
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_PCI_CFG_PPI          *This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                       Address,
  IN UINTN                        SetBits,
  IN UINTN                        ClearBits
  );

/**
  The EFI_PEI_PCI_CFG_PPI interfaces are used to abstract accesses to PCI
  controllers behind a PCI root bridge controller.
**/
struct _EFI_PEI_PCI_CFG_PPI {
  ///
  /// PCI read services.  See the Read() function description.
  ///
  EFI_PEI_PCI_CFG_PPI_IO  Read;

  ///
  /// PCI write services.  See the Write() function description.
  ///
  EFI_PEI_PCI_CFG_PPI_IO  Write;

  ///
  /// PCI read-modify-write services.  See the Modify() function description.
  ///
  EFI_PEI_PCI_CFG_PPI_RW  Modify;
};

extern EFI_GUID gEfiPciCfgPpiInServiceTableGuid;

#endif
