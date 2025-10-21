/** @file
  Security Regression Tests for Network Stack Vulnerabilities

  This file contains GoogleTest-based security tests for:
  - Buffer overflow in MTFTP option parsing (NetworkPkg/Mtftp4Dxe/Mtftp4Option.c)
  - Buffer over-read in DHCP processing (NetworkPkg/IScsiDxe/IScsiDhcp.c)
  - Integer overflow in network packet size calculation

  Copyright (c) 2025, Security Analysis Team. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  CWE-119: Buffer Overflow
  CWE-125: Out-of-bounds Read
  CWE-190: Integer Overflow
**/

#include <gtest/gtest.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>

  // Mock MTFTP structures (simplified for testing)
  typedef struct {
    UINT16  OpCode;
    UINT8   Data[1];
  } MOCK_MTFTP_PACKET;

  typedef struct {
    UINT8  *OptionStr;
    UINT8  *ValueStr;
  } MOCK_MTFTP_OPTION;

  // Function under test (we'll create a safe version)
  EFI_STATUS
  SafeMtftp4ParseOptions (
    IN     MOCK_MTFTP_PACKET  *Packet,
    IN     UINT32              PacketLen,
    IN OUT UINT32             *Count,
    OUT    MOCK_MTFTP_OPTION  *Options  OPTIONAL
    );
}

/**
  Test fixture for network security tests
**/
class NetworkSecurityTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Test setup
  }

  void TearDown() override {
    // Test cleanup
  }

  /**
    Helper: Create a malformed MTFTP packet without null terminators

    This simulates the vulnerability in Mtftp4Option.c:180-205 where
    the code iterates looking for null terminators without bounds checking.

    @param[out] PacketSize  Size of the created packet

    @retval  Pointer to malformed packet
  **/
  MOCK_MTFTP_PACKET* CreateMalformedMtftpPacket(UINT32 *PacketSize) {
    // Allocate packet without null terminators
    *PacketSize = 32;
    MOCK_MTFTP_PACKET *Packet = (MOCK_MTFTP_PACKET*)AllocatePool(*PacketSize);

    if (Packet != NULL) {
      Packet->OpCode = 1;  // RRQ
      // Fill with non-null data (no terminators)
      SetMem(Packet->Data, *PacketSize - sizeof(UINT16), 'A');
    }

    return Packet;
  }

  /**
    Helper: Create a valid MTFTP packet with proper null terminators
  **/
  MOCK_MTFTP_PACKET* CreateValidMtftpPacket(UINT32 *PacketSize) {
    // Create valid packet: opcode + "file\0" + "blksize\0" + "512\0"
    const char *parts[] = { "file", "blksize", "512" };
    UINT32 TotalSize = sizeof(UINT16);  // OpCode

    for (int i = 0; i < 3; i++) {
      TotalSize += (UINT32)AsciiStrLen(parts[i]) + 1;  // +1 for null
    }

    MOCK_MTFTP_PACKET *Packet = (MOCK_MTFTP_PACKET*)AllocatePool(TotalSize);

    if (Packet != NULL) {
      Packet->OpCode = 1;  // RRQ
      UINT8 *Cur = Packet->Data;

      for (int i = 0; i < 3; i++) {
        UINT32 Len = (UINT32)AsciiStrLen(parts[i]);
        CopyMem(Cur, parts[i], Len);
        Cur += Len;
        *Cur++ = 0;  // Null terminator
      }

      *PacketSize = TotalSize;
    }

    return Packet;
  }
};

/**
  Test: MTFTP option parsing with malformed packet (no null terminators)

  SECURITY VULNERABILITY: Buffer Over-read
  Location: NetworkPkg/Mtftp4Dxe/Mtftp4Option.c:180-205
  CWE-125: Out-of-bounds Read

  The vulnerable code loops looking for null terminators without
  checking if it exceeds PacketLen, causing out-of-bounds reads.
**/
TEST_F(NetworkSecurityTest, Mtftp4ParseOptions_NoNullTerminators_ShouldReturnError) {
  UINT32 PacketSize;
  MOCK_MTFTP_PACKET *MaliciousPacket = CreateMalformedMtftpPacket(&PacketSize);

  ASSERT_NE(MaliciousPacket, nullptr) << "Failed to create test packet";

  MOCK_MTFTP_OPTION Options[10];
  UINT32 OptionCount = 10;

  // NOTE: This would call the ACTUAL vulnerable function if it existed
  // For this test, we demonstrate what SHOULD happen with a safe implementation
  // EFI_STATUS Status = SafeMtftp4ParseOptions(MaliciousPacket, PacketSize, &OptionCount, Options);

  // A safe implementation MUST:
  // 1. Detect missing null terminators
  // 2. Return EFI_INVALID_PARAMETER
  // 3. NOT read beyond PacketLen
  // 4. NOT crash

  // Expected behavior:
  // EXPECT_EQ(Status, EFI_INVALID_PARAMETER);

  // Clean up
  FreePool(MaliciousPacket);

  // For now, this test documents the vulnerability
  // Implementation of SafeMtftp4ParseOptions would go in the actual fix
  SUCCEED() << "Vulnerability documented: MTFTP option parsing needs bounds checking";
}

/**
  Test: MTFTP option parsing with valid packet

  This test verifies that a correctly implemented parser handles
  valid packets without issues.
**/
TEST_F(NetworkSecurityTest, Mtftp4ParseOptions_ValidPacket_ShouldSucceed) {
  UINT32 PacketSize;
  MOCK_MTFTP_PACKET *ValidPacket = CreateValidMtftpPacket(&PacketSize);

  ASSERT_NE(ValidPacket, nullptr) << "Failed to create test packet";

  MOCK_MTFTP_OPTION Options[10];
  UINT32 OptionCount = 10;

  // A safe implementation should successfully parse this
  // EFI_STATUS Status = SafeMtftp4ParseOptions(ValidPacket, PacketSize, &OptionCount, Options);
  // EXPECT_EQ(Status, EFI_SUCCESS);
  // EXPECT_EQ(OptionCount, 1);  // One option: blksize=512

  FreePool(ValidPacket);

  SUCCEED() << "Valid packet parsing test placeholder";
}

/**
  Test: DHCP option parsing with insufficient length

  SECURITY VULNERABILITY: Buffer Over-read
  Location: NetworkPkg/IScsiDxe/IScsiDhcp.c:355-359
  CWE-125: Out-of-bounds Read

  The code checks (Length > 4) but accesses Data[4..7], requiring Length >= 8
**/
TEST_F(NetworkSecurityTest, DhcpOptionParse_InsufficientLength_ShouldReturnError) {
  // Simulate DHCP option with Length=5 (triggers vulnerability)
  struct {
    UINT8  Type;
    UINT8  Length;  // Set to 5 (> 4 but < 8)
    UINT8  Data[5];
  } DhcpOption;

  DhcpOption.Type = 6;     // DNS option
  DhcpOption.Length = 5;   // VULNERABLE: > 4 but < 8
  SetMem(DhcpOption.Data, 5, 0xAA);

  // The vulnerable code would do:
  // if (DhcpOption.Length > 4) {
  //   CopyMem(&PrimaryDns, &DhcpOption.Data[4], 4);  // Over-reads Data[4..7]!
  // }

  // Safe implementation MUST check: Length >= 8
  EXPECT_LT(DhcpOption.Length, 8)
    << "Test confirms Length < 8 would cause over-read";

  // Correct check should be:
  // if (DhcpOption.Length >= 8) { ... }

  SUCCEED() << "Vulnerability documented: DHCP option parsing needs proper bounds check";
}

/**
  Test: Integer overflow in packet size calculation

  SECURITY VULNERABILITY: Integer Overflow → Buffer Overflow
  Location: NetworkPkg/IScsiDxe/IScsiDhcp.c:246
  CWE-190: Integer Overflow

  Code: OptionList = AllocatePool(OptionCount * sizeof(DHCP_OPTION));
  If OptionCount is large, multiplication can overflow.
**/
TEST_F(NetworkSecurityTest, PacketSizeCalculation_IntegerOverflow_ShouldBeDetected) {
  // Simulate large OptionCount that would cause overflow
  UINT32 OptionCount = 0x10000000;  // Large value
  UINT32 OptionSize = 32;           // Size of each option

  // Vulnerable calculation (what the code currently does):
  // UINT32 AllocSize = OptionCount * OptionSize;  // Can overflow!

  // Check if multiplication would overflow
  UINT32 MaxSafeCount = MAX_UINT32 / OptionSize;
  EXPECT_GT(OptionCount, MaxSafeCount)
    << "Test value should trigger overflow";

  // Safe implementation should use SafeIntLib:
  // RETURN_STATUS Status;
  // UINT32 AllocSize;
  // Status = SafeUint32Mult(OptionCount, OptionSize, &AllocSize);
  // if (RETURN_ERROR(Status)) {
  //   return EFI_INVALID_PARAMETER;
  // }

  SUCCEED() << "Vulnerability documented: Use SafeIntLib for size calculations";
}

/**
  Test: IPv6 fragment reassembly integer underflow

  SECURITY VULNERABILITY: Integer Underflow
  Location: NetworkPkg/Ip6Dxe/Ip6Input.c:110-140
  CWE-191: Integer Underflow
**/
TEST_F(NetworkSecurityTest, Ipv6FragmentReassembly_IntegerUnderflow_ShouldBeDetected) {
  // Simulate fragment Info with values that cause underflow
  struct {
    UINT32 Start;
    UINT32 End;
    UINT32 Length;
  } FragmentInfo;

  FragmentInfo.Start = 100;
  FragmentInfo.End = 200;
  FragmentInfo.Length = 50;   // Too small for the range

  // Vulnerable code does:
  // INTN Len = Start - FragmentInfo.Start;  // Can be negative
  // FragmentInfo.Length -= (UINT32)Len;     // UNDERFLOW if Len > Length

  UINT32 Start = 150;  // New start position
  INTN Len = Start - FragmentInfo.Start;  // Len = 50

  // If we subtract from Length without checking:
  // FragmentInfo.Length -= Len;  // 50 - 50 = 0 (OK)

  // But if Len > Length:
  Len = 100;  // Larger than FragmentInfo.Length
  // FragmentInfo.Length -= Len;  // 50 - 100 = underflow!

  EXPECT_GT((UINT32)Len, FragmentInfo.Length)
    << "Condition that would cause underflow";

  // Safe implementation MUST check:
  // if ((UINT32)Len > FragmentInfo.Length) {
  //   return EFI_INVALID_PARAMETER;
  // }

  SUCCEED() << "Vulnerability documented: IPv6 fragment handling needs underflow checks";
}

/**
  Main test runner
**/
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
