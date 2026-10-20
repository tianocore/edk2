/** @file
  Host-based tests for PxeBcSupport.c.

  Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockUefiBootServicesTableLib.h>

extern "C" {
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Library/NetLib.h>
  #include "../PxeBcImpl.h"
}

extern "C" {
  VOID
  EFIAPI
  PxeBcIcmpErrorDpcHandle (
    IN VOID  *Context
    );

  VOID
  EFIAPI
  PxeBcIcmp6ErrorDpcHandle (
    IN VOID  *Context
    );
}

#define ICMP_TEST_FRAGMENT_COUNT   3
#define ICMP_TEST_FRAGMENT_LENGTH  200

static EFI_STATUS
EFIAPI
MockSignalEvent (
  IN EFI_EVENT  Event
  )
{
  return EFI_SUCCESS;
}

static EFI_STATUS
EFIAPI
MockIp4Receive (
  IN EFI_IP4_PROTOCOL          *This,
  IN EFI_IP4_COMPLETION_TOKEN  *Token
  )
{
  return EFI_SUCCESS;
}

static EFI_STATUS
EFIAPI
MockIp6Receive (
  IN EFI_IP6_PROTOCOL          *This,
  IN EFI_IP6_COMPLETION_TOKEN  *Token
  )
{
  return EFI_SUCCESS;
}

static VOID
InitializeIcmpFragments (
  OUT UINT8  FragmentBuffers[ICMP_TEST_FRAGMENT_COUNT][ICMP_TEST_FRAGMENT_LENGTH],
  IN  UINT8  ErrorType
  )
{
  UINTN  Index;

  for (Index = 0; Index < ICMP_TEST_FRAGMENT_COUNT; Index++) {
    SetMem (FragmentBuffers[Index], ICMP_TEST_FRAGMENT_LENGTH, (UINT8)(0x10 + Index));
  }

  FragmentBuffers[0][0] = ErrorType;
}

static VOID
ExpectMemoryMatches (
  IN CONST CHAR8  *BufferDescription,
  IN CONST VOID   *ActualBuffer,
  IN CONST VOID   *ExpectedBuffer,
  IN UINTN        BufferLength
  )
{
  CONST UINT8  *ActualBytes;
  CONST UINT8  *ExpectedBytes;
  UINTN        Index;

  ActualBytes   = (CONST UINT8 *)ActualBuffer;
  ExpectedBytes = (CONST UINT8 *)ExpectedBuffer;
  for (Index = 0; Index < BufferLength; Index++) {
    if (ActualBytes[Index] != ExpectedBytes[Index]) {
      EXPECT_EQ ((UINT32)ActualBytes[Index], (UINT32)ExpectedBytes[Index])
      << BufferDescription << " mismatch at byte offset " << Index;
      return;
    }
  }
}

static VOID
ExpectIcmpErrorAndTftpErrorUnchanged (
  IN EFI_PXE_BASE_CODE_MODE        *Mode,
  IN UINT8                         FragmentBuffers[ICMP_TEST_FRAGMENT_COUNT][ICMP_TEST_FRAGMENT_LENGTH],
  IN EFI_PXE_BASE_CODE_TFTP_ERROR  *TftpErrorBefore
  )
{
  UINT8  Expected[sizeof (EFI_PXE_BASE_CODE_ICMP_ERROR)];

  CopyMem (
    Expected,
    FragmentBuffers[0],
    ICMP_TEST_FRAGMENT_LENGTH
    );
  CopyMem (
    Expected + ICMP_TEST_FRAGMENT_LENGTH,
    FragmentBuffers[1],
    ICMP_TEST_FRAGMENT_LENGTH
    );
  CopyMem (
    Expected + (2 * ICMP_TEST_FRAGMENT_LENGTH),
    FragmentBuffers[2],
    sizeof (Expected) - (2 * ICMP_TEST_FRAGMENT_LENGTH)
    );

  ExpectMemoryMatches (
    "Gaps detected in packed ICMP error output buffer",
    &Mode->IcmpError,
    Expected,
    sizeof (Expected)
    );
  ExpectMemoryMatches (
    "Buffer overflow detected: adjacent TFTP error buffer was modified",
    &Mode->TftpError,
    TftpErrorBefore,
    sizeof (*TftpErrorBefore)
    );
}

class PxeBcIcmpErrorCopyTest : public ::testing::Test {
protected:
  EFI_SIGNAL_EVENT OriginalSignalEvent;

  void
  SetUp (
    ) override
  {
    OriginalSignalEvent = gBS->SignalEvent;
    gBS->SignalEvent    = MockSignalEvent;
  }

  void
  TearDown (
    ) override
  {
    gBS->SignalEvent = OriginalSignalEvent;
  }
};

TEST_F (PxeBcIcmpErrorCopyTest, Ipv4FragmentsAreTruncatedAtBufferEnd) {
  PXEBC_PRIVATE_DATA            Private = { 0 };
  EFI_IP4_PROTOCOL              Ip4     = { 0 };
  EFI_IP4_HEADER                Header  = { 0 };
  UINT8                         FragmentBuffers[ICMP_TEST_FRAGMENT_COUNT][ICMP_TEST_FRAGMENT_LENGTH];
  EFI_IP4_RECEIVE_DATA          *RxData;
  EFI_IP4_COMPLETION_TOKEN      *Token;
  EFI_PXE_BASE_CODE_TFTP_ERROR  TftpErrorBefore;
  UINTN                         RxDataSize;
  UINTN                         Index;

  InitializeIcmpFragments (FragmentBuffers, ICMP_DEST_UNREACHABLE);

  RxDataSize = sizeof (EFI_IP4_RECEIVE_DATA) +
               (sizeof (EFI_IP4_FRAGMENT_DATA) * (ICMP_TEST_FRAGMENT_COUNT - 1));
  RxData = (EFI_IP4_RECEIVE_DATA *)AllocateZeroPool (RxDataSize);
  ASSERT_NE (RxData, nullptr);

  RxData->Header        = &Header;
  RxData->FragmentCount = ICMP_TEST_FRAGMENT_COUNT;
  Header.Protocol       = EFI_IP_PROTO_ICMP;
  for (Index = 0; Index < ICMP_TEST_FRAGMENT_COUNT; Index++) {
    RxData->FragmentTable[Index].FragmentBuffer = FragmentBuffers[Index];
    RxData->FragmentTable[Index].FragmentLength = ICMP_TEST_FRAGMENT_LENGTH;
  }

  Private.Mode.UsingIpv6 = FALSE;
  Private.Ip4            = &Ip4;
  Ip4.Receive            = MockIp4Receive;
  Token                  = &Private.IcmpToken;
  Token->Status          = EFI_ICMP_ERROR;
  Token->Packet.RxData   = RxData;

  SetMem (&Private.Mode.TftpError, sizeof (Private.Mode.TftpError), 0xA5);
  CopyMem (
    &TftpErrorBefore,
    &Private.Mode.TftpError,
    sizeof (TftpErrorBefore)
    );

  PxeBcIcmpErrorDpcHandle (&Private);

  ExpectIcmpErrorAndTftpErrorUnchanged (&Private.Mode, FragmentBuffers, &TftpErrorBefore);
  FreePool (RxData);
}

TEST_F (PxeBcIcmpErrorCopyTest, Ipv6FragmentsAreTruncatedAtBufferEnd) {
  PXEBC_PRIVATE_DATA            Private = { 0 };
  EFI_IP6_PROTOCOL              Ip6     = { 0 };
  EFI_IP6_HEADER                Header  = { 0 };
  UINT8                         FragmentBuffers[ICMP_TEST_FRAGMENT_COUNT][ICMP_TEST_FRAGMENT_LENGTH];
  EFI_IP6_RECEIVE_DATA          *RxData;
  EFI_IP6_COMPLETION_TOKEN      *Token;
  EFI_PXE_BASE_CODE_TFTP_ERROR  TftpErrorBefore;
  UINTN                         RxDataSize;
  UINTN                         Index;

  InitializeIcmpFragments (FragmentBuffers, ICMP_V6_DEST_UNREACHABLE);

  RxDataSize = sizeof (EFI_IP6_RECEIVE_DATA) +
               (sizeof (EFI_IP6_FRAGMENT_DATA) * (ICMP_TEST_FRAGMENT_COUNT - 1));
  RxData = (EFI_IP6_RECEIVE_DATA *)AllocateZeroPool (RxDataSize);
  ASSERT_NE (RxData, nullptr);

  RxData->Header                = &Header;
  RxData->FragmentCount         = ICMP_TEST_FRAGMENT_COUNT;
  Header.NextHeader             = IP6_ICMP;
  Header.SourceAddress.Addr[0]  = 0x20;
  Header.SourceAddress.Addr[1]  = 0x01;
  Header.SourceAddress.Addr[15] = 0x01;
  for (Index = 0; Index < ICMP_TEST_FRAGMENT_COUNT; Index++) {
    RxData->FragmentTable[Index].FragmentBuffer = FragmentBuffers[Index];
    RxData->FragmentTable[Index].FragmentLength = ICMP_TEST_FRAGMENT_LENGTH;
  }

  Private.Mode.UsingIpv6 = TRUE;
  Private.Ip6            = &Ip6;
  Ip6.Receive            = MockIp6Receive;
  Token                  = &Private.Icmp6Token;
  Token->Status          = EFI_ICMP_ERROR;
  Token->Packet.RxData   = RxData;

  SetMem (&Private.Mode.TftpError, sizeof (Private.Mode.TftpError), 0xA5);
  CopyMem (
    &TftpErrorBefore,
    &Private.Mode.TftpError,
    sizeof (TftpErrorBefore)
    );

  PxeBcIcmp6ErrorDpcHandle (&Private);

  ExpectIcmpErrorAndTftpErrorUnchanged (&Private.Mode, FragmentBuffers, &TftpErrorBefore);
  FreePool (RxData);
}
