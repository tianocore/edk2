/** @file
  Application for Parallelhash Function Validation.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

//
// Parallelhash Test Sample common parameters.
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINTN  OutputByteLen = 64;

//
// Parallelhash Test Sample #1 from NIST Special Publication 800-185.
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  InputSample1[] = {
  // input data of sample1.
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27
};
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        InputSample1ByteLen   = 24;        // Length of sample1 input data in bytes.
GLOBAL_REMOVE_IF_UNREFERENCED CONST VOID   *CustomizationSample1 = "";        // Customization string (S) of sample1.
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        CustomSample1ByteLen  = 0;         // Customization string length of sample1 in bytes.
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        BlockSizeSample1      = 8;         // Block size of sample1.
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  ExpectOutputSample1[] = {
  // Expected output data of sample1.
  0xbc, 0x1e, 0xf1, 0x24, 0xda, 0x34, 0x49, 0x5e, 0x94, 0x8e, 0xad, 0x20, 0x7d, 0xd9, 0x84, 0x22,
  0x35, 0xda, 0x43, 0x2d, 0x2b, 0xbc, 0x54, 0xb4, 0xc1, 0x10, 0xe6, 0x4c, 0x45, 0x11, 0x05, 0x53,
  0x1b, 0x7f, 0x2a, 0x3e, 0x0c, 0xe0, 0x55, 0xc0, 0x28, 0x05, 0xe7, 0xc2, 0xde, 0x1f, 0xb7, 0x46,
  0xaf, 0x97, 0xa1, 0xd0, 0x01, 0xf4, 0x3b, 0x82, 0x4e, 0x31, 0xb8, 0x76, 0x12, 0x41, 0x04, 0x29
};

//
// Parallelhash Test Sample #2 from NIST Special Publication 800-185.
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  *InputSample2         = InputSample1;               // Input of sample2 is same as sample1.
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        InputSample2ByteLen   = 24;                         // Length of sample2 input data in bytes.
GLOBAL_REMOVE_IF_UNREFERENCED CONST VOID   *CustomizationSample2 = "Parallel Data";            // Customization string (S) of sample2.
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        CustomSample2ByteLen  = 13;                         // Customization string length of sample2 in bytes.
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        BlockSizeSample2      = 8;                          // Block size of sample2.
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  ExpectOutputSample2[] = {
  // Expected output data of sample2.
  0xcd, 0xf1, 0x52, 0x89, 0xb5, 0x4f, 0x62, 0x12, 0xb4, 0xbc, 0x27, 0x05, 0x28, 0xb4, 0x95, 0x26,
  0x00, 0x6d, 0xd9, 0xb5, 0x4e, 0x2b, 0x6a, 0xdd, 0x1e, 0xf6, 0x90, 0x0d, 0xda, 0x39, 0x63, 0xbb,
  0x33, 0xa7, 0x24, 0x91, 0xf2, 0x36, 0x96, 0x9c, 0xa8, 0xaf, 0xae, 0xa2, 0x9c, 0x68, 0x2d, 0x47,
  0xa3, 0x93, 0xc0, 0x65, 0xb3, 0x8e, 0x29, 0xfa, 0xe6, 0x51, 0xa2, 0x09, 0x1c, 0x83, 0x31, 0x10
};

//
// Parallelhash Test Sample #3 from NIST Special Publication 800-185.
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  InputSample3[] = {
  // input data of sample3.
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x10, 0x11, 0x12, 0x13,
  0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2a, 0x2b, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x50, 0x51, 0x52, 0x53,
  0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b
};
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        InputSample3ByteLen   = 72;                         // Length of sample3 input data in bytes.
GLOBAL_REMOVE_IF_UNREFERENCED CONST VOID   *CustomizationSample3 = "Parallel Data";            // Customization string (S) of sample3.
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        CustomSample3ByteLen  = 13;                         // Customization string length of sample3 in bytes.
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        BlockSizeSample3      = 12;                         // Block size of sample3.
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  ExpectOutputSample3[] = {
  // Expected output data of sample3.
  0x69, 0xd0, 0xfc, 0xb7, 0x64, 0xea, 0x05, 0x5d, 0xd0, 0x93, 0x34, 0xbc, 0x60, 0x21, 0xcb, 0x7e,
  0x4b, 0x61, 0x34, 0x8d, 0xff, 0x37, 0x5d, 0xa2, 0x62, 0x67, 0x1c, 0xde, 0xc3, 0xef, 0xfa, 0x8d,
  0x1b, 0x45, 0x68, 0xa6, 0xcc, 0xe1, 0x6b, 0x1c, 0xad, 0x94, 0x6d, 0xdd, 0xe2, 0x7f, 0x6c, 0xe2,
  0xb8, 0xde, 0xe4, 0xcd, 0x1b, 0x24, 0x85, 0x1e, 0xbf, 0x00, 0xeb, 0x90, 0xd4, 0x38, 0x13, 0xe9
};

UNIT_TEST_STATUS
EFIAPI
TestVerifyParallelHash256HashAll (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    Output[64];

  //
  // Test #1 using sample1.
  //
  Status = ParallelHash256HashAll (
             InputSample1,
             InputSample1ByteLen,
             BlockSizeSample1,
             Output,
             OutputByteLen,
             CustomizationSample1,
             CustomSample1ByteLen
             );
  UT_ASSERT_TRUE (Status);

  // Check the output with the expected output.
  UT_ASSERT_MEM_EQUAL (Output, ExpectOutputSample1, OutputByteLen);

  //
  // Test #2 using sample2.
  //
  Status = ParallelHash256HashAll (
             InputSample2,
             InputSample2ByteLen,
             BlockSizeSample2,
             Output,
             OutputByteLen,
             CustomizationSample2,
             CustomSample2ByteLen
             );
  UT_ASSERT_TRUE (Status);

  // Check the output with the expected output.
  UT_ASSERT_MEM_EQUAL (Output, ExpectOutputSample2, OutputByteLen);

  //
  // Test #3 using sample3.
  //
  Status = ParallelHash256HashAll (
             InputSample3,
             InputSample3ByteLen,
             BlockSizeSample3,
             Output,
             OutputByteLen,
             CustomizationSample3,
             CustomSample3ByteLen
             );
  UT_ASSERT_TRUE (Status);

  // Check the output with the expected output.
  UT_ASSERT_MEM_EQUAL (Output, ExpectOutputSample3, OutputByteLen);

  return EFI_SUCCESS;
}

TEST_DESC  mParallelhashTest[] = {
  //
  // -----Description------------------------------Class----------------------Function-----------------Pre---Post--Context
  //
  { "TestVerifyParallelHash256HashAll()", "CryptoPkg.BaseCryptLib.ParallelHash256HashAll", TestVerifyParallelHash256HashAll, NULL, NULL, NULL },
};

UINTN  mParallelhashTestNum = ARRAY_SIZE (mParallelhashTest);
