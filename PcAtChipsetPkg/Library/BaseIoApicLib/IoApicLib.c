/** @file 
  I/O APIC library.

  I/O APIC library assumes I/O APIC is enabled. It does not
  handles cases where I/O APIC is disabled.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/IoApicLib.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/LocalApicLib.h>

#include <Register/IoApic.h>

/**
  Read a 32-bit I/O APIC register.

  If Index is >= 0x100, then ASSERT().
  
  @param  Index  Specifies the I/O APIC register to read.

  @return  The 32-bit value read from the I/O APIC register specified by Index.
**/
UINT32
EFIAPI
IoApicRead (
  IN UINTN  Index
  )
{
  ASSERT (Index < 0x100);
  MmioWrite8 (PcdGet32 (PcdIoApicBaseAddress) + IOAPIC_INDEX_OFFSET, (UINT8)Index);
  return MmioRead32 (PcdGet32 (PcdIoApicBaseAddress) + IOAPIC_DATA_OFFSET);
}

/**
  Write a 32-bit I/O APIC register.

  If Index is >= 0x100, then ASSERT().
  
  @param  Index  Specifies the I/O APIC register to write.
  @param  Value  Specifies the value to write to the I/O APIC register specified by Index.

  @return  The 32-bit value written to I/O APIC register specified by Index.
**/
UINT32
EFIAPI
IoApicWrite (
  IN UINTN   Index,
  IN UINT32  Value
  )
{
  ASSERT (Index < 0x100);
  MmioWrite8 (PcdGet32 (PcdIoApicBaseAddress) + IOAPIC_INDEX_OFFSET, (UINT8)Index);
  return MmioWrite32 (PcdGet32 (PcdIoApicBaseAddress) + IOAPIC_DATA_OFFSET, Value);
}

/**
  Set the interrupt mask of an I/O APIC interrupt.

  If Irq is larger than the maximum number I/O APIC redirection entries, then ASSERT(). 

  @param  Irq     Specifies the I/O APIC interrupt to enable or disable.
  @param  Enable  If TRUE, then enable the I/O APIC interrupt specified by Irq.
                  If FALSE, then disable the I/O APIC interrupt specified by Irq.
**/
VOID
EFIAPI
IoApicEnableInterrupt (
  IN UINTN    Irq,
  IN BOOLEAN  Enable
  )
{
  IO_APIC_VERSION_REGISTER         Version;
  IO_APIC_REDIRECTION_TABLE_ENTRY  Entry;

  Version.Uint32 = IoApicRead (IO_APIC_VERSION_REGISTER_INDEX);
  ASSERT (Version.Bits.MaximumRedirectionEntry < 0xF0);
  ASSERT (Irq <= Version.Bits.MaximumRedirectionEntry);

  Entry.Uint32.Low = IoApicRead (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2);
  Entry.Bits.Mask = Enable ? 0 : 1;
  IoApicWrite (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2, Entry.Uint32.Low);
}

/**
  Configures an I/O APIC interrupt.
  
  Configure an I/O APIC Redirection Table Entry to deliver an interrupt in physical
  mode to the Local APIC of the currntly executing CPU.  The default state of the 
  entry is for the interrupt to be disabled (masked).  IoApicEnableInterrupts() must
  be used to enable(unmask) the I/O APIC Interrupt.
 
  If Irq is larger than the maximum number I/O APIC redirection entries, then ASSERT(). 
  If Vector >= 0x100, then ASSERT().
  If DeliveryMode is not supported, then ASSERT().

  @param  Irq             Specifies the I/O APIC interrupt to initialize.
  @param  Vector          The 8-bit interrupt vector associated with the I/O APIC
                          Interrupt.  Must be in the range 0x10..0xFE.
  @param  DeliveryMode    A 3-bit value that specifies how the recept of the I/O APIC
                          interrupt is handled.  The only supported values are:
                            0: IO_APIC_DELIVERY_MODE_FIXED
                            1: IO_APIC_DELIVERY_MODE_LOWEST_PRIORITY
                            2: IO_APIC_DELIVERY_MODE_SMI
                            4: IO_APIC_DELIVERY_MODE_NMI
                            5: IO_APIC_DELIVERY_MODE_INIT
                            7: IO_APIC_DELIVERY_MODE_EXTINT
  @param  LevelTriggered  TRUE specifies a level triggered interrupt.
                          FALSE specifies an edge triggered interrupt.
  @param  AssertionLevel  TRUE specified an active high interrupt.
                          FALSE specifies an active low interrupt.
**/
VOID
EFIAPI
IoApicConfigureInterrupt (
  IN UINTN    Irq,
  IN UINTN    Vector,
  IN UINTN    DeliveryMode,
  IN BOOLEAN  LevelTriggered,
  IN BOOLEAN  AssertionLevel
  )
{
  IO_APIC_VERSION_REGISTER         Version;
  IO_APIC_REDIRECTION_TABLE_ENTRY  Entry;

  Version.Uint32 = IoApicRead (IO_APIC_VERSION_REGISTER_INDEX);
  ASSERT (Version.Bits.MaximumRedirectionEntry < 0xF0);
  ASSERT (Irq <= Version.Bits.MaximumRedirectionEntry);
  ASSERT (Vector <= 0xFF);
  ASSERT (DeliveryMode < 8 && DeliveryMode != 6 && DeliveryMode != 3);
  
  Entry.Uint32.Low = IoApicRead (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2);
  Entry.Bits.Vector          = (UINT8)Vector;
  Entry.Bits.DeliveryMode    = (UINT32)DeliveryMode;
  Entry.Bits.DestinationMode = 0; 
  Entry.Bits.Polarity        = AssertionLevel ? 0 : 1;
  Entry.Bits.TriggerMode     = LevelTriggered ? 1 : 0;
  Entry.Bits.Mask            = 1;
  IoApicWrite (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2, Entry.Uint32.Low);

  Entry.Uint32.High = IoApicRead (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2 + 1);
  Entry.Bits.DestinationID = GetApicId ();
  IoApicWrite (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2 + 1, Entry.Uint32.High);
}
