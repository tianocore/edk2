/** @file
  GoogleTestLib class with APIs from the googletest project

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef GOOGLE_TEST_LIB_H_
#define GOOGLE_TEST_LIB_H_

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>

using ::testing::Throws;
using ::testing::ThrowsMessage;
using ::testing::HasSubstr;

//
// Extended macros for testing exceptions with a specific description string
// in the exception message.  Typically used to check that the expression
// that generates an ASSERT() matches the expected expression.
//
#define EXPECT_THROW_MESSAGE(statement, description)            \
  EXPECT_THAT (                                                 \
    []() { statement; },                                        \
    ThrowsMessage<std::runtime_error>(HasSubstr (description))  \
    )
#define ASSERT_THROW_MESSAGE(statement, description)            \
  ASSERT_THAT (                                                 \
    []() { statement; },                                        \
    ThrowsMessage<std::runtime_error>(HasSubstr (description))  \
    )

extern "C" {
  #include <Uefi.h>
}

//////////////////////////////////////////////////////////////////////////////
// Below are the action extensions to GoogleTest and gmock for EDK2 types.
// These actions are intended to be used in EXPECT_CALL (and related gmock
// macros) to support assignments to output arguments in the expected call.
//

// Action to support pointer types to a buffer (such as UINT8* or VOID*)
ACTION_TEMPLATE (
  SetArgBuffer,
  HAS_1_TEMPLATE_PARAMS (size_t, ArgNum),
  AND_2_VALUE_PARAMS (Buffer, ByteSize)
  ) {
  auto  ArgBuffer = std::get<ArgNum>(args);

  std::memcpy (ArgBuffer, Buffer, ByteSize);
}

//////////////////////////////////////////////////////////////////////////////
// Below are the matcher extensions to GoogleTest and gmock for EDK2 types.
// These matchers are intended to be used in EXPECT_CALL (and related gmock
// macros) to support comparisons to input arguments in the expected call.
//
// Note that these matchers can also be used in the EXPECT_THAT or ASSERT_THAT
// macros to compare whether two values are equal.
//

// Matcher to support pointer types to a buffer (such as UINT8* or VOID* or
// any structure pointer)
MATCHER_P2 (
  BufferEq,
  Buffer,
  ByteSize,
  std::string ("buffer data to ") + (negation ? "not " : "") + "be the same"
  ) {
  UINT8  *Actual   = (UINT8 *)arg;
  UINT8  *Expected = (UINT8 *)Buffer;

  for (size_t i = 0; i < ByteSize; i++) {
    if (Actual[i] != Expected[i]) {
      *result_listener << "byte at offset " << i
      << " does not match expected. [" << std::hex
      << "Actual: 0x" << std::setw (2) << std::setfill ('0')
      << (unsigned int)Actual[i] << ", "
      << "Expected: 0x" << std::setw (2) << std::setfill ('0')
      << (unsigned int)Expected[i] << "]";
      return false;
    }
  }

  *result_listener << "all bytes match";
  return true;
}

// Matcher to support CHAR16* type
MATCHER_P (
  Char16StrEq,
  String,
  std::string ("strings to ") + (negation ? "not " : "") + "be the same"
  ) {
  CHAR16  *Actual   = (CHAR16 *)arg;
  CHAR16  *Expected = (CHAR16 *)String;

  for (size_t i = 0; Actual[i] != 0; i++) {
    if (Actual[i] != Expected[i]) {
      *result_listener << "character at offset " << i
      << " does not match expected. [" << std::hex
      << "Actual: 0x" << std::setw (4) << std::setfill ('0')
      << Actual[i];

      if (std::isprint (Actual[i])) {
        *result_listener << " ('" << (char)Actual[i] << "')";
      }

      *result_listener << ", Expected: 0x" << std::setw (4) << std::setfill ('0')
      << Expected[i];

      if (std::isprint (Expected[i])) {
        *result_listener << " ('" << (char)Expected[i] << "')";
      }

      *result_listener << "]";

      return false;
    }
  }

  *result_listener << "strings match";
  return true;
}

#endif
