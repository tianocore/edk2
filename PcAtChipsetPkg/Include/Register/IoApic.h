/** @file
  I/O APIC Register Definitions from 82093AA I/O Advanced Programmable Interrupt
  Controller (IOAPIC), 1996.

  Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __IO_APIC_H__
#define __IO_APIC_H__

///
/// I/O APIC Register Offsets
///
#define IOAPIC_INDEX_OFFSET  0x00
#define IOAPIC_DATA_OFFSET   0x10

///
/// I/O APIC Indirect Register Indexes
///
#define IO_APIC_IDENTIFICATION_REGISTER_INDEX  0x00
#define IO_APIC_VERSION_REGISTER_INDEX         0x01
#define IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX  0x10

///
/// I/O APIC Interrupt Deliver Modes
///
#define IO_APIC_DELIVERY_MODE_FIXED            0
#define IO_APIC_DELIVERY_MODE_LOWEST_PRIORITY  1
#define IO_APIC_DELIVERY_MODE_SMI              2
#define IO_APIC_DELIVERY_MODE_NMI              4
#define IO_APIC_DELIVERY_MODE_INIT             5
#define IO_APIC_DELIVERY_MODE_EXTINT           7

#pragma pack(1)

typedef union {
  struct {
    UINT32    Reserved0      : 24;
    UINT32    Identification : 4;
    UINT32    Reserved1      : 4;
  } Bits;
  UINT32    Uint32;
} IO_APIC_IDENTIFICATION_REGISTER;

typedef union {
  struct {
    UINT32    Version                 : 8;
    UINT32    Reserved0               : 8;
    UINT32    MaximumRedirectionEntry : 8;
    UINT32    Reserved1               : 8;
  } Bits;
  UINT32    Uint32;
} IO_APIC_VERSION_REGISTER;

typedef union {
  struct {
    UINT32    Vector          :          8;
    UINT32    DeliveryMode    :    3;
    UINT32    DestinationMode : 1;
    UINT32    DeliveryStatus  :  1;
    UINT32    Polarity        :        1;
    UINT32    RemoteIRR       :       1;
    UINT32    TriggerMode     :     1;
    UINT32    Mask            :            1;
    UINT32    Reserved0       :       15;
    UINT32    Reserved1       :       24;
    UINT32    DestinationID   :   8;
  } Bits;
  struct {
    UINT32    Low;
    UINT32    High;
  } Uint32;
  UINT64    Uint64;
} IO_APIC_REDIRECTION_TABLE_ENTRY;

#pragma pack()

#endif
