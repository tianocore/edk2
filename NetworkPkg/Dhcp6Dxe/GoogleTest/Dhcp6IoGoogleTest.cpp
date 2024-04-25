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
  #include "Dhcp6IoGoogleTest.h"
}

////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////

#define DHCP6_PACKET_MAX_LEN  1500

// This definition is used by this test but is also required to compile
// by Dhcp6Io.c
#define DHCPV6_OPTION_IA_NA  3
#define DHCPV6_OPTION_IA_TA  4

#define SEARCH_PATTERN      0xDEADC0DE
#define SEARCH_PATTERN_LEN  sizeof(SEARCH_PATTERN)

////////////////////////////////////////////////////////////////////////
// Test structures for IA_NA and IA_TA options
////////////////////////////////////////////////////////////////////////
typedef struct {
  UINT16    Code;
  UINT16    Len;
  UINT32    IAID;
} DHCPv6_OPTION;

typedef struct {
  DHCPv6_OPTION    Header;
  UINT32           T1;
  UINT32           T2;
  UINT8            InnerOptions[0];
} DHCPv6_OPTION_IA_NA;

typedef struct {
  DHCPv6_OPTION    Header;
  UINT8            InnerOptions[0];
} DHCPv6_OPTION_IA_TA;

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
             &Instance,                    // Instance is not used in this function
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
             &Instance,                    // Instance is not used in this function
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

////////////////////////////////////////////////////////////////////////
// Dhcp6SeekInnerOptionSafe Tests
////////////////////////////////////////////////////////////////////////

// Define a fixture for your tests if needed
class Dhcp6SeekInnerOptionSafeTest : public ::testing::Test {
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
// This test verifies that Dhcp6SeekInnerOptionSafe returns EFI_SUCCESS when the IANA option is found.
TEST_F (Dhcp6SeekInnerOptionSafeTest, IANAValidOptionExpectSuccess) {
  EFI_STATUS           Result;
  UINT8                Option[sizeof (DHCPv6_OPTION_IA_NA) + SEARCH_PATTERN_LEN] = { 0 };
  UINT32               OptionLength                                              = sizeof (Option);
  DHCPv6_OPTION_IA_NA  *OptionPtr                                                = (DHCPv6_OPTION_IA_NA *)Option;
  UINT32               SearchPattern                                             = SEARCH_PATTERN;

  UINTN   SearchPatternLength = SEARCH_PATTERN_LEN;
  UINT8   *InnerOptionPtr     = NULL;
  UINT16  InnerOptionLength   = 0;

  OptionPtr->Header.Code = Dhcp6OptIana;
  OptionPtr->Header.Len  = HTONS (4 + 12); // Valid length has to be more than 12
  OptionPtr->Header.IAID = 0x12345678;
  OptionPtr->T1          = 0x11111111;
  OptionPtr->T2          = 0x22222222;
  CopyMem (OptionPtr->InnerOptions, &SearchPattern, SearchPatternLength);

  Result = Dhcp6SeekInnerOptionSafe (
             Dhcp6OptIana,
             Option,
             OptionLength,
             &InnerOptionPtr,
             &InnerOptionLength
             );
  ASSERT_EQ (Result, EFI_SUCCESS);
  ASSERT_EQ (InnerOptionLength, 4);
  ASSERT_EQ (CompareMem (InnerOptionPtr, &SearchPattern, SearchPatternLength), 0);
}

// Test Description:
// This test verifies that Dhcp6SeekInnerOptionSafe returns EFI_DEIVCE_ERROR when the IANA option size is invalid.
TEST_F (Dhcp6SeekInnerOptionSafeTest, IANAInvalidSizeExpectFail) {
  // Lets add an inner option of bytes we expect to find
  EFI_STATUS           Status;
  UINT8                Option[sizeof (DHCPv6_OPTION_IA_NA) + SEARCH_PATTERN_LEN] = { 0 };
  UINT32               OptionLength                                              = sizeof (Option);
  DHCPv6_OPTION_IA_NA  *OptionPtr                                                = (DHCPv6_OPTION_IA_NA *)Option;
  UINT32               SearchPattern                                             = SEARCH_PATTERN;

  UINTN   SearchPatternLength = SEARCH_PATTERN_LEN;
  UINT8   *InnerOptionPtr     = NULL;
  UINT16  InnerOptionLength   = 0;

  OptionPtr->Header.Code = Dhcp6OptIana;
  OptionPtr->Header.Len  = HTONS (4); // Set the length to lower than expected (12)
  OptionPtr->Header.IAID = 0x12345678;
  OptionPtr->T1          = 0x11111111;
  OptionPtr->T2          = 0x22222222;
  CopyMem (OptionPtr->InnerOptions, &SearchPattern, SearchPatternLength);

  // Set the InnerOptionLength to be less than the size of the option
  Status = Dhcp6SeekInnerOptionSafe (
             Dhcp6OptIana,
             Option,
             OptionLength,
             &InnerOptionPtr,
             &InnerOptionLength
             );
  ASSERT_EQ (Status, EFI_DEVICE_ERROR);

  // Now set the OptionLength to be less than the size of the option
  OptionLength = sizeof (DHCPv6_OPTION_IA_NA) - 1;
  Status       = Dhcp6SeekInnerOptionSafe (
                   Dhcp6OptIana,
                   Option,
                   OptionLength,
                   &InnerOptionPtr,
                   &InnerOptionLength
                   );
  ASSERT_EQ (Status, EFI_DEVICE_ERROR);
}

// Test Description:
// This test verifies that Dhcp6SeekInnerOptionSafe returns EFI_SUCCESS when the IATA option is found
TEST_F (Dhcp6SeekInnerOptionSafeTest, IATAValidOptionExpectSuccess) {
  // Lets add an inner option of bytes we expect to find
  EFI_STATUS           Status;
  UINT8                Option[sizeof (DHCPv6_OPTION_IA_TA) + SEARCH_PATTERN_LEN] = { 0 };
  UINT32               OptionLength                                              = sizeof (Option);
  DHCPv6_OPTION_IA_TA  *OptionPtr                                                = (DHCPv6_OPTION_IA_TA *)Option;
  UINT32               SearchPattern                                             = SEARCH_PATTERN;

  UINTN   SearchPatternLength = SEARCH_PATTERN_LEN;
  UINT8   *InnerOptionPtr     = NULL;
  UINT16  InnerOptionLength   = 0;

  OptionPtr->Header.Code = Dhcp6OptIata;
  OptionPtr->Header.Len  = HTONS (4 + 4); // Valid length has to be more than 4
  OptionPtr->Header.IAID = 0x12345678;
  CopyMem (OptionPtr->InnerOptions, &SearchPattern, SearchPatternLength);

  Status = Dhcp6SeekInnerOptionSafe (
             Dhcp6OptIata,
             Option,
             OptionLength,
             &InnerOptionPtr,
             &InnerOptionLength
             );
  ASSERT_EQ (Status, EFI_SUCCESS);
  ASSERT_EQ (InnerOptionLength, 4);
  ASSERT_EQ (CompareMem (InnerOptionPtr, &SearchPattern, SearchPatternLength), 0);
}

// Test Description:
// This test verifies that Dhcp6SeekInnerOptionSafe returns EFI_SUCCESS when the IATA option size is invalid.
TEST_F (Dhcp6SeekInnerOptionSafeTest, IATAInvalidSizeExpectFail) {
  // Lets add an inner option of bytes we expect to find
  EFI_STATUS           Status;
  UINT8                Option[sizeof (DHCPv6_OPTION_IA_TA) + SEARCH_PATTERN_LEN] = { 0 };
  UINT32               OptionLength                                              = sizeof (Option);
  DHCPv6_OPTION_IA_TA  *OptionPtr                                                = (DHCPv6_OPTION_IA_TA *)Option;
  UINT32               SearchPattern                                             = SEARCH_PATTERN;

  UINTN   SearchPatternLength = SEARCH_PATTERN_LEN;
  UINT8   *InnerOptionPtr     = NULL;
  UINT16  InnerOptionLength   = 0;

  OptionPtr->Header.Code = Dhcp6OptIata;
  OptionPtr->Header.Len  = HTONS (2); // Set the length to lower than expected (4)
  OptionPtr->Header.IAID = 0x12345678;
  CopyMem (OptionPtr->InnerOptions, &SearchPattern, SearchPatternLength);

  Status = Dhcp6SeekInnerOptionSafe (
             Dhcp6OptIata,
             Option,
             OptionLength,
             &InnerOptionPtr,
             &InnerOptionLength
             );
  ASSERT_EQ (Status, EFI_DEVICE_ERROR);

  // Now lets try modifying the OptionLength to be less than the size of the option
  OptionLength = sizeof (DHCPv6_OPTION_IA_TA) - 1;
  Status       = Dhcp6SeekInnerOptionSafe (
                   Dhcp6OptIata,
                   Option,
                   OptionLength,
                   &InnerOptionPtr,
                   &InnerOptionLength
                   );
  ASSERT_EQ (Status, EFI_DEVICE_ERROR);
}

// Test Description:
// This test verifies that any other Option Type fails
TEST_F (Dhcp6SeekInnerOptionSafeTest, InvalidOption) {
  // Lets add an inner option of bytes we expect to find
  EFI_STATUS           Result;
  UINT8                Option[sizeof (DHCPv6_OPTION_IA_TA) + SEARCH_PATTERN_LEN] = { 0 };
  UINT32               OptionLength                                              = sizeof (Option);
  DHCPv6_OPTION_IA_TA  *OptionPtr                                                = (DHCPv6_OPTION_IA_TA *)Option;
  UINT32               SearchPattern                                             = SEARCH_PATTERN;

  UINTN   SearchPatternLength = SEARCH_PATTERN_LEN;
  UINT8   *InnerOptionPtr     = NULL;
  UINT16  InnerOptionLength   = 0;

  OptionPtr->Header.Code = 0xC0DE;
  OptionPtr->Header.Len  = HTONS (2); // Set the length to lower than expected (4)
  OptionPtr->Header.IAID = 0x12345678;
  CopyMem (OptionPtr->InnerOptions, &SearchPattern, SearchPatternLength);

  Result = Dhcp6SeekInnerOptionSafe (0xC0DE, Option, OptionLength, &InnerOptionPtr, &InnerOptionLength);
  ASSERT_EQ (Result, EFI_DEVICE_ERROR);
}

////////////////////////////////////////////////////////////////////////
// Dhcp6SeekStsOption Tests
////////////////////////////////////////////////////////////////////////

#define PACKET_SIZE  (1500)

class Dhcp6SeekStsOptionTest : public ::testing::Test {
public:
  DHCP6_INSTANCE Instance      = { 0 };
  EFI_DHCP6_PACKET *Packet     = NULL;
  EFI_DHCP6_CONFIG_DATA Config = { 0 };

protected:
  // Add any setup code if needed
  virtual void
  SetUp (
    )
  {
    // Allocate a packet
    Packet = (EFI_DHCP6_PACKET *)AllocateZeroPool (PACKET_SIZE);
    ASSERT_NE (Packet, nullptr);

    // Initialize the packet
    Packet->Size = PACKET_SIZE;

    Instance.Config = &Config;
  }

  // Add any cleanup code if needed
  virtual void
  TearDown (
    )
  {
    // Clean up any resources or variables
    FreePool (Packet);
  }
};

// Test Description:
// This test verifies that Dhcp6SeekStsOption returns EFI_DEVICE_ERROR when the option is invalid
// This verifies that the calling function is working as expected
TEST_F (Dhcp6SeekStsOptionTest, SeekIATAOptionExpectFail) {
  EFI_STATUS    Status;
  UINT8         *Option             = NULL;
  UINT32        SearchPattern       = SEARCH_PATTERN;
  UINT16        SearchPatternLength = SEARCH_PATTERN_LEN;
  UINT16        *Len                = NULL;
  EFI_DHCP6_IA  Ia                  = { 0 };

  Ia.Descriptor.Type                = DHCPV6_OPTION_IA_TA;
  Ia.IaAddressCount                 = 1;
  Ia.IaAddress[0].PreferredLifetime = 0xDEADBEEF;
  Ia.IaAddress[0].ValidLifetime     = 0xDEADAAAA;
  Ia.IaAddress[0].IpAddress         = mAllDhcpRelayAndServersAddress;

  Packet->Length = sizeof (EFI_DHCP6_HEADER);

  Option = Dhcp6SeekStsOptionTest::Packet->Dhcp6.Option;

  // Let's append the option to the packet
  Status = Dhcp6AppendOption (
             Dhcp6SeekStsOptionTest::Packet,
             &Option,
             Dhcp6OptStatusCode,
             SearchPatternLength,
             (UINT8 *)&SearchPattern
             );
  ASSERT_EQ (Status, EFI_SUCCESS);

  // Inner option length - this will be overwritten later
  Len = (UINT16 *)(Option + 2);

  // Fill in the inner IA option
  Status = Dhcp6AppendIaOption (
             Dhcp6SeekStsOptionTest::Packet,
             &Option,
             &Ia,
             0x12345678,
             0x11111111,
             0x22222222
             );
  ASSERT_EQ (Status, EFI_SUCCESS);

  // overwrite the len of inner Ia option
  *Len = HTONS (3);

  Dhcp6SeekStsOptionTest::Instance.Config->IaDescriptor.Type = DHCPV6_OPTION_IA_TA;

  Option = NULL;
  Status = Dhcp6SeekStsOption (&(Dhcp6SeekStsOptionTest::Instance), Dhcp6SeekStsOptionTest::Packet, &Option);

  ASSERT_EQ (Status, EFI_DEVICE_ERROR);
}

// Test Description:
// This test verifies that Dhcp6SeekInnerOptionSafe returns EFI_SUCCESS when the IATA option size is invalid.
TEST_F (Dhcp6SeekStsOptionTest, SeekIANAOptionExpectSuccess) {
  EFI_STATUS    Status              = EFI_NOT_FOUND;
  UINT8         *Option             = NULL;
  UINT32        SearchPattern       = SEARCH_PATTERN;
  UINT16        SearchPatternLength = SEARCH_PATTERN_LEN;
  EFI_DHCP6_IA  Ia                  = { 0 };

  Ia.Descriptor.Type                = DHCPV6_OPTION_IA_NA;
  Ia.IaAddressCount                 = 1;
  Ia.IaAddress[0].PreferredLifetime = 0x11111111;
  Ia.IaAddress[0].ValidLifetime     = 0x22222222;
  Ia.IaAddress[0].IpAddress         = mAllDhcpRelayAndServersAddress;
  Packet->Length                    = sizeof (EFI_DHCP6_HEADER);

  Option = Dhcp6SeekStsOptionTest::Packet->Dhcp6.Option;

  Status = Dhcp6AppendOption (
             Dhcp6SeekStsOptionTest::Packet,
             &Option,
             Dhcp6OptStatusCode,
             SearchPatternLength,
             (UINT8 *)&SearchPattern
             );
  ASSERT_EQ (Status, EFI_SUCCESS);

  Status = Dhcp6AppendIaOption (
             Dhcp6SeekStsOptionTest::Packet,
             &Option,
             &Ia,
             0x12345678,
             0x11111111,
             0x22222222
             );
  ASSERT_EQ (Status, EFI_SUCCESS);

  Dhcp6SeekStsOptionTest::Instance.Config->IaDescriptor.Type = DHCPV6_OPTION_IA_NA;

  Option = NULL;
  Status = Dhcp6SeekStsOption (&(Dhcp6SeekStsOptionTest::Instance), Dhcp6SeekStsOptionTest::Packet, &Option);

  ASSERT_EQ (Status, EFI_SUCCESS);
}
