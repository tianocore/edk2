/** @file
  Unit tests for VortexAI neural network functions

  Copyright (c) 2026, Americo Simoes. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>

#define TEST_PASS     0
#define TEST_FAIL     1
#define SCALE_FACTOR  1000000

//
// Activation functions (copied from VortexAI.c for testing)
//
UINT64
EFIAPI
Relu (
  IN UINT64  x
  )
{
  if (x > SCALE_FACTOR) {
    return x;
  }

  return 0;
}

UINT64
EFIAPI
Sigmoid (
  IN UINT64  x
  )
{
  if (x > 5 * SCALE_FACTOR) {
    return SCALE_FACTOR;
  }

  if (x < -5 * SCALE_FACTOR) {
    return 0;
  }

  return (x + 5 * SCALE_FACTOR) / 10;
}

//
// Test counters
//
STATIC UINT32  gTestsPassed = 0;
STATIC UINT32  gTestsFailed = 0;

/**
  Print test result
**/
VOID
PrintTestResult (
  IN CHAR16  *TestName,
  IN UINTN   Result
  )
{
  if (Result == TEST_PASS) {
    Print (L"  ✅ %s\n", TestName);
    gTestsPassed++;
  } else {
    Print (L"  ❌ %s\n", TestName);
    gTestsFailed++;
  }
}

/**
  ReLU test
**/
UINTN
TestRelu (
  VOID
  )
{
  if (Relu (500000) != 0) {
    return TEST_FAIL;
  }

  if (Relu (1500000) != 1500000) {
    return TEST_FAIL;
  }

  return TEST_PASS;
}

/**
  Sigmoid test
**/
UINTN
TestSigmoid (
  VOID
  )
{
  if (Sigmoid (10 * SCALE_FACTOR) != SCALE_FACTOR) {
    return TEST_FAIL;
  }

  return TEST_PASS;
}

/**
  Main test entry point
**/
EFI_STATUS
EFIAPI
VortexAITestMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  Print (L"\n========================================\n");
  Print (L"  VortexAI Unit Tests\n");
  Print (L"========================================\n\n");

  PrintTestResult (L"ReLU function", TestRelu ());
  PrintTestResult (L"Sigmoid approximation", TestSigmoid ());

  Print (L"\n========================================\n");
  Print (L"Tests Passed: %d\n", gTestsPassed);
  Print (L"Tests Failed: %d\n", gTestsFailed);
  Print (L"========================================\n");

  if (gTestsFailed == 0) {
    Print (L"✅ ALL TESTS PASSED\n");
    return EFI_SUCCESS;
  } else {
    Print (L"❌ SOME TESTS FAILED\n");
    return EFI_DEVICE_ERROR;
  }
}
