/** @file
This PPI is the same as the PPI in the framework PciCfg, with one exception: this PPI does not include a modify API, while the PPI in the framework PciCfg does. 

Copyright (c) 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _PEI_PCI_CFG_H_
#define _PEI_PCI_CFG_H_

#include <Ppi/PciCfg.h>

#define ECP_PEI_PCI_CFG_PPI_GUID \
  {0xb0ee53d4, 0xa049, 0x4a79, { 0xb2, 0xff, 0x19, 0xd9, 0xfa, 0xef, 0xaa, 0x94}}

typedef struct _ECP_PEI_PCI_CFG_PPI ECP_PEI_PCI_CFG_PPI;

/**
  PCI read and write operation.
  
  Writes to or reads from a given location in the PCI configuration space.

  @param  PeiServices                   An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This                              Pointer to local data for the interface.
  @param  Width                            The width of the access. Enumerated in bytes.
                                                    See EFI_PEI_PCI_CFG_PPI_WIDTH in MDEPkg.
  @param  Address                         The physical address of the access. The format of
                                                    the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.
  @param  Buffer                            A pointer to the buffer of data.
  @retval EFI_SUCCESS                   The function completed successfully.
  @retval EFI_DEVICE_ERROR          There was a problem with the transaction.
  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting the operation at this
                                                 time.
**/
typedef
EFI_STATUS
(EFIAPI *ECP_PEI_PCI_CFG_PPI_IO) (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN EFI_PEI_PCI_CFG_PPI      * This,
  IN EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN UINT64                   Address,
  IN OUT VOID                 *Buffer
  );

/**
  The ECP_PEI_PCI_CFG_PPI interfaces are used to abstract accesses to PCI 
  controllers behind a PCI root bridge controller.
  @param Read
  PCI read services.  See the Read() function description.
  @param Write
  PCI write services.  See the Write() function description.

**/

struct _ECP_PEI_PCI_CFG_PPI {
  ECP_PEI_PCI_CFG_PPI_IO  Read;
  ECP_PEI_PCI_CFG_PPI_IO  Write;
};

extern EFI_GUID gEcpPeiPciCfgPpiGuid;

#endif
