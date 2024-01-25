/** @file
  Tests for Ip6Option.c.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <gtest/gtest.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include "../Ip6Impl.h"
  #include "../Ip6Option.h"
}

/////////////////////////////////////////////////////////////////////////
// Defines
///////////////////////////////////////////////////////////////////////

#define IP6_PREFIX_INFO_OPTION_DATA_LEN    32
#define OPTION_HEADER_IP6_PREFIX_DATA_LEN  (sizeof (IP6_OPTION_HEADER) + IP6_PREFIX_INFO_OPTION_DATA_LEN)

////////////////////////////////////////////////////////////////////////
// Symbol Definitions
// These functions are not directly under test - but required to compile
////////////////////////////////////////////////////////////////////////
UINT32  mIp6Id;

EFI_STATUS
Ip6SendIcmpError (
  IN IP6_SERVICE       *IpSb,
  IN NET_BUF           *Packet,
  IN EFI_IPv6_ADDRESS  *SourceAddress       OPTIONAL,
  IN EFI_IPv6_ADDRESS  *DestinationAddress,
  IN UINT8             Type,
  IN UINT8             Code,
  IN UINT32            *Pointer             OPTIONAL
  )
{
  // ..
  return EFI_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Ip6OptionValidation Tests
////////////////////////////////////////////////////////////////////////

// Define a fixture for your tests if needed
class Ip6OptionValidationTest : public ::testing::Test {
protected:
  // Add any setup code if needed
  virtual void
  SetUp (
    )
  {
    // Initialize any resources or variables
  }

  // Add any cleanup code if needed
  virtual void
  TearDown (
    )
  {
    // Clean up any resources or variables
  }
};

// Test Description:
// Null option should return false
TEST_F (Ip6OptionValidationTest, NullOptionShouldReturnFalse) {
  UINT8   *option   = nullptr;
  UINT16  optionLen = 10; // Provide a suitable length

  EXPECT_FALSE (Ip6IsNDOptionValid (option, optionLen));
}

// Test Description:
// Truncated option should return false
TEST_F (Ip6OptionValidationTest, TruncatedOptionShouldReturnFalse) {
  UINT8   option[]  = { 0x01 }; // Provide a truncated option
  UINT16  optionLen = 1;

  EXPECT_FALSE (Ip6IsNDOptionValid (option, optionLen));
}

// Test Description:
// Ip6OptionPrefixInfo Option with zero length should return false
TEST_F (Ip6OptionValidationTest, OptionWithZeroLengthShouldReturnFalse) {
  IP6_OPTION_HEADER  optionHeader;

  optionHeader.Type   = Ip6OptionPrefixInfo;
  optionHeader.Length = 0;
  UINT8  option[sizeof (IP6_OPTION_HEADER)];

  CopyMem (option, &optionHeader, sizeof (IP6_OPTION_HEADER));
  UINT16  optionLen = sizeof (IP6_OPTION_HEADER);

  EXPECT_FALSE (Ip6IsNDOptionValid (option, optionLen));
}

// Test Description:
// Ip6OptionPrefixInfo Option with valid length should return true
TEST_F (Ip6OptionValidationTest, ValidPrefixInfoOptionShouldReturnTrue) {
  IP6_OPTION_HEADER  optionHeader;

  optionHeader.Type   = Ip6OptionPrefixInfo;
  optionHeader.Length = 4; // Length 4 * 8 = 32
  UINT8  option[OPTION_HEADER_IP6_PREFIX_DATA_LEN];

  CopyMem (option, &optionHeader, sizeof (IP6_OPTION_HEADER));

  EXPECT_TRUE (Ip6IsNDOptionValid (option, IP6_PREFIX_INFO_OPTION_DATA_LEN));
}

// Test Description:
// Ip6OptionPrefixInfo Option with invalid length should return false
TEST_F (Ip6OptionValidationTest, InvalidPrefixInfoOptionLengthShouldReturnFalse) {
  IP6_OPTION_HEADER  optionHeader;

  optionHeader.Type   = Ip6OptionPrefixInfo;
  optionHeader.Length = 3; // Length 3 * 8 = 24 (Invalid)
  UINT8  option[sizeof (IP6_OPTION_HEADER)];

  CopyMem (option, &optionHeader, sizeof (IP6_OPTION_HEADER));
  UINT16  optionLen = sizeof (IP6_OPTION_HEADER);

  EXPECT_FALSE (Ip6IsNDOptionValid (option, optionLen));
}
