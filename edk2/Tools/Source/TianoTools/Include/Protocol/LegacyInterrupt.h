/** @file
  This protocol manages the legacy memory regions between 0xc0000 - 0xfffff

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    

  Module Name:  LegacyInterrupt.h

  @par Revision Reference:
  This protocol is defined in Framework for EFI Compatibility Support Module spec
  Version 0.96

**/

#ifndef _EFI_LEGACY_INTERRUPT_H_
#define _EFI_LEGACY_INTERRUPT_H_

#define EFI_LEGACY_INTERRUPT_PROTOCOL_GUID \
  { \
    0x31ce593d, 0x108a, 0x485d, {0xad, 0xb2, 0x78, 0xf2, 0x1f, 0x29, 0x66, 0xbe } \
  }

typedef struct _EFI_LEGACY_INTERRUPT_PROTOCOL EFI_LEGACY_INTERRUPT_PROTOCOL;

/**
  Get the number of PIRQs this hardware supports.

  @param  This Protocol instance pointer.
  @param  NumberPirsq Number of PIRQs.

  @retval  EFI_SUCCESS Number of PIRQs returned.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_INTERRUPT_GET_NUMBER_PIRQS) (
  IN EFI_LEGACY_INTERRUPT_PROTOCOL            *This,
  OUT UINT8                                   *NumberPirqs
  );

/**
  Gets the PCI location associated with this protocol.

  @param  This Protocol instance pointer.
  @param  Bus PCI Bus
  @param  Device PCI Device
  @param  Function PCI Function

  @retval  EFI_SUCCESS Bus/Device/Function returned

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_INTERRUPT_GET_LOCATION) (
  IN EFI_LEGACY_INTERRUPT_PROTOCOL            *This,
  OUT UINT8                                   *Bus,
  OUT UINT8                                   *Device,
  OUT UINT8                                   *Function
  );

/**
  Read the PIRQ register and return the data

  @param  This Protocol instance pointer.
  @param  PirqNumber PIRQ register to read
  @param  PirqData Data read

  @retval  EFI_SUCCESS Data was read
  @retval  EFI_INVALID_PARAMETER Invalid PIRQ number

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_INTERRUPT_READ_PIRQ) (
  IN EFI_LEGACY_INTERRUPT_PROTOCOL           *This,
  IN  UINT8                                  PirqNumber,
  OUT UINT8                                  *PirqData
  );

/**
  Write the specified PIRQ register with the given data.

  @param  This Protocol instance pointer.
  @param  PirqNumber PIRQ register to read.
  @param  PirqData Data written.

  @retval  EFI_SUCCESS Table pointer returned
  @retval  EFI_INVALID_PARAMETER Invalid PIRQ number

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_INTERRUPT_WRITE_PIRQ) (
  IN EFI_LEGACY_INTERRUPT_PROTOCOL           *This,
  IN  UINT8                                  PirqNumber,
  IN UINT8                                   PirqData
  );

/**
  @par Protocol Description:
  Abstracts the PIRQ programming from the generic EFI Compatibility Support Modules

  @param GetNumberPirqs
  Gets the number of PIRQs supported.

  @param GetLocation
  Gets the PCI bus, device, and function that associated with this protocol. 

  @param ReadPirq
  Reads the indicated PIRQ register.

  @param WritePirq
  Writes to the indicated PIRQ register. 

**/
struct _EFI_LEGACY_INTERRUPT_PROTOCOL {
  EFI_LEGACY_INTERRUPT_GET_NUMBER_PIRQS GetNumberPirqs;
  EFI_LEGACY_INTERRUPT_GET_LOCATION     GetLocation;
  EFI_LEGACY_INTERRUPT_READ_PIRQ        ReadPirq;
  EFI_LEGACY_INTERRUPT_WRITE_PIRQ       WritePirq;
};

extern EFI_GUID gEfiLegacyInterruptProtocolGuid;

#endif
