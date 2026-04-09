/** @file
  Implementation of MicrocodeLib.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Register/Intel/Cpuid.h>
#include <Register/Intel/ArchitecturalMsr.h>
#include <Register/Intel/Microcode.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Ppi/ShadowMicrocode.h>

/**
  Get microcode update signature of currently loaded microcode update.

  @return  Microcode signature.
**/
UINT32
EFIAPI
GetProcessorMicrocodeSignature (
  VOID
  )
{
  MSR_IA32_BIOS_SIGN_ID_REGISTER  BiosSignIdMsr;

  AsmWriteMsr64 (MSR_IA32_BIOS_SIGN_ID, 0);
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, NULL);
  BiosSignIdMsr.Uint64 = AsmReadMsr64 (MSR_IA32_BIOS_SIGN_ID);
  return BiosSignIdMsr.Bits.MicrocodeUpdateSignature;
}

/**
  Get the processor signature and platform ID for current processor.

  @param MicrocodeCpuId  Return the processor signature and platform ID.
**/
VOID
EFIAPI
GetProcessorMicrocodeCpuId (
  EDKII_PEI_MICROCODE_CPU_ID  *MicrocodeCpuId
  )
{
  MSR_IA32_PLATFORM_ID_REGISTER  PlatformIdMsr;

  ASSERT (MicrocodeCpuId != NULL);

  PlatformIdMsr.Uint64       = AsmReadMsr64 (MSR_IA32_PLATFORM_ID);
  MicrocodeCpuId->PlatformId = (UINT8)PlatformIdMsr.Bits.PlatformId;
  AsmCpuid (CPUID_VERSION_INFO, &MicrocodeCpuId->ProcessorSignature, NULL, NULL, NULL);
}

/**
  Return the total size of the microcode entry.

  Logic follows pseudo code in SDM as below:

     N = 512
     If (Update.DataSize != 00000000H)
       N = Update.TotalSize / 4

  If Microcode is NULL, then ASSERT.

  @param Microcode  Pointer to the microcode entry.

  @return The microcode total size.
**/
UINT32
EFIAPI
GetMicrocodeLength (
  IN CPU_MICROCODE_HEADER  *Microcode
  )
{
  UINT32  TotalSize;

  ASSERT (Microcode != NULL);

  TotalSize = 2048;
  if (Microcode->DataSize != 0) {
    TotalSize = Microcode->TotalSize;
  }

  return TotalSize;
}

/**
  Load the microcode to the processor.

  If Microcode is NULL, then ASSERT.

  @param Microcode  Pointer to the microcode entry.
**/
VOID
EFIAPI
LoadMicrocode (
  IN CPU_MICROCODE_HEADER  *Microcode
  )
{
  ASSERT (Microcode != NULL);

  AsmWriteMsr64 (MSR_IA32_BIOS_UPDT_TRIG, (UINT64)(UINTN)(Microcode + 1));
}

/**
  Determine if a microcode patch matchs the specific processor signature and flag.

  @param[in]  ProcessorSignature    The processor signature field value in a
                                    microcode patch.
  @param[in]  ProcessorFlags        The processor flags field value in a
                                    microcode patch.
  @param[in]  MicrocodeCpuId        A pointer to an array of EDKII_PEI_MICROCODE_CPU_ID
                                    structures.
  @param[in]  MicrocodeCpuIdCount   Number of elements in MicrocodeCpuId array.

  @retval TRUE     The specified microcode patch matches to one of the MicrocodeCpuId.
  @retval FALSE    The specified microcode patch doesn't match to any of the MicrocodeCpuId.
**/
BOOLEAN
IsProcessorMatchedMicrocode (
  IN UINT32                      ProcessorSignature,
  IN UINT32                      ProcessorFlags,
  IN EDKII_PEI_MICROCODE_CPU_ID  *MicrocodeCpuId,
  IN UINTN                       MicrocodeCpuIdCount
  )
{
  UINTN  Index;

  if (MicrocodeCpuIdCount == 0) {
    return TRUE;
  }

  for (Index = 0; Index < MicrocodeCpuIdCount; Index++) {
    if ((ProcessorSignature == MicrocodeCpuId[Index].ProcessorSignature) &&
        ((ProcessorFlags & (1 << MicrocodeCpuId[Index].PlatformId)) != 0))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Detect whether specified processor can find matching microcode patch and load it.

  Microcode format is as below:
  +----------------------------------------+-------------------------------------------------+
  |          CPU_MICROCODE_HEADER          |                                                 |
  +----------------------------------------+                                                 V
  |              Update Data               |                                               CPU_MICROCODE_HEADER.Checksum
  +----------------------------------------+-------+                                         ^
  |  CPU_MICROCODE_EXTENDED_TABLE_HEADER   |       |                                         |
  +----------------------------------------+       V                                         |
  |      CPU_MICROCODE_EXTENDED_TABLE[0]   |  CPU_MICROCODE_EXTENDED_TABLE_HEADER.Checksum   |
  |      CPU_MICROCODE_EXTENDED_TABLE[1]   |       ^                                         |
  |                   ...                  |       |                                         |
  +----------------------------------------+-------+-----------------------------------------+

  There may by multiple CPU_MICROCODE_EXTENDED_TABLE in this format.
  The count of CPU_MICROCODE_EXTENDED_TABLE is indicated by ExtendedSignatureCount
  of CPU_MICROCODE_EXTENDED_TABLE_HEADER structure.

  If Microcode is NULL, then ASSERT.

  @param Microcode            Pointer to a microcode entry.
  @param MicrocodeLength      The total length of the microcode entry.
  @param MinimumRevision      The microcode whose revision <= MinimumRevision is treated as invalid.
                              Caller can supply value get from GetProcessorMicrocodeSignature() to check
                              whether the microcode is newer than loaded one.
                              Caller can supply 0 to treat any revision (except 0) microcode as valid.
  @param MicrocodeCpuIds      Pointer to an array of processor signature and platform ID that represents
                              a set of processors.
                              Caller can supply zero-element array to skip the processor signature and
                              platform ID check.
  @param MicrocodeCpuIdCount  The number of elements in MicrocodeCpuIds.
  @param VerifyChecksum       FALSE to skip all the checksum verifications.

  @retval TRUE  The microcode is valid.
  @retval FALSE The microcode is invalid.
**/
BOOLEAN
EFIAPI
IsValidMicrocode (
  IN CPU_MICROCODE_HEADER        *Microcode,
  IN UINTN                       MicrocodeLength,
  IN UINT32                      MinimumRevision,
  IN EDKII_PEI_MICROCODE_CPU_ID  *MicrocodeCpuIds,
  IN UINTN                       MicrocodeCpuIdCount,
  IN BOOLEAN                     VerifyChecksum
  )
{
  UINTN                                Index;
  UINT32                               DataSize;
  UINT32                               TotalSize;
  CPU_MICROCODE_EXTENDED_TABLE         *ExtendedTable;
  CPU_MICROCODE_EXTENDED_TABLE_HEADER  *ExtendedTableHeader;
  UINT32                               ExtendedTableLength;
  UINT32                               Sum32;
  BOOLEAN                              Match;

  ASSERT (Microcode != NULL);

  //
  // It's invalid when:
  //   the input microcode buffer is so small that even cannot contain the header.
  //   the input microcode buffer is so large that exceeds MAX_ADDRESS.
  //
  if ((MicrocodeLength < sizeof (CPU_MICROCODE_HEADER)) || (MicrocodeLength > (MAX_ADDRESS - (UINTN)Microcode))) {
    return FALSE;
  }

  //
  // Per SDM, HeaderVersion and LoaderRevision should both be 1.
  //
  if ((Microcode->HeaderVersion != 1) || (Microcode->LoaderRevision != 1)) {
    return FALSE;
  }

  //
  // The microcode revision should be larger than the minimum revision.
  //
  if (Microcode->UpdateRevision <= MinimumRevision) {
    return FALSE;
  }

  DataSize = Microcode->DataSize;
  if (DataSize == 0) {
    DataSize = 2000;
  }

  //
  // Per SDM, DataSize should be multiple of DWORDs.
  //
  if ((DataSize % 4) != 0) {
    return FALSE;
  }

  TotalSize = GetMicrocodeLength (Microcode);

  //
  // Check whether the whole microcode is within the buffer.
  // TotalSize should be multiple of 1024.
  //
  if (((TotalSize % SIZE_1KB) != 0) || (TotalSize > MicrocodeLength)) {
    return FALSE;
  }

  //
  // The summation of all DWORDs in microcode should be zero.
  //
  if (VerifyChecksum && (CalculateSum32 ((UINT32 *)Microcode, TotalSize) != 0)) {
    return FALSE;
  }

  Sum32 = Microcode->ProcessorSignature.Uint32 + Microcode->ProcessorFlags + Microcode->Checksum;

  //
  // Check the processor signature and platform ID in the primary header.
  //
  Match = IsProcessorMatchedMicrocode (
            Microcode->ProcessorSignature.Uint32,
            Microcode->ProcessorFlags,
            MicrocodeCpuIds,
            MicrocodeCpuIdCount
            );
  if (Match) {
    return TRUE;
  }

  ExtendedTableLength = TotalSize - (DataSize + sizeof (CPU_MICROCODE_HEADER));
  if ((ExtendedTableLength < sizeof (CPU_MICROCODE_EXTENDED_TABLE_HEADER)) || ((ExtendedTableLength % 4) != 0)) {
    return FALSE;
  }

  //
  // Extended Table exist, check if the CPU in support list
  //
  ExtendedTableHeader = (CPU_MICROCODE_EXTENDED_TABLE_HEADER *)((UINTN)(Microcode + 1) + DataSize);
  if (ExtendedTableHeader->ExtendedSignatureCount > MAX_UINT32 / sizeof (CPU_MICROCODE_EXTENDED_TABLE)) {
    return FALSE;
  }

  if (ExtendedTableHeader->ExtendedSignatureCount * sizeof (CPU_MICROCODE_EXTENDED_TABLE)
      > ExtendedTableLength - sizeof (CPU_MICROCODE_EXTENDED_TABLE_HEADER))
  {
    return FALSE;
  }

  //
  // Check the extended table checksum
  //
  if (VerifyChecksum && (CalculateSum32 ((UINT32 *)ExtendedTableHeader, ExtendedTableLength) != 0)) {
    return FALSE;
  }

  ExtendedTable = (CPU_MICROCODE_EXTENDED_TABLE *)(ExtendedTableHeader + 1);
  for (Index = 0; Index < ExtendedTableHeader->ExtendedSignatureCount; Index++) {
    if (VerifyChecksum &&
        (ExtendedTable[Index].ProcessorSignature.Uint32 + ExtendedTable[Index].ProcessorFlag
         + ExtendedTable[Index].Checksum != Sum32))
    {
      //
      // The extended table entry is valid when the summation of Processor Signature, Processor Flags
      // and Checksum equal to the coresponding summation from primary header. Because:
      //    CalculateSum32 (Header + Update Binary) == 0
      //    CalculateSum32 (Header + Update Binary)
      //        - (Header.ProcessorSignature + Header.ProcessorFlag + Header.Checksum)
      //        + (Extended.ProcessorSignature + Extended.ProcessorFlag + Extended.Checksum) == 0
      // So,
      //    (Header.ProcessorSignature + Header.ProcessorFlag + Header.Checksum)
      //     == (Extended.ProcessorSignature + Extended.ProcessorFlag + Extended.Checksum)
      //
      continue;
    }

    Match = IsProcessorMatchedMicrocode (
              ExtendedTable[Index].ProcessorSignature.Uint32,
              ExtendedTable[Index].ProcessorFlag,
              MicrocodeCpuIds,
              MicrocodeCpuIdCount
              );
    if (Match) {
      return TRUE;
    }
  }

  return FALSE;
}
