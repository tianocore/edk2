/** @file
  Tests for Dhcp4Io.c.

  Copyright (c) 2026 Loongson Technology Corporation Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Library/DebugLib.h>
  #include "../Dhcp4Impl.h"
  #include "Dhcp4IoGoogleTest.h"
}

/**
  Build a DHCP packet with the given option data.
**/
static
EFI_DHCP4_PACKET *
BuildTestPacket (
  IN  UINT8  *OptionData,
  IN  UINTN  OptionLen
  )
{
  EFI_DHCP4_PACKET  *Packet;
  UINTN             TotalLen;

  TotalLen = sizeof (EFI_DHCP4_PACKET) + OptionLen;
  Packet   = (EFI_DHCP4_PACKET *)AllocateZeroPool (TotalLen);
  if (Packet == NULL) {
    return NULL;
  }

  Packet->Size   = (UINT32)TotalLen;
  Packet->Length = (UINT32)(DHCP_HEADER_SIZE + OptionLen);

  Packet->Dhcp4.Header.OpCode = BOOTP_REPLY;
  Packet->Dhcp4.Magik         = DHCP_OPTION_MAGIC;

  if ((OptionData != NULL) && (OptionLen > 0)) {
    CopyMem (Packet->Dhcp4.Option, OptionData, OptionLen);
  }

  return Packet;
}

// Test 1: Bad option at the end (Tag 42, Len 15, ASCII domain string).
TEST (Dhcp4IoTest, BadOptionAtEnd) {
  UINT8             OptionData[] = { 42, 15, 'n', 't', 'p', '.', 'g', 'o', 'o', 'd', 't', 'i', 'm', 'e', '.', 'c', 'n', 0xFF };
  EFI_DHCP4_PACKET  *Packet;
  UINTN             OriginalLen;

  Packet = BuildTestPacket (OptionData, sizeof (OptionData));
  ASSERT_NE (Packet, nullptr);
  OriginalLen = Packet->Length;

  DhcpRemoveMalformedNonAlertOptions (Packet);

  EXPECT_LT (Packet->Length, OriginalLen);
  FreePool (Packet);
}

// Test 2: Several malformed options in a row.
TEST (Dhcp4IoTest, SeveralBadOptionsInARow) {
  UINT8             OptionData[] = { 42, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                     42, 7,  1, 2, 3, 4, 5, 6, 7, 0xFF };
  EFI_DHCP4_PACKET  *Packet;
  UINTN             OriginalLen;

  Packet = BuildTestPacket (OptionData, sizeof (OptionData));
  ASSERT_NE (Packet, nullptr);
  OriginalLen = Packet->Length;

  DhcpRemoveMalformedNonAlertOptions (Packet);

  EXPECT_LT (Packet->Length, OriginalLen);
  FreePool (Packet);
}

// Test 3: RFC 3396 split option, valid (6 + 2 bytes -> merged 8 -> 2 IPs).
TEST (Dhcp4IoTest, Rfc3396ValidSplit) {
  UINT8             OptionData[] = { 42, 6, 10, 0, 0, 1, 10, 0, 42, 2, 0, 1, 0xFF };
  EFI_DHCP4_PACKET  *Packet;
  UINT32            OriginalLen;

  Packet = BuildTestPacket (OptionData, sizeof (OptionData));
  ASSERT_NE (Packet, nullptr);
  OriginalLen = Packet->Length;

  DhcpRemoveMalformedNonAlertOptions (Packet);

  EXPECT_EQ (Packet->Length, OriginalLen);
  // Ensure the retained packet passes strict validation.
  EXPECT_EQ (DhcpValidateOptions (Packet, nullptr), EFI_SUCCESS);

  FreePool (Packet);
}

// Test 4: RFC 3396 split option, invalid (6 + 3 bytes -> merged 9 -> malformed).
TEST (Dhcp4IoTest, Rfc3396InvalidSplit) {
  UINT8             OptionData[] = { 42, 6, 10, 0, 0, 1, 10, 0, 42, 3, 0, 1, 0, 0xFF };
  EFI_DHCP4_PACKET  *Packet;
  UINT32            OriginalLen;

  Packet = BuildTestPacket (OptionData, sizeof (OptionData));
  ASSERT_NE (Packet, nullptr);
  OriginalLen = Packet->Length;

  DhcpRemoveMalformedNonAlertOptions (Packet);

  // Ensure the packet length is reduced and the EOP is at the end.
  EXPECT_LT (Packet->Length, OriginalLen);
  EXPECT_EQ (Packet->Length, DHCP_HEADER_SIZE + 1);
  EXPECT_EQ (Packet->Dhcp4.Option[0], DHCP4_TAG_EOP);
  // Strict validation should now succeed.
  EXPECT_EQ (DhcpValidateOptions (Packet, nullptr), EFI_SUCCESS);

  FreePool (Packet);
}

// Test 5: Truncated length (Length claims 15 but fewer bytes are available).
TEST (Dhcp4IoTest, TruncatedLength) {
  UINT8             OptionData[] = { 42, 15, 1, 2, 3, 0xFF };
  EFI_DHCP4_PACKET  *Packet;

  Packet = BuildTestPacket (OptionData, sizeof (OptionData));
  ASSERT_NE (Packet, nullptr);

  // Should not crash or corrupt memory.
  DhcpRemoveMalformedNonAlertOptions (Packet);
  FreePool (Packet);
}

// Test 6: A valid SWITCH option that is not an Alert
// (failure to pass NULL during verification will cause a crash).
TEST (Dhcp4IoTest, ValidSwitchOption) {
  // Tag 19 (IP Forwarding), Len 1, Value 1
  UINT8             OptionData[] = { 19, 1, 1, 0xFF };
  EFI_DHCP4_PACKET  *Packet;
  UINTN             OriginalLen;

  Packet = BuildTestPacket (OptionData, sizeof (OptionData));
  ASSERT_NE (Packet, nullptr);
  OriginalLen = Packet->Length;

  DhcpRemoveMalformedNonAlertOptions (Packet);
  EXPECT_EQ (Packet->Length, OriginalLen);
  FreePool (Packet);
}

// Test 7: Zero-length option (validate OptionPresent logic)
TEST (Dhcp4IoTest, ZeroLengthOption) {
  // Tag 42 (NTP), Len 0 (malformed)
  UINT8             OptionData[] = { 42, 0, 0xFF };
  EFI_DHCP4_PACKET  *Packet;
  UINTN             OriginalLen;

  Packet = BuildTestPacket (OptionData, sizeof (OptionData));
  ASSERT_NE (Packet, nullptr);
  OriginalLen = Packet->Length;

  DhcpRemoveMalformedNonAlertOptions (Packet);
  EXPECT_LT (Packet->Length, OriginalLen);
  FreePool (Packet);
}

// Test 8: Real cross-region OVERLOAD scenario.
// A malformed Option 42 is placed in the main option area, and the OVERLOAD
// flag (Tag 52) points to BootFileName, which contains another Option 42 fragment.
// The sanitization logic must safely bail out and leave the packet untouched.
TEST (Dhcp4IoTest, RealOverloadCrossRegionSkip) {
  EFI_DHCP4_PACKET  *Packet;
  UINT32            OriginalLen;
  UINT8             OptionData[21];

  SetMem (OptionData, sizeof (OptionData), 0);

  // 1. Main option area: Option 52 (OVERLOAD), Length 1, Value 1 (BootFileName overloaded).
  OptionData[0] = 52;
  OptionData[1] = 1;
  OptionData[2] = 1;

  // 2. Main option area: Option 42 (NTP), Length 15 (Malformed, not a multiple of 4).
  OptionData[3] = 42;
  OptionData[4] = 15;
  SetMem (&OptionData[5], 15, 0xAA);

  // 3. Main option area: EOP.
  OptionData[20] = 0xFF;

  // Build the packet with the properly sized option buffer.
  Packet = BuildTestPacket (OptionData, sizeof (OptionData));
  ASSERT_NE (Packet, nullptr);

  // 4. Overloaded area (BootFileName): Option 42 (NTP), Length 2 (Valid fragment).
  Packet->Dhcp4.Header.BootFileName[0] = 42;
  Packet->Dhcp4.Header.BootFileName[1] = 2;
  Packet->Dhcp4.Header.BootFileName[2] = 1;
  Packet->Dhcp4.Header.BootFileName[3] = 2;
  Packet->Dhcp4.Header.BootFileName[4] = -1;

  OriginalLen = Packet->Length;

  // Run sanitization.
  DhcpRemoveMalformedNonAlertOptions (Packet);

  // Expectation: Since OVERLOAD is present, sanitization must exit early.
  EXPECT_EQ (Packet->Length, OriginalLen);
  EXPECT_EQ (Packet->Dhcp4.Option[3], 42);
  EXPECT_EQ (Packet->Dhcp4.Option[4], 15);

  FreePool (Packet);
}
