/** @file
*
*  Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
*
*  SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HostBasedTestStubLib/IpmiStubLib.h>

#include <Library/UnitTestLib.h>
#include <Protocol/IpmiBlobTransfer.h>
#include "../InternalIpmiBlobTransfer.h"

#define UNIT_TEST_NAME     "IPMI Blob Transfer Unit Tests"
#define UNIT_TEST_VERSION  "1.0"

UINT8  InvalidCompletion[] = {
  0xC0,             // CompletionCode
  0xCF, 0xC2, 0x00, // OpenBMC OEN
};
#define INVALID_COMPLETION_SIZE  4 * sizeof(UINT8)

UINT8  NoDataResponse[] = {
  0x00,             // CompletionCode
  0xCF, 0xC2, 0x00, // OpenBMC OEN
};
#define NO_DATA_RESPONSE_SIZE  4 * sizeof(UINT8)

UINT8  BadOenResponse[] = {
  0x00,             // CompletionCode
  0xFF, 0xC2, 0x00, // Wrong OEN
};
#define BAD_OEN_RESPONSE_SIZE  4 * sizeof(UINT8)

UINT8  BadCrcResponse[] = {
  0x00,                   // CompletionCode
  0xCF, 0xC2, 0x00,       // OpenBMC OEN
  0x00, 0x00,             // CRC
  0x01, 0x00, 0x00, 0x00, // Data
};
#define BAD_CRC_RESPONSE_SIZE  10 * sizeof(UINT8)

UINT8  ValidNoDataResponse[] = {
  0x00,             // CompletionCode
  0xCF, 0xC2, 0x00, // OpenBMC OEN
};

#define VALID_NODATA_RESPONSE_SIZE  4 * sizeof(UINT8)

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
GoodCrc (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8   Data[5] = { 0x12, 0x34, 0x56, 0x78, 0x90 };
  UINTN   DataSize;
  UINT16  Crc;

  DataSize = sizeof (Data);

  Crc = CalculateCrc16Ccitt (Data, DataSize);

  UT_ASSERT_EQUAL (Crc, 0xB928);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
BadCrc (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8   Data[5] = { 0x12, 0x34, 0x56, 0x78, 0x90 };
  UINTN   DataSize;
  UINT16  Crc;

  DataSize = sizeof (Data);

  Crc = CalculateCrc16Ccitt (Data, DataSize);

  UT_ASSERT_NOT_EQUAL (Crc, 0x3409);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SendIpmiBadCompletion (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *ResponseData;
  UINT32      *ResponseDataSize;
  EFI_STATUS  Status;
  VOID        *MockResponseResults = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (INVALID_COMPLETION_SIZE);
  ResponseDataSize    = (UINT32 *)AllocateZeroPool (sizeof (UINT32));
  CopyMem (MockResponseResults, &InvalidCompletion, INVALID_COMPLETION_SIZE);

  MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, INVALID_COMPLETION_SIZE, EFI_SUCCESS);

  ResponseData = (UINT8 *)AllocateZeroPool (*ResponseDataSize);
  Status       = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandGetCount, NULL, 0, ResponseData, ResponseDataSize);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_PROTOCOL_ERROR);
  FreePool (MockResponseResults);
  FreePool (ResponseDataSize);
  FreePool (ResponseData);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SendIpmiNoDataResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *ResponseData;
  UINT32      *ResponseDataSize;
  EFI_STATUS  Status;
  VOID        *MockResponseResults = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (NO_DATA_RESPONSE_SIZE);
  ResponseDataSize    = (UINT32 *)AllocateZeroPool (sizeof (UINT32));
  CopyMem (MockResponseResults, &NoDataResponse, NO_DATA_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, NO_DATA_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  ResponseData = (UINT8 *)AllocateZeroPool (sizeof (NoDataResponse));
  Status       = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandGetCount, NULL, 0, ResponseData, ResponseDataSize);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (*ResponseDataSize, 0);
  FreePool (MockResponseResults);
  FreePool (ResponseDataSize);
  FreePool (ResponseData);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SendIpmiBadOenResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *ResponseData;
  UINT32      *ResponseDataSize;
  EFI_STATUS  Status;
  VOID        *MockResponseResults = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (BAD_OEN_RESPONSE_SIZE);
  ResponseDataSize    = (UINT32 *)AllocateZeroPool (sizeof (UINT32));
  CopyMem (MockResponseResults, &BadOenResponse, BAD_OEN_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, BAD_OEN_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  ResponseData = (UINT8 *)AllocateZeroPool (sizeof (BadOenResponse));
  Status       = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandGetCount, NULL, 0, ResponseData, ResponseDataSize);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_PROTOCOL_ERROR);
  FreePool (MockResponseResults);
  FreePool (ResponseDataSize);
  FreePool (ResponseData);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SendIpmiBadCrcResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *ResponseData;
  UINT32      *ResponseDataSize;
  EFI_STATUS  Status;
  VOID        *MockResponseResults = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (BAD_CRC_RESPONSE_SIZE));
  ResponseDataSize    = (UINT32 *)AllocateZeroPool (sizeof (UINT32));
  CopyMem (MockResponseResults, &BadCrcResponse, BAD_CRC_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, BAD_CRC_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  ResponseData = (UINT8 *)AllocateZeroPool (sizeof (BadCrcResponse));
  Status       = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandGetCount, NULL, 0, ResponseData, ResponseDataSize);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_CRC_ERROR);
  FreePool (MockResponseResults);
  FreePool (ResponseDataSize);
  FreePool (ResponseData);
  return UNIT_TEST_PASSED;
}

UINT8  ValidGetCountResponse[] = {
  0x00,                   // CompletionCode
  0xCF, 0xC2, 0x00,       // OpenBMC OEN
  0xA4, 0x78,             // CRC
  0x01, 0x00, 0x00, 0x00, // Data
};
#define VALID_GET_COUNT_RESPONSE_SIZE  10 * sizeof(UINT8)

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SendIpmiValidCountResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8       *ResponseData;
  UINT32      *ResponseDataSize;
  EFI_STATUS  Status;
  VOID        *MockResponseResults = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_GET_COUNT_RESPONSE_SIZE));
  ResponseDataSize    = (UINT32 *)AllocateZeroPool (sizeof (UINT32));
  CopyMem (MockResponseResults, &ValidGetCountResponse, VALID_GET_COUNT_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_GET_COUNT_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  ResponseData = AllocateZeroPool (sizeof (ValidGetCountResponse));
  Status       = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandGetCount, NULL, 0, ResponseData, ResponseDataSize);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  FreePool (MockResponseResults);
  FreePool (ResponseDataSize);
  FreePool (ResponseData);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
GetCountValidCountResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT32      Count;
  VOID        *MockResponseResults = NULL;

  Count = 0;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_GET_COUNT_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidGetCountResponse, VALID_GET_COUNT_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_GET_COUNT_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = IpmiBlobTransferGetCount (&Count);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (Count, 1);
  FreePool (MockResponseResults);
  return UNIT_TEST_PASSED;
}

UINT8  ValidEnumerateResponse[] = {
  0x00,                   // CompletionCode
  0xCF, 0xC2, 0x00,       // OpenBMC OEN
  0x81, 0x13,             // CRC
  0x2F, 0x73, 0x6D, 0x62, // Data = "/smbios"
  0x69, 0x6F, 0x73, 0x00,
};
#define VALID_ENUMERATE_RESPONSE_SIZE  14 * sizeof(UINT8)

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
EnumerateValidResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  CHAR8       *BlobId;
  VOID        *MockResponseResults = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_ENUMERATE_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidEnumerateResponse, VALID_ENUMERATE_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_ENUMERATE_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  BlobId = AllocateZeroPool (sizeof (CHAR8) * BLOB_MAX_DATA_PER_PACKET);

  Status = IpmiBlobTransferEnumerate (0, BlobId);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_MEM_EQUAL (BlobId, "/smbios", 7);
  FreePool (MockResponseResults);
  FreePool (BlobId);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
EnumerateInvalidBuffer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  CHAR8       *BlobId;
  EFI_STATUS  Status;
  VOID        *MockResponseResults = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_ENUMERATE_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidEnumerateResponse, VALID_ENUMERATE_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_ENUMERATE_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  BlobId = NULL;

  UT_EXPECT_ASSERT_FAILURE (IpmiBlobTransferEnumerate (0, BlobId), NULL);

  FreePool (MockResponseResults);
  return UNIT_TEST_PASSED;
}

UINT8  ValidOpenResponse[] = {
  0x00,             // CompletionCode
  0xCF, 0xC2, 0x00, // OpenBMC OEN
  0x93, 0xD1,       // CRC
  0x03, 0x00,       // SessionId = 3
};
#define VALID_OPEN_RESPONSE_SIZE  8 * sizeof(UINT8)

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
OpenValidResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  CHAR8       *BlobId;
  UINT16      Flags;
  UINT16      SessionId;
  VOID        *MockResponseResults  = NULL;
  VOID        *MockResponseResults2 = NULL;
  VOID        *MockResponseResults3 = NULL;

  Flags = BLOB_TRANSFER_STAT_OPEN_W;

  //
  // An open call effectively leads to three IPMI commands
  // 1. GetCount of blobs
  // 2. Enumerate the requested blob
  // 3. Open the requested blob
  //
  // So we'll push three Ipmi responses in this case
  //

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_OPEN_RESPONSE_SIZE));

  CopyMem (MockResponseResults, &ValidOpenResponse, VALID_OPEN_RESPONSE_SIZE);
  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_OPEN_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  MockResponseResults2 = (UINT8 *)AllocateZeroPool (sizeof (VALID_ENUMERATE_RESPONSE_SIZE));
  CopyMem (MockResponseResults2, &ValidEnumerateResponse, VALID_ENUMERATE_RESPONSE_SIZE);
  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults2, VALID_ENUMERATE_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  MockResponseResults3 = (UINT8 *)AllocateZeroPool (sizeof (VALID_GET_COUNT_RESPONSE_SIZE));
  CopyMem (MockResponseResults3, &ValidGetCountResponse, VALID_GET_COUNT_RESPONSE_SIZE);
  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults3, VALID_GET_COUNT_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  BlobId = "/smbios";

  Status = IpmiBlobTransferOpen (BlobId, Flags, &SessionId);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (SessionId, 3);
  FreePool (MockResponseResults);
  return UNIT_TEST_PASSED;
}

UINT8  ValidReadResponse[] = {
  0x00,                   // CompletionCode
  0xCF, 0xC2, 0x00,       // OpenBMC OEN
  0x21, 0x6F,             // CRC
  0x00, 0x01, 0x02, 0x03, // Data to read
};

#define VALID_READ_RESPONSE_SIZE  10 * sizeof(UINT8)

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
ReadValidResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       *ResponseData;
  UINT8       ExpectedDataResponse[4] = { 0x00, 0x01, 0x02, 0x03 };
  VOID        *MockResponseResults    = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_READ_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidReadResponse, VALID_READ_RESPONSE_SIZE);
  ResponseData = AllocateZeroPool (sizeof (ValidReadResponse));

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_READ_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = IpmiBlobTransferRead (0, 0, 4, ResponseData);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_MEM_EQUAL (ResponseData, ExpectedDataResponse, 4);
  FreePool (MockResponseResults);
  FreePool (ResponseData);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
ReadInvalidBuffer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8       *ResponseData;
  EFI_STATUS  Status;
  VOID        *MockResponseResults = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_READ_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidReadResponse, VALID_READ_RESPONSE_SIZE);
  ResponseData = NULL;

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_READ_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  UT_EXPECT_ASSERT_FAILURE (IpmiBlobTransferRead (0, 0, 4, ResponseData), NULL);

  FreePool (MockResponseResults);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
WriteValidResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       SendData[4]          = { 0x00, 0x01, 0x02, 0x03 };
  VOID        *MockResponseResults = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_NODATA_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidNoDataResponse, VALID_NODATA_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_NODATA_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = IpmiBlobTransferWrite (0, 0, SendData, 4);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  FreePool (MockResponseResults);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
CommitValidResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       SendData[4]          = { 0x00, 0x01, 0x02, 0x03 };
  VOID        *MockResponseResults = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_NODATA_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidNoDataResponse, VALID_NODATA_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_NODATA_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = IpmiBlobTransferCommit (0, 4, SendData);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  FreePool (MockResponseResults);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
CloseValidResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  VOID        *MockResponseResults = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_NODATA_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidNoDataResponse, VALID_NODATA_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_NODATA_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = IpmiBlobTransferClose (1);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  FreePool (MockResponseResults);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
DeleteValidResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  VOID        *MockResponseResults = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_NODATA_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidNoDataResponse, VALID_NODATA_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_NODATA_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = IpmiBlobTransferDelete ("/smbios");

  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  FreePool (MockResponseResults);
  return UNIT_TEST_PASSED;
}

UINT8  ValidBlobStatResponse[] = {
  0x00,                   // CompletionCode
  0xCF, 0xC2, 0x00,       // OpenBMC OEN
  0x1F, 0x4F,             // Crc
  0x01, 0x00,             // BlobState
  0x02, 0x03, 0x04, 0x05, // BlobSize
  0x04,                   // MetaDataLen
  0x06, 0x07, 0x08, 0x09, // MetaData
};

#define VALID_BLOB_STAT_RESPONSE_SIZE  17 * sizeof(UINT8)

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
BlobStatValidResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT16      *BlobState;
  UINT32      *Size;
  UINT8       *MetadataLength;
  UINT8       *Metadata;
  UINT8       *ExpectedMetadata;
  CHAR8       *BlobId;
  VOID        *MockResponseResults = NULL;

  BlobState        = AllocateZeroPool (sizeof (UINT16));
  Size             = AllocateZeroPool (sizeof (UINT32));
  BlobId           = "BlobId";
  MetadataLength   = AllocateZeroPool (sizeof (UINT8));
  Metadata         = AllocateZeroPool (4 * sizeof (UINT8));
  ExpectedMetadata = AllocateZeroPool (4 * sizeof (UINT8));

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_BLOB_STAT_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidBlobStatResponse, VALID_BLOB_STAT_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_BLOB_STAT_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = IpmiBlobTransferStat (BlobId, BlobState, Size, MetadataLength, Metadata);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (*BlobState, 1);
  UT_ASSERT_EQUAL (*Size, 0x05040302);
  UT_ASSERT_EQUAL (*MetadataLength, 4);
  UT_ASSERT_MEM_EQUAL (Metadata, ExpectedMetadata, 4);
  FreePool (MockResponseResults);
  FreePool (BlobState);
  FreePool (Size);
  FreePool (MetadataLength);
  FreePool (Metadata);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
BlobStatInvalidBuffer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8       *Metadata;
  EFI_STATUS  Status;
  VOID        *MockResponseResults = NULL;

  Metadata = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_BLOB_STAT_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidBlobStatResponse, VALID_BLOB_STAT_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_BLOB_STAT_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  UT_EXPECT_ASSERT_FAILURE (IpmiBlobTransferStat (NULL, 0, 0, 0, Metadata), NULL);

  FreePool (MockResponseResults);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SessionStatValidResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT16      *BlobState;
  UINT32      *Size;
  UINT8       *MetadataLength;
  UINT8       *Metadata;
  UINT8       *ExpectedMetadata;
  VOID        *MockResponseResults = NULL;

  BlobState        = AllocateZeroPool (sizeof (UINT16));
  Size             = AllocateZeroPool (sizeof (UINT32));
  MetadataLength   = AllocateZeroPool (sizeof (UINT8));
  Metadata         = AllocateZeroPool (4 * sizeof (UINT8));
  ExpectedMetadata = AllocateZeroPool (4 * sizeof (UINT8));

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_BLOB_STAT_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidBlobStatResponse, VALID_BLOB_STAT_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_BLOB_STAT_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = IpmiBlobTransferSessionStat (0, BlobState, Size, MetadataLength, Metadata);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (*BlobState, 1);
  UT_ASSERT_EQUAL (*Size, 0x05040302);
  UT_ASSERT_EQUAL (*MetadataLength, 4);
  UT_ASSERT_MEM_EQUAL (Metadata, ExpectedMetadata, 4);
  FreePool (MockResponseResults);
  FreePool (BlobState);
  FreePool (Size);
  FreePool (MetadataLength);
  FreePool (Metadata);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SessionStatInvalidBuffer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8       *Metadata;
  EFI_STATUS  Status;
  VOID        *MockResponseResults = NULL;

  Metadata = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_BLOB_STAT_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidBlobStatResponse, VALID_BLOB_STAT_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_BLOB_STAT_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  UT_EXPECT_ASSERT_FAILURE (IpmiBlobTransferSessionStat (0, 0, 0, 0, Metadata), NULL);

  FreePool (MockResponseResults);
  return UNIT_TEST_PASSED;
}

/**
  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
WriteMetaValidResponse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  VOID        *MockResponseResults = NULL;

  MockResponseResults = (UINT8 *)AllocateZeroPool (sizeof (VALID_NODATA_RESPONSE_SIZE));
  CopyMem (MockResponseResults, &ValidNoDataResponse, VALID_NODATA_RESPONSE_SIZE);

  Status = MockIpmiSubmitCommand ((UINT8 *)MockResponseResults, VALID_NODATA_RESPONSE_SIZE, EFI_SUCCESS);
  if (EFI_ERROR (Status)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = IpmiBlobTransferWriteMeta (0, 0, NULL, 0);

  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  FreePool (MockResponseResults);
  return UNIT_TEST_PASSED;
}

/**
  Initialize the unit test framework, suite, and unit tests for the
  sample unit tests and run the unit tests.
  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
EFI_STATUS
EFIAPI
SetupAndRunUnitTests (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      IpmiBlobTransfer;

  Framework = NULL;
  DEBUG ((DEBUG_INFO, "%a: v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to setup Test Framework. Exiting with status = %r\n", Status));
    ASSERT (FALSE);
    return Status;
  }

  //
  // Populate the Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&IpmiBlobTransfer, Framework, "IPMI Blob Transfer Tests", "UnitTest.IpmiBlobTransferCB", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for IPMI Blob Transfer Tests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }

  // CalculateCrc16Ccitt
  Status = AddTestCase (IpmiBlobTransfer, "Test CRC Calculation", "GoodCrc", GoodCrc, NULL, NULL, NULL);
  Status = AddTestCase (IpmiBlobTransfer, "Test Bad CRC Calculation", "BadCrc", BadCrc, NULL, NULL, NULL);
  // IpmiBlobTransferSendIpmi
  Status = AddTestCase (IpmiBlobTransfer, "Send IPMI returns bad completion", "SendIpmiBadCompletion", SendIpmiBadCompletion, NULL, NULL, NULL);
  Status = AddTestCase (IpmiBlobTransfer, "Send IPMI returns successfully with no data", "SendIpmiNoDataResponse", SendIpmiNoDataResponse, NULL, NULL, NULL);
  Status = AddTestCase (IpmiBlobTransfer, "Send IPMI returns successfully with bad OEN", "SendIpmiBadOenResponse", SendIpmiBadOenResponse, NULL, NULL, NULL);
  Status = AddTestCase (IpmiBlobTransfer, "Send IPMI returns successfully with bad CRC", "SendIpmiBadCrcResponse", SendIpmiBadCrcResponse, NULL, NULL, NULL);
  Status = AddTestCase (IpmiBlobTransfer, "Send IPMI returns with valid GetCount data", "SendIpmiValidCountResponse", SendIpmiValidCountResponse, NULL, NULL, NULL);
  // IpmiBlobTransferGetCount
  Status = AddTestCase (IpmiBlobTransfer, "GetCount call with valid data", "GetCountValidCountResponse", GetCountValidCountResponse, NULL, NULL, NULL);
  // IpmiBlobTransferEnumerate
  Status = AddTestCase (IpmiBlobTransfer, "Enumerate call with valid data", "EnumerateValidResponse", EnumerateValidResponse, NULL, NULL, NULL);
  Status = AddTestCase (IpmiBlobTransfer, "Enumerate call with invalid output buffer", "EnumerateInvalidBuffer", EnumerateInvalidBuffer, NULL, NULL, NULL);
  // IpmiBlobTransferOpen
  Status = AddTestCase (IpmiBlobTransfer, "Open call with valid data", "OpenValidResponse", OpenValidResponse, NULL, NULL, NULL);
  // IpmiBlobTransferRead
  Status = AddTestCase (IpmiBlobTransfer, "Read call with valid data", "ReadValidResponse", ReadValidResponse, NULL, NULL, NULL);
  Status = AddTestCase (IpmiBlobTransfer, "Read call with invalid buffer", "ReadInvalidBuffer", ReadInvalidBuffer, NULL, NULL, NULL);
  // IpmiBlobTransferWrite
  Status = AddTestCase (IpmiBlobTransfer, "Write call with valid data", "WriteValidResponse", WriteValidResponse, NULL, NULL, NULL);
  // IpmiBlobTransferCommit
  Status = AddTestCase (IpmiBlobTransfer, "Commit call with valid data", "CommitValidResponse", CommitValidResponse, NULL, NULL, NULL);
  // IpmiBlobTransferClose
  Status = AddTestCase (IpmiBlobTransfer, "Close call with valid data", "CloseValidResponse", CloseValidResponse, NULL, NULL, NULL);
  // IpmiBlobTransferDelete
  Status = AddTestCase (IpmiBlobTransfer, "Delete call with valid data", "DeleteValidResponse", DeleteValidResponse, NULL, NULL, NULL);
  // IpmiBlobTransferStat
  Status = AddTestCase (IpmiBlobTransfer, "Blob Stat call with valid data", "BlobStatValidResponse", BlobStatValidResponse, NULL, NULL, NULL);
  Status = AddTestCase (IpmiBlobTransfer, "Blob Stat call with invalid buffer", "BlobStatInvalidBuffer", BlobStatInvalidBuffer, NULL, NULL, NULL);
  // IpmiBlobTransferSessionStat
  Status = AddTestCase (IpmiBlobTransfer, "Session Stat call with valid data", "SessionStatValidResponse", SessionStatValidResponse, NULL, NULL, NULL);
  Status = AddTestCase (IpmiBlobTransfer, "Session Stat call with invalid buffer", "SessionStatInvalidBuffer", SessionStatInvalidBuffer, NULL, NULL, NULL);
  // IpmiBlobTransferWriteMeta
  Status = AddTestCase (IpmiBlobTransfer, "WriteMeta call with valid data", "WriteMetaValidResponse", WriteMetaValidResponse, NULL, NULL, NULL);

  // Execute the tests.
  Status = RunAllTestSuites (Framework);
  return Status;
}

/**
  Standard UEFI entry point for target based
  unit test execution from UEFI Shell.
**/
EFI_STATUS
EFIAPI
BaseLibUnitTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return SetupAndRunUnitTests ();
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  return SetupAndRunUnitTests ();
}
