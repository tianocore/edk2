/** @file
  UEFI RNG (Random Number Generator) Protocol test application.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/        

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Protocol/Rng.h>

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS         Status;
  EFI_RNG_PROTOCOL   *Rng;
  UINTN              RngAlgListSize;
  EFI_RNG_ALGORITHM  RngAlgList[10];
  EFI_RNG_ALGORITHM  *PtrRngAlg;
  UINTN              RngAlgCount;
  UINT8              *Rand;
  UINTN              RandSize;
  UINTN              Index;
  UINTN              Index2;

  Status    = EFI_SUCCESS;
  PtrRngAlg = NULL;
  Rand      = NULL;
    
  Print (L"UEFI RNG Protocol Testing :\n");
  Print (L"----------------------------\n");

  //-----------------------------------------
  // Basic UEFI RNG Protocol Test
  //-----------------------------------------
  Print (L" -- Locate UEFI RNG Protocol : ");
  Status = gBS->LocateProtocol (&gEfiRngProtocolGuid, NULL, (VOID **)&Rng);
  if (EFI_ERROR (Status)) {
    Print (L"[Fail - Status = %r]\n", Status);
    goto Exit;
  } else {
    Print (L"[Pass]\n");
  }

  //-----------------------------------------
  // Rng->GetInfo() interface test.
  //-----------------------------------------
  
  Print (L" -- Call RNG->GetInfo() interface : ");
  RngAlgListSize = 0;
  Status = Rng->GetInfo (Rng, &RngAlgListSize, NULL);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    Print (L"[Fail - Status = %r]\n", Status);
  }
  //
  // Print out the supported RNG algorithm GUIDs
  //
  RngAlgCount = RngAlgListSize / sizeof (EFI_RNG_ALGORITHM);
  Print (L"\n     >> Supported RNG Algorithm (Count = %d) : ", RngAlgCount);
  Status = Rng->GetInfo (Rng, &RngAlgListSize, RngAlgList);
  for (Index = 0; Index < RngAlgCount; Index++) {
    PtrRngAlg = (EFI_RNG_ALGORITHM *)(&RngAlgList[Index]);
    Print (L"\n          %d) ", Index);
    Print (L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", PtrRngAlg->Data1,
             PtrRngAlg->Data2, PtrRngAlg->Data3, PtrRngAlg->Data4[0], PtrRngAlg->Data4[1],
             PtrRngAlg->Data4[2], PtrRngAlg->Data4[3], PtrRngAlg->Data4[4],
             PtrRngAlg->Data4[5], PtrRngAlg->Data4[6], PtrRngAlg->Data4[7]);    
  }

  //-----------------------------------------
  // Rng->GetRNG() interface test.
  //-----------------------------------------
  Print (L"\n -- Call RNG->GetRNG() interface : ");

  //
  // Allocate one buffer to store random data.
  //
  RandSize = 32;
  Rand     = AllocatePool (RandSize);
  if (Rand == NULL) {
    goto Exit;
  }
  
  //
  // RNG with default algorithm
  //
  Print (L"\n     >> RNG with default algorithm : ");
  Status = Rng->GetRNG (Rng, NULL, RandSize, Rand);
  if (EFI_ERROR (Status)) {
    Print (L"[Fail - Status = %r]", Status);
  } else {
    Print (L"[Pass]");
  }
  
  //
  // RNG with SP800-90-HMAC-256
  //
  Print (L"\n     >> RNG with SP800-90-HMAC-256 : ");
  Status = Rng->GetRNG (Rng, &gEfiRngAlgorithmSp80090Hmac256Guid, RandSize, Rand);
  if (EFI_ERROR (Status)) {
    Print (L"[Fail - Status = %r]", Status);
  } else {
    Print (L"[Pass]");
  }

  //
  // RNG with SP800-90-HASH-256
  //
  Print (L"\n     >> RNG with SP800-90-Hash-256 : ");
  Status = Rng->GetRNG (Rng, &gEfiRngAlgorithmSp80090Hash256Guid, RandSize, Rand);
  if (EFI_ERROR (Status)) {
    Print (L"[Fail - Status = %r]", Status);
  } else {
    Print (L"[Pass]");
  }

  //
  // RNG with SP800-90-CTR-256
  //
  Print (L"\n     >> RNG with SP800-90-CTR-256 : ");
  Status = Rng->GetRNG (Rng, &gEfiRngAlgorithmSp80090Ctr256Guid, RandSize, Rand);
  if (EFI_ERROR (Status)) {
    Print (L"[Fail - Status = %r]", Status);
  } else {
    Print (L"[Pass]");
  }

  //
  // RNG with X9.31-3DES
  //
  Print (L"\n     >> RNG with X9.31-3DES : ");
  Status = Rng->GetRNG (Rng, &gEfiRngAlgorithmX9313DesGuid, RandSize, Rand);
  if (EFI_ERROR (Status)) {
    Print (L"[Fail - Status = %r]", Status);
  } else {
    Print (L"[Pass]");
  }

  //
  // RNG with X9.31-AES
  //
  Print (L"\n     >> RNG with X9.31-AES : ");
  Status = Rng->GetRNG (Rng, &gEfiRngAlgorithmX931AesGuid, RandSize, Rand);
  if (EFI_ERROR (Status)) {
    Print (L"[Fail - Status = %r]", Status);
  } else {
    Print (L"[Pass]");
  }

  //
  // RNG with RAW Entropy
  //
  Print (L"\n     >> RNG with RAW Entropy : ");
  Status = Rng->GetRNG (Rng, &gEfiRngAlgorithmRaw, RandSize, Rand);
  if (EFI_ERROR (Status)) {
    Print (L"[Fail - Status = %r]", Status);
  } else {
    Print (L"[Pass]");
  }

  //-----------------------------------------
  // Random Number Generator test.
  //-----------------------------------------
  Print (L"\n -- Random Number Generation Test with default RNG Algorithm (20 Rounds): ");

  RandSize = 1;
  for (Index = 0; Index < 20; Index++) {
    Status = Rng->GetRNG (Rng, NULL, RandSize, Rand);
    if (EFI_ERROR (Status)) {
      Print (L"[Fail - Status = %r]", Status);
      break;
    } else {
      Print (L"\n          %02d) - ", Index + 1);
      for (Index2 = 0; Index2 < RandSize; Index2++) {
        Print (L"%02x", Rand[Index2]);
      }
    }

    RandSize +=1;
  }

  //-----------------------------------------
  // Random Number Generator test.
  //-----------------------------------------
  Print (L"\n -- RAW Entropy Generation Test (20 Rounds) : ");

  RandSize = 32;
  for (Index = 0; Index < 20; Index++) {
    Status = Rng->GetRNG (Rng, &gEfiRngAlgorithmRaw, RandSize, Rand);
    if (EFI_ERROR (Status)) {
      Print (L"[Fail - Status = %r]", Status);
      break;
    } else {
      Print (L"\n          %02d) - ", Index + 1);
      for (Index2 = 0; Index2 < RandSize; Index2++) {
        Print (L"%02x", Rand[Index2]);
      }
    }
  }

  Print (L"\n -- Exit UEFI RNG Protocol Test (Status = %r).\n", Status);
  
Exit:
  if (Rand != NULL) {
    FreePool (Rand);
  }
  return Status;
}
