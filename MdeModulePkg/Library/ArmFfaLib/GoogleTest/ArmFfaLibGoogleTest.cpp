/** @file
  Unit tests for ArmFfaLib partition discovery via FFA_PARTITION_INFO_GET_REGS.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/GoogleTestLib.h>
#include <vector>

extern "C" {
  #include <Uefi.h>
  #include <Pi/PiMultiPhase.h>
  #include <Library/ArmFfaLib.h>
  #include <Library/ArmSmcLib.h>
  #include <Library/ArmSvcLib.h>
  #include <Library/BaseLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/HobLib.h>
  #include <IndustryStandard/ArmFfaSvc.h>
  #include <IndustryStandard/ArmFfaPartInfo.h>
  #include "../ArmFfaCommon.h"
}

using namespace testing;

//
// Bytes available for descriptors in x3 - x17.
//
#define REGS_PAYLOAD_SIZE  (15 * sizeof (UINTN))

#define LEGACY_DESC_SIZE  ((UINT16)sizeof (EFI_FFA_PART_INFO_DESC))
#define V1_3_DESC_SIZE    ((UINT16)sizeof (EFI_FFA_PART_INFO_DESC_1_3))

#define SPMC_TAG  0x55AA

STATIC EFI_GUID  mServiceGuid = {
  0x12345678, 0x9ABC, 0xDEF0, { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF }
};

//
// Minimal SPMC model for FFA_PARTITION_INFO_GET_REGS.
//
class FakeSpmc {
public:
  UINT32 TotalCount     = 0;
  UINT16 DescSize       = 0;
  UINT16 EntriesPerCall = 0;
  UINTN FfaError        = 0;

  //
  // Optionally corrupt the metadata returned by the CorruptCall-th (1-based) call.
  //
  UINT32 CorruptCall          = 0;
  BOOLEAN CorruptCurrentIndex = FALSE;
  UINT16 CurrentIndex         = 0;
  UINT16 CorruptDescSize      = 0;

  std::vector<ARM_FFA_ARGS> Requests;

  static UINT8
  EntryPattern (
    UINT32  Index
    )
  {
    return (UINT8)(0xA0 + Index);
  }

  VOID
  Handle (
    ARM_FFA_ARGS  *Args
    )
  {
    UINT16   StartIndex;
    UINT16   LastIndex;
    UINT16   ReturnedIndex;
    UINT16   ReturnedDescSize;
    UINT32   Index;
    UINT8    *Payload;
    BOOLEAN  Corrupt;

    Requests.push_back (*Args);
    Corrupt = (BOOLEAN)(Requests.size () == CorruptCall);

    if ((Args->Arg0 != ARM_FID_FFA_PARTITION_INFO_GET_REGS) || (FfaError != 0)) {
      ZeroMem (Args, sizeof (*Args));
      Args->Arg0 = ARM_FID_FFA_ERROR;
      Args->Arg2 = (FfaError != 0) ? FfaError : (UINTN)ARM_FFA_RET_NOT_SUPPORTED;
      return;
    }

    StartIndex = (UINT16)(Args->Arg3 & FFA_PART_INFO_IDX_MASK);
    if (StartIndex >= TotalCount) {
      ZeroMem (Args, sizeof (*Args));
      Args->Arg0 = ARM_FID_FFA_ERROR;
      Args->Arg2 = (UINTN)ARM_FFA_RET_INVALID_PARAMETERS;
      return;
    }

    LastIndex        = (UINT16)(TotalCount - 1);
    ReturnedIndex    = (UINT16)MIN ((UINT32)LastIndex, (UINT32)StartIndex + EntriesPerCall - 1);
    ReturnedDescSize = DescSize;
    if (Corrupt && CorruptCurrentIndex) {
      ReturnedIndex = CurrentIndex;
    }

    if (Corrupt && (CorruptDescSize != 0)) {
      ReturnedDescSize = CorruptDescSize;
    }

    ZeroMem (Args, sizeof (*Args));
    Args->Arg0 = ARM_FID_FFA_SUCCESS_AARCH64;

    //
    // Never write past x17, even when the metadata claims more entries.
    //
    Payload = (UINT8 *)&Args->Arg3;
    for (Index = StartIndex;
         (Index <= ReturnedIndex) && (((Index - StartIndex + 1) * ReturnedDescSize) <= REGS_PAYLOAD_SIZE);
         Index++)
    {
      SetMem (Payload + (Index - StartIndex) * ReturnedDescSize, ReturnedDescSize, EntryPattern (Index));
    }

    Args->Arg2 = ((UINT64)LastIndex << FFA_PART_INFO_METADATA_LAST_IDX_SHIFT) |
                 ((UINT64)ReturnedIndex << FFA_PART_INFO_METADATA_CURRENT_IDX_SHIFT) |
                 ((UINT64)SPMC_TAG << FFA_PART_INFO_METADATA_TAG_SHIFT) |
                 ((UINT64)ReturnedDescSize << FFA_PART_INFO_METADATA_DESC_SIZE_SHIFT);
  }
};

STATIC FakeSpmc  *mSpmc = NULL;

extern "C" {
  VOID
  ArmCallSmc (
    IN OUT ARM_SMC_ARGS  *Args
    )
  {
    mSpmc->Handle ((ARM_FFA_ARGS *)Args);
  }

  VOID
  ArmCallSvc (
    IN OUT ARM_SVC_ARGS  *Args
    )
  {
    mSpmc->Handle ((ARM_FFA_ARGS *)Args);
  }

  EFI_STATUS
  EFIAPI
  ArmFfaLibGetPartId (
    OUT UINT16  *PartId
    )
  {
    *PartId = 0;
    return EFI_SUCCESS;
  }

  EFI_STATUS
  EFIAPI
  ArmFfaLibGetRxTxBuffers (
    OUT VOID    **TxBuffer OPTIONAL,
    OUT UINT64  *TxBufferSize OPTIONAL,
    OUT VOID    **RxBuffer OPTIONAL,
    OUT UINT64  *RxBufferSize OPTIONAL
    )
  {
    return EFI_UNSUPPORTED;
  }
}

class ArmFfaLibPartInfoRegsTest : public Test {
protected:
  FakeSpmc Spmc;

  void
  SetUp (
    ) override
  {
    mSpmc = &Spmc;
  }

  void
  TearDown (
    ) override
  {
    mSpmc = NULL;
  }

  VOID
  Configure (
    UINT32  TotalCount,
    UINT16  DescSize,
    UINT16  EntriesPerCall
    )
  {
    Spmc.TotalCount     = TotalCount;
    Spmc.DescSize       = DescSize;
    Spmc.EntriesPerCall = EntriesPerCall;
  }

  UINT16
  RequestedStartIndex (
    UINTN  Call
    )
  {
    return (UINT16)((Spmc.Requests[Call].Arg3 >> FFA_PART_INFO_START_IDX_SHIFT) & FFA_PART_INFO_IDX_MASK);
  }

  UINT16
  RequestedTag (
    UINTN  Call
    )
  {
    return (UINT16)((Spmc.Requests[Call].Arg3 >> FFA_PART_INFO_START_TAG_SHIFT) & FFA_PART_INFO_TAG_MASK);
  }

  VOID
  ExpectStartIndices (
    std::vector<UINT16>  Expected
    )
  {
    ASSERT_EQ (Spmc.Requests.size (), Expected.size ());
    for (UINTN Call = 0; Call < Expected.size (); Call++) {
      EXPECT_EQ (RequestedStartIndex (Call), Expected[Call]) << "call " << Call;
      EXPECT_EQ (RequestedTag (Call), (Call == 0) ? 0 : SPMC_TAG) << "call " << Call;
    }
  }

  static VOID
  ExpectEntries (
    CONST UINT8  *Buffer,
    UINT16       DescSize,
    UINT32       Count
    )
  {
    for (UINT32 Index = 0; Index < Count; Index++) {
      for (UINT16 Byte = 0; Byte < DescSize; Byte++) {
        ASSERT_EQ (Buffer[Index * DescSize + Byte], FakeSpmc::EntryPattern (Index)) << "entry " << Index;
      }
    }
  }

  //
  // Malformed responses must be rejected by every register based API.
  //
  VOID
  ExpectAllApisReject (
    VOID
    )
  {
    EFI_FFA_PART_INFO_DESC  Legacy[8];
    UINT8                   Buffer[8 * LEGACY_DESC_SIZE];
    UINT32                  Count;
    UINT32                  Size;

    Count = ARRAY_SIZE (Legacy);
    EXPECT_EQ (ArmFfaLibPartitionInfoGetRegs (NULL, &Count, Legacy), EFI_INVALID_PARAMETER);

    Spmc.Requests.clear ();
    Size = sizeof (Buffer);
    EXPECT_EQ (ArmFfaLibPartitionInfoGetRegsEx (NULL, &Size, &Count, Buffer), EFI_INVALID_PARAMETER);
  }
};

TEST_F (ArmFfaLibPartInfoRegsTest, CountReturnsTotalCount) {
  UINT32  Count;

  Configure (7, LEGACY_DESC_SIZE, 5);

  EXPECT_EQ (ArmFfaLibPartitionCountGetRegs (NULL, &Count), EFI_SUCCESS);
  EXPECT_EQ (Count, 7U);
  ExpectStartIndices ({ 0 });
}

TEST_F (ArmFfaLibPartInfoRegsTest, CountRejectsNullCount) {
  EXPECT_EQ (ArmFfaLibPartitionCountGetRegs (NULL, NULL), EFI_INVALID_PARAMETER);
  EXPECT_TRUE (Spmc.Requests.empty ());
}

TEST_F (ArmFfaLibPartInfoRegsTest, CountReturnsFfaError) {
  UINT32  Count;

  Configure (7, LEGACY_DESC_SIZE, 5);
  Spmc.FfaError = (UINTN)ARM_FFA_RET_NOT_SUPPORTED;

  EXPECT_EQ (ArmFfaLibPartitionCountGetRegs (NULL, &Count), EFI_UNSUPPORTED);
}

TEST_F (ArmFfaLibPartInfoRegsTest, CountRejectsMalformedMetadata) {
  UINT32  Count;

  Configure (7, LEGACY_DESC_SIZE - 1, 5);

  EXPECT_EQ (ArmFfaLibPartitionCountGetRegs (NULL, &Count), EFI_INVALID_PARAMETER);
}

TEST_F (ArmFfaLibPartInfoRegsTest, ServiceGuidIsSentOnEveryCall) {
  UINT64  Uuid[2];
  UINT8   Buffer[5 * V1_3_DESC_SIZE];
  UINT32  Size;
  UINT32  Count;

  ConvertGuidToUuid (&mServiceGuid, (GUID *)Uuid);
  Configure (5, V1_3_DESC_SIZE, 2);
  Size = sizeof (Buffer);

  ASSERT_EQ (ArmFfaLibPartitionInfoGetRegsEx (&mServiceGuid, &Size, &Count, Buffer), EFI_SUCCESS);
  ASSERT_EQ (Spmc.Requests.size (), 3U);
  for (CONST ARM_FFA_ARGS &Request : Spmc.Requests) {
    EXPECT_EQ (Request.Arg0, (UINTN)ARM_FID_FFA_PARTITION_INFO_GET_REGS);
    EXPECT_EQ (Request.Arg1, Uuid[0]);
    EXPECT_EQ (Request.Arg2, Uuid[1]);
  }
}

TEST_F (ArmFfaLibPartInfoRegsTest, LegacyReadsAllPages) {
  EFI_FFA_PART_INFO_DESC  PartDesc[10];
  UINT32                  Count;

  Configure (7, LEGACY_DESC_SIZE, 5);
  Count = ARRAY_SIZE (PartDesc);

  ASSERT_EQ (ArmFfaLibPartitionInfoGetRegs (NULL, &Count, PartDesc), EFI_SUCCESS);
  EXPECT_EQ (Count, 7U);
  ExpectStartIndices ({ 0, 5 });
  ExpectEntries ((UINT8 *)PartDesc, LEGACY_DESC_SIZE, 7);
}

TEST_F (ArmFfaLibPartInfoRegsTest, LegacyStopsAtCallerCount) {
  EFI_FFA_PART_INFO_DESC  PartDesc[7];
  UINT32                  Count;

  Configure (7, LEGACY_DESC_SIZE, 5);
  SetMem (PartDesc, sizeof (PartDesc), 0xFF);
  Count = 3;

  ASSERT_EQ (ArmFfaLibPartitionInfoGetRegs (NULL, &Count, PartDesc), EFI_SUCCESS);
  EXPECT_EQ (Count, 3U);
  ExpectStartIndices ({ 0 });
  ExpectEntries ((UINT8 *)PartDesc, LEGACY_DESC_SIZE, 3);
  EXPECT_EQ (((UINT8 *)&PartDesc[3])[0], 0xFF);
}

TEST_F (ArmFfaLibPartInfoRegsTest, LegacySingleEntryPagesRequestEachIndexOnce) {
  EFI_FFA_PART_INFO_DESC  PartDesc[3];
  UINT32                  Count;

  Configure (3, LEGACY_DESC_SIZE, 1);
  Count = ARRAY_SIZE (PartDesc);

  ASSERT_EQ (ArmFfaLibPartitionInfoGetRegs (NULL, &Count, PartDesc), EFI_SUCCESS);
  EXPECT_EQ (Count, 3U);
  ExpectStartIndices ({ 0, 1, 2 });
  ExpectEntries ((UINT8 *)PartDesc, LEGACY_DESC_SIZE, 3);
}

TEST_F (ArmFfaLibPartInfoRegsTest, LegacyRejectsV1_3Descriptors) {
  EFI_FFA_PART_INFO_DESC  PartDesc[3];
  UINT32                  Count;

  Configure (3, V1_3_DESC_SIZE, 2);
  Count = ARRAY_SIZE (PartDesc);

  EXPECT_EQ (ArmFfaLibPartitionInfoGetRegs (NULL, &Count, PartDesc), EFI_INVALID_PARAMETER);
}

TEST_F (ArmFfaLibPartInfoRegsTest, LegacyRejectsInvalidParameters) {
  EFI_FFA_PART_INFO_DESC  PartDesc[1];
  UINT32                  Count;

  Configure (3, LEGACY_DESC_SIZE, 2);

  EXPECT_EQ (ArmFfaLibPartitionInfoGetRegs (NULL, NULL, PartDesc), EFI_INVALID_PARAMETER);
  Count = 1;
  EXPECT_EQ (ArmFfaLibPartitionInfoGetRegs (NULL, &Count, NULL), EFI_INVALID_PARAMETER);
  Count = 0;
  EXPECT_EQ (ArmFfaLibPartitionInfoGetRegs (NULL, &Count, PartDesc), EFI_BUFFER_TOO_SMALL);
  EXPECT_TRUE (Spmc.Requests.empty ());
}

TEST_F (ArmFfaLibPartInfoRegsTest, ExQueryReturnsRequiredSize) {
  UINT32  Size;
  UINT32  Count;

  Configure (5, V1_3_DESC_SIZE, 2);
  Size = 0;

  EXPECT_EQ (ArmFfaLibPartitionInfoGetRegsEx (NULL, &Size, &Count, NULL), EFI_BUFFER_TOO_SMALL);
  EXPECT_EQ (Count, 5U);
  EXPECT_EQ (Size, 5U * V1_3_DESC_SIZE);
  ExpectStartIndices ({ 0 });
}

TEST_F (ArmFfaLibPartInfoRegsTest, ExReadsAllPages) {
  UINT8   Buffer[5 * V1_3_DESC_SIZE];
  UINT32  Size;
  UINT32  Count;

  Configure (5, V1_3_DESC_SIZE, 2);
  Size = sizeof (Buffer);

  ASSERT_EQ (ArmFfaLibPartitionInfoGetRegsEx (NULL, &Size, &Count, Buffer), EFI_SUCCESS);
  EXPECT_EQ (Count, 5U);
  EXPECT_EQ (Size, sizeof (Buffer));
  ExpectStartIndices ({ 0, 2, 4 });
  ExpectEntries (Buffer, V1_3_DESC_SIZE, 5);
}

TEST_F (ArmFfaLibPartInfoRegsTest, ExSingleEntryPagesRequestEachIndexOnce) {
  UINT8   Buffer[3 * V1_3_DESC_SIZE];
  UINT32  Size;
  UINT32  Count;

  Configure (3, V1_3_DESC_SIZE, 1);
  Size = sizeof (Buffer);

  ASSERT_EQ (ArmFfaLibPartitionInfoGetRegsEx (NULL, &Size, &Count, Buffer), EFI_SUCCESS);
  EXPECT_EQ (Count, 3U);
  ExpectStartIndices ({ 0, 1, 2 });
  ExpectEntries (Buffer, V1_3_DESC_SIZE, 3);
}

TEST_F (ArmFfaLibPartInfoRegsTest, ExCopiesOnlyCompleteDescriptors) {
  UINT8   Buffer[5 * V1_3_DESC_SIZE];
  UINT32  Size;
  UINT32  Count;

  Configure (5, V1_3_DESC_SIZE, 2);
  SetMem (Buffer, sizeof (Buffer), 0xFF);
  Size = 2 * V1_3_DESC_SIZE + 4;

  ASSERT_EQ (ArmFfaLibPartitionInfoGetRegsEx (NULL, &Size, &Count, Buffer), EFI_SUCCESS);
  EXPECT_EQ (Count, 2U);
  EXPECT_EQ (Size, 2U * V1_3_DESC_SIZE);
  ExpectStartIndices ({ 0 });
  ExpectEntries (Buffer, V1_3_DESC_SIZE, 2);
  EXPECT_EQ (Buffer[2 * V1_3_DESC_SIZE], 0xFF);
}

TEST_F (ArmFfaLibPartInfoRegsTest, ExCopiesAcrossPagesUntilFull) {
  UINT8   Buffer[5 * V1_3_DESC_SIZE];
  UINT32  Size;
  UINT32  Count;

  Configure (5, V1_3_DESC_SIZE, 2);
  SetMem (Buffer, sizeof (Buffer), 0xFF);
  Size = 3 * V1_3_DESC_SIZE + 4;

  ASSERT_EQ (ArmFfaLibPartitionInfoGetRegsEx (NULL, &Size, &Count, Buffer), EFI_SUCCESS);
  EXPECT_EQ (Count, 3U);
  EXPECT_EQ (Size, 3U * V1_3_DESC_SIZE);
  ExpectStartIndices ({ 0, 2 });
  ExpectEntries (Buffer, V1_3_DESC_SIZE, 3);
  EXPECT_EQ (Buffer[3 * V1_3_DESC_SIZE], 0xFF);
}

TEST_F (ArmFfaLibPartInfoRegsTest, ExBufferSmallerThanOneDescriptor) {
  UINT8   Buffer[V1_3_DESC_SIZE - 1];
  UINT32  Size;
  UINT32  Count;

  Configure (5, V1_3_DESC_SIZE, 2);
  Size = sizeof (Buffer);

  EXPECT_EQ (ArmFfaLibPartitionInfoGetRegsEx (NULL, &Size, &Count, Buffer), EFI_BUFFER_TOO_SMALL);
  EXPECT_EQ (Count, 5U);
  EXPECT_EQ (Size, 5U * V1_3_DESC_SIZE);
}

TEST_F (ArmFfaLibPartInfoRegsTest, ExAcceptsLegacyDescriptors) {
  UINT8   Buffer[7 * LEGACY_DESC_SIZE];
  UINT32  Size;
  UINT32  Count;

  Configure (7, LEGACY_DESC_SIZE, 5);
  Size = sizeof (Buffer);

  ASSERT_EQ (ArmFfaLibPartitionInfoGetRegsEx (NULL, &Size, &Count, Buffer), EFI_SUCCESS);
  EXPECT_EQ (Count, 7U);
  EXPECT_EQ (Size, sizeof (Buffer));
  ExpectStartIndices ({ 0, 5 });
  ExpectEntries (Buffer, LEGACY_DESC_SIZE, 7);
}

TEST_F (ArmFfaLibPartInfoRegsTest, ExReportsMaximumCount) {
  UINT32  Size;
  UINT32  Count;

  Configure (MAX_UINT16 + 1, V1_3_DESC_SIZE, 2);
  Size = 0;

  EXPECT_EQ (ArmFfaLibPartitionInfoGetRegsEx (NULL, &Size, &Count, NULL), EFI_BUFFER_TOO_SMALL);
  EXPECT_EQ (Count, MAX_UINT16 + 1U);
  EXPECT_EQ (Size, (MAX_UINT16 + 1U) * V1_3_DESC_SIZE);
}

TEST_F (ArmFfaLibPartInfoRegsTest, ExRejectsInvalidParameters) {
  UINT8   Buffer[V1_3_DESC_SIZE];
  UINT32  Size;
  UINT32  Count;

  Configure (5, V1_3_DESC_SIZE, 2);

  Size = sizeof (Buffer);
  EXPECT_EQ (ArmFfaLibPartitionInfoGetRegsEx (NULL, NULL, &Count, Buffer), EFI_INVALID_PARAMETER);
  EXPECT_EQ (ArmFfaLibPartitionInfoGetRegsEx (NULL, &Size, NULL, Buffer), EFI_INVALID_PARAMETER);
  EXPECT_EQ (ArmFfaLibPartitionInfoGetRegsEx (NULL, &Size, &Count, NULL), EFI_INVALID_PARAMETER);
  EXPECT_TRUE (Spmc.Requests.empty ());
}

TEST_F (ArmFfaLibPartInfoRegsTest, ExRejectsDescriptorSizeChange) {
  UINT8   Buffer[5 * V1_3_DESC_SIZE];
  UINT32  Size;
  UINT32  Count;

  Configure (5, V1_3_DESC_SIZE, 2);
  Spmc.CorruptCall     = 2;
  Spmc.CorruptDescSize = LEGACY_DESC_SIZE;
  Size                 = sizeof (Buffer);

  EXPECT_EQ (ArmFfaLibPartitionInfoGetRegsEx (NULL, &Size, &Count, Buffer), EFI_INVALID_PARAMETER);
}

TEST_F (ArmFfaLibPartInfoRegsTest, RejectsCurrentIndexBeforeStartIndex) {
  Configure (7, LEGACY_DESC_SIZE, 2);
  Spmc.CorruptCall         = 2;
  Spmc.CorruptCurrentIndex = TRUE;
  Spmc.CurrentIndex        = 1;

  ExpectAllApisReject ();
}

TEST_F (ArmFfaLibPartInfoRegsTest, RejectsCurrentIndexBeyondLastIndex) {
  Configure (3, LEGACY_DESC_SIZE, 5);
  Spmc.CorruptCall         = 1;
  Spmc.CorruptCurrentIndex = TRUE;
  Spmc.CurrentIndex        = 3;

  ExpectAllApisReject ();
}

TEST_F (ArmFfaLibPartInfoRegsTest, RejectsPayloadBeyondRegisters) {
  //
  // Six 24-byte descriptors need 144 bytes, but x3 - x17 only hold 120.
  //
  Configure (7, LEGACY_DESC_SIZE, 6);

  ExpectAllApisReject ();
}

TEST_F (ArmFfaLibPartInfoRegsTest, RejectsDescriptorSizeBelowLegacy) {
  Configure (7, LEGACY_DESC_SIZE - 1, 5);

  ExpectAllApisReject ();
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
