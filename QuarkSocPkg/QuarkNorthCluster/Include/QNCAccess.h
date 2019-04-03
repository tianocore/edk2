/** @file
Macros to simplify and abstract the interface to PCI configuration.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _QNC_ACCESS_H_
#define _QNC_ACCESS_H_

#include "QuarkNcSocId.h"
#include "QNCCommonDefinitions.h"

#define EFI_LPC_PCI_ADDRESS( Register ) \
  EFI_PCI_ADDRESS(PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, PCI_FUNCTION_NUMBER_QNC_LPC, Register)

//
// QNC Controller PCI access macros
//
#define QNC_RCRB_BASE (QNCMmio32 (PciDeviceMmBase (0, PCI_DEVICE_NUMBER_QNC_LPC, 0), R_QNC_LPC_RCBA) & B_QNC_LPC_RCBA_MASK)

//
// Device 0x1f, Function 0
//

#define LpcPciCfg32( Register ) \
  QNCMmPci32(0,PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, Register )

#define LpcPciCfg32Or( Register, OrData ) \
  QNCMmPci32Or( 0,PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, Register, OrData )

#define LpcPciCfg32And( Register, AndData ) \
  QNCMmPci32And( 0,PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, Register, AndData )

#define LpcPciCfg32AndThenOr( Register, AndData, OrData ) \
  QNCMmPci32AndThenOr( 0,PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, Register, AndData, OrData )

#define LpcPciCfg16( Register ) \
  QNCMmPci16( 0,PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, Register )

#define LpcPciCfg16Or( Register, OrData ) \
  QNCMmPci16Or(  0,PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, Register, OrData )

#define LpcPciCfg16And( Register, AndData ) \
  QNCMmPci16And( 0,PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, Register, AndData )

#define LpcPciCfg16AndThenOr( Register, AndData, OrData ) \
  QNCMmPci16AndThenOr( 0,PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, Register, AndData, OrData )

#define LpcPciCfg8( Register ) \
  QNCMmPci8( 0,PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, Register )

#define LpcPciCfg8Or( Register, OrData ) \
  QNCMmPci8Or(  0,PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, Register, OrData )

#define LpcPciCfg8And( Register, AndData ) \
  QNCMmPci8And(  0,PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, Register, AndData )

#define LpcPciCfg8AndThenOr( Register, AndData, OrData ) \
  QNCMmPci8AndThenOr(  0,PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, 0, Register, AndData, OrData )

//
// Root Complex Register Block
//

#define MmRcrb32( Register ) \
  QNCMmio32( QNC_RCRB_BASE, Register )

#define MmRcrb32Or( Register, OrData ) \
  QNCMmio32Or( QNC_RCRB_BASE, Register, OrData )

#define MmRcrb32And( Register, AndData ) \
  QNCMmio32And( QNC_RCRB_BASE, Register, AndData )

#define MmRcrb32AndThenOr( Register, AndData, OrData ) \
  QNCMmio32AndThenOr( QNC_RCRB_BASE, Register, AndData, OrData )

#define MmRcrb16( Register ) \
  QNCMmio16( QNC_RCRB_BASE, Register )

#define MmRcrb16Or( Register, OrData ) \
  QNCMmio16Or( QNC_RCRB_BASE, Register, OrData )

#define MmRcrb16And( Register, AndData ) \
  QNCMmio16And( QNC_RCRB_BASE, Register, AndData )

#define MmRcrb16AndThenOr( Register, AndData, OrData ) \
  QNCMmio16AndThenOr( QNC_RCRB_BASE, Register, AndData, OrData )

#define MmRcrb8( Register ) \
  QNCMmio8( QNC_RCRB_BASE, Register )

#define MmRcrb8Or( Register, OrData ) \
  QNCMmio8Or( QNC_RCRB_BASE, Register, OrData )

#define MmRcrb8And( Register, AndData ) \
  QNCMmio8And( QNC_RCRB_BASE, Register, AndData )

#define MmRcrb8AndThenOr( Register, AndData, OrData ) \
  QNCMmio8AndThenOr( QNC_RCRB_BASE, Register, AndData, OrData )

//
// Memory Controller PCI access macros
//

//
// Device 0, Function 0
//

#define McD0PciCfg64(Register)                              QNCMmPci32           (0, MC_BUS, 0, 0, Register)
#define McD0PciCfg64Or(Register, OrData)                    QNCMmPci32Or         (0, MC_BUS, 0, 0, Register, OrData)
#define McD0PciCfg64And(Register, AndData)                  QNCMmPci32And        (0, MC_BUS, 0, 0, Register, AndData)
#define McD0PciCfg64AndThenOr(Register, AndData, OrData)    QNCMmPci32AndThenOr  (0, MC_BUS, 0, 0, Register, AndData, OrData)

#define McD0PciCfg32(Register)                              QNCMmPci32           (0, MC_BUS, 0, 0, Register)
#define McD0PciCfg32Or(Register, OrData)                    QNCMmPci32Or         (0, MC_BUS, 0, 0, Register, OrData)
#define McD0PciCfg32And(Register, AndData)                  QNCMmPci32And        (0, MC_BUS, 0, 0, Register, AndData)
#define McD0PciCfg32AndThenOr(Register, AndData, OrData)    QNCMmPci32AndThenOr  (0, MC_BUS, 0, 0, Register, AndData, OrData)

#define McD0PciCfg16(Register)                              QNCMmPci16           (0, MC_BUS, 0, 0, Register)
#define McD0PciCfg16Or(Register, OrData)                    QNCMmPci16Or         (0, MC_BUS, 0, 0, Register, OrData)
#define McD0PciCfg16And(Register, AndData)                  QNCMmPci16And        (0, MC_BUS, 0, 0, Register, AndData)
#define McD0PciCfg16AndThenOr(Register, AndData, OrData)    QNCMmPci16AndThenOr  (0, MC_BUS, 0, 0, Register, AndData, OrData)

#define McD0PciCfg8(Register)                               QNCMmPci8            (0, MC_BUS, 0, 0, Register)
#define McD0PciCfg8Or(Register, OrData)                     QNCMmPci8Or          (0, MC_BUS, 0, 0, Register, OrData)
#define McD0PciCfg8And(Register, AndData)                   QNCMmPci8And         (0, MC_BUS, 0, 0, Register, AndData)
#define McD0PciCfg8AndThenOr( Register, AndData, OrData )   QNCMmPci8AndThenOr   (0, MC_BUS, 0, 0, Register, AndData, OrData)


//
// Memory Controller Hub Memory Mapped IO register access ???
//
#define MCH_REGION_BASE                     (McD0PciCfg64 (MC_MCHBAR_OFFSET) & ~BIT0)
#define McMmioAddress(Register)             ((UINTN) MCH_REGION_BASE + (UINTN) (Register))

#define McMmio32Ptr(Register)               ((volatile UINT32*) McMmioAddress (Register))
#define McMmio64Ptr(Register)               ((volatile UINT64*) McMmioAddress (Register))

#define McMmio64(Register)                            *McMmio64Ptr( Register )
#define McMmio64Or(Register, OrData)                  (McMmio64 (Register) |= (UINT64)(OrData))
#define McMmio64And(Register, AndData)                (McMmio64 (Register) &= (UINT64)(AndData))
#define McMmio64AndThenOr(Register, AndData, OrData)  (McMmio64 ( Register ) = (McMmio64( Register ) & (UINT64)(AndData)) | (UINT64)(OrData))

#define McMmio32(Register)                            *McMmio32Ptr (Register)
#define McMmio32Or(Register, OrData)                  (McMmio32 (Register) |= (UINT32)(OrData))
#define McMmio32And(Register, AndData)                (McMmio32 (Register) &= (UINT32)(AndData))
#define McMmio32AndThenOr(Register, AndData, OrData)  (McMmio32 (Register) = (McMmio32 (Register) & (UINT32) (AndData)) | (UINT32) (OrData))

#define McMmio16Ptr(Register)                         ((volatile UINT16*) McMmioAddress (Register))
#define McMmio16(Register)                            *McMmio16Ptr (Register)
#define McMmio16Or(Register, OrData)                  (McMmio16 (Register) |= (UINT16) (OrData))
#define McMmio16And(Register, AndData)                (McMmio16 (Register) &= (UINT16) (AndData))
#define McMmio16AndThenOr(Register, AndData, OrData)  (McMmio16 (Register) = (McMmio16 (Register) & (UINT16) (AndData)) | (UINT16) (OrData))

#define McMmio8Ptr(Register)                          ((volatile UINT8 *)McMmioAddress (Register))
#define McMmio8(Register)                             *McMmio8Ptr (Register)
#define McMmio8Or(Register, OrData)                   (McMmio8 (Register) |= (UINT8) (OrData))
#define McMmio8And(Register, AndData)                 (McMmio8 (Register) &= (UINT8) (AndData))
#define McMmio8AndThenOr(Register, AndData, OrData)   (McMmio8 (Register) = (McMmio8 (Register) & (UINT8) (AndData)) | (UINT8) (OrData))

//
// QNC memory mapped related data structure deifinition
//
typedef enum {
  QNCMmioWidthUint8  = 0,
  QNCMmioWidthUint16 = 1,
  QNCMmioWidthUint32 = 2,
  QNCMmioWidthUint64 = 3,
  QNCMmioWidthMaximum
} QNC_MEM_IO_WIDTH;

#endif

