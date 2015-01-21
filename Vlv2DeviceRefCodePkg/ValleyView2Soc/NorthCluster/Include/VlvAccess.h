
/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  vlvAccess.h

Abstract:

  Macros to simplify and abstract the interface to PCI configuration.

--*/

#ifndef _VLVACCESS_H_INCLUDED_
#define _VLVACCESS_H_INCLUDED_

#include "Valleyview.h"
#include "VlvCommonDefinitions.h"
#include <Library/IoLib.h>

//
// Memory Mapped IO access macros used by MSG BUS LIBRARY
//
#define MmioAddress( BaseAddr, Register ) \
  ( (UINTN)BaseAddr + \
    (UINTN)(Register) \
  )


//
// UINT32
//

#define Mmio32Ptr( BaseAddr, Register ) \
  ( (volatile UINT32 *)MmioAddress( BaseAddr, Register ) )

#define Mmio32( BaseAddr, Register ) \
  *Mmio32Ptr( BaseAddr, Register )

#define Mmio32Or( BaseAddr, Register, OrData ) \
  Mmio32( BaseAddr, Register ) = \
    (UINT32) ( \
      Mmio32( BaseAddr, Register ) | \
      (UINT32)(OrData) \
    )

#define Mmio32And( BaseAddr, Register, AndData ) \
  Mmio32( BaseAddr, Register ) = \
    (UINT32) ( \
      Mmio32( BaseAddr, Register ) & \
      (UINT32)(AndData) \
    )

#define Mmio32AndThenOr( BaseAddr, Register, AndData, OrData ) \
  Mmio32( BaseAddr, Register ) = \
    (UINT32) ( \
      ( Mmio32( BaseAddr, Register ) & \
          (UINT32)(AndData) \
      ) | \
      (UINT32)(OrData) \
    )

//
// UINT16
//

#define Mmio16Ptr( BaseAddr, Register ) \
  ( (volatile UINT16 *)MmioAddress( BaseAddr, Register ) )

#define Mmio16( BaseAddr, Register ) \
  *Mmio16Ptr( BaseAddr, Register )

#define Mmio16Or( BaseAddr, Register, OrData ) \
  Mmio16( BaseAddr, Register ) = \
    (UINT16) ( \
      Mmio16( BaseAddr, Register ) | \
      (UINT16)(OrData) \
    )

#define Mmio16And( BaseAddr, Register, AndData ) \
  Mmio16( BaseAddr, Register ) = \
    (UINT16) ( \
      Mmio16( BaseAddr, Register ) & \
      (UINT16)(AndData) \
    )

#define Mmio16AndThenOr( BaseAddr, Register, AndData, OrData ) \
  Mmio16( BaseAddr, Register ) = \
    (UINT16) ( \
      ( Mmio16( BaseAddr, Register ) & \
          (UINT16)(AndData) \
      ) | \
      (UINT16)(OrData) \
    )

//
// UINT8
//

#define Mmio8Ptr( BaseAddr, Register ) \
  ( (volatile UINT8 *)MmioAddress( BaseAddr, Register ) )

#define Mmio8( BaseAddr, Register ) \
  *Mmio8Ptr( BaseAddr, Register )

#define Mmio8Or( BaseAddr, Register, OrData ) \
  Mmio8( BaseAddr, Register ) = \
    (UINT8) ( \
      Mmio8( BaseAddr, Register ) | \
      (UINT8)(OrData) \
    )

#define Mmio8And( BaseAddr, Register, AndData ) \
  Mmio8( BaseAddr, Register ) = \
    (UINT8) ( \
      Mmio8( BaseAddr, Register ) & \
      (UINT8)(AndData) \
    )

#define Mmio8AndThenOr( BaseAddr, Register, AndData, OrData ) \
  Mmio8( BaseAddr, Register ) = \
    (UINT8) ( \
      ( Mmio8( BaseAddr, Register ) & \
          (UINT8)(AndData) \
        ) | \
      (UINT8)(OrData) \
    )

//
// MSG BUS API
//

#define MSG_BUS_ENABLED     0x000000F0
#define MSGBUS_MASKHI       0xFFFFFF00
#define MSGBUS_MASKLO       0x000000FF

#define MESSAGE_BYTE_EN          BIT4
#define MESSAGE_WORD_EN          BIT4 | BIT5
#define MESSAGE_DWORD_EN         BIT4 | BIT5 | BIT6 | BIT7

#define SIDEBAND_OPCODE          0x78
#define MEMREAD_OPCODE           0x00000000
#define MEMWRITE_OPCODE          0x01000000



/***************************/
//
// Memory mapped PCI IO
//

#define PciCfgPtr(Bus, Device, Function, Register )\
    (UINTN)(Bus << 20) + \
    (UINTN)(Device << 15) + \
    (UINTN)(Function << 12) + \
    (UINTN)(Register)

#define PciCfg32Read_CF8CFC(B,D,F,R) \
  (UINT32)(IoOut32(0xCF8,(0x80000000|(B<<16)|(D<<11)|(F<<8)|(R))),IoIn32(0xCFC))

#define PciCfg32Write_CF8CFC(B,D,F,R,Data) \
  (IoOut32(0xCF8,(0x80000000|(B<<16)|(D<<11)|(F<<8)|(R))),IoOut32(0xCFC,Data))

#define PciCfg32Or_CF8CFC(B,D,F,R,O) \
  PciCfg32Write_CF8CFC(B,D,F,R, \
    (PciCfg32Read_CF8CFC(B,D,F,R) | (O)))

#define PciCfg32And_CF8CFC(B,D,F,R,A) \
  PciCfg32Write_CF8CFC(B,D,F,R, \
    (PciCfg32Read_CF8CFC(B,D,F,R) & (A)))

#define PciCfg32AndThenOr_CF8CFC(B,D,F,R,A,O) \
  PciCfg32Write_CF8CFC(B,D,F,R, \
    (PciCfg32Read_CF8CFC(B,D,F,R) & (A)) | (O))

//
// Device 0, Function 0
//
#define McD0PciCfg64(Register)                              MmPci64           (0, MC_BUS, 0, 0, Register)
#define McD0PciCfg64Or(Register, OrData)                    MmPci64Or         (0, MC_BUS, 0, 0, Register, OrData)
#define McD0PciCfg64And(Register, AndData)                  MmPci64And        (0, MC_BUS, 0, 0, Register, AndData)
#define McD0PciCfg64AndThenOr(Register, AndData, OrData)    MmPci64AndThenOr  (0, MC_BUS, 0, 0, Register, AndData, OrData)

#define McD0PciCfg32(Register)                              MmPci32           (0, MC_BUS, 0, 0, Register)
#define McD0PciCfg32Or(Register, OrData)                    MmPci32Or         (0, MC_BUS, 0, 0, Register, OrData)
#define McD0PciCfg32And(Register, AndData)                  MmPci32And        (0, MC_BUS, 0, 0, Register, AndData)
#define McD0PciCfg32AndThenOr(Register, AndData, OrData)    MmPci32AndThenOr  (0, MC_BUS, 0, 0, Register, AndData, OrData)

#define McD0PciCfg16(Register)                              MmPci16           (0, MC_BUS, 0, 0, Register)
#define McD0PciCfg16Or(Register, OrData)                    MmPci16Or         (0, MC_BUS, 0, 0, Register, OrData)
#define McD0PciCfg16And(Register, AndData)                  MmPci16And        (0, MC_BUS, 0, 0, Register, AndData)
#define McD0PciCfg16AndThenOr(Register, AndData, OrData)    MmPci16AndThenOr  (0, MC_BUS, 0, 0, Register, AndData, OrData)

#define McD0PciCfg8(Register)                               MmPci8            (0, MC_BUS, 0, 0, Register)
#define McD0PciCfg8Or(Register, OrData)                     MmPci8Or          (0, MC_BUS, 0, 0, Register, OrData)
#define McD0PciCfg8And(Register, AndData)                   MmPci8And         (0, MC_BUS, 0, 0, Register, AndData)
#define McD0PciCfg8AndThenOr( Register, AndData, OrData )   MmPci8AndThenOr   (0, MC_BUS, 0, 0, Register, AndData, OrData)


//
// Device 2, Function 0
//
#define McD2PciCfg64(Register)                              MmPci64           (0, MC_BUS, 2, 0, Register)
#define McD2PciCfg64Or(Register, OrData)                    MmPci64Or         (0, MC_BUS, 2, 0, Register, OrData)
#define McD2PciCfg64And(Register, AndData)                  MmPci64And        (0, MC_BUS, 2, 0, Register, AndData)
#define McD2PciCfg64AndThenOr(Register, AndData, OrData)    MmPci64AndThenOr  (0, MC_BUS, 2, 0, Register, AndData, OrData)

#define McD2PciCfg32(Register)                              MmPci32           (0, MC_BUS, 2, 0, Register)
#define McD2PciCfg32Or(Register, OrData)                    MmPci32Or         (0, MC_BUS, 2, 0, Register, OrData)
#define McD2PciCfg32And(Register, AndData)                  MmPci32And        (0, MC_BUS, 2, 0, Register, AndData)
#define McD2PciCfg32AndThenOr(Register, AndData, OrData)    MmPci32AndThenOr  (0, MC_BUS, 2, 0, Register, AndData, OrData)

#define McD2PciCfg16(Register)                              MmPci16           (0, MC_BUS, 2, 0, Register)
#define McD2PciCfg16Or(Register, OrData)                    MmPci16Or         (0, MC_BUS, 2, 0, Register, OrData)
#define McD2PciCfg16And(Register, AndData)                  MmPci16And        (0, MC_BUS, 2, 0, Register, AndData)
#define McD2PciCfg16AndThenOr(Register, AndData, OrData)    MmPci16AndThenOr  (0, MC_BUS, 2, 0, Register, AndData, OrData)

#define McD2PciCfg8(Register)                               MmPci8            (0, MC_BUS, 2, 0, Register)
#define McD2PciCfg8Or(Register, OrData)                     MmPci8Or          (0, MC_BUS, 2, 0, Register, OrData)
#define McD2PciCfg8And(Register, AndData)                   MmPci8And         (0, MC_BUS, 2, 0, Register, AndData)

//
// IO
//

#ifndef IoIn8

#define IoIn8(Port) \
  IoRead8(Port)

#define IoIn16(Port) \
  IoRead16(Port)

#define IoIn32(Port) \
  IoRead32(Port)

#define IoOut8(Port, Data) \
  IoWrite8(Port, Data)

#define IoOut16(Port, Data) \
  IoWrite16(Port, Data)

#define IoOut32(Port, Data) \
  IoWrite32(Port, Data)

#endif

#endif
