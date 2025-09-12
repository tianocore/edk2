/** @file
  Shared TSC Timer implementation for ACPI Timer Library

  Copyright (C) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include "AcpiTimerLib.h"

#include "TscTimerLib.h"

//
// Hypervisor CPUID leaf constants
//
#define CPUID_HYPERVISOR_BASE_LEAF  0x40000000
#define CPUID_HYPERVISOR_MAX_LEAF   0x40010000
#define CPUID_HYPERVISOR_STEP       0x100

//
// Offset in the hypervisor CPUID leaf space where TSC frequency is located
//
#define CPUID_TSC_LEAF_OFFSET_KVM  0x10
#define CPUID_TSC_LEAF_OFFSET_XEN  0x3

//
// Timer sources
//
#define OVMF_TIMER_SOURCE_TSC   1
#define OVMF_TIMER_SOURCE_ACPI  2

//
// Timer source selector and TSC frequency
// mTimerSource: 0 = uninitialized, OVMF_TIMER_SOURCE_TSC = TSC, OVMF_TIMER_SOURCE_ACPI = ACPI
// mTscFrequency: TSC frequency in Hz (only valid when mTimerSource == OVMF_TIMER_SOURCE_TSC)
//
STATIC UINT32  mTimerSource  = 0;
STATIC UINT64  mTscFrequency = 0;

/**
  Detect and initialize the timer source.

  This function detects whether TSC or ACPI timer should be used as the timer
  source, and initializes the timer frequency accordingly.
**/
VOID
InternalInitializeTimerSource (
  VOID
  )
{
  UINT32  Offset;
  UINT32  Eax;
  CHAR8   Sig[12];
  UINT64  TscFrequencyKhz;

  if (mTimerSource != 0) {
    return;
  }

  //
  // Scan hypervisor CPUID leaves
  //
  for (Offset = CPUID_HYPERVISOR_BASE_LEAF; Offset < CPUID_HYPERVISOR_MAX_LEAF; Offset += CPUID_HYPERVISOR_STEP) {
    AsmCpuid (Offset, &Eax, (UINT32 *)&Sig[0], (UINT32 *)&Sig[4], (UINT32 *)&Sig[8]);

    if (Eax == 0) {
      break;
    }

    //
    // Check for KVM hypervisor
    //
    if (CompareMem (Sig, "KVMKVMKVM\0\0\0", 12) == 0) {
      if (Eax >= Offset + CPUID_TSC_LEAF_OFFSET_KVM) {
        AsmCpuid (Offset + CPUID_TSC_LEAF_OFFSET_KVM, (UINT32 *)&TscFrequencyKhz, NULL, NULL, NULL);
        if (TscFrequencyKhz != 0) {
          mTimerSource  = OVMF_TIMER_SOURCE_TSC;
          mTscFrequency = TscFrequencyKhz * 1000;   // Convert from kHz to Hz
          DEBUG ((DEBUG_INFO, "KVM detected; Using TSC timer with frequency %lu Hz\n", mTscFrequency));
          return;
        }
      }
    }
    //
    // Check for Xen hypervisor
    //
    else if (CompareMem (Sig, "XenVMMXenVMM", 12) == 0) {
      if (Eax >= Offset + CPUID_TSC_LEAF_OFFSET_XEN) {
        AsmCpuid (Offset + CPUID_TSC_LEAF_OFFSET_XEN, NULL, NULL, (UINT32 *)&TscFrequencyKhz, NULL);
        if (TscFrequencyKhz != 0) {
          mTimerSource  = OVMF_TIMER_SOURCE_TSC;
          mTscFrequency = TscFrequencyKhz * 1000;   // Convert from kHz to Hz
          DEBUG ((DEBUG_INFO, "Xen detected; Using TSC timer with frequency %lu Hz\n", mTscFrequency));
          return;
        }
      }
    }
  }

  //
  // Fall back to ACPI timer
  //
  mTimerSource = OVMF_TIMER_SOURCE_ACPI;
  DEBUG ((DEBUG_INFO, "Using ACPI timer\n"));
}

/**
  Implementation of GetPerformanceCounter which returns either the TSC value or
  the ACPI tick count, depending on the timer source.

  @return The current value of the free running performance counter.

**/
UINT64
TscGetPerformanceCounter (
  VOID
  )
{
  InternalInitializeTimerSource ();

  return mTimerSource == OVMF_TIMER_SOURCE_ACPI ? AcpiGetPerformanceCounter () : AsmReadTsc ();
}

/**
  Implementation of GetPerformanceCounterProperties which returns either the
  properties of the TSC or the ACPI timer, depending on the timer source.

  @param  StartValue  The value the performance counter starts with when it
                      rolls over.
  @param  EndValue    The value that the performance counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
TscGetPerformanceCounterProperties (
  OUT      UINT64  *StartValue   OPTIONAL,
  OUT      UINT64  *EndValue     OPTIONAL
  )
{
  InternalInitializeTimerSource ();

  if (mTimerSource == OVMF_TIMER_SOURCE_ACPI) {
    return AcpiGetPerformanceCounterProperties (StartValue, EndValue);
  }

  if (StartValue != NULL) {
    *StartValue = 0;
  }

  if (EndValue != NULL) {
    *EndValue = 0xffffffffffffffffULL;
  }

  return mTscFrequency;
}
