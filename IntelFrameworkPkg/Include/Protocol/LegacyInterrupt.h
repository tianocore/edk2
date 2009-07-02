/** @file
  This protocol abstracts the PIRQ programming from the generic EFI Compatibility Support Modules (CSMs).

  Copyright (c) 2007 - 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This protocol is defined in Framework for EFI Compatibility Support Module spec
  Version 0.97.

**/

#ifndef _EFI_LEGACY_INTERRUPT_H_
#define _EFI_LEGACY_INTERRUPT_H_

#include <PiDxe.h>

#define EFI_LEGACY_INTERRUPT_PROTOCOL_GUID \
  { \
    0x31ce593d, 0x108a, 0x485d, {0xad, 0xb2, 0x78, 0xf2, 0x1f, 0x29, 0x66, 0xbe } \
  }

typedef struct _EFI_LEGACY_INTERRUPT_PROTOCOL EFI_LEGACY_INTERRUPT_PROTOCOL;

/**
  Get the number of PIRQs this hardware supports.

  @param  This                  Protocol instance pointer.
  @param  NumberPirsq           Number of PIRQs that are supported.

  @retval EFI_SUCCESS           The number of PIRQs was returned successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_INTERRUPT_GET_NUMBER_PIRQS)(
  IN EFI_LEGACY_INTERRUPT_PROTOCOL            *This,
  OUT UINT8                                   *NumberPirqs
  );

/**
  Gets the PCI location associated with this protocol.

  @param  This                  Protocol instance pointer.
  @param  Bus                   PCI Bus
  @param  Device                PCI Device
  @param  Function              PCI Function

  @retval EFI_SUCCESS           The Bus, Device, and Function were returned successfully

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_INTERRUPT_GET_LOCATION)(
  IN EFI_LEGACY_INTERRUPT_PROTOCOL            *This,
  OUT UINT8                                   *Bus,
  OUT UINT8                                   *Device,
  OUT UINT8                                   *Function
  );

/**
  Read the PIRQ register and return the data

  @param  This                  Protocol instance pointer.
  @param  PirqNumber            PIRQ register to read
  @param  PirqData              Data read

  @retval EFI_SUCCESS           Data was read
  @retval EFI_INVALID_PARAMETER Invalid PIRQ number

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_INTERRUPT_READ_PIRQ)(
  IN EFI_LEGACY_INTERRUPT_PROTOCOL           *This,
  IN  UINT8                                  PirqNumber,
  OUT UINT8                                  *PirqData
  );

/**
  Write the specified PIRQ register with the given data.

  @param  This                  Protocol instance pointer.
  @param  PirqNumber            PIRQ register to read.
  @param  PirqData              Data to write.

  @retval EFI_SUCCESS           The PIRQ was programmed
  @retval EFI_INVALID_PARAMETER Invalid PIRQ number

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_INTERRUPT_WRITE_PIRQ)(
  IN EFI_LEGACY_INTERRUPT_PROTOCOL           *This,
  IN  UINT8                                  PirqNumber,
  IN UINT8                                   PirqData
  );

struct _EFI_LEGACY_INTERRUPT_PROTOCOL {
  ///
  ///   Gets the number of PIRQs supported.
  ///
  EFI_LEGACY_INTERRUPT_GET_NUMBER_PIRQS GetNumberPirqs;

  ///
  /// Gets the PCI bus, device, and function that associated with this protocol.
  ///
  EFI_LEGACY_INTERRUPT_GET_LOCATION     GetLocation;

  ///
  /// Reads the indicated PIRQ register.
  ///
  EFI_LEGACY_INTERRUPT_READ_PIRQ        ReadPirq;

  ///
  /// Writes to the indicated PIRQ register.
  ///
  EFI_LEGACY_INTERRUPT_WRITE_PIRQ       WritePirq;
};

extern EFI_GUID gEfiLegacyInterruptProtocolGuid;

#endif
