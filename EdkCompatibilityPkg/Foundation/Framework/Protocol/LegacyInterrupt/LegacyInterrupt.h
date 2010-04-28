/*++

Copyright (c) 1999 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  LegacyInterrupt.h
    
Abstract:

  This protocol manages the PIRQ for PCI devices

Revision History

  The EFI Legacy Interrupt Protocol is compliant with CSM spec 0.96.

--*/

#ifndef _EFI_LEGACY_INTERRUPT_H_
#define _EFI_LEGACY_INTERRUPT_H_

#define EFI_LEGACY_INTERRUPT_PROTOCOL_GUID \
  { \
    0x31ce593d, 0x108a, 0x485d, {0xad, 0xb2, 0x78, 0xf2, 0x1f, 0x29, 0x66, 0xbe} \
  }

EFI_FORWARD_DECLARATION (EFI_LEGACY_INTERRUPT_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_INTERRUPT_GET_NUMBER_PIRQS) (
  IN EFI_LEGACY_INTERRUPT_PROTOCOL            * This,
  OUT UINT8                                   *NumberPirqs
  );

/*++

  Routine Description:
      Return the number of PIRQs this hardware supports.

  Arguments:
    This                - Protocol instance pointer.
    NumberPirsq         - Number of PIRQs.

  Returns:
    EFI_SUCCESS   - Number of PIRQs returned.

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_INTERRUPT_GET_LOCATION) (
  IN EFI_LEGACY_INTERRUPT_PROTOCOL            * This,
  OUT UINT8                                   *Bus,
  OUT UINT8                                   *Device,
  OUT UINT8                                   *Function
  );

/*++

  Routine Description:
      Return PCI location of this device. $PIR table requires this info.

  Arguments:
    This                - Protocol instance pointer.
    Bus                 - PCI Bus
    Device              - PCI Device
    Function            - PCI Function

  Returns:
    EFI_SUCCESS   - Bus/Device/Function returned

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_INTERRUPT_READ_PIRQ) (
  IN EFI_LEGACY_INTERRUPT_PROTOCOL           * This,
  IN  UINT8                                  PirqNumber,
  OUT UINT8                                  *PirqData
  );

/*++

  Routine Description:
      Read the PIRQ register and return the data

  Arguments:
    This                - Protocol instance pointer.
    PirqNumber          - PIRQ register to read
    PirqData            - Data read

  Returns:
    EFI_SUCCESS   - Data was read
    EFI_INVALID_PARAMETER - Invalid PIRQ number

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_INTERRUPT_WRITE_PIRQ) (
  IN EFI_LEGACY_INTERRUPT_PROTOCOL           * This,
  IN  UINT8                                  PirqNumber,
  IN UINT8                                   PirqData
  );

/*++

  Routine Description:
      Write the specified PIRQ register with the given data.

  Arguments:
    This                - Protocol instance pointer.
    PirqNumber          - PIRQ register to read.
    PirqData            - Data written.

  Returns:
    EFI_SUCCESS   - Table pointer returned
    EFI_INVALID_PARAMETER - Invalid PIRQ number

--*/
struct _EFI_LEGACY_INTERRUPT_PROTOCOL {
  EFI_LEGACY_INTERRUPT_GET_NUMBER_PIRQS GetNumberPirqs;
  EFI_LEGACY_INTERRUPT_GET_LOCATION     GetLocation;
  EFI_LEGACY_INTERRUPT_READ_PIRQ        ReadPirq;
  EFI_LEGACY_INTERRUPT_WRITE_PIRQ       WritePirq;
};

extern EFI_GUID gEfiLegacyInterruptProtocolGuid;

#endif
