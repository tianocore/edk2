/** @file
  GoogleTest for MmCommunicationDxe.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Protocol/MmControl.h>
  #include <Protocol/SmmControl2.h>
  #include <Protocol/MmCommunication.h>
  #include <Pi/PiMultiPhase.h>
  #include <Guid/MmCommBuffer.h>

  //
  // Declare the function under test (defined in MmCommunicationDxe.c)
  //
  EFI_STATUS
  EFIAPI
  ProcessCommunicationBuffer (
    IN OUT VOID   *CommBuffer,
    IN OUT UINTN  *CommSize OPTIONAL
    );

  //
  // Globals defined in MmCommunicationDxe.c that we need to set up
  //
  extern MM_COMM_BUFFER             mMmCommonBuffer;
  extern EFI_SMM_CONTROL2_PROTOCOL  *mSmmControl2;
}

////////////////////////////////////////////////////////////////////////
// Symbol Definitions
// These functions are not directly under test - but required to compile
////////////////////////////////////////////////////////////////////////

//
// Mock Trigger function for SmmControl2 protocol
//
static EFI_STATUS EFIAPI
MockTrigger (
  IN CONST EFI_MM_CONTROL_PROTOCOL  *This,
  IN OUT UINT8                      *CommandPort       OPTIONAL,
  IN OUT UINT8                      *DataPort          OPTIONAL,
  IN BOOLEAN                        Periodic           OPTIONAL,
  IN UINTN                          ActivationInterval OPTIONAL
  )
{
  return EFI_SUCCESS;
}

//
// Mock Deactivate (Clear) function
//
static EFI_STATUS EFIAPI
MockClear (
  IN CONST EFI_MM_CONTROL_PROTOCOL  *This,
  IN BOOLEAN                        Periodic OPTIONAL
  )
{
  return EFI_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////

#define COMM_BUFFER_PAGES  4  // 16 KiB common buffer
#define COMM_BUFFER_SIZE   EFI_PAGES_TO_SIZE (COMM_BUFFER_PAGES)

////////////////////////////////////////////////////////////////////////
// MmCommunicationOverflowTest Tests
////////////////////////////////////////////////////////////////////////

class MmCommunicationOverflowTest : public ::testing::Test {
public:
  UINT8 mCommonBuffer[COMM_BUFFER_SIZE];
  MM_COMM_BUFFER_STATUS mCommonBufferStatus;
  EFI_MM_CONTROL_PROTOCOL mMockSmmControl2;
  UINT8 mCommBuffer[COMM_BUFFER_SIZE];

protected:
  virtual void
  SetUp (
    )
  {
    ZeroMem (mCommonBuffer, sizeof (mCommonBuffer));
    ZeroMem (&mCommonBufferStatus, sizeof (mCommonBufferStatus));
    ZeroMem (mCommBuffer, sizeof (mCommBuffer));

    // Set up the global MM common buffer struct
    mMmCommonBuffer.PhysicalStart = (EFI_PHYSICAL_ADDRESS)(UINTN)mCommonBuffer;
    mMmCommonBuffer.NumberOfPages = COMM_BUFFER_PAGES;
    mMmCommonBuffer.Status        = (EFI_PHYSICAL_ADDRESS)(UINTN)&mCommonBufferStatus;

    // Set up the mock SmmControl2 protocol
    mMockSmmControl2.Trigger              = MockTrigger;
    mMockSmmControl2.Clear                = MockClear;
    mMockSmmControl2.MinimumTriggerPeriod = 0;
    mSmmControl2                          = &mMockSmmControl2;
  }

  virtual void
  TearDown (
    )
  {
    // Clean up any resources or variables
  }
};

// Test Description:
// Normal V1 path works with small MessageLength
TEST_F (MmCommunicationOverflowTest, V1NormalMessageLengthSucceeds) {
  EFI_MM_COMMUNICATE_HEADER  *Header = (EFI_MM_COMMUNICATE_HEADER *)mCommBuffer;

  ZeroMem (&Header->HeaderGuid, sizeof (EFI_GUID));
  Header->MessageLength = 64;

  UINTN  CommSize = OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data) + 64;

  EFI_STATUS  Status = ProcessCommunicationBuffer (mCommBuffer, &CommSize);

  // BufferSize = 24 + 64 = 88, well within 16KiB
  ASSERT_EQ (Status, EFI_SUCCESS);
}

// Test Description:
// V1 path - CWE-190 integer overflow wraps BufferSize to 0.
// MessageLength = MAX_UINT64 - OFFSET_OF(...) + 1 causes addition to wrap to 0.
TEST_F (MmCommunicationOverflowTest, V1MessageLengthOverflowWrapsToZero) {
  EFI_MM_COMMUNICATE_HEADER  *Header = (EFI_MM_COMMUNICATE_HEADER *)mCommBuffer;

  ZeroMem (&Header->HeaderGuid, sizeof (EFI_GUID));

  //
  // Craft MessageLength so that OFFSET_OF(Data) + MessageLength = 0 (wraps)
  // OFFSET_OF(EFI_MM_COMMUNICATE_HEADER, Data) = 0x18 (24)
  // MAX_UINT64 - 0x18 + 1 = 0xFFFFFFFFFFFFFFE8
  //
  Header->MessageLength = MAX_UINT64 - OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data) + 1;

  EFI_STATUS  Status = ProcessCommunicationBuffer (mCommBuffer, NULL);

  // Without fix: BufferSize wraps to 0, passes bounds check, returns EFI_SUCCESS
  ASSERT_EQ (Status, EFI_INVALID_PARAMETER);
}

// Test Description:
// V1 path - Integer overflow wraps BufferSize to a small nonzero value (0x08)
TEST_F (MmCommunicationOverflowTest, V1MessageLengthOverflowWrapsToSmallValue) {
  EFI_MM_COMMUNICATE_HEADER  *Header = (EFI_MM_COMMUNICATE_HEADER *)mCommBuffer;

  ZeroMem (&Header->HeaderGuid, sizeof (EFI_GUID));

  //
  // MessageLength = MAX_UINT64 - 0x18 + 1 + 8 = MAX_UINT64 - 0x0F
  // Result: OFFSET_OF(Data) + MessageLength = 0x18 + (MAX_UINT64 - 0x0F) = 0x08 (wraps)
  //
  Header->MessageLength = MAX_UINT64 - OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data) + 1 + 8;

  EFI_STATUS  Status = ProcessCommunicationBuffer (mCommBuffer, NULL);

  // Without fix: BufferSize = 8, passes bounds check, returns EFI_SUCCESS
  ASSERT_EQ (Status, EFI_INVALID_PARAMETER);
}

// Test Description:
// V1 path - Large MessageLength without overflow is rejected by bounds check
TEST_F (MmCommunicationOverflowTest, V1LargeMessageLengthWithoutOverflowIsRejected) {
  EFI_MM_COMMUNICATE_HEADER  *Header = (EFI_MM_COMMUNICATE_HEADER *)mCommBuffer;

  ZeroMem (&Header->HeaderGuid, sizeof (EFI_GUID));

  //
  // Set MessageLength larger than common buffer but not large enough to overflow.
  // BufferSize = 0x18 + 0x10000 = 0x10018, which exceeds 16KiB (0x4000).
  //
  Header->MessageLength = 0x10000;

  EFI_STATUS  Status = ProcessCommunicationBuffer (mCommBuffer, NULL);

  ASSERT_EQ (Status, EFI_INVALID_PARAMETER);
}

// Test Description:
// NULL CommBuffer is rejected
TEST_F (MmCommunicationOverflowTest, NullCommBufferReturnsInvalidParameter) {
  EFI_STATUS  Status = ProcessCommunicationBuffer (NULL, NULL);

  ASSERT_EQ (Status, EFI_INVALID_PARAMETER);
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
