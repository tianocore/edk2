
/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  VlvCommonDefinitions.h

Abstract:

  Macros to simplify and abstract the interface to PCI configuration.

--*/

///
/// PCI CONFIGURATION MAP REGISTER OFFSETS
///
#ifndef PCI_VID
#define PCI_VID             0x0000  ///< Vendor ID Register
#define PCI_DID             0x0002  ///< Device ID Register
#define PCI_CMD             0x0004  ///< PCI Command Register
#define PCI_STS             0x0006  ///< PCI Status Register
#define PCI_RID             0x0008  ///< Revision ID Register
#define PCI_IFT             0x0009  ///< Interface Type
#define PCI_SCC             0x000A  ///< Sub Class Code Register
#define PCI_BCC             0x000B  ///< Base Class Code Register
#define PCI_CLS             0x000C  ///< Cache Line Size
#define PCI_PMLT            0x000D  ///< Primary Master Latency Timer
#define PCI_HDR             0x000E  ///< Header Type Register
#define PCI_BIST            0x000F  ///< Built in Self Test Register
#define PCI_BAR0            0x0010  ///< Base Address Register 0
#define PCI_BAR1            0x0014  ///< Base Address Register 1
#define PCI_BAR2            0x0018  ///< Base Address Register 2
#define PCI_PBUS            0x0018  ///< Primary Bus Number Register
#define PCI_SBUS            0x0019  ///< Secondary Bus Number Register
#define PCI_SUBUS           0x001A  ///< Subordinate Bus Number Register
#define PCI_SMLT            0x001B  ///< Secondary Master Latency Timer
#define PCI_BAR3            0x001C  ///< Base Address Register 3
#define PCI_IOBASE          0x001C  ///< I/O base Register
#define PCI_IOLIMIT         0x001D  ///< I/O Limit Register
#define PCI_SECSTATUS       0x001E  ///< Secondary Status Register
#define PCI_BAR4            0x0020  ///< Base Address Register 4
#define PCI_MEMBASE         0x0020  ///< Memory Base Register
#define PCI_MEMLIMIT        0x0022  ///< Memory Limit Register
#define PCI_BAR5            0x0024  ///< Base Address Register 5
#define PCI_PRE_MEMBASE     0x0024  ///< Prefetchable memory Base register
#define PCI_PRE_MEMLIMIT    0x0026  ///< Prefetchable memory Limit register
#define PCI_PRE_MEMBASE_U   0x0028  ///< Prefetchable memory base upper 32 bits
#define PCI_PRE_MEMLIMIT_U  0x002C  ///< Prefetchable memory limit upper 32 bits
#define PCI_SVID            0x002C  ///< Subsystem Vendor ID
#define PCI_SID             0x002E  ///< Subsystem ID
#define PCI_IOBASE_U        0x0030  ///< I/O base Upper Register
#define PCI_IOLIMIT_U       0x0032  ///< I/O Limit Upper Register
#define PCI_CAPP            0x0034  ///< Capabilities Pointer
#define PCI_EROM            0x0038  ///< Expansion ROM Base Address
#define PCI_INTLINE         0x003C  ///< Interrupt Line Register
#define PCI_INTPIN          0x003D  ///< Interrupt Pin Register
#define PCI_MAXGNT          0x003E  ///< Max Grant Register
#define PCI_BRIDGE_CNTL     0x003E  ///< Bridge Control Register
#define PCI_MAXLAT          0x003F  ///< Max Latency Register
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
#define BIT16                    0x00010000
#define BIT17                    0x00020000
#define BIT18                    0x00040000
#define BIT19                    0x00080000
#define BIT20                    0x00100000
#define BIT21                    0x00200000
#define BIT22                    0x00400000
#define BIT23                    0x00800000
#define BIT24                    0x01000000
#define BIT25                    0x02000000
#define BIT26                    0x04000000
#define BIT27                    0x08000000
#define BIT28                    0x10000000
#define BIT29                    0x20000000
#define BIT30                    0x40000000
#define BIT31                    0x80000000
#endif

#ifndef _PCIACCESS_H_INCLUDED_
#define _PCIACCESS_H_INCLUDED_
#ifndef PCI_EXPRESS_BASE_ADDRESS
 #define PCI_EXPRESS_BASE_ADDRESS 0xE0000000
#endif

#ifndef MmPciAddress
#define MmPciAddress( Segment, Bus, Device, Function, Register ) \
  ( (UINTN)PCI_EXPRESS_BASE_ADDRESS + \
    (UINTN)(Bus << 20) + \
    (UINTN)(Device << 15) + \
    (UINTN)(Function << 12) + \
    (UINTN)(Register) \
  )
#endif

//
// UINT64
//
#define MmPci64Ptr( Segment, Bus, Device, Function, Register ) \
  ( (volatile UINT64 *)MmPciAddress( Segment, Bus, Device, Function, Register ) )

#define MmPci64( Segment, Bus, Device, Function, Register ) \
  *MmPci64Ptr( Segment, Bus, Device, Function, Register )

#define MmPci64Or( Segment, Bus, Device, Function, Register, OrData ) \
  MmPci64( Segment, Bus, Device, Function, Register ) = \
    (UINT64) ( \
      MmPci64( Segment, Bus, Device, Function, Register ) | \
      (UINT64)(OrData) \
    )

#define MmPci64And( Segment, Bus, Device, Function, Register, AndData ) \
  MmPci64( Segment, Bus, Device, Function, Register ) = \
    (UINT64) ( \
      MmPci64( Segment, Bus, Device, Function, Register ) & \
      (UINT64)(AndData) \
    )

#define MmPci64AndThenOr( Segment, Bus, Device, Function, Register, AndData, OrData ) \
  MmPci64( Segment, Bus, Device, Function, Register ) = \
    (UINT64) ( \
      ( MmPci64( Segment, Bus, Device, Function, Register ) & \
          (UINT64)(AndData) \
      ) | \
      (UINT64)(OrData) \
    )

//
// UINT32
//

#define MmPci32Ptr( Segment, Bus, Device, Function, Register ) \
  ( (volatile UINT32 *) MmPciAddress( Segment, Bus, Device, Function, Register ) )

#define MmPci32( Segment, Bus, Device, Function, Register ) \
  *MmPci32Ptr( Segment, Bus, Device, Function, Register )

#define MmPci32Or( Segment, Bus, Device, Function, Register, OrData ) \
  MmPci32( Segment, Bus, Device, Function, Register ) = \
    (UINT32) ( \
      MmPci32( Segment, Bus, Device, Function, Register ) | \
      (UINT32)(OrData) \
    )

#define MmPci32And( Segment, Bus, Device, Function, Register, AndData ) \
  MmPci32( Segment, Bus, Device, Function, Register ) = \
    (UINT32) ( \
      MmPci32( Segment, Bus, Device, Function, Register ) & \
      (UINT32)(AndData) \
    )

#define MmPci32AndThenOr( Segment, Bus, Device, Function, Register, AndData, OrData ) \
  MmPci32( Segment, Bus, Device, Function, Register ) = \
    (UINT32) ( \
      ( MmPci32( Segment, Bus, Device, Function, Register ) & \
          (UINT32)(AndData) \
      ) | \
      (UINT32)(OrData) \
    )

//
// UINT16
//

#define MmPci16Ptr( Segment, Bus, Device, Function, Register ) \
  ( (volatile UINT16 *)MmPciAddress( Segment, Bus, Device, Function, Register ) )

#define MmPci16( Segment, Bus, Device, Function, Register ) \
  *MmPci16Ptr( Segment, Bus, Device, Function, Register )

#define MmPci16Or( Segment, Bus, Device, Function, Register, OrData ) \
  MmPci16( Segment, Bus, Device, Function, Register ) = \
    (UINT16) ( \
      MmPci16( Segment, Bus, Device, Function, Register ) | \
      (UINT16)(OrData) \
    )

#define MmPci16And( Segment, Bus, Device, Function, Register, AndData ) \
  MmPci16( Segment, Bus, Device, Function, Register ) = \
    (UINT16) ( \
      MmPci16( Segment, Bus, Device, Function, Register ) & \
      (UINT16)(AndData) \
    )

#define MmPci16AndThenOr( Segment, Bus, Device, Function, Register, AndData, OrData ) \
  MmPci16( Segment, Bus, Device, Function, Register ) = \
    (UINT16) ( \
      ( MmPci16( Segment, Bus, Device, Function, Register ) & \
          (UINT16)(AndData) \
      ) | \
      (UINT16)(OrData) \
    )

//
// UINT8
//

#define MmPci8Ptr( Segment, Bus, Device, Function, Register ) \
  ( (volatile UINT8 *)MmPciAddress( Segment, Bus, Device, Function, Register ) )

#define MmPci8( Segment, Bus, Device, Function, Register ) \
  *MmPci8Ptr( Segment, Bus, Device, Function, Register )

#define MmPci8Or( Segment, Bus, Device, Function, Register, OrData ) \
  MmPci8( Segment, Bus, Device, Function, Register ) = \
    (UINT8) ( \
      MmPci8( Segment, Bus, Device, Function, Register ) | \
      (UINT8)(OrData) \
    )

#define MmPci8And( Segment, Bus, Device, Function, Register, AndData ) \
  MmPci8( Segment, Bus, Device, Function, Register ) = \
    (UINT8) ( \
      MmPci8( Segment, Bus, Device, Function, Register ) & \
      (UINT8)(AndData) \
    )

#define MmPci8AndThenOr( Segment, Bus, Device, Function, Register, AndData, OrData ) \
  MmPci8( Segment, Bus, Device, Function, Register ) = \
    (UINT8) ( \
      ( MmPci8( Segment, Bus, Device, Function, Register ) & \
          (UINT8)(AndData) \
        ) | \
      (UINT8)(OrData) \
    )

#endif
