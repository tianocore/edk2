/** @file
  Bounds regression tests for Brotli UEFI and GUIDed section decompression.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>
#include "../BrotliDecompressLibInternal.h"

#define TEST_SCRATCH_SIZE  SIZE_8MB
#define GUARD_SIZE         64
#define GUARD_BYTE         0xA5

STATIC UINT8  *mScratch;

// Each payload expands to the indicated number of 'A' bytes.
STATIC CONST UINT8  mEmpty[]  = { 0x3B };
STATIC CONST UINT8  m8[]      = { 0x1B, 0x07, 0x00, 0xF8, 0x25, 0x82, 0x42, 0x84, 0x00, 0x00 };
STATIC CONST UINT8  m8192[]   = { 0x1B, 0xFF, 0x1F, 0xF8, 0x25, 0x82, 0xE2, 0xB1, 0x40, 0x20, 0xF7, 0x02, 0x00 };
STATIC CONST UINT8  m65536[]  = { 0x1B, 0xFF, 0xFF, 0xF8, 0x25, 0x82, 0xE2, 0xB1, 0x40, 0x20, 0xF7, 0x1E, 0x00 };
STATIC CONST UINT8  m65537[]  = { 0x5B, 0x00, 0x00, 0x81, 0x5F, 0x22, 0x28, 0x1E, 0x0B, 0x04, 0x74, 0xEF, 0x01, 0x00 };
STATIC CONST UINT8  m131073[] = { 0x5B, 0x00, 0x00, 0x82, 0x5F, 0x22, 0x28, 0x1E, 0x0B, 0x04, 0x74, 0xEF, 0x03, 0x00 };

typedef struct {
  CHAR8          *Name;
  CONST UINT8    *Payload;
  UINT32         PayloadSize;
  UINT32         DestinationSize;
  EFI_STATUS     Status;
} BROTLI_TEST_CASE;

STATIC BROTLI_TEST_CASE  mOutputCases[] = {
  { "Empty output",              mEmpty,  sizeof (mEmpty),  0,      EFI_SUCCESS           },
  { "Small output",              m8,      sizeof (m8),      8,      EFI_SUCCESS           },
  { "Partial chunk",             m8192,   sizeof (m8192),   8192,   EFI_SUCCESS           },
  { "Exact chunk",               m65536,  sizeof (m65536),  65536,  EFI_SUCCESS           },
  { "Chunk plus one byte",       m65537,  sizeof (m65537),  65537,  EFI_SUCCESS           },
  { "Multiple chunks",           m131073, sizeof (m131073), 131073, EFI_SUCCESS           },
  { "Undeclared output",         m8192,   sizeof (m8192),   0,      EFI_INVALID_PARAMETER },
  { "Understated output",        m8192,   sizeof (m8192),   8,      EFI_INVALID_PARAMETER },
  { "Partial chunk minus one",   m8192,   sizeof (m8192),   8191,   EFI_INVALID_PARAMETER },
  { "Overstated output",         m8192,   sizeof (m8192),   8193,   EFI_INVALID_PARAMETER },
  { "Exact chunk minus one",     m65536,  sizeof (m65536),  65535,  EFI_INVALID_PARAMETER },
  { "Final byte overflow",       m65537,  sizeof (m65537),  65536,  EFI_INVALID_PARAMETER },
  { "First full chunk overflow", m131073, sizeof (m131073), 65535,  EFI_INVALID_PARAMETER },
  { "Later full chunk overflow", m131073, sizeof (m131073), 65536,  EFI_INVALID_PARAMETER },
  { "Multiple chunks minus one", m131073, sizeof (m131073), 131072, EFI_INVALID_PARAMETER }
};

/**
  Initialize a wrapper header and copy the encoded payload.
**/
STATIC
VOID
MakeSource (
  OUT UINT8       *Source,
  IN CONST UINT8  *Payload,
  IN UINTN        PayloadSize,
  IN UINT64       DestinationSize,
  IN UINT64       ScratchSize
  )
{
  WriteUnaligned64 ((UINT64 *)Source, DestinationSize);
  WriteUnaligned64 ((UINT64 *)(Source + BROTLI_DECODE_MAX), ScratchSize);
  CopyMem (Source + BROTLI_SCRATCH_MAX, Payload, PayloadSize);
}

/**
  Check a region of a buffer against an expected byte value.
**/
STATIC
BOOLEAN
BufferIsFilled (
  IN CONST UINT8  *Buffer,
  IN UINTN        Size,
  IN UINT8        Value
  )
{
  UINTN  Index;

  for (Index = 0; Index < Size; Index++) {
    if (Buffer[Index] != Value) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Check successful output and both destination guards, including on errors.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestOutputBounds (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BROTLI_TEST_CASE  *TestCase;
  UINT8             Source[BROTLI_SCRATCH_MAX + sizeof (m131073)];
  UINT8             *Buffer;
  UINT32            DestinationSize;
  UINT32            ScratchSize;

  TestCase = (BROTLI_TEST_CASE *)Context;
  MakeSource (Source, TestCase->Payload, TestCase->PayloadSize, TestCase->DestinationSize, TEST_SCRATCH_SIZE);
  UT_ASSERT_STATUS_EQUAL (
    BrotliUefiDecompressGetInfo (Source, BROTLI_SCRATCH_MAX + TestCase->PayloadSize, &DestinationSize, &ScratchSize),
    EFI_SUCCESS
    );
  UT_ASSERT_EQUAL (DestinationSize, TestCase->DestinationSize);
  UT_ASSERT_EQUAL (ScratchSize, TEST_SCRATCH_SIZE);

  Buffer = AllocatePool (DestinationSize + 2 * GUARD_SIZE);
  UT_ASSERT_NOT_NULL (Buffer);
  SetMem (Buffer, DestinationSize + 2 * GUARD_SIZE, GUARD_BYTE);
  UT_ASSERT_STATUS_EQUAL (
    BrotliUefiDecompress (Source, BROTLI_SCRATCH_MAX + TestCase->PayloadSize, Buffer + GUARD_SIZE, mScratch),
    TestCase->Status
    );
  UT_ASSERT_TRUE (BufferIsFilled (Buffer, GUARD_SIZE, GUARD_BYTE));
  UT_ASSERT_TRUE (BufferIsFilled (Buffer + GUARD_SIZE + DestinationSize, GUARD_SIZE, GUARD_BYTE));
  if (TestCase->Status == EFI_SUCCESS) {
    UT_ASSERT_TRUE (BufferIsFilled (Buffer + GUARD_SIZE, DestinationSize, 'A'));
  }

  FreePool (Buffer);
  return UNIT_TEST_PASSED;
}

/**
  Reject every short header and both unrepresentable size fields.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestHeaderBounds (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8   *ShortSource;
  UINT8   Source[BROTLI_SCRATCH_MAX];
  UINT8   Destination[8];
  UINT32  DestinationSize;
  UINT32  ScratchSize;
  UINTN   Index;
  UINTN   Field;
  UINT64  Sizes[] = { LShiftU64 (1, 32), LShiftU64 (1, 32) + 8, MAX_UINT64 };

  for (Index = 0; Index < BROTLI_SCRATCH_MAX; Index++) {
    ShortSource = AllocateZeroPool (MAX (Index, 1));
    UT_ASSERT_NOT_NULL (ShortSource);
    DestinationSize = 0x12345678;
    ScratchSize     = 0x87654321;
    UT_ASSERT_STATUS_EQUAL (
      BrotliUefiDecompressGetInfo (ShortSource, (UINT32)Index, &DestinationSize, &ScratchSize),
      EFI_INVALID_PARAMETER
      );
    UT_ASSERT_EQUAL (DestinationSize, 0x12345678);
    UT_ASSERT_EQUAL (ScratchSize, 0x87654321);
    UT_ASSERT_STATUS_EQUAL (BrotliUefiDecompress (ShortSource, Index, Destination, mScratch), EFI_INVALID_PARAMETER);
    FreePool (ShortSource);
  }

  for (Field = 0; Field < 2; Field++) {
    for (Index = 0; Index < ARRAY_SIZE (Sizes); Index++) {
      MakeSource (Source, mEmpty, 0, 8, TEST_SCRATCH_SIZE);
      WriteUnaligned64 ((UINT64 *)(Source + Field * BROTLI_INFO_SIZE), Sizes[Index]);
      DestinationSize = 0x12345678;
      ScratchSize     = 0x87654321;
      UT_ASSERT_STATUS_EQUAL (
        BrotliUefiDecompressGetInfo (Source, sizeof (Source), &DestinationSize, &ScratchSize),
        EFI_UNSUPPORTED
        );
      UT_ASSERT_EQUAL (DestinationSize, 0x12345678);
      UT_ASSERT_EQUAL (ScratchSize, 0x87654321);
      UT_ASSERT_STATUS_EQUAL (BrotliUefiDecompress (Source, sizeof (Source), Destination, mScratch), EFI_UNSUPPORTED);
    }
  }

  MakeSource (Source, mEmpty, 0, MAX_UINT32, MAX_UINT32);
  UT_ASSERT_STATUS_EQUAL (BrotliUefiDecompressGetInfo (Source, sizeof (Source), &DestinationSize, &ScratchSize), EFI_SUCCESS);
  UT_ASSERT_EQUAL (DestinationSize, MAX_UINT32);
  UT_ASSERT_EQUAL (ScratchSize, MAX_UINT32);
  if (MAX_UINTN > MAX_UINT32) {
    UT_ASSERT_STATUS_EQUAL (
      BrotliUefiDecompress (Source, (UINTN)LShiftU64 (1, 32), Destination, mScratch),
      EFI_UNSUPPORTED
      );
  }

  return UNIT_TEST_PASSED;
}

/**
  Exhaustion must fail without touching either scratch guard.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestScratchBounds (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8  Source[BROTLI_SCRATCH_MAX + sizeof (m8)];
  UINT8  Destination[8];
  UINT8  *Buffer;
  UINTN  Sizes[] = { 0, 1, 8, FILE_BUFFER_SIZE, 2 * FILE_BUFFER_SIZE };
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (Sizes); Index++) {
    Buffer = AllocatePool (Sizes[Index] + 2 * GUARD_SIZE);
    UT_ASSERT_NOT_NULL (Buffer);
    SetMem (Buffer, Sizes[Index] + 2 * GUARD_SIZE, GUARD_BYTE);
    MakeSource (Source, m8, sizeof (m8), sizeof (Destination), Sizes[Index]);
    UT_ASSERT_STATUS_EQUAL (
      BrotliUefiDecompress (Source, sizeof (Source), Destination, Buffer + GUARD_SIZE),
      EFI_INVALID_PARAMETER
      );
    UT_ASSERT_TRUE (BufferIsFilled (Buffer, GUARD_SIZE, GUARD_BYTE));
    UT_ASSERT_TRUE (BufferIsFilled (Buffer + GUARD_SIZE + Sizes[Index], GUARD_SIZE, GUARD_BYTE));
    FreePool (Buffer);
  }

  return UNIT_TEST_PASSED;
}

/**
  Reject missing or malformed payloads without reporting successful decoding.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestMalformedStreams (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8  Source[BROTLI_SCRATCH_MAX + sizeof (m8)];
  UINT8  Destination[8];
  UINTN  PayloadSize;

  MakeSource (Source, m8, sizeof (m8), sizeof (Destination), TEST_SCRATCH_SIZE);
  for (PayloadSize = 0; PayloadSize < sizeof (m8); PayloadSize++) {
    UT_ASSERT_STATUS_EQUAL (
      BrotliUefiDecompress (Source, BROTLI_SCRATCH_MAX + PayloadSize, Destination, mScratch),
      EFI_INVALID_PARAMETER
      );
  }

  SetMem (Source + BROTLI_SCRATCH_MAX, sizeof (m8), 0xFF);
  UT_ASSERT_STATUS_EQUAL (BrotliUefiDecompress (Source, sizeof (Source), Destination, mScratch), EFI_INVALID_PARAMETER);
  return UNIT_TEST_PASSED;
}

/**
  Required buffers must be present; empty output may use a NULL destination.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestNullBuffers (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8   Source[BROTLI_SCRATCH_MAX + sizeof (m8)];
  UINT8   Destination[8];
  UINT32  DestinationSize;
  UINT32  ScratchSize;

  MakeSource (Source, m8, sizeof (m8), sizeof (Destination), TEST_SCRATCH_SIZE);
  UT_ASSERT_STATUS_EQUAL (BrotliUefiDecompressGetInfo (NULL, sizeof (Source), &DestinationSize, &ScratchSize), EFI_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (BrotliUefiDecompressGetInfo (Source, sizeof (Source), NULL, &ScratchSize), EFI_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (BrotliUefiDecompressGetInfo (Source, sizeof (Source), &DestinationSize, NULL), EFI_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (BrotliUefiDecompress (NULL, sizeof (Source), Destination, mScratch), EFI_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (BrotliUefiDecompress (Source, sizeof (Source), NULL, mScratch), EFI_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (BrotliUefiDecompress (Source, sizeof (Source), Destination, NULL), EFI_INVALID_PARAMETER);
  MakeSource (Source, mEmpty, sizeof (mEmpty), 0, TEST_SCRATCH_SIZE);
  UT_ASSERT_STATUS_EQUAL (BrotliUefiDecompress (Source, BROTLI_SCRATCH_MAX + sizeof (mEmpty), NULL, mScratch), EFI_SUCCESS);
  return UNIT_TEST_PASSED;
}

/**
  Exercise normal and extended section sizes, offsets, and valid decoding.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestGuidedSections (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64                     SectionBuffer[8];
  EFI_GUID_DEFINED_SECTION   *Section;
  EFI_GUID_DEFINED_SECTION2  *Section2;
  UINT8                      Destination[8];
  VOID                       *Output;
  UINT32                     OutputSize;
  UINT32                     ScratchSize;
  UINT32                     AuthenticationStatus;
  UINT16                     Attributes;
  UINT32                     HeaderSize;
  UINT32                     SectionSize;
  UINT16                     *DataOffset;
  UINT32                     *ExtendedSize;
  UINT32                     Sizes[3];
  UINT16                     Offsets[4];
  UINTN                      Variant;
  UINTN                      Index;
  UINTN                      Padding;

  for (Variant = 0; Variant < 2; Variant++) {
    for (Padding = 0; Padding <= 4; Padding += 4) {
      ZeroMem (SectionBuffer, sizeof (SectionBuffer));
      Section = (EFI_GUID_DEFINED_SECTION *)SectionBuffer;
      if (Variant == 0) {
        HeaderSize                     = sizeof (*Section);
        Section->SectionDefinitionGuid = gBrotliCustomDecompressGuid;
        Section->Attributes            = EFI_GUIDED_SECTION_PROCESSING_REQUIRED;
        DataOffset                     = &Section->DataOffset;
        ExtendedSize                   = NULL;
      } else {
        SetMem (Section->CommonHeader.Size, sizeof (Section->CommonHeader.Size), 0xFF);
        Section2                        = (EFI_GUID_DEFINED_SECTION2 *)SectionBuffer;
        HeaderSize                      = sizeof (*Section2);
        Section2->SectionDefinitionGuid = gBrotliCustomDecompressGuid;
        Section2->Attributes            = EFI_GUIDED_SECTION_PROCESSING_REQUIRED;
        DataOffset                      = &Section2->DataOffset;
        ExtendedSize                    = &Section2->CommonHeader.ExtendedSize;
      }

      Section->CommonHeader.Type = EFI_SECTION_GUID_DEFINED;
      *DataOffset                = (UINT16)(HeaderSize + Padding);
      SectionSize                = *DataOffset + BROTLI_SCRATCH_MAX + sizeof (m8);
      if (ExtendedSize != NULL) {
        *ExtendedSize = SectionSize;
      } else {
        Section->CommonHeader.Size[0] = (UINT8)SectionSize;
      }

      MakeSource ((UINT8 *)SectionBuffer + *DataOffset, m8, sizeof (m8), sizeof (Destination), TEST_SCRATCH_SIZE);
      UT_ASSERT_STATUS_EQUAL (
        BrotliGuidedSectionGetInfo (SectionBuffer, &OutputSize, &ScratchSize, &Attributes),
        RETURN_SUCCESS
        );
      UT_ASSERT_EQUAL (OutputSize, sizeof (Destination));
      UT_ASSERT_EQUAL (ScratchSize, TEST_SCRATCH_SIZE);
      UT_ASSERT_EQUAL (Attributes, EFI_GUIDED_SECTION_PROCESSING_REQUIRED);
      Output               = Destination;
      AuthenticationStatus = MAX_UINT32;
      UT_ASSERT_STATUS_EQUAL (BrotliGuidedSectionExtraction (SectionBuffer, &Output, mScratch, &AuthenticationStatus), RETURN_SUCCESS);
      UT_ASSERT_EQUAL (AuthenticationStatus, 0);
      UT_ASSERT_TRUE (BufferIsFilled (Destination, sizeof (Destination), 'A'));

      if (Variant == 0) {
        ZeroMem (&Section->SectionDefinitionGuid, sizeof (EFI_GUID));
      } else {
        ZeroMem (&Section2->SectionDefinitionGuid, sizeof (EFI_GUID));
      }

      UT_ASSERT_STATUS_EQUAL (
        BrotliGuidedSectionGetInfo (SectionBuffer, &OutputSize, &ScratchSize, &Attributes),
        RETURN_INVALID_PARAMETER
        );
      UT_ASSERT_STATUS_EQUAL (BrotliGuidedSectionExtraction (SectionBuffer, &Output, mScratch, &AuthenticationStatus), RETURN_INVALID_PARAMETER);
      if (Variant == 0) {
        Section->SectionDefinitionGuid = gBrotliCustomDecompressGuid;
      } else {
        Section2->SectionDefinitionGuid = gBrotliCustomDecompressGuid;
      }
    }

    Offsets[0] = 0;
    Offsets[1] = (UINT16)(HeaderSize - 1);
    Offsets[2] = (UINT16)(SectionSize + 1);
    Offsets[3] = MAX_UINT16;
    for (Index = 0; Index < ARRAY_SIZE (Offsets); Index++) {
      *DataOffset = Offsets[Index];
      UT_ASSERT_STATUS_EQUAL (
        BrotliGuidedSectionGetInfo (SectionBuffer, &OutputSize, &ScratchSize, &Attributes),
        RETURN_INVALID_PARAMETER
        );
      UT_ASSERT_STATUS_EQUAL (BrotliGuidedSectionExtraction (SectionBuffer, &Output, mScratch, &AuthenticationStatus), RETURN_INVALID_PARAMETER);
    }

    *DataOffset = (UINT16)HeaderSize;
    Sizes[0]    = 0;
    Sizes[1]    = sizeof (EFI_COMMON_SECTION_HEADER);
    Sizes[2]    = HeaderSize - 1;
    for (Index = 0; Index < ARRAY_SIZE (Sizes); Index++) {
      if (ExtendedSize != NULL) {
        *ExtendedSize = Sizes[Index];
      } else {
        Section->CommonHeader.Size[0] = (UINT8)Sizes[Index];
      }

      UT_ASSERT_STATUS_EQUAL (
        BrotliGuidedSectionGetInfo (SectionBuffer, &OutputSize, &ScratchSize, &Attributes),
        RETURN_INVALID_PARAMETER
        );
      UT_ASSERT_STATUS_EQUAL (BrotliGuidedSectionExtraction (SectionBuffer, &Output, mScratch, &AuthenticationStatus), RETURN_INVALID_PARAMETER);
    }

    for (Index = 0; Index < BROTLI_SCRATCH_MAX; Index++) {
      if (ExtendedSize != NULL) {
        *ExtendedSize = HeaderSize + (UINT32)Index;
      } else {
        Section->CommonHeader.Size[0] = (UINT8)(HeaderSize + Index);
      }

      UT_ASSERT_STATUS_EQUAL (
        BrotliGuidedSectionGetInfo (SectionBuffer, &OutputSize, &ScratchSize, &Attributes),
        RETURN_INVALID_PARAMETER
        );
      UT_ASSERT_STATUS_EQUAL (BrotliGuidedSectionExtraction (SectionBuffer, &Output, mScratch, &AuthenticationStatus), RETURN_INVALID_PARAMETER);
    }
  }

  UT_ASSERT_STATUS_EQUAL (BrotliGuidedSectionGetInfo (NULL, &OutputSize, &ScratchSize, &Attributes), RETURN_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (BrotliGuidedSectionGetInfo (SectionBuffer, NULL, &ScratchSize, &Attributes), RETURN_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (BrotliGuidedSectionGetInfo (SectionBuffer, &OutputSize, NULL, &Attributes), RETURN_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (BrotliGuidedSectionGetInfo (SectionBuffer, &OutputSize, &ScratchSize, NULL), RETURN_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (BrotliGuidedSectionExtraction (NULL, &Output, mScratch, &AuthenticationStatus), RETURN_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (BrotliGuidedSectionExtraction (SectionBuffer, NULL, mScratch, &AuthenticationStatus), RETURN_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (BrotliGuidedSectionExtraction (SectionBuffer, &Output, mScratch, NULL), RETURN_INVALID_PARAMETER);
  return UNIT_TEST_PASSED;
}

/**
  Run the host-based Brotli bounds tests.
**/
INT32
main (
  IN INT32  Argc,
  IN CHAR8  *Argv[]
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      Suite;
  UINTN                       Index;

  Framework = NULL;
  mScratch  = AllocatePool (TEST_SCRATCH_SIZE);
  if (mScratch == NULL) {
    return 1;
  }

  Status = InitUnitTestFramework (&Framework, "Brotli bounds tests", gEfiCallerBaseName, "1.0");
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = CreateUnitTestSuite (&Suite, Framework, "Brotli decompression bounds", "Brotli.Bounds", NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  for (Index = 0; Index < ARRAY_SIZE (mOutputCases); Index++) {
    Status = AddTestCase (Suite, mOutputCases[Index].Name, mOutputCases[Index].Name, TestOutputBounds, NULL, NULL, &mOutputCases[Index]);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  }

  Status = AddTestCase (Suite, "Header sizes", "HeaderSizes", TestHeaderBounds, NULL, NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = AddTestCase (Suite, "Scratch exhaustion", "ScratchExhaustion", TestScratchBounds, NULL, NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = AddTestCase (Suite, "Malformed streams", "MalformedStreams", TestMalformedStreams, NULL, NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = AddTestCase (Suite, "NULL buffers", "NullBuffers", TestNullBuffers, NULL, NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = AddTestCase (Suite, "GUIDed sections", "GuidedSections", TestGuidedSections, NULL, NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = RunAllTestSuites (Framework);

Exit:
  if (Framework != NULL) {
    FreeUnitTestFramework (Framework);
  }

  FreePool (mScratch);
  return EFI_ERROR (Status) ? 1 : 0;
}
