/** @file
   VortexTest Application - Verifies CTT Temporal Resonance

   Copyright (c) 2026, Americo Simoes. All rights reserved.
   SPDX-License-Identifier: BSD-2-Clause-Patent
 **/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Protocol/VortexOracle.h>

/**
   Entry point for the test application.

   @param[in] ImageHandle  The firmware allocated handle for the EFI image.
   @param[in] SystemTable  A pointer to the EFI System Table.

   @return EFI_SUCCESS     The application exited successfully.
 **/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;
  VORTEX_ORACLE_PROTOCOL *Vortex;
  UINT64 ResonantPi;
  UINT32 PiInteger;
  UINT32 PiFraction;

  Print (L"VortexTest: Looking for VortexOracle protocol...\n");

  // Locate the protocol using the GUID defined in the DEC file
  Status = gBS->LocateProtocol (&gVortexOracleProtocolGuid, NULL, (VOID **)&Vortex);
  if (EFI_ERROR (Status))
  {
    Print (L"ERROR: Could not find VortexOracle protocol! Status = %r\n", Status);
    return Status;
  }

  Print (L"VortexTest: VortexOracle protocol found!\n\n");

  // Call the protocol function
  Status = Vortex->GetResonance (Vortex, &ResonantPi);
  if (EFI_ERROR (Status))
  {
    Print (L"ERROR: Failed to get resonance data.\n");
    return Status;
  }

  // Display results using fixed-point math
  PiInteger  = (UINT32)(ResonantPi / 1000000);
  PiFraction = (UINT32)(ResonantPi % 1000000);

  Print (L"VortexOracle Driver Test Results:\n");
  Print (L"================================\n");
  Print (L"Resonant Pi (Fixed-Point): %llu\n", ResonantPi);
  Print (L"Resonant π formatted: %d.%06d\n", PiInteger, PiFraction);

  // Validation logic
  if (PiInteger == 3 && PiFraction > 141000)
  {
    Print (L"\nSUCCESS: Resonant π is within expected convergence range!\n");
  }

  Print (L"\nVortexTest completed.\n");

  return EFI_SUCCESS;
}
