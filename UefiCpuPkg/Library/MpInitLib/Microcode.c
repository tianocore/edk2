/** @file
  Implementation of loading microcode on processors.

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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

  Microcode Payload as the following format:
  +----------------------------------------+------------------+
  |          CPU_MICROCODE_HEADER          |                  |
  +----------------------------------------+  CheckSum Part1  |
  |            Microcode Binary            |                  |
  +----------------------------------------+------------------+
  |  CPU_MICROCODE_EXTENDED_TABLE_HEADER   |                  |
  +----------------------------------------+  CheckSum Part2  |
  |      CPU_MICROCODE_EXTENDED_TABLE      |                  |
  |                   ...                  |                  |
  +----------------------------------------+------------------+

  There may by multiple CPU_MICROCODE_EXTENDED_TABLE in this format.
  The count of CPU_MICROCODE_EXTENDED_TABLE is indicated by ExtendedSignatureCount
  of CPU_MICROCODE_EXTENDED_TABLE_HEADER structure.

  When we are trying to verify the CheckSum32 with extended table.
  We should use the fields of exnteded table to replace the corresponding
  fields in CPU_MICROCODE_HEADER structure, and recalculate the
  CheckSum32 with CPU_MICROCODE_HEADER + Microcode Binary. We named
  it as CheckSum Part3.

  The CheckSum Part2 is used to verify the CPU_MICROCODE_EXTENDED_TABLE_HEADER
  and CPU_MICROCODE_EXTENDED_TABLE parts. We should make sure CheckSum Part2
  is correct before we are going to verify each CPU_MICROCODE_EXTENDED_TABLE.

  Only ProcessorSignature, ProcessorFlag and CheckSum are different between
  CheckSum Part1 and CheckSum Part3. To avoid multiple computing CheckSum Part3.
  Save an in-complete CheckSum32 from CheckSum Part1 for common parts.
  When we are going to calculate CheckSum32, just should use the corresponding part
  of the ProcessorSignature, ProcessorFlag and CheckSum with in-complete CheckSum32.

  Notes: CheckSum32 is not a strong verification.
         It does not guarantee that the data has not been modified.
         CPU has its own mechanism to verify Microcode Binary part.

  @param[in]  CpuMpData    The pointer to CPU MP Data structure.
  @param[in]  IsBspCallIn  Indicate whether the caller is BSP or not.
**/
VOID
MicrocodeDetect (
  IN CPU_MP_DATA             *CpuMpData,
  IN BOOLEAN                 IsBspCallIn
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
  UINT32                                  InCompleteCheckSum32;
  BOOLEAN                                 CorrectMicrocode;
  VOID                                    *MicrocodeData;
  MSR_IA32_PLATFORM_ID_REGISTER           PlatformIdMsr;
  UINT32                                  ProcessorFlags;
  UINT32                                  ThreadId;

  //
  // set ProcessorFlags to suppress incorrect compiler/analyzer warnings
  //
  ProcessorFlags = 0;

  if (CpuMpData->MicrocodePatchRegionSize == 0) {
    //
    // There is no microcode patches
    //
    return;
  }

  CurrentRevision = GetCurrentMicrocodeSignature ();
  if (CurrentRevision != 0 && !IsBspCallIn) {
    //
    // Skip loading microcode if it has been loaded successfully
    //
    return;
  }

  GetProcessorLocationByApicId (GetInitialApicId (), NULL, NULL, &ThreadId);
  if (ThreadId != 0) {
    //
    // Skip loading microcode if it is not the first thread in one core.
    //
    return;
  }

  ExtendedTableLength = 0;
  //
  // Here data of CPUID leafs have not been collected into context buffer, so
  // GetProcessorCpuid() cannot be used here to retrieve CPUID data.
  //
  AsmCpuid (CPUID_VERSION_INFO, &Eax.Uint32, NULL, NULL, NULL);

  //
  // The index of platform information resides in bits 50:52 of MSR IA32_PLATFORM_ID
  //
  PlatformIdMsr.Uint64 = AsmReadMsr64 (MSR_IA32_PLATFORM_ID);
  PlatformId = (UINT8) PlatformIdMsr.Bits.PlatformId;

  //
  // Check whether AP has same processor with BSP.
  // If yes, direct use microcode info saved by BSP.
  //
  if (!IsBspCallIn) {
    if ((CpuMpData->ProcessorSignature == Eax.Uint32) &&
        (CpuMpData->ProcessorFlags & (1 << PlatformId)) != 0) {
        MicrocodeData = (VOID *)(UINTN) CpuMpData->MicrocodeDataAddress;
        LatestRevision = CpuMpData->MicrocodeRevision;
        goto Done;
    }
  }

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

    if (MicrocodeEntryPoint->DataSize == 0) {
      TotalSize = sizeof (CPU_MICROCODE_HEADER) + 2000;
    } else {
      TotalSize = sizeof (CPU_MICROCODE_HEADER) + MicrocodeEntryPoint->DataSize;
    }

    ///
    /// Check overflow and whether TotalSize is aligned with 4 bytes.
    ///
    if ( ((UINTN)MicrocodeEntryPoint + TotalSize) > MicrocodeEnd ||
         (TotalSize & 0x3) != 0
       ) {
      MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) (((UINTN) MicrocodeEntryPoint) + SIZE_1KB);
      continue;
    }

    //
    // Save an in-complete CheckSum32 from CheckSum Part1 for common parts.
    //
    InCompleteCheckSum32 = CalculateSum32 (
                             (UINT32 *) MicrocodeEntryPoint,
                             TotalSize
                             );
    InCompleteCheckSum32 -= MicrocodeEntryPoint->ProcessorSignature.Uint32;
    InCompleteCheckSum32 -= MicrocodeEntryPoint->ProcessorFlags;
    InCompleteCheckSum32 -= MicrocodeEntryPoint->Checksum;

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
        //
        // Calculate CheckSum Part1.
        //
        CheckSum32 = InCompleteCheckSum32;
        CheckSum32 += MicrocodeEntryPoint->ProcessorSignature.Uint32;
        CheckSum32 += MicrocodeEntryPoint->ProcessorFlags;
        CheckSum32 += MicrocodeEntryPoint->Checksum;
        if (CheckSum32 == 0) {
          CorrectMicrocode = TRUE;
          ProcessorFlags = MicrocodeEntryPoint->ProcessorFlags;
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
            //
            // Calculate CheckSum Part2.
            //
            CheckSum32 = CalculateSum32 ((UINT32 *) ExtendedTableHeader, ExtendedTableLength);
            if (CheckSum32 == 0) {
              //
              // Checksum correct
              //
              ExtendedTableCount = ExtendedTableHeader->ExtendedSignatureCount;
              ExtendedTable      = (CPU_MICROCODE_EXTENDED_TABLE *) (ExtendedTableHeader + 1);
              for (Index = 0; Index < ExtendedTableCount; Index ++) {
                //
                // Calculate CheckSum Part3.
                //
                CheckSum32 = InCompleteCheckSum32;
                CheckSum32 += ExtendedTable->ProcessorSignature.Uint32;
                CheckSum32 += ExtendedTable->ProcessorFlag;
                CheckSum32 += ExtendedTable->Checksum;
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
                    ProcessorFlags = ExtendedTable->ProcessorFlag;
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

Done:
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

  if (IsBspCallIn && (LatestRevision != 0)) {
    //
    // Save BSP processor info and microcode info for later AP use.
    //
    CpuMpData->ProcessorSignature   = Eax.Uint32;
    CpuMpData->ProcessorFlags       = ProcessorFlags;
    CpuMpData->MicrocodeDataAddress = (UINTN) MicrocodeData;
    CpuMpData->MicrocodeRevision    = LatestRevision;
    DEBUG ((DEBUG_INFO, "BSP Microcode:: signature [0x%08x], ProcessorFlags [0x%08x], \
       MicroData [0x%08x], Revision [0x%08x]\n", Eax.Uint32, ProcessorFlags, (UINTN) MicrocodeData, LatestRevision));
  }
}
