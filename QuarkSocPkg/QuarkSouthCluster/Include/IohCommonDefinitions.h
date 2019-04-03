/** @file
This header file provides common definitions just for MCH using to avoid including extra module's file.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _IOH_COMMON_DEFINITIONS_H_
#define _IOH_COMMON_DEFINITIONS_H_

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
#define IohMmioAddress( BaseAddr, Register ) \
    ( (UINTN)BaseAddr + \
      (UINTN)(Register) \
    )

//
// UINT64
//
#define IohMmio64Ptr( BaseAddr, Register ) \
    ( (volatile UINT64 *)IohMmioAddress( BaseAddr, Register ) )

#define IohMmio64( BaseAddr, Register ) \
    *IohMmio64Ptr( BaseAddr, Register )

#define IohMmio64Or( BaseAddr, Register, OrData ) \
    IohMmio64( BaseAddr, Register ) = \
      (UINT64) ( \
        IohMmio64( BaseAddr, Register ) | \
        (UINT64)(OrData) \
      )

#define IohMmio64And( BaseAddr, Register, AndData ) \
    IohMmio64( BaseAddr, Register ) = \
      (UINT64) ( \
        IohMmio64( BaseAddr, Register ) & \
        (UINT64)(AndData) \
      )

#define IohMmio64AndThenOr( BaseAddr, Register, AndData, OrData ) \
    IohMmio64( BaseAddr, Register ) = \
      (UINT64) ( \
        ( IohMmio64( BaseAddr, Register ) & \
            (UINT64)(AndData) \
        ) | \
        (UINT64)(OrData) \
      )

//
// UINT32
//
#define IohMmio32Ptr( BaseAddr, Register ) \
    ( (volatile UINT32 *)IohMmioAddress( BaseAddr, Register ) )

#define IohMmio32( BaseAddr, Register ) \
    *IohMmio32Ptr( BaseAddr, Register )

#define IohMmio32Or( BaseAddr, Register, OrData ) \
    IohMmio32( BaseAddr, Register ) = \
      (UINT32) ( \
        IohMmio32( BaseAddr, Register ) | \
        (UINT32)(OrData) \
      )

#define IohMmio32And( BaseAddr, Register, AndData ) \
    IohMmio32( BaseAddr, Register ) = \
      (UINT32) ( \
        IohMmio32( BaseAddr, Register ) & \
        (UINT32)(AndData) \
      )

#define IohMmio32AndThenOr( BaseAddr, Register, AndData, OrData ) \
    IohMmio32( BaseAddr, Register ) = \
      (UINT32) ( \
        ( IohMmio32( BaseAddr, Register ) & \
            (UINT32)(AndData) \
        ) | \
        (UINT32)(OrData) \
      )
//
// UINT16
//

#define IohMmio16Ptr( BaseAddr, Register ) \
    ( (volatile UINT16 *)IohMmioAddress( BaseAddr, Register ) )

#define IohMmio16( BaseAddr, Register ) \
    *IohMmio16Ptr( BaseAddr, Register )

#define IohMmio16Or( BaseAddr, Register, OrData ) \
    IohMmio16( BaseAddr, Register ) = \
      (UINT16) ( \
        IohMmio16( BaseAddr, Register ) | \
        (UINT16)(OrData) \
      )

#define IohMmio16And( BaseAddr, Register, AndData ) \
    IohMmio16( BaseAddr, Register ) = \
      (UINT16) ( \
        IohMmio16( BaseAddr, Register ) & \
        (UINT16)(AndData) \
      )

#define IohMmio16AndThenOr( BaseAddr, Register, AndData, OrData ) \
    IohMmio16( BaseAddr, Register ) = \
      (UINT16) ( \
        ( IohMmio16( BaseAddr, Register ) & \
            (UINT16)(AndData) \
        ) | \
        (UINT16)(OrData) \
      )
//
// UINT8
//
#define IohMmio8Ptr( BaseAddr, Register ) \
    ( (volatile UINT8 *)IohMmioAddress( BaseAddr, Register ) )

#define IohMmio8( BaseAddr, Register ) \
    *IohMmio8Ptr( BaseAddr, Register )

#define IohMmio8Or( BaseAddr, Register, OrData ) \
    IohMmio8( BaseAddr, Register ) = \
      (UINT8) ( \
        IohMmio8( BaseAddr, Register ) | \
        (UINT8)(OrData) \
      )

#define IohMmio8And( BaseAddr, Register, AndData ) \
    IohMmio8( BaseAddr, Register ) = \
      (UINT8) ( \
        IohMmio8( BaseAddr, Register ) & \
        (UINT8)(AndData) \
      )

#define IohMmio8AndThenOr( BaseAddr, Register, AndData, OrData ) \
    IohMmio8( BaseAddr, Register ) = \
      (UINT8) ( \
        ( IohMmio8( BaseAddr, Register ) & \
            (UINT8)(AndData) \
          ) | \
        (UINT8)(OrData) \
      )

//
//  Common Memory mapped Pci access macros ------------------------------------------
//
#define Ioh_PCI_EXPRESS_BASE_ADDRESS  0xE0000000


#define IohMmPciAddress( Segment, Bus, Device, Function, Register ) \
  ( (UINTN)Ioh_PCI_EXPRESS_BASE_ADDRESS + \
    (UINTN)(Bus << 20) + \
    (UINTN)(Device << 15) + \
    (UINTN)(Function << 12) + \
    (UINTN)(Register) \
  )

//
// UINT32
//
#define IohMmPci32Ptr( Segment, Bus, Device, Function, Register ) \
  ( (volatile UINT32 *)IohMmPciAddress( Segment, Bus, Device, Function, Register ) )

#define IohMmPci32( Segment, Bus, Device, Function, Register ) \
  *IohMmPci32Ptr( Segment, Bus, Device, Function, Register )

#define IohMmPci32Or( Segment, Bus, Device, Function, Register, OrData ) \
  IohMmPci32( Segment, Bus, Device, Function, Register ) = \
    (UINT32) ( \
      IohMmPci32( Segment, Bus, Device, Function, Register ) | \
      (UINT32)(OrData) \
    )

#define IohMmPci32And( Segment, Bus, Device, Function, Register, AndData ) \
  IohMmPci32( Segment, Bus, Device, Function, Register ) = \
    (UINT32) ( \
      IohMmPci32( Segment, Bus, Device, Function, Register ) & \
      (UINT32)(AndData) \
    )

#define IohMmPci32AndThenOr( Segment, Bus, Device, Function, Register, AndData, OrData ) \
  IohMmPci32( Segment, Bus, Device, Function, Register ) = \
    (UINT32) ( \
      ( IohMmPci32( Segment, Bus, Device, Function, Register ) & \
          (UINT32)(AndData) \
      ) | \
      (UINT32)(OrData) \
    )
//
// UINT16
//
#define IohMmPci16Ptr( Segment, Bus, Device, Function, Register ) \
  ( (volatile UINT16 *)IohMmPciAddress( Segment, Bus, Device, Function, Register ) )

#define IohMmPci16( Segment, Bus, Device, Function, Register ) \
  *IohMmPci16Ptr( Segment, Bus, Device, Function, Register )

#define IohMmPci16Or( Segment, Bus, Device, Function, Register, OrData ) \
  IohMmPci16( Segment, Bus, Device, Function, Register ) = \
    (UINT16) ( \
      IohMmPci16( Segment, Bus, Device, Function, Register ) | \
      (UINT16)(OrData) \
    )

#define IohMmPci16And( Segment, Bus, Device, Function, Register, AndData ) \
  IohMmPci16( Segment, Bus, Device, Function, Register ) = \
    (UINT16) ( \
      IohMmPci16( Segment, Bus, Device, Function, Register ) & \
      (UINT16)(AndData) \
    )

#define IohMmPci16AndThenOr( Segment, Bus, Device, Function, Register, AndData, OrData ) \
  IohMmPci16( Segment, Bus, Device, Function, Register ) = \
    (UINT16) ( \
      ( IohMmPci16( Segment, Bus, Device, Function, Register ) & \
          (UINT16)(AndData) \
      ) | \
      (UINT16)(OrData) \
    )
//
// UINT8
//
#define IohMmPci8Ptr( Segment, Bus, Device, Function, Register ) \
  ( (volatile UINT8 *)IohMmPciAddress( Segment, Bus, Device, Function, Register ) )

#define IohMmPci8( Segment, Bus, Device, Function, Register ) \
  *IohMmPci8Ptr( Segment, Bus, Device, Function, Register )

#define IohMmPci8Or( Segment, Bus, Device, Function, Register, OrData ) \
  IohMmPci8( Segment, Bus, Device, Function, Register ) = \
    (UINT8) ( \
      IohMmPci8( Segment, Bus, Device, Function, Register ) | \
      (UINT8)(OrData) \
    )

#define IohMmPci8And( Segment, Bus, Device, Function, Register, AndData ) \
  IohMmPci8( Segment, Bus, Device, Function, Register ) = \
    (UINT8) ( \
      IohMmPci8( Segment, Bus, Device, Function, Register ) & \
      (UINT8)(AndData) \
    )

#define IohMmPci8AndThenOr( Segment, Bus, Device, Function, Register, AndData, OrData ) \
  IohMmPci8( Segment, Bus, Device, Function, Register ) = \
    (UINT8) ( \
      ( IohMmPci8( Segment, Bus, Device, Function, Register ) & \
          (UINT8)(AndData) \
        ) | \
      (UINT8)(OrData) \
    )

#endif
