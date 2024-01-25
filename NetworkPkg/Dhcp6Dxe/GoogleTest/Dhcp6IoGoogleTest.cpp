/** @file
  Tests for Dhcp6Io.c.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <gtest/gtest.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include <Library/BaseMemoryLib.h>
  #include "../Dhcp6Impl.h"
  #include "../Dhcp6Utility.h"
}

////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////

#define DHCP6_PACKET_MAX_LEN  1500

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// Symbol Definitions
// These functions are not directly under test - but required to compile
////////////////////////////////////////////////////////////////////////

// This definition is used by this test but is also required to compile
// by Dhcp6Io.c
EFI_IPv6_ADDRESS  mAllDhcpRelayAndServersAddress = {
  { 0xFF, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2 }
};

EFI_STATUS
EFIAPI
UdpIoSendDatagram (
  IN  UDP_IO           *UdpIo,
  IN  NET_BUF          *Packet,
  IN  UDP_END_POINT    *EndPoint OPTIONAL,
  IN  EFI_IP_ADDRESS   *Gateway  OPTIONAL,
  IN  UDP_IO_CALLBACK  CallBack,
  IN  VOID             *Context
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UdpIoRecvDatagram (
  IN  UDP_IO           *UdpIo,
  IN  UDP_IO_CALLBACK  CallBack,
  IN  VOID             *Context,
  IN  UINT32           HeadLen
  )
{
  return EFI_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Dhcp6AppendOptionTest Tests
////////////////////////////////////////////////////////////////////////

class Dhcp6AppendOptionTest : public ::testing::Test {
public:
  UINT8 *Buffer = NULL;
  EFI_DHCP6_PACKET *Packet;

protected:
  // Add any setup code if needed
  virtual void
  SetUp (
    )
  {
    // Initialize any resources or variables
    Buffer = (UINT8 *)AllocateZeroPool (DHCP6_PACKET_MAX_LEN);
    ASSERT_NE (Buffer, (UINT8 *)NULL);

    Packet       = (EFI_DHCP6_PACKET *)Buffer;
    Packet->Size = DHCP6_PACKET_MAX_LEN;
  }

  // Add any cleanup code if needed
  virtual void
  TearDown (
    )
  {
    // Clean up any resources or variables
    if (Buffer != NULL) {
      FreePool (Buffer);
    }
  }
};

// Test Description:
// Attempt to append an option to a packet that is too small by a duid that is too large
TEST_F (Dhcp6AppendOptionTest, InvalidDataExpectBufferTooSmall) {
  UINT8           *Cursor;
  EFI_DHCP6_DUID  *UntrustedDuid;
  EFI_STATUS      Status;

  UntrustedDuid = (EFI_DHCP6_DUID *)AllocateZeroPool (sizeof (EFI_DHCP6_DUID));
  ASSERT_NE (UntrustedDuid, (EFI_DHCP6_DUID *)NULL);

  UntrustedDuid->Length = NTOHS (0xFFFF);

  Cursor = Dhcp6AppendOptionTest::Packet->Dhcp6.Option;

  Status = Dhcp6AppendOption (
             Dhcp6AppendOptionTest::Packet,
             &Cursor,
             HTONS (Dhcp6OptServerId),
             UntrustedDuid->Length,
             UntrustedDuid->Duid
             );

  ASSERT_EQ (Status, EFI_BUFFER_TOO_SMALL);
}

// Test Description:
// Attempt to append an option to a packet that is large enough
TEST_F (Dhcp6AppendOptionTest, ValidDataExpectSuccess) {
  UINT8           *Cursor;
  EFI_DHCP6_DUID  *UntrustedDuid;
  EFI_STATUS      Status;
  UINTN           OriginalLength;

  UINT8  Duid[6] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };

  Packet->Length = sizeof (EFI_DHCP6_HEADER);
  OriginalLength = Packet->Length;

  UntrustedDuid = (EFI_DHCP6_DUID *)AllocateZeroPool (sizeof (EFI_DHCP6_DUID));
  ASSERT_NE (UntrustedDuid, (EFI_DHCP6_DUID *)NULL);

  UntrustedDuid->Length = NTOHS (sizeof (Duid));
  CopyMem (UntrustedDuid->Duid, Duid, sizeof (Duid));

  Cursor = Dhcp6AppendOptionTest::Packet->Dhcp6.Option;

  Status = Dhcp6AppendOption (
             Dhcp6AppendOptionTest::Packet,
             &Cursor,
             HTONS (Dhcp6OptServerId),
             UntrustedDuid->Length,
             UntrustedDuid->Duid
             );

  ASSERT_EQ (Status, EFI_SUCCESS);

  // verify that the pointer to cursor moved by the expected amount
  ASSERT_EQ (Cursor, (UINT8 *)Dhcp6AppendOptionTest::Packet->Dhcp6.Option + sizeof (Duid) + 4);

  // verify that the length of the packet is now the expected amount
  ASSERT_EQ (Dhcp6AppendOptionTest::Packet->Length, OriginalLength + sizeof (Duid) + 4);
}

////////////////////////////////////////////////////////////////////////
// Dhcp6AppendETOption Tests
////////////////////////////////////////////////////////////////////////

class Dhcp6AppendETOptionTest : public ::testing::Test {
public:
  UINT8 *Buffer = NULL;
  EFI_DHCP6_PACKET *Packet;

protected:
  // Add any setup code if needed
  virtual void
  SetUp (
    )
  {
    // Initialize any resources or variables
    Buffer = (UINT8 *)AllocateZeroPool (DHCP6_PACKET_MAX_LEN);
    ASSERT_NE (Buffer, (UINT8 *)NULL);

    Packet         = (EFI_DHCP6_PACKET *)Buffer;
    Packet->Size   = DHCP6_PACKET_MAX_LEN;
    Packet->Length = sizeof (EFI_DHCP6_HEADER);
  }

  // Add any cleanup code if needed
  virtual void
  TearDown (
    )
  {
    // Clean up any resources or variables
    if (Buffer != NULL) {
      FreePool (Buffer);
    }
  }
};

// Test Description:
// Attempt to append an option to a packet that is too small by a duid that is too large
TEST_F (Dhcp6AppendETOptionTest, InvalidDataExpectBufferTooSmall) {
  UINT8           *Cursor;
  EFI_STATUS      Status;
  DHCP6_INSTANCE  Instance;
  UINT16          ElapsedTimeVal;
  UINT16          *ElapsedTime;

  Cursor      = Dhcp6AppendETOptionTest::Packet->Dhcp6.Option;
  ElapsedTime = &ElapsedTimeVal;

  Packet->Length = Packet->Size - 2;

  Status = Dhcp6AppendETOption (
             Dhcp6AppendETOptionTest::Packet,
             &Cursor,
             &Instance, // Instance is not used in this function
             &ElapsedTime
             );

  // verify that we error out because the packet is too small for the option header
  ASSERT_EQ (Status, EFI_BUFFER_TOO_SMALL);

  // reset the length
  Packet->Length = sizeof (EFI_DHCP6_HEADER);
}

// Test Description:
// Attempt to append an option to a packet that is large enough
TEST_F (Dhcp6AppendETOptionTest, ValidDataExpectSuccess) {
  UINT8           *Cursor;
  EFI_STATUS      Status;
  DHCP6_INSTANCE  Instance;
  UINT16          ElapsedTimeVal;
  UINT16          *ElapsedTime;
  UINTN           ExpectedSize;
  UINTN           OriginalLength;

  Cursor         = Dhcp6AppendETOptionTest::Packet->Dhcp6.Option;
  ElapsedTime    = &ElapsedTimeVal;
  ExpectedSize   = 6;
  OriginalLength = Packet->Length;

  Status = Dhcp6AppendETOption (
             Dhcp6AppendETOptionTest::Packet,
             &Cursor,
             &Instance, // Instance is not used in this function
             &ElapsedTime
             );

  // verify that the status is EFI_SUCCESS
  ASSERT_EQ (Status, EFI_SUCCESS);

  // verify that the pointer to cursor moved by the expected amount
  ASSERT_EQ (Cursor, (UINT8 *)Dhcp6AppendETOptionTest::Packet->Dhcp6.Option + ExpectedSize);

  // verify that the length of the packet is now the expected amount
  ASSERT_EQ (Dhcp6AppendETOptionTest::Packet->Length, OriginalLength + ExpectedSize);
}

////////////////////////////////////////////////////////////////////////
// Dhcp6AppendIaOption Tests
////////////////////////////////////////////////////////////////////////

class Dhcp6AppendIaOptionTest : public ::testing::Test {
public:
  UINT8 *Buffer = NULL;
  EFI_DHCP6_PACKET *Packet;
  EFI_DHCP6_IA *Ia;

protected:
  // Add any setup code if needed
  virtual void
  SetUp (
    )
  {
    // Initialize any resources or variables
    Buffer = (UINT8 *)AllocateZeroPool (DHCP6_PACKET_MAX_LEN);
    ASSERT_NE (Buffer, (UINT8 *)NULL);

    Packet       = (EFI_DHCP6_PACKET *)Buffer;
    Packet->Size = DHCP6_PACKET_MAX_LEN;

    Ia = (EFI_DHCP6_IA *)AllocateZeroPool (sizeof (EFI_DHCP6_IA) + sizeof (EFI_DHCP6_IA_ADDRESS) * 2);
    ASSERT_NE (Ia, (EFI_DHCP6_IA *)NULL);

    CopyMem (Ia->IaAddress, mAllDhcpRelayAndServersAddress.Addr, sizeof (EFI_IPv6_ADDRESS));
    CopyMem (Ia->IaAddress + 1, mAllDhcpRelayAndServersAddress.Addr, sizeof (EFI_IPv6_ADDRESS));

    Ia->IaAddressCount = 2;
  }

  // Add any cleanup code if needed
  virtual void
  TearDown (
    )
  {
    // Clean up any resources or variables
    if (Buffer != NULL) {
      FreePool (Buffer);
    }

    if (Ia != NULL) {
      FreePool (Ia);
    }
  }
};

// Test Description:
// Attempt to append an option to a packet that doesn't have enough space
// for the option header
TEST_F (Dhcp6AppendIaOptionTest, IaNaInvalidDataExpectBufferTooSmall) {
  UINT8       *Cursor;
  EFI_STATUS  Status;

  Packet->Length = Packet->Size - 2;

  Ia->Descriptor.Type = Dhcp6OptIana;
  Ia->Descriptor.IaId = 0x12345678;

  Cursor = Dhcp6AppendIaOptionTest::Packet->Dhcp6.Option;

  Status = Dhcp6AppendIaOption (
             Dhcp6AppendIaOptionTest::Packet,
             &Cursor,
             Ia,
             0x12345678,
             0x11111111,
             Dhcp6OptIana
             );

  // verify that we error out because the packet is too small for the option header
  ASSERT_EQ (Status, EFI_BUFFER_TOO_SMALL);

  // reset the length
  Packet->Length = sizeof (EFI_DHCP6_HEADER);
}

// Test Description:
// Attempt to append an option to a packet that doesn't have enough space
// for the option header
TEST_F (Dhcp6AppendIaOptionTest, IaTaInvalidDataExpectBufferTooSmall) {
  UINT8       *Cursor;
  EFI_STATUS  Status;

  // Use up nearly all the space in the packet
  Packet->Length = Packet->Size - 2;

  Ia->Descriptor.Type = Dhcp6OptIata;
  Ia->Descriptor.IaId = 0x12345678;

  Cursor = Dhcp6AppendIaOptionTest::Packet->Dhcp6.Option;

  Status = Dhcp6AppendIaOption (
             Dhcp6AppendIaOptionTest::Packet,
             &Cursor,
             Ia,
             0,
             0,
             Dhcp6OptIata
             );

  // verify that we error out because the packet is too small for the option header
  ASSERT_EQ (Status, EFI_BUFFER_TOO_SMALL);

  // reset the length
  Packet->Length = sizeof (EFI_DHCP6_HEADER);
}

TEST_F (Dhcp6AppendIaOptionTest, IaNaValidDataExpectSuccess) {
  UINT8       *Cursor;
  EFI_STATUS  Status;
  UINTN       ExpectedSize;
  UINTN       OriginalLength;

  //
  // 2 bytes for the option header type
  //
  ExpectedSize = 2;
  //
  // 2 bytes for the option header length
  //
  ExpectedSize += 2;
  //
  // 4 bytes for the IAID
  //
  ExpectedSize += 4;
  //
  // + 4 bytes for the T1
  //
  ExpectedSize += 4;
  //
  // + 4 bytes for the T2
  //
  ExpectedSize += 4;
  //
  // + (4 + sizeof (EFI_DHCP6_IA_ADDRESS)) * 2;
  //   + 2 bytes for the option header type
  //   + 2 bytes for the option header length
  //   + sizeof (EFI_DHCP6_IA_ADDRESS) for the IA Address
  //
  ExpectedSize += (4 + sizeof (EFI_DHCP6_IA_ADDRESS)) * 2;

  Cursor = Dhcp6AppendIaOptionTest::Packet->Dhcp6.Option;

  Packet->Length = sizeof (EFI_DHCP6_HEADER);
  OriginalLength = Packet->Length;

  Ia->Descriptor.Type = Dhcp6OptIana;
  Ia->Descriptor.IaId = 0x12345678;

  Status = Dhcp6AppendIaOption (
             Dhcp6AppendIaOptionTest::Packet,
             &Cursor,
             Ia,
             0x12345678,
             0x12345678,
             Dhcp6OptIana
             );

  // verify that the pointer to cursor moved by the expected amount
  ASSERT_EQ (Cursor, (UINT8 *)Dhcp6AppendIaOptionTest::Packet->Dhcp6.Option + ExpectedSize);

  // verify that the length of the packet is now the expected amount
  ASSERT_EQ (Dhcp6AppendIaOptionTest::Packet->Length, OriginalLength + ExpectedSize);

  // verify that the status is EFI_SUCCESS
  ASSERT_EQ (Status, EFI_SUCCESS);
}

TEST_F (Dhcp6AppendIaOptionTest, IaTaValidDataExpectSuccess) {
  UINT8       *Cursor;
  EFI_STATUS  Status;
  UINTN       ExpectedSize;
  UINTN       OriginalLength;

  //
  // 2 bytes for the option header type
  //
  ExpectedSize = 2;
  //
  // 2 bytes for the option header length
  //
  ExpectedSize += 2;
  //
  // 4 bytes for the IAID
  //
  ExpectedSize += 4;
  //
  // + (4 + sizeof (EFI_DHCP6_IA_ADDRESS)) * 2;
  //   + 2 bytes for the option header type
  //   + 2 bytes for the option header length
  //   + sizeof (EFI_DHCP6_IA_ADDRESS) for the IA Address
  //
  ExpectedSize += (4 + sizeof (EFI_DHCP6_IA_ADDRESS)) * 2;

  Cursor = Dhcp6AppendIaOptionTest::Packet->Dhcp6.Option;

  Packet->Length = sizeof (EFI_DHCP6_HEADER);
  OriginalLength = Packet->Length;

  Ia->Descriptor.Type = Dhcp6OptIata;
  Ia->Descriptor.IaId = 0x12345678;

  Status = Dhcp6AppendIaOption (
             Dhcp6AppendIaOptionTest::Packet,
             &Cursor,
             Ia,
             0,
             0,
             Dhcp6OptIata
             );

  // verify that the pointer to cursor moved by the expected amount
  ASSERT_EQ (Cursor, (UINT8 *)Dhcp6AppendIaOptionTest::Packet->Dhcp6.Option + ExpectedSize);

  // verify that the length of the packet is now the expected amount
  ASSERT_EQ (Dhcp6AppendIaOptionTest::Packet->Length, OriginalLength + ExpectedSize);

  // verify that the status is EFI_SUCCESS
  ASSERT_EQ (Status, EFI_SUCCESS);
}
