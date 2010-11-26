/*++

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

 EfiPciCfg.h

Abstract:

  Abstract the common fields of PciCfg definition between Framework 0.9x
  and PI 1.0.
 
--*/

#ifndef _EFI_PCI_CFG_H_
#define _EFI_PCI_CFG_H_

//
// Framework specification 0.9x definition.
//
typedef enum {
  PeiPciCfgWidthUint8   = 0,
  PeiPciCfgWidthUint16  = 1,
  PeiPciCfgWidthUint32  = 2,
  PeiPciCfgWidthUint64  = 3,
  PeiPciCfgWidthMaximum
} PEI_PCI_CFG_PPI_WIDTH;

#define PEI_PCI_CFG_ADDRESS(bus, dev, func, reg)  ( \
      (UINT64) ((((UINTN) bus) << 24) + (((UINTN) dev) << 16) + (((UINTN) func) << 8) + ((UINTN) reg)) \
    ) & 0x00000000ffffffff

//
// PI 1.0 definition.
//
typedef enum {
  EfiPeiPciCfgWidthUint8   = 0,
  EfiPeiPciCfgWidthUint16  = 1,
  EfiPeiPciCfgWidthUint32  = 2,
  EfiPeiPciCfgWidthUint64  = 3,
  EfiPeiPciCfgWidthMaximum
} EFI_PEI_PCI_CFG_PPI_WIDTH;

#define EFI_PEI_PCI_CFG_ADDRESS(bus, dev, func, reg)   \
      (UINT64) ((((UINTN) (bus)) << 24) | \
                (((UINTN) (dev)) << 16) | \
                (((UINTN) (func)) << 8) | \
                ((reg) < 256 ? ((UINTN) (reg)): ((UINT64) (reg) << 32)))

#if (PI_SPECIFICATION_VERSION < 0x00010000)

typedef struct {
  UINT8 Register;
  UINT8 Function;
  UINT8 Device;
  UINT8 Bus;
  UINT8 Reserved[4];
} PEI_PCI_CFG_PPI_PCI_ADDRESS;

typedef PEI_PCI_CFG_PPI_PCI_ADDRESS        EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS;

#else

typedef struct {
  UINT8 Register;
  UINT8 Function;
  UINT8 Device;
  UINT8 Bus;
  UINT32 ExtendedRegister;
} EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS;
#endif

#endif
