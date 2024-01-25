/** @file
  Host based unit test for PxeBcDhcp6.c.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <gtest/gtest.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include "../PxeBcImpl.h"
  #include "../PxeBcDhcp6.h"
  #include "PxeBcDhcp6GoogleTest.h"
}

///////////////////////////////////////////////////////////////////////////////
// Definitions
///////////////////////////////////////////////////////////////////////////////

#define PACKET_SIZE  (1500)

typedef struct {
  UINT16    OptionCode;   // The option code for DHCP6_OPT_SERVER_ID (e.g., 0x03)
  UINT16    OptionLen;    // The length of the option (e.g., 16 bytes)
  UINT8     ServerId[16]; // The 16-byte DHCPv6 Server Identifier
} DHCP6_OPTION_SERVER_ID;

///////////////////////////////////////////////////////////////////////////////
/// Symbol Definitions
///////////////////////////////////////////////////////////////////////////////

EFI_STATUS
MockUdpWrite (
  IN EFI_PXE_BASE_CODE_PROTOCOL      *This,
  IN UINT16                          OpFlags,
  IN EFI_IP_ADDRESS                  *DestIp,
  IN EFI_PXE_BASE_CODE_UDP_PORT      *DestPort,
  IN EFI_IP_ADDRESS                  *GatewayIp   OPTIONAL,
  IN EFI_IP_ADDRESS                  *SrcIp       OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT  *SrcPort     OPTIONAL,
  IN UINTN                           *HeaderSize  OPTIONAL,
  IN VOID                            *HeaderPtr   OPTIONAL,
  IN UINTN                           *BufferSize,
  IN VOID                            *BufferPtr
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
MockUdpRead (
  IN EFI_PXE_BASE_CODE_PROTOCOL      *This,
  IN UINT16                          OpFlags,
  IN OUT EFI_IP_ADDRESS              *DestIp      OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT  *DestPort    OPTIONAL,
  IN OUT EFI_IP_ADDRESS              *SrcIp       OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT  *SrcPort     OPTIONAL,
  IN UINTN                           *HeaderSize  OPTIONAL,
  IN VOID                            *HeaderPtr   OPTIONAL,
  IN OUT UINTN                       *BufferSize,
  IN VOID                            *BufferPtr
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
MockConfigure (
  IN EFI_UDP6_PROTOCOL     *This,
  IN EFI_UDP6_CONFIG_DATA  *UdpConfigData OPTIONAL
  )
{
  return EFI_SUCCESS;
}

// Needed by PxeBcSupport
EFI_STATUS
EFIAPI
QueueDpc (
  IN EFI_TPL            DpcTpl,
  IN EFI_DPC_PROCEDURE  DpcProcedure,
  IN VOID               *DpcContext    OPTIONAL
  )
{
  return EFI_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// PxeBcHandleDhcp6OfferTest Tests
///////////////////////////////////////////////////////////////////////////////

class PxeBcHandleDhcp6OfferTest : public ::testing::Test {
public:
  PXEBC_PRIVATE_DATA Private = { 0 };
  EFI_UDP6_PROTOCOL Udp6Read;
  EFI_PXE_BASE_CODE_MODE Mode = { 0 };

protected:
  // Add any setup code if needed
  virtual void
  SetUp (
    )
  {
    Private.Dhcp6Request = (EFI_DHCP6_PACKET *)AllocateZeroPool (PACKET_SIZE);

    // Need to setup the EFI_PXE_BASE_CODE_PROTOCOL
    // The function under test really only needs the following:
    //  UdpWrite
    //  UdpRead

    Private.PxeBc.UdpWrite = (EFI_PXE_BASE_CODE_UDP_WRITE)MockUdpWrite;
    Private.PxeBc.UdpRead  = (EFI_PXE_BASE_CODE_UDP_READ)MockUdpRead;

    // Need to setup EFI_UDP6_PROTOCOL
    // The function under test really only needs the following:
    //  Configure

    Udp6Read.Configure = (EFI_UDP6_CONFIGURE)MockConfigure;
    Private.Udp6Read   = &Udp6Read;

    // Need to setup the EFI_PXE_BASE_CODE_MODE
    Private.PxeBc.Mode = &Mode;

    // for this test it doesn't really matter what the Dhcpv6 ack is set to
  }

  // Add any cleanup code if needed
  virtual void
  TearDown (
    )
  {
    if (Private.Dhcp6Request != NULL) {
      FreePool (Private.Dhcp6Request);
    }

    // Clean up any resources or variables
  }
};

// Note:
// Testing PxeBcHandleDhcp6Offer() is difficult because it depends on a
// properly setup Private structure. Attempting to properly test this function
// without a signficant refactor is a fools errand. Instead, we will test
// that we can prevent an overflow in the function.
TEST_F (PxeBcHandleDhcp6OfferTest, BasicUsageTest) {
  PXEBC_DHCP6_PACKET_CACHE  *Cache6 = NULL;
  EFI_DHCP6_PACKET_OPTION   Option  = { 0 };

  Private.SelectIndex = 1; // SelectIndex is 1-based
  Cache6              = &Private.OfferBuffer[Private.SelectIndex - 1].Dhcp6;

  Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER] = &Option;
  // Setup the DHCPv6 offer packet
  Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER]->OpCode = DHCP6_OPT_SERVER_ID;
  Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER]->OpLen  = NTOHS (1337);

  ASSERT_EQ (PxeBcHandleDhcp6Offer (&(PxeBcHandleDhcp6OfferTest::Private)), EFI_DEVICE_ERROR);
}

class PxeBcCacheDnsServerAddressesTest : public ::testing::Test {
public:
  PXEBC_PRIVATE_DATA Private = { 0 };

protected:
  // Add any setup code if needed
  virtual void
  SetUp (
    )
  {
  }

  // Add any cleanup code if needed
  virtual void
  TearDown (
    )
  {
  }
};

// Test Description
// Test that we cache the DNS server address from the DHCPv6 offer packet
TEST_F (PxeBcCacheDnsServerAddressesTest, BasicUsageTest) {
  UINT8                     SearchPattern[16] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF };
  EFI_DHCP6_PACKET_OPTION   *Option;
  PXEBC_DHCP6_PACKET_CACHE  *Cache6 = NULL;

  Option = (EFI_DHCP6_PACKET_OPTION *)AllocateZeroPool (sizeof (EFI_DHCP6_PACKET_OPTION) + sizeof (SearchPattern));
  ASSERT_NE (Option, nullptr);

  Option->OpCode = DHCP6_OPT_SERVER_ID;
  Option->OpLen  = NTOHS (sizeof (SearchPattern));
  CopyMem (Option->Data, SearchPattern, sizeof (SearchPattern));

  Private.SelectIndex                         = 1; // SelectIndex is 1-based
  Cache6                                      = &Private.OfferBuffer[Private.SelectIndex - 1].Dhcp6;
  Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER] = Option;

  Private.DnsServer = nullptr;

  ASSERT_EQ (PxeBcCacheDnsServerAddresses (&(PxeBcCacheDnsServerAddressesTest::Private), Cache6), EFI_SUCCESS);
  ASSERT_NE (Private.DnsServer, nullptr);
  ASSERT_EQ (CompareMem (Private.DnsServer, SearchPattern, sizeof (SearchPattern)), 0);

  if (Private.DnsServer) {
    FreePool (Private.DnsServer);
  }

  if (Option) {
    FreePool (Option);
  }
}
// Test Description
// Test that we can prevent an overflow in the function
TEST_F (PxeBcCacheDnsServerAddressesTest, AttemptOverflowTest) {
  EFI_DHCP6_PACKET_OPTION   Option  = { 0 };
  PXEBC_DHCP6_PACKET_CACHE  *Cache6 = NULL;

  Private.SelectIndex                         = 1; // SelectIndex is 1-based
  Cache6                                      = &Private.OfferBuffer[Private.SelectIndex - 1].Dhcp6;
  Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER] = &Option;
  // Setup the DHCPv6 offer packet
  Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER]->OpCode = DHCP6_OPT_SERVER_ID;
  Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER]->OpLen  = NTOHS (1337);

  Private.DnsServer = NULL;

  ASSERT_EQ (PxeBcCacheDnsServerAddresses (&(PxeBcCacheDnsServerAddressesTest::Private), Cache6), EFI_DEVICE_ERROR);
  ASSERT_EQ (Private.DnsServer, nullptr);

  if (Private.DnsServer) {
    FreePool (Private.DnsServer);
  }
}

// Test Description
// Test that we can prevent an underflow in the function
TEST_F (PxeBcCacheDnsServerAddressesTest, AttemptUnderflowTest) {
  EFI_DHCP6_PACKET_OPTION   Option  = { 0 };
  PXEBC_DHCP6_PACKET_CACHE  *Cache6 = NULL;

  Private.SelectIndex                         = 1; // SelectIndex is 1-based
  Cache6                                      = &Private.OfferBuffer[Private.SelectIndex - 1].Dhcp6;
  Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER] = &Option;
  // Setup the DHCPv6 offer packet
  Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER]->OpCode = DHCP6_OPT_SERVER_ID;
  Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER]->OpLen  = NTOHS (2);

  Private.DnsServer = NULL;

  ASSERT_EQ (PxeBcCacheDnsServerAddresses (&(PxeBcCacheDnsServerAddressesTest::Private), Cache6), EFI_DEVICE_ERROR);
  ASSERT_EQ (Private.DnsServer, nullptr);

  if (Private.DnsServer) {
    FreePool (Private.DnsServer);
  }
}

// Test Description
// Test that we can handle recursive dns (multiple dns entries)
TEST_F (PxeBcCacheDnsServerAddressesTest, MultipleDnsEntries) {
  EFI_DHCP6_PACKET_OPTION   Option  = { 0 };
  PXEBC_DHCP6_PACKET_CACHE  *Cache6 = NULL;

  Private.SelectIndex                         = 1; // SelectIndex is 1-based
  Cache6                                      = &Private.OfferBuffer[Private.SelectIndex - 1].Dhcp6;
  Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER] = &Option;
  // Setup the DHCPv6 offer packet
  Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER]->OpCode = DHCP6_OPT_SERVER_ID;

  EFI_IPv6_ADDRESS  addresses[2] = {
    // 2001:db8:85a3::8a2e:370:7334
    { 0x20, 0x01, 0x0d, 0xb8, 0x85, 0xa3, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x2e, 0x03, 0x70, 0x73, 0x34 },
    // fe80::d478:91c3:ecd7:4ff9
    { 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd4, 0x78, 0x91, 0xc3, 0xec, 0xd7, 0x4f, 0xf9 }
  };

  CopyMem (Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER]->Data, &addresses, sizeof (addresses));

  Cache6->OptList[PXEBC_DHCP6_IDX_DNS_SERVER]->OpLen = NTOHS (sizeof (addresses));

  Private.DnsServer = NULL;

  ASSERT_EQ (PxeBcCacheDnsServerAddresses (&(PxeBcCacheDnsServerAddressesTest::Private), Cache6), EFI_SUCCESS);

  ASSERT_NE (Private.DnsServer, nullptr);

  //
  // This is expected to fail until DnsServer supports multiple DNS servers
  //
  // This is tracked in https://bugzilla.tianocore.org/show_bug.cgi?id=1886
  //
  // Disabling:
  // ASSERT_EQ (CompareMem(Private.DnsServer, &addresses, sizeof(addresses)), 0);

  if (Private.DnsServer) {
    FreePool (Private.DnsServer);
  }
}
