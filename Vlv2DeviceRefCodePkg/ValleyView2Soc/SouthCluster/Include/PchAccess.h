/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



  @file
  PchAccess.h

  @brief
  Macros that simplify accessing PCH devices's PCI registers.

  ** NOTE ** these macros assume the PCH device is on BUS 0

**/
#ifndef _PCH_ACCESS_H_
#define _PCH_ACCESS_H_

#include "PchRegs.h"
#include "PchCommonDefinitions.h"

#ifndef STALL_ONE_MICRO_SECOND
#define STALL_ONE_MICRO_SECOND 1
#endif
#ifndef STALL_ONE_SECOND
#define STALL_ONE_SECOND 1000000
#endif

///
/// Memory Mapped PCI Access macros
///
///
/// PCI Device MM Base
///
#ifndef MmPciAddress
#define MmPciAddress(Segment, Bus, Device, Function, Register) \
  ((UINTN) PatchPcdGet64 (PcdPciExpressBaseAddress) + \
   (UINTN) (Bus << 20) + \
   (UINTN) (Device << 15) + \
   (UINTN) (Function << 12) + \
   (UINTN) (Register) \
  )
#endif
///
/// Pch Controller PCI access macros
///
#define PCH_RCRB_BASE ( \
  MmioRead32 (MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_LPC, \
  PCI_FUNCTION_NUMBER_PCH_LPC), \
  R_PCH_LPC_RCBA)) & B_PCH_LPC_RCBA_BAR \
  )

///
/// Device 0x1b, Function 0
///
#define PchAzaliaPciCfg32(Register) \
  MmioRead32 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_AZALIA, \
  0, \
  Register) \
  )

#define PchAzaliaPciCfg32Or(Register, OrData) \
  MmioOr32 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_AZALIA, \
  0, \
  Register), \
  OrData \
  )

#define PchAzaliaPciCfg32And(Register, AndData) \
  MmioAnd32 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_AZALIA, \
  0, \
  Register), \
  AndData \
  )

#define PchAzaliaPciCfg32AndThenOr(Register, AndData, OrData) \
  MmioAndThenOr32 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_AZALIA, \
  0, \
  Register), \
  OrData \
  )

#define PchAzaliaPciCfg16(Register) \
  MmioRead16 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_AZALIA, \
  0, \
  Register) \
  )

#define PchAzaliaPciCfg16Or(Register, OrData) \
  MmioOr16 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_AZALIA, \
  0, \
  Register), \
  OrData \
  )

#define PchAzaliaPciCfg16And(Register, AndData) \
  MmioAnd16 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_AZALIA, \
  0, \
  Register), \
  AndData \
  )

#define PchAzaliaPciCfg16AndThenOr(Register, AndData, OrData) \
  MmioAndThenOr16 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_AZALIA, \
  0, \
  Register), \
  AndData, \
  OrData \
  )

#define PchAzaliaPciCfg8(Register)  MmioRead8 (MmPciAddress (0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_AZALIA, 0, Register))

#define PchAzaliaPciCfg8Or(Register, OrData) \
  MmioOr8 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_AZALIA, \
  0, \
  Register), \
  OrData \
  )

#define PchAzaliaPciCfg8And(Register, AndData) \
  MmioAnd8 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_AZALIA, \
  0, \
  Register), \
  AndData \
  )

#define PchAzaliaPciCfg8AndThenOr(Register, AndData, OrData) \
  MmioAndThenOr8 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_AZALIA, \
  0, \
  Register), \
  AndData, \
  OrData \
  )

///
/// Device 0x1f, Function 0
///
#define PchLpcPciCfg32(Register)  MmioRead32 (MmPciAddress (0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPC, 0, Register))

#define PchLpcMmioOr32 (Register, OrData) \
  MmioOr32 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_LPC, \
  0, \
  Register), \
  OrData \
  )

#define PchLpcPciCfg32And(Register, AndData) \
  MmioAnd32 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_LPC, \
  0, \
  Register), \
  AndData \
  )

#define PchLpcPciCfg32AndThenOr(Register, AndData, OrData) \
  MmioAndThenOr32 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_LPC, \
  0, \
  Register), \
  AndData, \
  OrData \
  )

#define PchLpcPciCfg16(Register)  MmioRead16 (MmPciAddress (0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPC, 0, Register))

#define PchLpcPciCfg16Or(Register, OrData) \
  MmioOr16 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_LPC, \
  0, \
  Register), \
  OrData \
  )

#define PchLpcPciCfg16And(Register, AndData) \
  MmioAndThenOr16 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_LPC, \
  0, \
  Register), \
  AndData \
  )

#define PchLpcPciCfg16AndThenOr(Register, AndData, OrData) \
  MmioAndThenOr16 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_LPC, \
  0, \
  Register), \
  AndData, \
  OrData \
  )

#define PchLpcPciCfg8(Register) MmioRead8 (MmPciAddress (0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPC, 0, Register))

#define PchLpcPciCfg8Or(Register, OrData) \
  MmioOr8 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_LPC, \
  0, \
  Register), \
  OrData \
  )

#define PchLpcPciCfg8And(Register, AndData) \
  MmioAnd8 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_LPC, \
  0, \
  Register), \
  AndData \
  )

#define PchLpcPciCfg8AndThenOr(Register, AndData, OrData) \
  MmioAndThenOr8 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_LPC, \
  0, \
  Register), \
  AndData, \
  OrData \
  )


///
/// SATA device 0x13, Function 0
///
#define PchSataPciCfg32(Register) MmioRead32 (MmPciAddress (0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_SATA, PCI_FUNCTION_NUMBER_PCH_SATA, Register))

#define PchSataPciCfg32Or(Register, OrData) \
  MmioOr32 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_SATA, \
  PCI_FUNCTION_NUMBER_PCH_SATA, \
  Register), \
  OrData \
  )

#define PchSataPciCfg32And(Register, AndData) \
  MmioAnd32 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_SATA, \
  PCI_FUNCTION_NUMBER_PCH_SATA, \
  Register), \
  AndData \
  )

#define PchSataPciCfg32AndThenOr(Register, AndData, OrData) \
  MmioAndThenOr32 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_SATA, \
  PCI_FUNCTION_NUMBER_PCH_SATA, \
  Register), \
  AndData, \
  OrData \
  )

#define PchSataPciCfg16(Register) MmioRead16 (MmPciAddress (0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_SATA, PCI_FUNCTION_NUMBER_PCH_SATA, Register))

#define PchSataPciCfg16Or(Register, OrData) \
  MmioOr16 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_SATA, \
  PCI_FUNCTION_NUMBER_PCH_SATA, \
  Register), \
  OrData \
  )

#define PchSataPciCfg16And(Register, AndData) \
  MmioAndThenOr16 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_SATA, \
  PCI_FUNCTION_NUMBER_PCH_SATA, \
  Register), \
  AndData \
  )

#define PchSataPciCfg16AndThenOr(Register, AndData, OrData) \
  MmioAndThenOr16 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_SATA, \
  PCI_FUNCTION_NUMBER_PCH_SATA, \
  Register), \
  AndData, \
  OrData \
  )

#define PchSataPciCfg8(Register)  MmioRead8 (MmPciAddress (0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_SATA, PCI_FUNCTION_NUMBER_PCH_SATA, Register))

#define PchSataPciCfg8Or(Register, OrData) \
  MmioOr8 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_SATA, \
  PCI_FUNCTION_NUMBER_PCH_SATA, \
  Register), \
  OrData \
  )

#define PchSataPciCfg8And(Register, AndData) \
  MmioAnd8 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_SATA, \
  PCI_FUNCTION_NUMBER_PCH_SATA, \
  Register), \
  AndData \
  )

#define PchSataPciCfg8AndThenOr(Register, AndData, OrData) \
  MmioAndThenOr8 ( \
  MmPciAddress (0, \
  DEFAULT_PCI_BUS_NUMBER_PCH, \
  PCI_DEVICE_NUMBER_PCH_SATA, \
  PCI_FUNCTION_NUMBER_PCH_SATA, \
  Register), \
  AndData, \
  OrData \
  )


///
/// Root Complex Register Block
///
#define PchMmRcrb32(Register)                           MmioRead32 (PCH_RCRB_BASE + Register)

#define PchMmRcrb32Or(Register, OrData)                 MmioOr32 (PCH_RCRB_BASE + Register, OrData)

#define PchMmRcrb32And(Register, AndData)               MmioAnd32 (PCH_RCRB_BASE + Register, AndData)

#define PchMmRcrb32AndThenOr(Register, AndData, OrData) MmioAndThenOr32 (PCH_RCRB_BASE + Register, AndData, OrData)

#define PchMmRcrb16(Register)                           MmioRead16 (PCH_RCRB_BASE + Register)

#define PchMmRcrb16Or(Register, OrData)                 MmioOr16 (PCH_RCRB_BASE + Register, OrData)

#define PchMmRcrb16And(Register, AndData)               MmioAnd16 (PCH_RCRB_BASE + Register, AndData)

#define PchMmRcrb16AndThenOr(Register, AndData, OrData) MmioAndThenOr16 (PCH_RCRB_BASE + Register, AndData, OrData)

#define PchMmRcrb8(Register)                            MmioRead8 (PCH_RCRB_BASE + Register)

#define PchMmRcrb8Or(Register, OrData)                  MmioOr8 (PCH_RCRB_BASE + Register, OrData)

#define PchMmRcrb8And(Register, AndData)                MmioAnd8 (PCH_RCRB_BASE + Register, AndData)

#define PchMmRcrb8AndThenOr(Register, AndData, OrData)  MmioAndThenOr8 (PCH_RCRB_BASE + Register, AndData, OrData)


///
/// Message Bus
///

///
/// Message Bus Registers
///
#define MC_MCR            0x000000D0 // Cunit Message Control Register
#define MC_MDR            0x000000D4 // Cunit Message Data Register
#define MC_MCRX           0x000000D8 // Cunit Message Control Register Extension

///
/// Message Bus API
///
#define MSG_BUS_ENABLED   0x000000F0
#define MSGBUS_MASKHI     0xFFFFFF00
#define MSGBUS_MASKLO     0x000000FF
#define MESSAGE_DWORD_EN  BIT4 | BIT5 | BIT6 | BIT7

#define PchMsgBusRead32(PortId, Register, Dbuff, ReadOpCode, WriteOpCode) \
{ \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCRX), (UINT32) (Register & MSGBUS_MASKHI)); \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCR ), (UINT32) ((ReadOpCode << 24) | (PortId << 16) | ((Register & MSGBUS_MASKLO) << 8) | MESSAGE_DWORD_EN)); \
  (Dbuff) = MmioRead32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MDR)); \
}

#define PchMsgBusAnd32(PortId, Register, Dbuff, AndData, ReadOpCode, WriteOpCode) \
{ \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCRX), (UINT32) (Register & MSGBUS_MASKHI)); \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCR ), (UINT32) ((ReadOpCode << 24) | (PortId << 16) | ((Register & MSGBUS_MASKLO) << 8) | MESSAGE_DWORD_EN)); \
  (Dbuff) = MmioRead32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MDR)); \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCRX), (UINT32) (Register & MSGBUS_MASKHI)); \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MDR ), (UINT32) (Dbuff & AndData)); \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCR ), (UINT32) ((WriteOpCode << 24) | (PortId << 16) | ((Register & MSGBUS_MASKLO) << 8) | MESSAGE_DWORD_EN)); \
}

#define PchMsgBusOr32(PortId, Register, Dbuff, OrData, ReadOpCode, WriteOpCode) \
{ \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCRX), (UINT32) (Register & MSGBUS_MASKHI)); \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCR ), (UINT32) ((ReadOpCode << 24) | (PortId << 16) | ((Register & MSGBUS_MASKLO) << 8) | MESSAGE_DWORD_EN)); \
  (Dbuff) = MmioRead32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MDR)); \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCRX), (UINT32) (Register & MSGBUS_MASKHI)); \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MDR ), (UINT32) (Dbuff | OrData)); \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCR ), (UINT32) ((WriteOpCode << 24) | (PortId << 16) | ((Register & MSGBUS_MASKLO) << 8) | MESSAGE_DWORD_EN)); \
}

#define PchMsgBusAndThenOr32(PortId, Register, Dbuff, AndData, OrData, ReadOpCode, WriteOpCode) \
{ \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCRX), (UINT32) (Register & MSGBUS_MASKHI)); \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCR ), (UINT32) ((ReadOpCode << 24) | (PortId << 16) | ((Register & MSGBUS_MASKLO) << 8) | MESSAGE_DWORD_EN)); \
  (Dbuff) = MmioRead32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MDR)); \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCRX), (UINT32) (Register & MSGBUS_MASKHI)); \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MDR ), (UINT32) ((Dbuff & AndData) | OrData)); \
  MmioWrite32 ((UINTN) (PatchPcdGet64 (PcdPciExpressBaseAddress) + MC_MCR ), (UINT32) ((WriteOpCode << 24) | (PortId << 16) | ((Register & MSGBUS_MASKLO) << 8) | MESSAGE_DWORD_EN)); \
}

typedef struct _PCH_MSG_BUS_TABLE_STRUCT {
  UINT32      PortId;
  UINT32      Address;
  UINT32      AndMask;
  UINT32      OrMask;
  UINT32      ReadOpCode;
  UINT32      WriteOpCode;
} PCH_MSG_BUS_TABLE_STRUCT_TABLE_STRUCT;

#endif
