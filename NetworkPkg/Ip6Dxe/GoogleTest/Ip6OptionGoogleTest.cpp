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
  #include "Ip6OptionGoogleTest.h"
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

////////////////////////////////////////////////////////////////////////
// Ip6IsOptionValid Tests
////////////////////////////////////////////////////////////////////////

// Define a fixture for your tests if needed
class Ip6IsOptionValidTest : public ::testing::Test {
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

// Test Description
// Verify that a NULL option is Invalid
TEST_F (Ip6IsOptionValidTest, NullOptionShouldReturnTrue) {
  NET_BUF  Packet = { 0 };
  // we need to define enough of the packet to make the function work
  // The function being tested will pass IpSb to Ip6SendIcmpError which is defined above
  IP6_SERVICE  *IpSb = NULL;

  EFI_IPv6_ADDRESS  SourceAddress      = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IPv6_ADDRESS  DestinationAddress = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IP6_HEADER    Ip6Header          = { 0 };

  Ip6Header.SourceAddress      = SourceAddress;
  Ip6Header.DestinationAddress = DestinationAddress;
  Packet.Ip.Ip6                = &Ip6Header;

  EXPECT_FALSE (Ip6IsOptionValid (IpSb, &Packet, NULL, 0, 0));
}

// Test Description
// Verify that an unknown option with a length of 0 and type of <unknown> does not cause an infinite loop
TEST_F (Ip6IsOptionValidTest, VerifyNoInfiniteLoopOnUnknownOptionLength0) {
  NET_BUF  Packet = { 0 };
  // we need to define enough of the packet to make the function work
  // The function being tested will pass IpSb to Ip6SendIcmpError which is defined above
  UINT32  DeadCode = 0xDeadC0de;
  // Don't actually use this pointer, just pass it to the function, nothing will be done with it
  IP6_SERVICE  *IpSb = (IP6_SERVICE *)&DeadCode;

  EFI_IPv6_ADDRESS  SourceAddress      = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IPv6_ADDRESS  DestinationAddress = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IP6_HEADER    Ip6Header          = { 0 };

  Ip6Header.SourceAddress      = SourceAddress;
  Ip6Header.DestinationAddress = DestinationAddress;
  Packet.Ip.Ip6                = &Ip6Header;

  IP6_OPTION_HEADER  optionHeader;

  optionHeader.Type   = 23;   // Unknown Option
  optionHeader.Length = 0;    // This will cause an infinite loop if the function is not working correctly

  // This should be a valid option even though the length is 0
  EXPECT_TRUE (Ip6IsOptionValid (IpSb, &Packet, (UINT8 *)&optionHeader, sizeof (optionHeader), 0));
}

// Test Description
// Verify that an unknown option with a length of 1 and type of <unknown> does not cause an infinite loop
TEST_F (Ip6IsOptionValidTest, VerifyNoInfiniteLoopOnUnknownOptionLength1) {
  NET_BUF  Packet = { 0 };
  // we need to define enough of the packet to make the function work
  // The function being tested will pass IpSb to Ip6SendIcmpError which is defined above
  UINT32  DeadCode = 0xDeadC0de;
  // Don't actually use this pointer, just pass it to the function, nothing will be done with it
  IP6_SERVICE  *IpSb = (IP6_SERVICE *)&DeadCode;

  EFI_IPv6_ADDRESS  SourceAddress      = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IPv6_ADDRESS  DestinationAddress = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IP6_HEADER    Ip6Header          = { 0 };

  Ip6Header.SourceAddress      = SourceAddress;
  Ip6Header.DestinationAddress = DestinationAddress;
  Packet.Ip.Ip6                = &Ip6Header;

  IP6_OPTION_HEADER  optionHeader;

  optionHeader.Type   = 23;   // Unknown Option
  optionHeader.Length = 1;    // This will cause an infinite loop if the function is not working correctly

  EXPECT_TRUE (Ip6IsOptionValid (IpSb, &Packet, (UINT8 *)&optionHeader, sizeof (optionHeader), 0));
}

// Test Description
// Verify that an unknown option with a length of 2 and type of <unknown> does not cause an infinite loop
TEST_F (Ip6IsOptionValidTest, VerifyIpSkipUnknownOption) {
  NET_BUF  Packet = { 0 };
  // we need to define enough of the packet to make the function work
  // The function being tested will pass IpSb to Ip6SendIcmpError which is defined above
  UINT32  DeadCode = 0xDeadC0de;
  // Don't actually use this pointer, just pass it to the function, nothing will be done with it
  IP6_SERVICE  *IpSb = (IP6_SERVICE *)&DeadCode;

  EFI_IPv6_ADDRESS  SourceAddress      = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IPv6_ADDRESS  DestinationAddress = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IP6_HEADER    Ip6Header          = { 0 };

  Ip6Header.SourceAddress      = SourceAddress;
  Ip6Header.DestinationAddress = DestinationAddress;
  Packet.Ip.Ip6                = &Ip6Header;

  IP6_OPTION_HEADER  optionHeader;

  optionHeader.Type   = 23;   // Unknown Option
  optionHeader.Length = 2;    // Valid length for an unknown option

  EXPECT_TRUE (Ip6IsOptionValid (IpSb, &Packet, (UINT8 *)&optionHeader, sizeof (optionHeader), 0));
}

// Test Description
// Verify that Ip6OptionPad1 is valid with a length of 0
TEST_F (Ip6IsOptionValidTest, VerifyIp6OptionPad1) {
  NET_BUF  Packet = { 0 };
  // we need to define enough of the packet to make the function work
  // The function being tested will pass IpSb to Ip6SendIcmpError which is defined above
  UINT32  DeadCode = 0xDeadC0de;
  // Don't actually use this pointer, just pass it to the function, nothing will be done with it
  IP6_SERVICE  *IpSb = (IP6_SERVICE *)&DeadCode;

  EFI_IPv6_ADDRESS  SourceAddress      = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IPv6_ADDRESS  DestinationAddress = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IP6_HEADER    Ip6Header          = { 0 };

  Ip6Header.SourceAddress      = SourceAddress;
  Ip6Header.DestinationAddress = DestinationAddress;
  Packet.Ip.Ip6                = &Ip6Header;

  IP6_OPTION_HEADER  optionHeader;

  optionHeader.Type   = Ip6OptionPad1;
  optionHeader.Length = 0;

  EXPECT_TRUE (Ip6IsOptionValid (IpSb, &Packet, (UINT8 *)&optionHeader, sizeof (optionHeader), 0));
}

// Test Description
// Verify that Ip6OptionPadN doesn't overflow with various lengths
TEST_F (Ip6IsOptionValidTest, VerifyIp6OptionPadN) {
  NET_BUF  Packet = { 0 };
  // we need to define enough of the packet to make the function work
  // The function being tested will pass IpSb to Ip6SendIcmpError which is defined above
  UINT32  DeadCode = 0xDeadC0de;
  // Don't actually use this pointer, just pass it to the function, nothing will be done with it
  IP6_SERVICE  *IpSb = (IP6_SERVICE *)&DeadCode;

  EFI_IPv6_ADDRESS  SourceAddress      = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IPv6_ADDRESS  DestinationAddress = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IP6_HEADER    Ip6Header          = { 0 };

  Ip6Header.SourceAddress      = SourceAddress;
  Ip6Header.DestinationAddress = DestinationAddress;
  Packet.Ip.Ip6                = &Ip6Header;

  IP6_OPTION_HEADER  optionHeader;

  optionHeader.Type   = Ip6OptionPadN;
  optionHeader.Length = 0xFF;
  EXPECT_TRUE (Ip6IsOptionValid (IpSb, &Packet, (UINT8 *)&optionHeader, sizeof (optionHeader), 0));

  optionHeader.Length = 0xFE;
  EXPECT_TRUE (Ip6IsOptionValid (IpSb, &Packet, (UINT8 *)&optionHeader, sizeof (optionHeader), 0));

  optionHeader.Length = 0xFD;
  EXPECT_TRUE (Ip6IsOptionValid (IpSb, &Packet, (UINT8 *)&optionHeader, sizeof (optionHeader), 0));

  optionHeader.Length = 0xFC;
  EXPECT_TRUE (Ip6IsOptionValid (IpSb, &Packet, (UINT8 *)&optionHeader, sizeof (optionHeader), 0));
}

// Test Description
// Verify an unknown option doesn't cause an infinite loop with various lengths
TEST_F (Ip6IsOptionValidTest, VerifyNoInfiniteLoopOnUnknownOptionLengthAttemptOverflow) {
  NET_BUF  Packet = { 0 };
  // we need to define enough of the packet to make the function work
  // The function being tested will pass IpSb to Ip6SendIcmpError which is defined above
  UINT32  DeadCode = 0xDeadC0de;
  // Don't actually use this pointer, just pass it to the function, nothing will be done with it
  IP6_SERVICE  *IpSb = (IP6_SERVICE *)&DeadCode;

  EFI_IPv6_ADDRESS  SourceAddress      = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IPv6_ADDRESS  DestinationAddress = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IP6_HEADER    Ip6Header          = { 0 };

  Ip6Header.SourceAddress      = SourceAddress;
  Ip6Header.DestinationAddress = DestinationAddress;
  Packet.Ip.Ip6                = &Ip6Header;

  IP6_OPTION_HEADER  optionHeader;

  optionHeader.Type   = 23;   // Unknown Option
  optionHeader.Length = 0xFF;
  EXPECT_TRUE (Ip6IsOptionValid (IpSb, &Packet, (UINT8 *)&optionHeader, sizeof (optionHeader), 0));

  optionHeader.Length = 0xFE;
  EXPECT_TRUE (Ip6IsOptionValid (IpSb, &Packet, (UINT8 *)&optionHeader, sizeof (optionHeader), 0));

  optionHeader.Length = 0xFD;
  EXPECT_TRUE (Ip6IsOptionValid (IpSb, &Packet, (UINT8 *)&optionHeader, sizeof (optionHeader), 0));

  optionHeader.Length = 0xFC;
  EXPECT_TRUE (Ip6IsOptionValid (IpSb, &Packet, (UINT8 *)&optionHeader, sizeof (optionHeader), 0));
}

// Test Description
// Verify that the function supports multiple options
TEST_F (Ip6IsOptionValidTest, MultiOptionSupport) {
  UINT16   HdrLen;
  NET_BUF  Packet = { 0 };
  // we need to define enough of the packet to make the function work
  // The function being tested will pass IpSb to Ip6SendIcmpError which is defined above
  UINT32  DeadCode = 0xDeadC0de;
  // Don't actually use this pointer, just pass it to the function, nothing will be done with it
  IP6_SERVICE  *IpSb = (IP6_SERVICE *)&DeadCode;

  EFI_IPv6_ADDRESS  SourceAddress      = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IPv6_ADDRESS  DestinationAddress = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29 };
  EFI_IP6_HEADER    Ip6Header          = { 0 };

  Ip6Header.SourceAddress      = SourceAddress;
  Ip6Header.DestinationAddress = DestinationAddress;
  Packet.Ip.Ip6                = &Ip6Header;

  UINT8              ExtHdr[1024] = { 0 };
  UINT8              *Cursor      = ExtHdr;
  IP6_OPTION_HEADER  *Option      = (IP6_OPTION_HEADER *)ExtHdr;

  // Let's start chaining options

  Option->Type   = 23;   // Unknown Option
  Option->Length = 0xFC;

  Cursor += sizeof (IP6_OPTION_HEADER) + 0xFC;

  Option       = (IP6_OPTION_HEADER *)Cursor;
  Option->Type = Ip6OptionPad1;

  Cursor += sizeof (1);

  // Type and length aren't processed, instead it just moves the pointer forward by 4 bytes
  Option         = (IP6_OPTION_HEADER *)Cursor;
  Option->Type   = Ip6OptionRouterAlert;
  Option->Length = 4;

  Cursor += sizeof (IP6_OPTION_HEADER) + 4;

  Option         = (IP6_OPTION_HEADER *)Cursor;
  Option->Type   = Ip6OptionPadN;
  Option->Length = 0xFC;

  Cursor += sizeof (IP6_OPTION_HEADER) + 0xFC;

  Option         = (IP6_OPTION_HEADER *)Cursor;
  Option->Type   = Ip6OptionRouterAlert;
  Option->Length = 4;

  Cursor += sizeof (IP6_OPTION_HEADER) + 4;

  // Total 524

  HdrLen = (UINT16)(Cursor - ExtHdr);

  EXPECT_TRUE (Ip6IsOptionValid (IpSb, &Packet, ExtHdr, HdrLen, 0));
}
