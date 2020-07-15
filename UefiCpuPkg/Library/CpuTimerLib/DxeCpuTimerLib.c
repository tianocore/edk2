/** @file
  CPUID Leaf 0x15 for Core Crystal Clock frequency instance of Timer Library.

  Copyright (c) 2019 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>

extern GUID mCpuCrystalFrequencyHobGuid;

/**
  CPUID Leaf 0x15 for Core Crystal Clock Frequency.

  The TSC counting frequency is determined by using CPUID leaf 0x15. Frequency in MHz = Core XTAL frequency * EBX/EAX.
  In newer flavors of the CPU, core xtal frequency is returned in ECX or 0 if not supported.
  @return The number of TSC counts per second.

**/
UINT64
CpuidCoreClockCalculateTscFrequency (
  VOID
  );

//
// Cached CPU Crystal counter frequency
//
UINT64  mCpuCrystalCounterFrequency = 0;


/**
  Internal function to retrieves the 64-bit frequency in Hz.

  Internal function to retrieves the 64-bit frequency in Hz.

  @return The frequency in Hz.

**/
UINT64
InternalGetPerformanceCounterFrequency (
  VOID
  )
{
  return mCpuCrystalCounterFrequency;
}

/**
  The constructor function is to initialize CpuCrystalCounterFrequency.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeCpuTimerLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HOB_GUID_TYPE   *GuidHob;

  //
  // Initialize CpuCrystalCounterFrequency
  //
  GuidHob = GetFirstGuidHob (&mCpuCrystalFrequencyHobGuid);
  if (GuidHob != NULL) {
    mCpuCrystalCounterFrequency = *(UINT64*)GET_GUID_HOB_DATA (GuidHob);
  } else {
    mCpuCrystalCounterFrequency = CpuidCoreClockCalculateTscFrequency ();
  }

  if (mCpuCrystalCounterFrequency == 0) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

