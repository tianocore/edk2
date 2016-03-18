/** @file
  HPET register definitions from the IA-PC HPET (High Precision Event Timers) 
  Specification, Revision 1.0a, October 2004.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __HPET_REGISTER_H__
#define __HPET_REGISTER_H__

///
/// HPET General Register Offsets
///
#define HPET_GENERAL_CAPABILITIES_ID_OFFSET   0x000
#define HPET_GENERAL_CONFIGURATION_OFFSET     0x010
#define HPET_GENERAL_INTERRUPT_STATUS_OFFSET  0x020

///
/// HPET Timer Register Offsets
///
#define HPET_MAIN_COUNTER_OFFSET              0x0F0
#define HPET_TIMER_CONFIGURATION_OFFSET       0x100
#define HPET_TIMER_COMPARATOR_OFFSET          0x108
#define HPET_TIMER_MSI_ROUTE_OFFSET           0x110

///
/// Stride between sets of HPET Timer Registers
///
#define HPET_TIMER_STRIDE         0x20

#pragma pack(1)

///
/// HPET General Capabilities and ID Register
///
typedef union {
  struct {
    UINT32  Revision:8;
    UINT32  NumberOfTimers:5;
    UINT32  CounterSize:1;
    UINT32  Reserved0:1;
    UINT32  LegacyRoute:1;
    UINT32  VendorId:16;
    UINT32  CounterClockPeriod:32;
  } Bits;
  UINT64  Uint64;
} HPET_GENERAL_CAPABILITIES_ID_REGISTER;

///
/// HPET General Configuration Register
///
typedef union {
  struct {
    UINT32  MainCounterEnable:1;
    UINT32  LegacyRouteEnable:1;
    UINT32  Reserved0:30;
    UINT32  Reserved1:32;
  } Bits;
  UINT64  Uint64;
} HPET_GENERAL_CONFIGURATION_REGISTER;

///
/// HPET Timer Configuration Register
///
typedef union {
  struct {
    UINT32  Reserved0:1;
    UINT32  LevelTriggeredInterrupt:1;
    UINT32  InterruptEnable:1;
    UINT32  PeriodicInterruptEnable:1;
    UINT32  PeriodicInterruptCapablity:1;
    UINT32  CounterSizeCapablity:1;
    UINT32  ValueSetEnable:1;
    UINT32  Reserved1:1;
    UINT32  CounterSizeEnable:1;
    UINT32  InterruptRoute:5;
    UINT32  MsiInterruptEnable:1;
    UINT32  MsiInterruptCapablity:1;
    UINT32  Reserved2:16;
    UINT32  InterruptRouteCapability;
  } Bits;
  UINT64  Uint64;
} HPET_TIMER_CONFIGURATION_REGISTER;

///
/// HPET Timer MSI Route Register
///
typedef union {
  struct {
    UINT32  Value:32;
    UINT32  Address:32;
  } Bits;
  UINT64  Uint64;
} HPET_TIMER_MSI_ROUTE_REGISTER;

#pragma pack()

#endif
