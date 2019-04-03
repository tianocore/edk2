/*++

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



Module Name:

  PchCommonDefinitions.h

Abstract:

  This header file provides common definitions for PCH

--*/
#ifndef _PCH_COMMON_DEFINITIONS_H_
#define _PCH_COMMON_DEFINITIONS_H_

//
//  MMIO access macros
//
#define PchMmioAddress(BaseAddr, Register)  ((UINTN) BaseAddr + (UINTN) (Register))

//
// 32 bit MMIO access
//
#define PchMmio32Ptr(BaseAddr, Register)  ((volatile UINT32 *) PchMmioAddress (BaseAddr, Register))

#define PchMmio32(BaseAddr, Register)     *PchMmio32Ptr (BaseAddr, Register)

#define PchMmio32Or(BaseAddr, Register, OrData) \
  PchMmio32 (BaseAddr, Register) = (UINT32) \
    (PchMmio32 (BaseAddr, Register) | (UINT32) (OrData))

#define PchMmio32And(BaseAddr, Register, AndData) \
  PchMmio32 (BaseAddr, Register) = (UINT32) \
    (PchMmio32 (BaseAddr, Register) & (UINT32) (AndData))

#define PchMmio32AndThenOr(BaseAddr, Register, AndData, OrData) \
  PchMmio32 (BaseAddr, Register) = (UINT32) \
    ((PchMmio32 (BaseAddr, Register) & (UINT32) (AndData)) | (UINT32) (OrData))

//
// 16 bit MMIO access
//
#define PchMmio16Ptr(BaseAddr, Register)  ((volatile UINT16 *) PchMmioAddress (BaseAddr, Register))

#define PchMmio16(BaseAddr, Register)     *PchMmio16Ptr (BaseAddr, Register)

#define PchMmio16Or(BaseAddr, Register, OrData) \
  PchMmio16 (BaseAddr, Register) = (UINT16) \
    (PchMmio16 (BaseAddr, Register) | (UINT16) (OrData))

#define PchMmio16And(BaseAddr, Register, AndData) \
  PchMmio16 (BaseAddr, Register) = (UINT16) \
    (PchMmio16 (BaseAddr, Register) & (UINT16) (AndData))

#define PchMmio16AndThenOr(BaseAddr, Register, AndData, OrData) \
  PchMmio16 (BaseAddr, Register) = (UINT16) \
    ((PchMmio16 (BaseAddr, Register) & (UINT16) (AndData)) | (UINT16) (OrData))

//
// 8 bit MMIO access
//
#define PchMmio8Ptr(BaseAddr, Register) ((volatile UINT8 *) PchMmioAddress (BaseAddr, Register))

#define PchMmio8(BaseAddr, Register)    *PchMmio8Ptr (BaseAddr, Register)

#define PchMmio8Or(BaseAddr, Register, OrData) \
  PchMmio8 (BaseAddr, Register) = (UINT8) \
    (PchMmio8 (BaseAddr, Register) | (UINT8) (OrData))

#define PchMmio8And(BaseAddr, Register, AndData) \
  PchMmio8 (BaseAddr, Register) = (UINT8) \
    (PchMmio8 (BaseAddr, Register) & (UINT8) (AndData))

#define PchMmio8AndThenOr(BaseAddr, Register, AndData, OrData) \
  PchMmio8 (BaseAddr, Register) = (UINT8) \
    ((PchMmio8 (BaseAddr, Register) & (UINT8) (AndData)) | (UINT8) (OrData))

//
// Memory Mapped PCI Access macros
//
#define PCH_PCI_EXPRESS_BASE_ADDRESS  0xE0000000
//
// PCI Device MM Base
//
#define PchPciDeviceMmBase(Bus, Device, Function) \
    ( \
      (UINTN) PCH_PCI_EXPRESS_BASE_ADDRESS + (UINTN) (Bus << 20) + (UINTN) (Device << 15) + (UINTN) \
        (Function << 12) \
    )

//
// PCI Device MM Address
//
#define PchPciDeviceMmAddress(Segment, Bus, Device, Function, Register) \
    ( \
      (UINTN) PCH_PCI_EXPRESS_BASE_ADDRESS + (UINTN) (Bus << 20) + (UINTN) (Device << 15) + (UINTN) \
        (Function << 12) + (UINTN) (Register) \
    )

//
// 32 bit PCI access
//
#define PchMmPci32Ptr(Segment, Bus, Device, Function, Register) \
    ((volatile UINT32 *) PchPciDeviceMmAddress (Segment, Bus, Device, Function, Register))

#define PchMmPci32(Segment, Bus, Device, Function, Register)  *PchMmPci32Ptr (Segment, Bus, Device, Function, Register)

#define PchMmPci32Or(Segment, Bus, Device, Function, Register, OrData) \
  PchMmPci32 ( \
  Segment, \
  Bus, \
  Device, \
  Function, \
  Register \
  ) = (UINT32) (PchMmPci32 (Segment, Bus, Device, Function, Register) | (UINT32) (OrData))

#define PchMmPci32And(Segment, Bus, Device, Function, Register, AndData) \
  PchMmPci32 ( \
  Segment, \
  Bus, \
  Device, \
  Function, \
  Register \
  ) = (UINT32) (PchMmPci32 (Segment, Bus, Device, Function, Register) & (UINT32) (AndData))

#define PchMmPci32AndThenOr(Segment, Bus, Device, Function, Register, AndData, OrData) \
  PchMmPci32 ( \
  Segment, \
  Bus, \
  Device, \
  Function, \
  Register \
  ) = (UINT32) ((PchMmPci32 (Segment, Bus, Device, Function, Register) & (UINT32) (AndData)) | (UINT32) (OrData))

//
// 16 bit PCI access
//
#define PchMmPci16Ptr(Segment, Bus, Device, Function, Register) \
    ((volatile UINT16 *) PchPciDeviceMmAddress (Segment, Bus, Device, Function, Register))

#define PchMmPci16(Segment, Bus, Device, Function, Register)  *PchMmPci16Ptr (Segment, Bus, Device, Function, Register)

#define PchMmPci16Or(Segment, Bus, Device, Function, Register, OrData) \
  PchMmPci16 ( \
  Segment, \
  Bus, \
  Device, \
  Function, \
  Register \
  ) = (UINT16) (PchMmPci16 (Segment, Bus, Device, Function, Register) | (UINT16) (OrData))

#define PchMmPci16And(Segment, Bus, Device, Function, Register, AndData) \
  PchMmPci16 ( \
  Segment, \
  Bus, \
  Device, \
  Function, \
  Register \
  ) = (UINT16) (PchMmPci16 (Segment, Bus, Device, Function, Register) & (UINT16) (AndData))

#define PchMmPci16AndThenOr(Segment, Bus, Device, Function, Register, AndData, OrData) \
  PchMmPci16 ( \
  Segment, \
  Bus, \
  Device, \
  Function, \
  Register \
  ) = (UINT16) ((PchMmPci16 (Segment, Bus, Device, Function, Register) & (UINT16) (AndData)) | (UINT16) (OrData))

//
// 8 bit PCI access
//
#define PchMmPci8Ptr(Segment, Bus, Device, Function, Register) \
    ((volatile UINT8 *) PchPciDeviceMmAddress (Segment, Bus, Device, Function, Register))

#define PchMmPci8(Segment, Bus, Device, Function, Register) *PchMmPci8Ptr (Segment, Bus, Device, Function, Register)

#define PchMmPci8Or(Segment, Bus, Device, Function, Register, OrData) \
  PchMmPci8 ( \
  Segment, \
  Bus, \
  Device, \
  Function, \
  Register \
  ) = (UINT8) (PchMmPci8 (Segment, Bus, Device, Function, Register) | (UINT8) (OrData))

#define PchMmPci8And(Segment, Bus, Device, Function, Register, AndData) \
  PchMmPci8 ( \
  Segment, \
  Bus, \
  Device, \
  Function, \
  Register \
  ) = (UINT8) (PchMmPci8 (Segment, Bus, Device, Function, Register) & (UINT8) (AndData))

#define PchMmPci8AndThenOr(Segment, Bus, Device, Function, Register, AndData, OrData) \
  PchMmPci8 ( \
  Segment, \
  Bus, \
  Device, \
  Function, \
  Register \
  ) = (UINT8) ((PchMmPci8 (Segment, Bus, Device, Function, Register) & (UINT8) (AndData)) | (UINT8) (OrData))

#endif
