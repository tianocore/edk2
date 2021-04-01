/** @file
  Public include file for Microcode library.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MICROCODE_LIB_H_
#define MICROCODE_LIB_H_

#include <Register/Intel/Microcode.h>
#include <Ppi/ShadowMicrocode.h>

/**
  Get microcode update signature of currently loaded microcode update.

  @return  Microcode signature.
**/
UINT32
EFIAPI
GetProcessorMicrocodeSignature (
  VOID
  );

/**
  Get the processor signature and platform ID for current processor.

  @param MicrocodeCpuId  Return the processor signature and platform ID.
**/
VOID
EFIAPI
GetProcessorMicrocodeCpuId (
  EDKII_PEI_MICROCODE_CPU_ID  *MicrocodeCpuId
  );

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
  IN CPU_MICROCODE_HEADER *Microcode
  );

/**
  Load the microcode to the processor.

  If Microcode is NULL, then ASSERT.

  @param Microcode  Pointer to the microcode entry.
**/
VOID
EFIAPI
LoadMicrocode (
  IN CPU_MICROCODE_HEADER *Microcode
  );

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
  IN CPU_MICROCODE_HEADER       *Microcode,
  IN UINTN                      MicrocodeLength,
  IN UINT32                     MinimumRevision,
  IN EDKII_PEI_MICROCODE_CPU_ID *MicrocodeCpuIds,
  IN UINTN                      MicrocodeCpuIdCount,
  IN BOOLEAN                    VerifyChecksum
  );

#endif
