/** @file
  Implementation of loading microcode on processors.

  Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MpLib.h"

/**
  Get microcode update signature of currently loaded microcode update.

  @return  Microcode signature.
**/
UINT32
GetCurrentMicrocodeSignature (
  VOID
  )
{
  MSR_IA32_BIOS_SIGN_ID_REGISTER   BiosSignIdMsr;

  AsmWriteMsr64 (MSR_IA32_BIOS_SIGN_ID, 0);
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, NULL);
  BiosSignIdMsr.Uint64 = AsmReadMsr64 (MSR_IA32_BIOS_SIGN_ID);
  return BiosSignIdMsr.Bits.MicrocodeUpdateSignature;
}

/**
  Detect whether specified processor can find matching microcode patch and load it.

  @param[in]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
MicrocodeDetect (
  IN CPU_MP_DATA             *CpuMpData
  )
{
  UINT32                                  ExtendedTableLength;
  UINT32                                  ExtendedTableCount;
  CPU_MICROCODE_EXTENDED_TABLE            *ExtendedTable;
  CPU_MICROCODE_EXTENDED_TABLE_HEADER     *ExtendedTableHeader;
  CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint;
  UINTN                                   MicrocodeEnd;
  UINTN                                   Index;
  UINT8                                   PlatformId;
  CPUID_VERSION_INFO_EAX                  Eax;
  UINT32                                  CurrentRevision;
  UINT32                                  LatestRevision;
  UINTN                                   TotalSize;
  UINT32                                  CheckSum32;
  BOOLEAN                                 CorrectMicrocode;
  VOID                                    *MicrocodeData;
  MSR_IA32_PLATFORM_ID_REGISTER           PlatformIdMsr;

  if (CpuMpData->MicrocodePatchRegionSize == 0) {
    //
    // There is no microcode patches
    //
    return;
  }

  CurrentRevision = GetCurrentMicrocodeSignature ();
  if (CurrentRevision != 0) {
    //
    // Skip loading microcode if it has been loaded successfully
    //
    return;
  }

  ExtendedTableLength = 0;
  //
  // Here data of CPUID leafs have not been collected into context buffer, so
  // GetProcessorCpuid() cannot be used here to retrieve sCPUID data.
  //
  AsmCpuid (CPUID_VERSION_INFO, &Eax.Uint32, NULL, NULL, NULL);

  //
  // The index of platform information resides in bits 50:52 of MSR IA32_PLATFORM_ID
  //
  PlatformIdMsr.Uint64 = AsmReadMsr64 (MSR_IA32_PLATFORM_ID);
  PlatformId = (UINT8) PlatformIdMsr.Bits.PlatformId;

  LatestRevision = 0;
  MicrocodeData  = NULL;
  MicrocodeEnd = (UINTN) (CpuMpData->MicrocodePatchAddress + CpuMpData->MicrocodePatchRegionSize);
  MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) (UINTN) CpuMpData->MicrocodePatchAddress;
  do {
    //
    // Check if the microcode is for the Cpu and the version is newer
    // and the update can be processed on the platform
    //
    CorrectMicrocode = FALSE;
    if (MicrocodeEntryPoint->HeaderVersion == 0x1) {
      //
      // It is the microcode header. It is not the padding data between microcode patches
      // because the padding data should not include 0x00000001 and it should be the repeated
      // byte format (like 0xXYXYXYXY....).
      //
      if (MicrocodeEntryPoint->ProcessorSignature.Uint32 == Eax.Uint32 &&
          MicrocodeEntryPoint->UpdateRevision > LatestRevision &&
          (MicrocodeEntryPoint->ProcessorFlags & (1 << PlatformId))
          ) {
        if (MicrocodeEntryPoint->DataSize == 0) {
          CheckSum32 = CalculateSum32 ((UINT32 *) MicrocodeEntryPoint, 2048);
        } else {
          CheckSum32 = CalculateSum32 (
                         (UINT32 *) MicrocodeEntryPoint,
                         MicrocodeEntryPoint->DataSize + sizeof (CPU_MICROCODE_HEADER)
                         );
        }
        if (CheckSum32 == 0) {
          CorrectMicrocode = TRUE;
        }
      } else if ((MicrocodeEntryPoint->DataSize != 0) &&
                 (MicrocodeEntryPoint->UpdateRevision > LatestRevision)) {
        ExtendedTableLength = MicrocodeEntryPoint->TotalSize - (MicrocodeEntryPoint->DataSize +
                                sizeof (CPU_MICROCODE_HEADER));
        if (ExtendedTableLength != 0) {
          //
          // Extended Table exist, check if the CPU in support list
          //
          ExtendedTableHeader = (CPU_MICROCODE_EXTENDED_TABLE_HEADER *) ((UINT8 *) (MicrocodeEntryPoint)
                                  + MicrocodeEntryPoint->DataSize + sizeof (CPU_MICROCODE_HEADER));
          //
          // Calculate Extended Checksum
          //
          if ((ExtendedTableLength % 4) == 0) {
            CheckSum32 = CalculateSum32 ((UINT32 *) ExtendedTableHeader, ExtendedTableLength);
            if (CheckSum32 == 0) {
              //
              // Checksum correct
              //
              ExtendedTableCount = ExtendedTableHeader->ExtendedSignatureCount;
              ExtendedTable      = (CPU_MICROCODE_EXTENDED_TABLE *) (ExtendedTableHeader + 1);
              for (Index = 0; Index < ExtendedTableCount; Index ++) {
                CheckSum32 = CalculateSum32 ((UINT32 *) ExtendedTable, sizeof(CPU_MICROCODE_EXTENDED_TABLE));
                if (CheckSum32 == 0) {
                  //
                  // Verify Header
                  //
                  if ((ExtendedTable->ProcessorSignature.Uint32 == Eax.Uint32) &&
                      (ExtendedTable->ProcessorFlag & (1 << PlatformId)) ) {
                    //
                    // Find one
                    //
                    CorrectMicrocode = TRUE;
                    break;
                  }
                }
                ExtendedTable ++;
              }
            }
          }
        }
      }
    } else {
      //
      // It is the padding data between the microcode patches for microcode patches alignment.
      // Because the microcode patch is the multiple of 1-KByte, the padding data should not
      // exist if the microcode patch alignment value is not larger than 1-KByte. So, the microcode
      // alignment value should be larger than 1-KByte. We could skip SIZE_1KB padding data to
      // find the next possible microcode patch header.
      //
      MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) (((UINTN) MicrocodeEntryPoint) + SIZE_1KB);
      continue;
    }
    //
    // Get the next patch.
    //
    if (MicrocodeEntryPoint->DataSize == 0) {
      TotalSize = 2048;
    } else {
      TotalSize = MicrocodeEntryPoint->TotalSize;
    }

    if (CorrectMicrocode) {
      LatestRevision = MicrocodeEntryPoint->UpdateRevision;
      MicrocodeData = (VOID *) ((UINTN) MicrocodeEntryPoint + sizeof (CPU_MICROCODE_HEADER));
    }

    MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) (((UINTN) MicrocodeEntryPoint) + TotalSize);
  } while (((UINTN) MicrocodeEntryPoint < MicrocodeEnd));

  if (LatestRevision > CurrentRevision) {
    //
    // BIOS only authenticate updates that contain a numerically larger revision
    // than the currently loaded revision, where Current Signature < New Update
    // Revision. A processor with no loaded update is considered to have a
    // revision equal to zero.
    //
    ASSERT (MicrocodeData != NULL);
    AsmWriteMsr64 (
        MSR_IA32_BIOS_UPDT_TRIG,
        (UINT64) (UINTN) MicrocodeData
        );
    //
    // Get and check new microcode signature
    //
    CurrentRevision = GetCurrentMicrocodeSignature ();
    if (CurrentRevision != LatestRevision) {
      AcquireSpinLock(&CpuMpData->MpLock);
      DEBUG ((EFI_D_ERROR, "Updated microcode signature [0x%08x] does not match \
                loaded microcode signature [0x%08x]\n", CurrentRevision, LatestRevision));
      ReleaseSpinLock(&CpuMpData->MpLock);
    }
  }
}
