/** @file
This header file provides common definitions just for MCH using to avoid including extra module's file.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _QNC_COMMON_DEFINITIONS_H_
#define _QNC_COMMON_DEFINITIONS_H_

//
// PCI CONFIGURATION MAP REGISTER OFFSETS
//
#ifndef PCI_VID
#define PCI_VID             0x0000        // Vendor ID Register
#define PCI_DID             0x0002        // Device ID Register
#define PCI_CMD             0x0004        // PCI Command Register
#define PCI_STS             0x0006        // PCI Status Register
#define PCI_RID             0x0008        // Revision ID Register
#define PCI_IFT             0x0009        // Interface Type
#define PCI_SCC             0x000A        // Sub Class Code Register
#define PCI_BCC             0x000B        // Base Class Code Register
#define PCI_CLS             0x000C        // Cache Line Size
#define PCI_PMLT            0x000D        // Primary Master Latency Timer
#define PCI_HDR             0x000E        // Header Type Register
#define PCI_BIST            0x000F        // Built in Self Test Register
#define PCI_BAR0            0x0010        // Base Address Register 0
#define PCI_BAR1            0x0014        // Base Address Register 1
#define PCI_BAR2            0x0018        // Base Address Register 2
#define PCI_PBUS            0x0018        // Primary Bus Number Register
#define PCI_SBUS            0x0019        // Secondary Bus Number Register
#define PCI_SUBUS           0x001A        // Subordinate Bus Number Register
#define PCI_SMLT            0x001B        // Secondary Master Latency Timer
#define PCI_BAR3            0x001C        // Base Address Register 3
#define PCI_IOBASE          0x001C        // I/O base Register
#define PCI_IOLIMIT         0x001D        // I/O Limit Register
#define PCI_SECSTATUS       0x001E        // Secondary Status Register
#define PCI_BAR4            0x0020        // Base Address Register 4
#define PCI_MEMBASE         0x0020        // Memory Base Register
#define PCI_MEMLIMIT        0x0022        // Memory Limit Register
#define PCI_BAR5            0x0024        // Base Address Register 5
#define PCI_PRE_MEMBASE     0x0024        // Prefetchable memory Base register
#define PCI_PRE_MEMLIMIT    0x0026        // Prefetchable memory Limit register
#define PCI_PRE_MEMBASE_U   0x0028        // Prefetchable memory base upper 32 bits
#define PCI_PRE_MEMLIMIT_U  0x002C        // Prefetchable memory limit upper 32 bits
#define PCI_SVID            0x002C        // Subsystem Vendor ID
#define PCI_SID             0x002E        // Subsystem ID
#define PCI_IOBASE_U        0x0030        // I/O base Upper Register
#define PCI_IOLIMIT_U       0x0032        // I/O Limit Upper Register
#define PCI_CAPP            0x0034        // Capabilities Pointer
#define PCI_EROM            0x0038        // Expansion ROM Base Address
#define PCI_INTLINE         0x003C        // Interrupt Line Register
#define PCI_INTPIN          0x003D        // Interrupt Pin Register
#define PCI_MAXGNT          0x003E        // Max Grant Register
#define PCI_BRIDGE_CNTL     0x003E        // Bridge Control Register
#define PCI_MAXLAT          0x003F        // Max Latency Register
#endif
//
// Bit Difinitions
//
#ifndef BIT0
#define BIT0                     0x0001
#define BIT1                     0x0002
#define BIT2                     0x0004
#define BIT3                     0x0008
#define BIT4                     0x0010
#define BIT5                     0x0020
#define BIT6                     0x0040
#define BIT7                     0x0080
#define BIT8                     0x0100
#define BIT9                     0x0200
#define BIT10                    0x0400
#define BIT11                    0x0800
#define BIT12                    0x1000
#define BIT13                    0x2000
#define BIT14                    0x4000
#define BIT15                    0x8000
#define BIT16                0x00010000
#define BIT17                0x00020000
#define BIT18                0x00040000
#define BIT19                0x00080000
#define BIT20                0x00100000
#define BIT21                0x00200000
#define BIT22                0x00400000
#define BIT23                0x00800000
#define BIT24                0x01000000
#define BIT25                0x02000000
#define BIT26                0x04000000
#define BIT27                0x08000000
#define BIT28                0x10000000
#define BIT29                0x20000000
#define BIT30                0x40000000
#define BIT31                0x80000000
#endif


//
//  Common Memory mapped Io access macros ------------------------------------------
//
#define QNCMmioAddress( BaseAddr, Register ) \
    ( (UINTN)BaseAddr + \
      (UINTN)(Register) \
    )

//
// UINT64
//
#define QNCMmio64Ptr( BaseAddr, Register ) \
    ( (volatile UINT64 *)QNCMmioAddress( BaseAddr, Register ) )

#define QNCMmio64( BaseAddr, Register ) \
    *QNCMmio64Ptr( BaseAddr, Register )

#define QNCMmio64Or( BaseAddr, Register, OrData ) \
    QNCMmio64( BaseAddr, Register ) = \
      (UINT64) ( \
        QNCMmio64( BaseAddr, Register ) | \
        (UINT64)(OrData) \
      )

#define QNCMmio64And( BaseAddr, Register, AndData ) \
    QNCMmio64( BaseAddr, Register ) = \
      (UINT64) ( \
        QNCMmio64( BaseAddr, Register ) & \
        (UINT64)(AndData) \
      )

#define QNCMmio64AndThenOr( BaseAddr, Register, AndData, OrData ) \
    QNCMmio64( BaseAddr, Register ) = \
      (UINT64) ( \
        ( QNCMmio64( BaseAddr, Register ) & \
            (UINT64)(AndData) \
        ) | \
        (UINT64)(OrData) \
      )

//
// UINT32
//
#define QNCMmio32Ptr( BaseAddr, Register ) \
    ( (volatile UINT32 *)QNCMmioAddress( BaseAddr, Register ) )

#define QNCMmio32( BaseAddr, Register ) \
    *QNCMmio32Ptr( BaseAddr, Register )

#define QNCMmio32Or( BaseAddr, Register, OrData ) \
    QNCMmio32( BaseAddr, Register ) = \
      (UINT32) ( \
        QNCMmio32( BaseAddr, Register ) | \
        (UINT32)(OrData) \
      )

#define QNCMmio32And( BaseAddr, Register, AndData ) \
    QNCMmio32( BaseAddr, Register ) = \
      (UINT32) ( \
        QNCMmio32( BaseAddr, Register ) & \
        (UINT32)(AndData) \
      )

#define QNCMmio32AndThenOr( BaseAddr, Register, AndData, OrData ) \
    QNCMmio32( BaseAddr, Register ) = \
      (UINT32) ( \
        ( QNCMmio32( BaseAddr, Register ) & \
            (UINT32)(AndData) \
        ) | \
        (UINT32)(OrData) \
      )
//
// UINT16
//

#define QNCMmio16Ptr( BaseAddr, Register ) \
    ( (volatile UINT16 *)QNCMmioAddress( BaseAddr, Register ) )

#define QNCMmio16( BaseAddr, Register ) \
    *QNCMmio16Ptr( BaseAddr, Register )

#define QNCMmio16Or( BaseAddr, Register, OrData ) \
    QNCMmio16( BaseAddr, Register ) = \
      (UINT16) ( \
        QNCMmio16( BaseAddr, Register ) | \
        (UINT16)(OrData) \
      )

#define QNCMmio16And( BaseAddr, Register, AndData ) \
    QNCMmio16( BaseAddr, Register ) = \
      (UINT16) ( \
        QNCMmio16( BaseAddr, Register ) & \
        (UINT16)(AndData) \
      )

#define QNCMmio16AndThenOr( BaseAddr, Register, AndData, OrData ) \
    QNCMmio16( BaseAddr, Register ) = \
      (UINT16) ( \
        ( QNCMmio16( BaseAddr, Register ) & \
            (UINT16)(AndData) \
        ) | \
        (UINT16)(OrData) \
      )
//
// UINT8
//
#define QNCMmio8Ptr( BaseAddr, Register ) \
    ( (volatile UINT8 *)QNCMmioAddress( BaseAddr, Register ) )

#define QNCMmio8( BaseAddr, Register ) \
    *QNCMmio8Ptr( BaseAddr, Register )

#define QNCMmio8Or( BaseAddr, Register, OrData ) \
    QNCMmio8( BaseAddr, Register ) = \
      (UINT8) ( \
        QNCMmio8( BaseAddr, Register ) | \
        (UINT8)(OrData) \
      )

#define QNCMmio8And( BaseAddr, Register, AndData ) \
    QNCMmio8( BaseAddr, Register ) = \
      (UINT8) ( \
        QNCMmio8( BaseAddr, Register ) & \
        (UINT8)(AndData) \
      )

#define QNCMmio8AndThenOr( BaseAddr, Register, AndData, OrData ) \
    QNCMmio8( BaseAddr, Register ) = \
      (UINT8) ( \
        ( QNCMmio8( BaseAddr, Register ) & \
            (UINT8)(AndData) \
          ) | \
        (UINT8)(OrData) \
      )

//
//  Common Memory mapped Pci access macros ------------------------------------------
//

#define QNCMmPciAddress( Segment, Bus, Device, Function, Register ) \
  ( (UINTN) QncGetPciExpressBaseAddress() + \
    (UINTN)(Bus << 20) + \
    (UINTN)(Device << 15) + \
    (UINTN)(Function << 12) + \
    (UINTN)(Register) \
  )

//
// Macro to calculate the Pci device's base memory mapped address
//
#define PciDeviceMmBase( Bus, Device, Function) \
    ( (UINTN) QncGetPciExpressBaseAddress () + \
      (UINTN)(Bus << 20) + \
      (UINTN)(Device << 15) + \
      (UINTN)(Function << 12) \
    )

//
// UINT32
//
#define QNCMmPci32Ptr( Segment, Bus, Device, Function, Register ) \
  ( (volatile UINT32 *)QNCMmPciAddress( Segment, Bus, Device, Function, Register ) )

#define QNCMmPci32( Segment, Bus, Device, Function, Register ) \
  *QNCMmPci32Ptr( Segment, Bus, Device, Function, Register )

#define QNCMmPci32Or( Segment, Bus, Device, Function, Register, OrData ) \
  QNCMmPci32( Segment, Bus, Device, Function, Register ) = \
    (UINT32) ( \
      QNCMmPci32( Segment, Bus, Device, Function, Register ) | \
      (UINT32)(OrData) \
    )

#define QNCMmPci32And( Segment, Bus, Device, Function, Register, AndData ) \
  QNCMmPci32( Segment, Bus, Device, Function, Register ) = \
    (UINT32) ( \
      QNCMmPci32( Segment, Bus, Device, Function, Register ) & \
      (UINT32)(AndData) \
    )

#define QNCMmPci32AndThenOr( Segment, Bus, Device, Function, Register, AndData, OrData ) \
  QNCMmPci32( Segment, Bus, Device, Function, Register ) = \
    (UINT32) ( \
      ( QNCMmPci32( Segment, Bus, Device, Function, Register ) & \
          (UINT32)(AndData) \
      ) | \
      (UINT32)(OrData) \
    )
//
// UINT16
//
#define QNCMmPci16Ptr( Segment, Bus, Device, Function, Register ) \
  ( (volatile UINT16 *)QNCMmPciAddress( Segment, Bus, Device, Function, Register ) )

#define QNCMmPci16( Segment, Bus, Device, Function, Register ) \
  *QNCMmPci16Ptr( Segment, Bus, Device, Function, Register )

#define QNCMmPci16Or( Segment, Bus, Device, Function, Register, OrData ) \
  QNCMmPci16( Segment, Bus, Device, Function, Register ) = \
    (UINT16) ( \
      QNCMmPci16( Segment, Bus, Device, Function, Register ) | \
      (UINT16)(OrData) \
    )

#define QNCMmPci16And( Segment, Bus, Device, Function, Register, AndData ) \
  QNCMmPci16( Segment, Bus, Device, Function, Register ) = \
    (UINT16) ( \
      QNCMmPci16( Segment, Bus, Device, Function, Register ) & \
      (UINT16)(AndData) \
    )

#define QNCMmPci16AndThenOr( Segment, Bus, Device, Function, Register, AndData, OrData ) \
  QNCMmPci16( Segment, Bus, Device, Function, Register ) = \
    (UINT16) ( \
      ( QNCMmPci16( Segment, Bus, Device, Function, Register ) & \
          (UINT16)(AndData) \
      ) | \
      (UINT16)(OrData) \
    )
//
// UINT8
//
#define QNCMmPci8Ptr( Segment, Bus, Device, Function, Register ) \
  ( (volatile UINT8 *)QNCMmPciAddress( Segment, Bus, Device, Function, Register ) )

#define QNCMmPci8( Segment, Bus, Device, Function, Register ) \
  *QNCMmPci8Ptr( Segment, Bus, Device, Function, Register )

#define QNCMmPci8Or( Segment, Bus, Device, Function, Register, OrData ) \
  QNCMmPci8( Segment, Bus, Device, Function, Register ) = \
    (UINT8) ( \
      QNCMmPci8( Segment, Bus, Device, Function, Register ) | \
      (UINT8)(OrData) \
    )

#define QNCMmPci8And( Segment, Bus, Device, Function, Register, AndData ) \
  QNCMmPci8( Segment, Bus, Device, Function, Register ) = \
    (UINT8) ( \
      QNCMmPci8( Segment, Bus, Device, Function, Register ) & \
      (UINT8)(AndData) \
    )

#define QNCMmPci8AndThenOr( Segment, Bus, Device, Function, Register, AndData, OrData ) \
  QNCMmPci8( Segment, Bus, Device, Function, Register ) = \
    (UINT8) ( \
      ( QNCMmPci8( Segment, Bus, Device, Function, Register ) & \
          (UINT8)(AndData) \
        ) | \
      (UINT8)(OrData) \
    )

#endif
