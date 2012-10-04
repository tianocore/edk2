/** @file
  ACPI Timer implements one instance of Timer Library.

  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>

  This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <IndustryStandard/Pci22.h>
#include <IndustryStandard/Acpi.h>

//
// PCI Location of PIIX4 Power Management PCI Configuration Registers
//
#define PIIX4_POWER_MANAGEMENT_BUS       0x00
#define PIIX4_POWER_MANAGEMENT_DEVICE    0x01
#define PIIX4_POWER_MANAGEMENT_FUNCTION  0x03

//
// Macro to access PIIX4 Power Management PCI Configuration Registers
//
#define PIIX4_PCI_POWER_MANAGEMENT_REGISTER(Register) \
  PCI_LIB_ADDRESS (                                   \
    PIIX4_POWER_MANAGEMENT_BUS,                       \
    PIIX4_POWER_MANAGEMENT_DEVICE,                    \
    PIIX4_POWER_MANAGEMENT_FUNCTION,                  \
    Register                                          \
    )

//
// PIIX4 Power Management PCI Configuration Registers
//
#define PMBA                PIIX4_PCI_POWER_MANAGEMENT_REGISTER (0x40)
#define   PMBA_RTE          BIT0
#define PMREGMISC           PIIX4_PCI_POWER_MANAGEMENT_REGISTER (0x80)
#define   PMIOSE            BIT0

//
// The ACPI Time in the PIIX4 is a 24-bit counter
//
#define ACPI_TIMER_COUNT_SIZE  BIT24

//
// Offset in the PIIX4 Power Management Base Address to the ACPI Timer 
//
#define ACPI_TIMER_OFFSET      0x8

/**
  The constructor function enables ACPI IO space.

  If ACPI I/O space not enabled, this function will enable it.
  It will always return RETURN_SUCCESS.

  @retval EFI_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
RETURN_STATUS
EFIAPI
AcpiTimerLibConstructor (
  VOID
  )
{
  //
  // Check to see if the PIIX4 Power Management Base Address is already enabled
  //
  if ((PciRead8 (PMREGMISC) & PMIOSE) == 0) {
    //
    // If the PIIX4 Power Management Base Address is not programmed, 
    // then program the PIIX4 Power Management Base Address from a PCD.
    //
    PciAndThenOr32 (PMBA, (UINT32)(~0x0000FFC0), PcdGet16 (PcdAcpiPmBaseAddress));

    //
    // Enable PMBA I/O port decodes in PMREGMISC
    //
    PciOr8 (PMREGMISC, PMIOSE);
  }
  
  return RETURN_SUCCESS;
}

/**
  Internal function to read the current tick counter of ACPI.

  Internal function to read the current tick counter of ACPI.

  @return The tick counter read.

**/
UINT32
InternalAcpiGetTimerTick (
  VOID
  )
{
  //
  //   Read PMBA to read and return the current ACPI timer value.
  //
  return IoRead32 ((PciRead32 (PMBA) & ~PMBA_RTE) + ACPI_TIMER_OFFSET);
}

/**
  Stalls the CPU for at least the given number of ticks.

  Stalls the CPU for at least the given number of ticks. It's invoked by
  MicroSecondDelay() and NanoSecondDelay().

  @param  Delay     A period of time to delay in ticks.

**/
VOID
InternalAcpiDelay (
  IN      UINT32                    Delay
  )
{
  UINT32                            Ticks;
  UINT32                            Times;

  Times    = Delay >> 22;
  Delay   &= BIT22 - 1;
  do {
    //
    // The target timer count is calculated here
    //
    Ticks    = InternalAcpiGetTimerTick () + Delay;
    Delay    = BIT22;
    //
    // Wait until time out
    // Delay >= 2^23 could not be handled by this function
    // Timer wrap-arounds are handled correctly by this function
    //
    while (((Ticks - InternalAcpiGetTimerTick ()) & BIT23) == 0) {
      CpuPause ();
    }
  } while (Times-- > 0);
}

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return MicroSeconds

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  )
{
  InternalAcpiDelay (
    (UINT32)DivU64x32 (
              MultU64x32 (
                MicroSeconds,
                ACPI_TIMER_FREQUENCY
                ),
              1000000u
              )
    );
  return MicroSeconds;
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  @param  NanoSeconds The minimum number of nanoseconds to delay.

  @return NanoSeconds

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN      UINTN                     NanoSeconds
  )
{
  InternalAcpiDelay (
    (UINT32)DivU64x32 (
              MultU64x32 (
                NanoSeconds,
                ACPI_TIMER_FREQUENCY
                ),
              1000000000u
              )
    );
  return NanoSeconds;
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  Retrieves the current value of a 64-bit free running performance counter. The
  counter can either count up by 1 or count down by 1. If the physical
  performance counter counts by a larger increment, then the counter values
  must be translated. The properties of the counter can be retrieved from
  GetPerformanceCounterProperties().

  @return The current value of the free running performance counter.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  return (UINT64)InternalAcpiGetTimerTick ();
}

/**
  Retrieves the 64-bit frequency in Hz and the range of performance counter
  values.

  If StartValue is not NULL, then the value that the performance counter starts
  with immediately after is it rolls over is returned in StartValue. If
  EndValue is not NULL, then the value that the performance counter end with
  immediately before it rolls over is returned in EndValue. The 64-bit
  frequency of the performance counter in Hz is always returned. If StartValue
  is less than EndValue, then the performance counter counts up. If StartValue
  is greater than EndValue, then the performance counter counts down. For
  example, a 64-bit free running counter that counts up would have a StartValue
  of 0 and an EndValue of 0xFFFFFFFFFFFFFFFF. A 24-bit free running counter
  that counts down would have a StartValue of 0xFFFFFF and an EndValue of 0.

  @param  StartValue  The value the performance counter starts with when it
                      rolls over.
  @param  EndValue    The value that the performance counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT      UINT64                    *StartValue,  OPTIONAL
  OUT      UINT64                    *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = 0;
  }

  if (EndValue != NULL) {
    *EndValue = ACPI_TIMER_COUNT_SIZE - 1;
  }

  return ACPI_TIMER_FREQUENCY;
}

/**
  Converts elapsed ticks of performance counter to time in nanoseconds.

  This function converts the elapsed ticks of running performance counter to
  time value in unit of nanoseconds.

  @param  Ticks     The number of elapsed ticks of running performance counter.

  @return The elapsed time in nanoseconds.

**/
UINT64
EFIAPI
GetTimeInNanoSecond (
  IN      UINT64                     Ticks
  )
{
  UINT64  NanoSeconds;
  UINT32  Remainder;

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x32Remainder (Ticks, ACPI_TIMER_FREQUENCY, &Remainder), 1000000000u);

  //
  // Frequency < 0x100000000, so Remainder < 0x100000000, then (Remainder * 1,000,000,000)
  // will not overflow 64-bit.
  //
  NanoSeconds += DivU64x32 (MultU64x32 ((UINT64) Remainder, 1000000000u), ACPI_TIMER_FREQUENCY);

  return NanoSeconds;
}
