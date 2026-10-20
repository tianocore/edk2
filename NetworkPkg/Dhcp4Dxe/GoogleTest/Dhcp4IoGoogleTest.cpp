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
  UINT8             OptionData[] = { 42, 6, 10, 0, 0, 1, 10, 0,
                                     42, 2, 0,  1, 0xFF };
  EFI_DHCP4_PACKET  *Packet;
  UINTN             OriginalLen;

  Packet = BuildTestPacket (OptionData, sizeof (OptionData));
  ASSERT_NE (Packet, nullptr);
  OriginalLen = Packet->Length;

  DhcpRemoveMalformedNonAlertOptions (Packet);

  EXPECT_EQ (Packet->Length, OriginalLen);
  FreePool (Packet);
}

// Test 4: RFC 3396 split option, invalid (6 + 3 bytes -> merged 9 -> malformed).
TEST (Dhcp4IoTest, Rfc3396InvalidSplit) {
  UINT8             OptionData[] = { 42, 6, 10, 0, 0, 1, 10, 0,
                                     42, 3, 0,  1, 0, 0xFF };
  EFI_DHCP4_PACKET  *Packet;
  UINTN             OriginalLen;

  Packet = BuildTestPacket (OptionData, sizeof (OptionData));
  ASSERT_NE (Packet, nullptr);
  OriginalLen = Packet->Length;

  DhcpRemoveMalformedNonAlertOptions (Packet);

  EXPECT_LT (Packet->Length, OriginalLen);
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
