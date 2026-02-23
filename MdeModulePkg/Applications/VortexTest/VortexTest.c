/** @file
  VortexTest UEFI Application - Tests the VortexOracle driver

  Locates the VortexOracle protocol and retrieves the computed
  resonant pi value.

  Copyright (c) 2026, Americo Simoes. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>

//
// This should match the protocol GUID from VortexOracle.inf
// Replace with your actual GUID from VortexOracle.inf
//
#define VORTEX_ORACLE_PROTOCOL_GUID \
  { 0x7ce398d8, 0xb276, 0x4b5c, { 0x9e, 0x02, 0x14, 0xe7, 0x47, 0x98, 0x6c, 0xe8 } }

//
// Protocol structure (must match the driver)
//
typedef struct _VORTEX_ORACLE_PROTOCOL {
  UINT64 (EFIAPI *GetResonantPiX1000000)(VOID);
  UINT64 (EFIAPI *GetChebyshevRatioX1000000)(VOID);
  UINT64 (EFIAPI *GetMagnificationX1000000)(VOID);
  UINT64 (EFIAPI *GetLcm33)(VOID);
} VORTEX_ORACLE_PROTOCOL;

/**
  Entry point for the test application.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @return EFI_SUCCESS     The application exited successfully.
**/
EFI_STATUS
EFIAPI
ShellAppMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS              Status;
  VORTEX_ORACLE_PROTOCOL  *VortexOracle;
  UINT64                  ResonantPiX1000000;
  UINT64                  ChebyshevRatioX1000000;
  UINT64                  MagnificationX1000000;
  UINT64                  Lcm33;
  UINT32                  PiInteger;
  UINT32                  PiFraction;
  CHAR16                  OutputString[128];
  EFI_GUID                VortexOracleGuid = VORTEX_ORACLE_PROTOCOL_GUID;

  Print (L"VortexTest: Looking for VortexOracle protocol...\n");

  // Locate the VortexOracle protocol
  Status = gBS->LocateProtocol (
                  &VortexOracleGuid,
                  NULL,
                  (VOID **)&VortexOracle
                  );

  if (EFI_ERROR (Status)) {
    Print (L"ERROR: Could not find VortexOracle protocol! Status = %r\n", Status);
    return Status;
  }

  Print (L"VortexTest: VortexOracle protocol found!\n\n");

  // Call the protocol functions
  ResonantPiX1000000     = VortexOracle->GetResonantPiX1000000 ();
  ChebyshevRatioX1000000 = VortexOracle->GetChebyshevRatioX1000000 ();
  MagnificationX1000000  = VortexOracle->GetMagnificationX1000000 ();
  Lcm33                  = VortexOracle->GetLcm33 ();

  // Display results
  Print (L"VortexOracle Driver Test Results:\n");
  Print (L"================================\n");
  Print (L"LCM(1..33) = %llu\n", Lcm33);
  Print (L"Chebyshev Ratio * 1,000,000 = %llu\n", ChebyshevRatioX1000000);
  Print (L"Magnification * 1,000,000    = %llu\n", MagnificationX1000000);
  Print (L"Resonant Pi * 1,000,000      = %llu\n", ResonantPiX1000000);

  // Convert resonant pi to decimal format using safe math functions
  PiInteger  = (UINT32)DivU64x32 (ResonantPiX1000000, 1000000);
  PiFraction = (UINT32)ModU64x32 (ResonantPiX1000000, 1000000);

  UnicodeSPrint (
    OutputString,
    sizeof (OutputString),
    L"Resonant π = %d.%06d",
    PiInteger,
    PiFraction
    );
  Print (L"\n%s\n", OutputString);

  // Check if it's close to actual π (3.141592...)
  if ((PiInteger == 3) && (PiFraction > 141000) && (PiFraction < 142000)) {
    Print (L"\nSUCCESS: Resonant π is close to actual π!\n");
  } else {
    Print (L"\nNOTE: Resonant π differs from actual π. Expected ~3.141592\n");
  }

  Print (L"\nVortexTest completed.\n");

  return EFI_SUCCESS;
}
