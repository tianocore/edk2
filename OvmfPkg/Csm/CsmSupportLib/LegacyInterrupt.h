/** @file
  Legacy Region Support

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LEGACY_INTERRUPT_H_
#define _LEGACY_INTERRUPT_H_

#include <PiDxe.h>

#include <Protocol/LegacyInterrupt.h>

#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <OvmfPlatforms.h>


#define LEGACY_INT_BUS  0
#define LEGACY_INT_DEV_PIIX4  0x01
#define LEGACY_INT_DEV_Q35    0x1f
#define LEGACY_INT_FUNC 0

#define PIRQN           0x00  // PIRQ Null
#define PIRQA           0x60
#define PIRQB           0x61
#define PIRQC           0x62
#define PIRQD           0x63
#define PIRQE           0x68
#define PIRQF           0x69
#define PIRQG           0x6A
#define PIRQH           0x6B

#define MAX_PIRQ_NUMBER 8

/**
  Return the number of PIRQs supported by this chipset.

  @param[in]  This         Pointer to LegacyInterrupt Protocol
  @param[out] NumberPirqs  The pointer to return the max IRQ number supported

  @retval EFI_SUCCESS   Max PIRQs successfully returned

**/
EFI_STATUS
EFIAPI
GetNumberPirqs (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  OUT UINT8                          *NumberPirqs
  );

/**
  Return PCI location of this device.
  $PIR table requires this info.

  @param[in]   This                - Protocol instance pointer.
  @param[out]  Bus                 - PCI Bus
  @param[out]  Device              - PCI Device
  @param[out]  Function            - PCI Function

  @retval  EFI_SUCCESS   Bus/Device/Function returned

**/
EFI_STATUS
EFIAPI
GetLocation (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  OUT UINT8                          *Bus,
  OUT UINT8                          *Device,
  OUT UINT8                          *Function
  );

/**
  Read the given PIRQ register

  @param[in]  This        Protocol instance pointer
  @param[in]  PirqNumber  The Pirq register 0 = A, 1 = B etc
  @param[out] PirqData    Value read

  @retval EFI_SUCCESS   Decoding change affected.
  @retval EFI_INVALID_PARAMETER   Invalid PIRQ number

**/
EFI_STATUS
EFIAPI
ReadPirq (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  IN  UINT8                          PirqNumber,
  OUT UINT8                          *PirqData
  );

/**
  Write the given PIRQ register

  @param[in]  This        Protocol instance pointer
  @param[in]  PirqNumber  The Pirq register 0 = A, 1 = B etc
  @param[out] PirqData    Value to write

  @retval EFI_SUCCESS   Decoding change affected.
  @retval EFI_INVALID_PARAMETER   Invalid PIRQ number

**/
EFI_STATUS
EFIAPI
WritePirq (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  IN  UINT8                          PirqNumber,
  IN  UINT8                          PirqData
  );

#endif

