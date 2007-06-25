/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
        bc.c

Abstract:

--*/

#include "Bc.h"

//
// helper routines
//
VOID
CvtNum (
  IN UINTN  Number,
  IN UINT8  *Buffer,
  IN INTN   Length
  )
/*++

  Routine Description:
    Convert number to ASCII value

  Arguments:
    Number              - Numeric value to convert to decimal ASCII value.
    Buffer              - Buffer to place ASCII version of the Number
    Length              - Length of Buffer.

  Returns:
    none                - none

--*/
{
  UINTN Remainder;

  while (Length--) {
    Remainder = Number % 10;
    Number /= 10;
    Buffer[Length] = (UINT8) ('0' + Remainder);
  }
}

VOID
UtoA10 (
  IN UINTN Number,
  IN UINT8 *Buffer
  )
/*++

  Routine Description:
    Convert number to decimal ASCII value at Buffer location

  Arguments:
    Number              - Numeric value to convert to decimal ASCII value.
    Buffer              - Buffer to place ASCII version of the Number

  Returns:
    none                - none

--*/
{
  INTN  Index;
  UINT8 BuffArray[31];

  BuffArray[30] = 0;
  CvtNum (Number, BuffArray, 30);

  for (Index = 0; Index < 30; ++Index) {
    if (BuffArray[Index] != '0') {
      break;
    }
  }

  CopyMem (Buffer, BuffArray + Index, 31 - Index);
}

UINTN
AtoU (
  IN UINT8 *Buffer
  )
/*++

  Routine Description:
    Convert ASCII numeric string to a UINTN value

  Arguments:
    Number              - Numeric value to convert to decimal ASCII value.
    Buffer              - Buffer to place ASCII version of the Number

  Returns:
    Value                - UINTN value of the ASCII string.

--*/
{
  UINTN Value;
  INT8  Character;

  Value     = 0;
  Character = *Buffer++;
  do {
    Value     = Value * 10 + Character - '0';
    Character = *Buffer++;
  } while (Character);

  return Value;
}

UINT64
AtoU64 (
  IN UINT8 *Buffer
  )
/*++

  Routine Description:
    Convert ASCII numeric string to a UINTN value

  Arguments:
    Number              - Numeric value to convert to decimal ASCII value.
    Buffer              - Buffer to place ASCII version of the Number

  Returns:
    Value                - UINTN value of the ASCII string.

--*/
{
  UINT64  Value;
  UINT8   Character;

  Value = 0;
  while ((Character = *Buffer++) != '\0') {
    Value = MultU64x32 (Value, 10) + (Character - '0');
  }

  return Value;
}
//
// random number generator
//
#define RANDOM_MULTIPLIER   2053
#define RANDOM_ADD_IN_VALUE 19

VOID
SeedRandom (
  IN PXE_BASECODE_DEVICE  *Private,
  IN UINT16               InitialSeed
  )
/*++

  Routine Description:
    Initialize the Seed for the random number generator

  Arguments:

  Returns:
    none                -

--*/
{
  if (Private != NULL) {
    Private->RandomSeed = InitialSeed;
  }
}

UINT16
Random (
  IN PXE_BASECODE_DEVICE  *Private
  )
/*++

  Routine Description:
    Generate and return a pseudo-random number

  Arguments:

  Returns:
    Number           - UINT16 random number

--*/
{
  UINTN Number;

  if (Private != NULL) {
    Number = -(INTN) Private->RandomSeed * RANDOM_MULTIPLIER + RANDOM_ADD_IN_VALUE;

    return Private->RandomSeed = (UINT16) Number;
  } else {
    return 0;
  }
}
//
// calculate the internet checksum (RFC 1071)
// return 16 bit ones complement of ones complement sum of 16 bit words
//
UINT16
IpChecksum (
  IN UINT16 *Packet,
  IN UINTN  Length
  )
/*++

  Routine Description:
    Calculate the internet checksum (see RFC 1071)

  Arguments:
    Packet             - Buffer which contains the data to be checksummed
    Length             - Length to be checksummed

  Returns:
    Checksum           - Returns the 16 bit ones complement of
                         ones complement sum of 16 bit words

--*/
{
  UINT32  Sum;
  UINT8   Odd;

  Sum = 0;
  Odd = (UINT8) (Length & 1);
  Length >>= 1;
  while (Length--) {
    Sum += *Packet++;
  }

  if (Odd) {
    Sum += *(UINT8 *) Packet;
  }

  Sum = (Sum & 0xffff) + (Sum >> 16);
  //
  // in case above carried
  //
  Sum += Sum >> 16;

  return (UINT16) (~ (UINT16) Sum);
}

UINT16
IpChecksum2 (
  IN UINT16 *Header,
  IN UINTN  HeaderLen,
  IN UINT16 *Message,
  IN UINTN  MessageLen
  )
/*++

  Routine Description:
    Calculate the internet checksum (see RFC 1071)
    on a non contiguous header and data

  Arguments:
    Header        - Buffer which contains the data to be checksummed
    HeaderLen     - Length to be checksummed
    Message       - Buffer which contains the data to be checksummed
    MessageLen    - Length to be checksummed

  Returns:
    Checksum      - Returns the 16 bit ones complement of
                    ones complement sum of 16 bit words

--*/
{
  UINT32  Sum;

  Sum = (UINT16)~IpChecksum (Header, HeaderLen);
  Sum = Sum + (UINT16)~IpChecksum (Message, MessageLen);
  //
  // in case above carried
  //
  Sum += Sum >> 16;

  return (UINT16) (~ (UINT16) Sum);
}

UINT16
UpdateChecksum (
  IN UINT16 OldChksum,
  IN UINT16 OldWord,
  IN UINT16 NewWord
  )
/*++

  Routine Description:
    Adjust the internet checksum (see RFC 1071) on a single word update.

  Arguments:
    OldChkSum          - Checksum previously calculated
    OldWord            - Value
    NewWord            - New Value

  Returns:
    Checksum           - Returns the 16 bit ones complement of
                         ones complement sum of 16 bit words

--*/
{
  UINT32  sum;

  sum = ~OldChksum + NewWord - OldWord;
  //
  // in case above carried
  //
  sum += sum >> 16;
  return (UINT16) (~ (UINT16) sum);
}

STATIC
BOOLEAN
SetMakeCallback (
  IN PXE_BASECODE_DEVICE *Private
  )
/*++

  Routine Description:
    See if a callback is in play

  Arguments:
    Private                - Pointer to Pxe BaseCode Protocol

  Returns:
    0                  - Callbacks are active on the handle
    1                  - Callbacks are not active on the handle

--*/
{
  Private->EfiBc.Mode->MakeCallbacks = (BOOLEAN) (gBS->HandleProtocol (
                                                        Private->Handle,
                                                        &gEfiPxeBaseCodeCallbackProtocolGuid,
                                                        (VOID *) &Private->CallbackProtocolPtr
                                                        ) == EFI_SUCCESS);

  DEBUG (
    (EFI_D_INFO,
    "\nMode->MakeCallbacks == %d  ",
    Private->EfiBc.Mode->MakeCallbacks)
    );

  DEBUG (
    (EFI_D_INFO,
    "\nPrivate->CallbackProtocolPtr == %xh  ",
    Private->CallbackProtocolPtr)
    );

  if (Private->CallbackProtocolPtr != NULL) {
    DEBUG (
      (EFI_D_INFO,
      "\nCallbackProtocolPtr->Revision = %xh  ",
      Private->CallbackProtocolPtr->Revision)
      );

    DEBUG (
      (EFI_D_INFO,
      "\nCallbackProtocolPtr->Callback = %xh  ",
      Private->CallbackProtocolPtr->Callback)
      );
  }

  return Private->EfiBc.Mode->MakeCallbacks;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
WaitForReceive (
  IN PXE_BASECODE_DEVICE        *Private,
  IN EFI_PXE_BASE_CODE_FUNCTION Function,
  IN EFI_EVENT                  TimeoutEvent,
  IN OUT UINTN                  *HeaderSizePtr,
  IN OUT UINTN                  *BufferSizePtr,
  IN OUT UINT16                 *ProtocolPtr
  )
/*++

  Routine Description:
    Routine which does an SNP->Receive over a timeout period and doing callbacks

  Arguments:
    Private       - Pointer to Pxe BaseCode Protocol
    Function      - What PXE function to callback
    TimeoutEvent  - Timer event that will trigger when we have waited too
                    long for an incoming packet
    HeaderSizePtr - Pointer to the size of the Header size
    BufferSizePtr - Pointer to the size of the Buffer size
    ProtocolPtr   - The protocol to sniff for (namely, UDP/etc)

  Returns:
    0             - Something was returned
    !0            - Like there was nothing to receive (EFI_TIMEOUT/NOT_READY)

--*/
{
  EFI_SIMPLE_NETWORK_PROTOCOL *SnpPtr;
  EFI_PXE_CALLBACK            CallbackPtr;
  EFI_STATUS                  StatCode;
  EFI_EVENT                   CallbackEvent;

  //
  // Initialize pointer to SNP interface
  //
  SnpPtr = Private->SimpleNetwork;

  //
  // Initialize pointer to PxeBc callback routine - if any
  //
  CallbackPtr = (Private->EfiBc.Mode->MakeCallbacks) ? Private->CallbackProtocolPtr->Callback : NULL;

  //
  // Create callback event and set timer
  //
  StatCode = gBS->CreateEvent (
                    EVT_TIMER,
                    TPL_CALLBACK,
                    NULL,
                    NULL,
                    &CallbackEvent
                    );

  if (EFI_ERROR (StatCode)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // every 100 milliseconds
  //
  StatCode = gBS->SetTimer (
                    CallbackEvent,
                    TimerPeriodic,
                    1000000
                    );

  if (EFI_ERROR (StatCode)) {
    gBS->CloseEvent (CallbackEvent);
    return EFI_DEVICE_ERROR;
  }
  //
  // Loop until a packet is received or a receive error is detected or
  // a callback abort is detected or a timeout event occurs.
  //
  for (;;)
  {
    //
    // Poll for received packet.
    //
    *BufferSizePtr = BUFFER_ALLOCATE_SIZE;

    StatCode = SnpPtr->Receive (
                        SnpPtr,
                        HeaderSizePtr,
                        BufferSizePtr,
                        Private->ReceiveBufferPtr,
                        0,
                        0,
                        ProtocolPtr
                        );

    if (!EFI_ERROR (StatCode)) {
      //
      // Packet was received.  Make received callback then return.
      //
      if (CallbackPtr != NULL) {
        StatCode = CallbackPtr (
                    Private->CallbackProtocolPtr,
                    Function,
                    TRUE,
                    (UINT32) *BufferSizePtr,
                    (EFI_PXE_BASE_CODE_PACKET *) Private->ReceiveBufferPtr
                    );

        if (StatCode != EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE) {
          StatCode = EFI_ABORTED;
        } else {
          StatCode = EFI_SUCCESS;
        }
      }

      break;
    }

    if (StatCode != EFI_NOT_READY) {
      break;
    }

    //
    // Check for callback event.
    //
    if (!EFI_ERROR (gBS->CheckEvent (CallbackEvent))) {
      //
      // Make periodic callback if callback pointer is initialized.
      //
      if (CallbackPtr != NULL) {
        StatCode = CallbackPtr (
                    Private->CallbackProtocolPtr,
                    Function,
                    FALSE,
                    0,
                    NULL
                    );

        //
        // Abort if directed to by callback routine.
        //
        if (StatCode != EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE) {
          StatCode = EFI_ABORTED;
          break;
        }
      }
    }
    //
    // Check for timeout event.
    //
    if (TimeoutEvent == 0) {
      StatCode = EFI_TIMEOUT;
      break;
    }

    if (!EFI_ERROR (gBS->CheckEvent (TimeoutEvent))) {
      StatCode = EFI_TIMEOUT;
      break;
    }
    //
    // Check IGMP timer events.
    //
    IgmpCheckTimers (Private);
  }

  gBS->CloseEvent (CallbackEvent);

  return StatCode;
}

EFI_STATUS
SendPacket (
  PXE_BASECODE_DEVICE           *Private,
  VOID                          *HeaderPtr,
  VOID                          *PacketPtr,
  INTN                          PacketLen,
  VOID                          *HardwareAddr,
  UINT16                        MediaProtocol,
  IN EFI_PXE_BASE_CODE_FUNCTION Function
  )
/*++

  Routine Description:
    Routine which does an SNP->Transmit of a buffer

  Arguments:
    Private       - Pointer to Pxe BaseCode Protocol
    HeaderPtr          - Pointer to the buffer
    PacketPtr          - Pointer to the packet to send
    PacketLen        - The length of the entire packet to send
    HardwareAddr        - Pointer to the MAC address of the destination
    MediaProtocol - What type of frame to create (RFC 1700) - IE. Ethernet
    Function      - What PXE function to callback

  Returns:
    0             - Something was sent
    !0            - An error was encountered during sending of a packet

--*/
{
  EFI_SIMPLE_NETWORK_PROTOCOL *SnpPtr;
  EFI_SIMPLE_NETWORK_MODE     *SnpModePtr;
  EFI_PXE_CALLBACK            CallbackPtr;
  EFI_STATUS                  StatCode;
  EFI_EVENT                   TimeoutEvent;
  UINT32                      IntStatus;
  VOID                        *TxBuf;

  //
  //
  //
  CallbackPtr = Private->EfiBc.Mode->MakeCallbacks ? Private->CallbackProtocolPtr->Callback : 0;

  SnpPtr      = Private->SimpleNetwork;
  SnpModePtr  = SnpPtr->Mode;

  //
  // clear prior interrupt status
  //
  StatCode = SnpPtr->GetStatus (SnpPtr, &IntStatus, 0);

  if (EFI_ERROR (StatCode)) {
    DEBUG (
      (EFI_D_WARN,
      "\nSendPacket()  Exit #1  %xh (%r)",
      StatCode,
      StatCode)
      );
    return StatCode;
  }

  Private->DidTransmit = FALSE;

  if (CallbackPtr != NULL) {
    if (CallbackPtr (
          Private->CallbackProtocolPtr,
          Function,
          FALSE,
          (UINT32) PacketLen,
          PacketPtr
          ) != EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE) {
      DEBUG (
        (EFI_D_WARN,
        "\nSendPacket()  Exit #2  %xh (%r)",
        EFI_ABORTED,
        EFI_ABORTED)
        );
      return EFI_ABORTED;
    }
  }
  //
  // put packet in transmit queue
  // headersize should be zero if not filled in
  //
  StatCode = gBS->CreateEvent (
                    EVT_TIMER,
                    TPL_CALLBACK,
                    NULL,
                    NULL,
                    &TimeoutEvent
                    );

  if (EFI_ERROR (StatCode)) {
    DEBUG (
      (EFI_D_ERROR,
      "Could not create transmit timeout event.  %r\n",
      StatCode)
      );
    return EFI_DEVICE_ERROR;
  }

  //
  // 5 milliseconds
  //
  StatCode = gBS->SetTimer (
                    TimeoutEvent,
                    TimerRelative,
                    50000
                    );

  if (EFI_ERROR (StatCode)) {
    DEBUG (
      (EFI_D_ERROR,
      "Could not set transmit timeout event timer.  %r\n",
      StatCode)
      );
    gBS->CloseEvent (TimeoutEvent);
    return EFI_DEVICE_ERROR;
  }

  for (;;) {
    StatCode = SnpPtr->Transmit (
                        SnpPtr,
                        (UINTN) SnpPtr->Mode->MediaHeaderSize,
                        (UINTN) (PacketLen + SnpPtr->Mode->MediaHeaderSize),
                        HeaderPtr,
                        &SnpModePtr->CurrentAddress,
                        (EFI_MAC_ADDRESS *) HardwareAddr,
                        &MediaProtocol
                        );

    if (StatCode != EFI_NOT_READY) {
      break;
    }

    if (!EFI_ERROR (gBS->CheckEvent (TimeoutEvent))) {
      StatCode = EFI_TIMEOUT;
      break;
    }
  }

  gBS->CloseEvent (TimeoutEvent);

  if (EFI_ERROR (StatCode)) {
    DEBUG (
      (EFI_D_WARN,
      "\nSendPacket()  Exit #3  %xh (%r)",
      StatCode,
      StatCode)
      );
    return StatCode;
  }
  //
  // remove transmit buffer from snp's unused queue
  // done this way in case someday things are buffered and we don't get it back
  // immediately
  //
  StatCode = gBS->CreateEvent (
                    EVT_TIMER,
                    TPL_CALLBACK,
                    NULL,
                    NULL,
                    &TimeoutEvent
                    );

  if (EFI_ERROR (StatCode)) {
    DEBUG (
      (EFI_D_ERROR,
      "Could not create transmit status timeout event.  %r\n",
      StatCode)
      );
    return EFI_DEVICE_ERROR;
  }

  //
  // 5 milliseconds
  //
  StatCode = gBS->SetTimer (
                    TimeoutEvent,
                    TimerRelative,
                    50000
                    );

  if (EFI_ERROR (StatCode)) {
    DEBUG (
      (EFI_D_ERROR,
      "Could not set transmit status timeout event timer.  %r\n",
      StatCode)
      );
    gBS->CloseEvent (TimeoutEvent);
    return EFI_DEVICE_ERROR;
  }

  for (;;) {
    StatCode = SnpPtr->GetStatus (SnpPtr, &IntStatus, &TxBuf);

    if (EFI_ERROR (StatCode)) {
      DEBUG (
        (EFI_D_WARN,
        "\nSendPacket()  Exit #4  %xh (%r)",
        StatCode,
        StatCode)
        );
      break;
    }

    if (IntStatus & EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT) {
      Private->DidTransmit = TRUE;
    }

    if (TxBuf != NULL) {
      break;
    }

    if (!EFI_ERROR (gBS->CheckEvent (TimeoutEvent))) {
      StatCode = EFI_TIMEOUT;
      break;
    }
  }

  gBS->CloseEvent (TimeoutEvent);

  return StatCode;
}
//
//
//
EFI_BIS_PROTOCOL *
PxebcBisStart (
  IN PXE_BASECODE_DEVICE      *Private,
  OUT BIS_APPLICATION_HANDLE  *BisAppHandle,
  OUT OPTIONAL EFI_BIS_DATA            **BisDataSigInfo
  )
/*++
Routine description:
  Locate BIS interface and if found, try to start it.

Parameters:
  Private := Pointer to PxeBc protocol
  BisAppHandle := Pointer to BIS application handle storage
  BisDataSigInfo := Pointer to BIS signature information storage
Returns:
--*/
{
  EFI_STATUS        EfiStatus;
  EFI_HANDLE        BisHandleBuffer;
  UINTN             BisHandleCount;
  EFI_BIS_PROTOCOL  *BisPtr;
  EFI_BIS_VERSION   BisInterfaceVersion;
  BOOLEAN           BisCheckFlag;

  BisHandleCount  = sizeof (EFI_HANDLE);
  BisCheckFlag    = FALSE;

  //
  // Locate BIS protocol handle (if present).
  // If BIS protocol handle is not found, return NULL.
  //
  DEBUG ((EFI_D_INFO, "\ngBS->LocateHandle()  "));

  EfiStatus = gBS->LocateHandle (
                    ByProtocol,
                    &gEfiBisProtocolGuid,
                    NULL,
                    &BisHandleCount,
                    &BisHandleBuffer
                    );

  if (EFI_ERROR (EfiStatus)) {
    //
    // Any error means that there is no BIS.
    // Note - It could mean that there are more than
    // one BIS protocols installed, but that scenario
    // is not yet supported.
    //
    DEBUG (
      (EFI_D_WARN,
      "\nPxebcBisStart()""\n  gBS->LocateHandle()  %r (%xh)\n",
      EfiStatus,
      EfiStatus)
      );

    return NULL;
  }

  if (BisHandleCount != sizeof (BisHandleBuffer)) {
    //
    // This really should never happen, but I am paranoid.
    //
    DEBUG (
      (EFI_D_NET,
      "\nPxebcBisStart()  BisHandleCount != %d\n",
      sizeof BisHandleBuffer)
      );

    return NULL;
  }

  DEBUG ((EFI_D_INFO, "BIS handle found."));

  //
  // Locate BIS protocol interface.
  // If the BIS protocol interface cannot be found, return NULL.
  //
  DEBUG ((EFI_D_INFO, "\ngBS->HandleProtocol()  "));

  EfiStatus = gBS->HandleProtocol (
                    BisHandleBuffer,
                    &gEfiBisProtocolGuid,
                    (VOID **) &BisPtr
                    );

  if (EFI_ERROR (EfiStatus)) {
    DEBUG (
      (EFI_D_WARN,
      "\nPxebcBisStart()""\n  gBS->HandleProtocol()  %r (%xh)\n",
      EfiStatus,
      EfiStatus)
      );

    return NULL;
  }

  if (BisPtr == NULL) {
    //
    // This really should never happen.
    //
    DEBUG (
      (EFI_D_NET,
      "\nPxebcBisStart()""\n  gBS->HandleProtocoL()  ""BIS protocol interface pointer is NULL!\n")
      );

    return NULL;
  }

  DEBUG ((EFI_D_INFO, "BIS protocol interface found."));

  //
  // Check that all of the BIS API function pointers are not NULL.
  //
  if (BisPtr->Initialize == NULL ||
      BisPtr->Shutdown == NULL ||
      BisPtr->Free == NULL ||
      BisPtr->GetBootObjectAuthorizationCertificate == NULL ||
      BisPtr->GetBootObjectAuthorizationCheckFlag == NULL ||
      BisPtr->GetBootObjectAuthorizationUpdateToken == NULL ||
      BisPtr->GetSignatureInfo == NULL ||
      BisPtr->UpdateBootObjectAuthorization == NULL ||
      BisPtr->VerifyBootObject == NULL ||
      BisPtr->VerifyObjectWithCredential == NULL
      ) {
    DEBUG (
      (
      EFI_D_NET,
      "\nPxebcBisStart()""\n  BIS protocol interface is invalid."
      "\n  At least one BIS protocol function pointer is NULL.\n"
      )
      );

    return NULL;
  }
  //
  // Initialize BIS.
  // If BIS does not initialize, return NULL.
  //
  DEBUG ((EFI_D_INFO, "\nBisPtr->Initialize()  "));

  BisInterfaceVersion.Major = BIS_VERSION_1;

  EfiStatus = BisPtr->Initialize (
                        BisPtr,
                        BisAppHandle,
                        &BisInterfaceVersion,
                        NULL
                        );

  if (EFI_ERROR (EfiStatus)) {
    DEBUG (
      (EFI_D_WARN,
      "\nPxebcBisStart()""\n  BisPtr->Initialize()  %r (%xh)\n",
      EfiStatus,
      EfiStatus)
      );

    return NULL;
  }

  DEBUG (
    (EFI_D_INFO,
    "  BIS version: %d.%d",
    BisInterfaceVersion.Major,
    BisInterfaceVersion.Minor)
    );

  //
  // If the requested BIS API version is not supported,
  // shutdown BIS and return NULL.
  //
  if (BisInterfaceVersion.Major != BIS_VERSION_1) {
    DEBUG (
      (EFI_D_WARN,
      "\nPxebcBisStart()""\n  BIS version %d.%d not supported by PXE BaseCode.\n",
      BisInterfaceVersion.Major,
      BisInterfaceVersion.Minor)
      );

    BisPtr->Shutdown (*BisAppHandle);
    return NULL;
  }
  //
  // Get BIS check flag.
  // If the BIS check flag cannot be read, shutdown BIS and return NULL.
  //
  DEBUG ((EFI_D_INFO, "\nBisPtr->GetBootObjectAuthorizationCheckFlag()  "));

  EfiStatus = BisPtr->GetBootObjectAuthorizationCheckFlag (*BisAppHandle, &BisCheckFlag);

  if (EFI_ERROR (EfiStatus)) {
    DEBUG (
      (EFI_D_WARN,
      "\nPxebcBisStart()""\n  BisPtr->GetBootObjectAuthorizationCheckFlag()  %r (%xh)\n",
      EfiStatus,
      EfiStatus)
      );

    BisPtr->Shutdown (*BisAppHandle);
    return NULL;
  }
  //
  // If the BIS check flag is FALSE, shutdown BIS and return NULL.
  //
  if (!BisCheckFlag) {
    DEBUG ((EFI_D_INFO, "\nBIS check flag is FALSE.\n"));
    BisPtr->Shutdown (*BisAppHandle);
    return NULL;
  } else {
    DEBUG ((EFI_D_INFO, "\nBIS check flag is TRUE."));
  }
  //
  // Early out if caller does not want signature information.
  //
  if (BisDataSigInfo == NULL) {
    return BisPtr;
  }
  //
  // Get BIS signature information.
  // If the signature information cannot be read or is invalid,
  // shutdown BIS and return NULL.
  //
  DEBUG ((EFI_D_INFO, "\nBisPtr->GetSignatureInfo()  "));

  EfiStatus = BisPtr->GetSignatureInfo (*BisAppHandle, BisDataSigInfo);

  if (EFI_ERROR (EfiStatus)) {
    DEBUG (
      (EFI_D_WARN,
      "\nPxebcBisStart()""\n  BisPtr_GetSignatureInfo()  %r (%xh)\n",
      EfiStatus,
      EfiStatus)
      );

    BisPtr->Shutdown (*BisAppHandle);
    return NULL;
  }

  if (*BisDataSigInfo == NULL) {
    //
    // This should never happen.
    //
    DEBUG (
      (EFI_D_NET,
      "\nPxebcBisStart()""\n  BisPtr->GetSignatureInfo()  Data pointer is NULL!\n")
      );

    BisPtr->Shutdown (*BisAppHandle);
    return NULL;
  }

  if ((*BisDataSigInfo)->Length < sizeof (EFI_BIS_SIGNATURE_INFO) ||
      (*BisDataSigInfo)->Length % sizeof (EFI_BIS_SIGNATURE_INFO) ||
      (*BisDataSigInfo)->Length > sizeof (EFI_BIS_SIGNATURE_INFO) * 63
      ) {
    //
    // This should never happen.
    //
    DEBUG (
      (EFI_D_NET,
      "\nPxebcBisStart()""\n  BisPtr->GetSignatureInfo()  Invalid BIS siginfo length.\n")
      );

    BisPtr->Free (*BisAppHandle, *BisDataSigInfo);
    BisPtr->Shutdown (*BisAppHandle);
    return NULL;
  }

  return BisPtr;
}

VOID
PxebcBisStop (
  EFI_BIS_PROTOCOL        *BisPtr,
  BIS_APPLICATION_HANDLE  BisAppHandle,
  EFI_BIS_DATA            *BisDataSigInfo
  )
/*++
Routine description:
  Stop the BIS interface and release allocations.

Parameters:
  BisPtr := Pointer to BIS interface
  BisAppHandle := BIS application handle
  BisDataSigInfo := Pointer to BIS signature information data

Returns:

--*/
{
  if (BisPtr == NULL) {
    return ;
  }
  //
  // Free BIS allocated resources and shutdown BIS.
  // Return TRUE - BIS support is officially detected.
  //
  if (BisDataSigInfo != NULL) {
    BisPtr->Free (BisAppHandle, BisDataSigInfo);
  }

  BisPtr->Shutdown (BisAppHandle);
}

BOOLEAN
PxebcBisVerify (
  PXE_BASECODE_DEVICE *Private,
  VOID                *FileBuffer,
  UINTN               FileLength,
  VOID                *CredentialBuffer,
  UINTN               CredentialLength
  )
/*++
Routine description:
  Verify image and credential file.

Parameters:
  Private := Pointer to PxeBc interface
  FileBuffer := Pointer to image buffer
  FileLength := Image length in bytes
  CredentialBuffer := Pointer to credential buffer
  CredentialLength := Credential length in bytes

Returns:
  TRUE := verified
  FALSE := not verified
--*/
{
  EFI_BIS_PROTOCOL        *BisPtr;
  BIS_APPLICATION_HANDLE  BisAppHandle;
  EFI_BIS_DATA            FileData;
  EFI_BIS_DATA            CredentialData;
  EFI_STATUS              EfiStatus;
  BOOLEAN                 IsVerified;

  if (Private == NULL || FileBuffer == NULL || FileLength == 0 || CredentialBuffer == NULL || CredentialLength == 0) {
    return FALSE;
  }

  BisPtr = PxebcBisStart (Private, &BisAppHandle, NULL);

  if (BisPtr == NULL) {
    return FALSE;
  }

  FileData.Length       = (UINT32) FileLength;
  FileData.Data         = FileBuffer;
  CredentialData.Length = (UINT32) CredentialLength;
  CredentialData.Data   = CredentialBuffer;

  EfiStatus = BisPtr->VerifyBootObject (
                        BisAppHandle,
                        &CredentialData,
                        &FileData,
                        &IsVerified
                        );

  PxebcBisStop (BisPtr, BisAppHandle, NULL);

  return (BOOLEAN) ((EFI_ERROR (EfiStatus)) ? FALSE : (IsVerified ? TRUE : FALSE));
}

BOOLEAN
PxebcBisDetect (
  PXE_BASECODE_DEVICE *Private
  )
/*++
Routine description:
  Check for BIS interface presence.

Parameters:
  Private := Pointer to PxeBc interface

Returns:
  TRUE := BIS present
  FALSE := BIS not present
--*/
{
  EFI_BIS_PROTOCOL        *BisPtr;
  BIS_APPLICATION_HANDLE  BisAppHandle;
  EFI_BIS_DATA            *BisDataSigInfo;

  BisPtr = PxebcBisStart (Private, &BisAppHandle, &BisDataSigInfo);

  if (BisPtr == NULL) {
    return FALSE;
  }

  PxebcBisStop (BisPtr, BisAppHandle, BisDataSigInfo);

  return TRUE;
}

VOID *BCNotifyReg;

EFI_STATUS
EFIAPI
BcStart (
  IN EFI_PXE_BASE_CODE_PROTOCOL *This,
  IN BOOLEAN                    UseIPv6
  )
/*++

  Routine Description:
    Start and initialize the BaseCode protocol, Simple Network protocol and UNDI.

  Arguments:
    Private                - Pointer to Pxe BaseCode Protocol
    UseIPv6            - Do we want to support IPv6?

  Returns:
    EFI_SUCCESS
    EFI_INVALID_PARAMETER
    EFI_UNSUPPORTED
    EFI_ALREADY_STARTED
    EFI_OUT_OF_RESOURCES
    Status is also returned from SNP.Start() and SNP.Initialize().

--*/
{
  EFI_SIMPLE_NETWORK_PROTOCOL *SnpPtr;
  EFI_SIMPLE_NETWORK_MODE     *SnpModePtr;
  EFI_STATUS                  StatCode;
  PXE_BASECODE_DEVICE         *Private;

  //
  // Lock the instance data
  //
  StatCode = EFI_SUCCESS;

  if (This == NULL) {
    DEBUG ((EFI_D_ERROR, "BC *This pointer == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  Private = CR (This, PXE_BASECODE_DEVICE, EfiBc, PXE_BASECODE_DEVICE_SIGNATURE);

  if (Private == NULL) {
    DEBUG ((EFI_D_ERROR, "PXE_BASECODE_DEVICE pointer == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  EfiAcquireLock (&Private->Lock);

  //
  // Make sure BaseCode is not already started.
  //
  if (This->Mode->Started) {
    DEBUG ((EFI_D_WARN, "\nBcStart()  BC is already started.\n"));
    EfiReleaseLock (&Private->Lock);
    return EFI_ALREADY_STARTED;
  }

  //
  // Fail if IPv6 is requested and not supported.
  //
  if (UseIPv6) {
    DEBUG ((EFI_D_WARN, "\nBcStart()  IPv6 is not supported.\n"));
    EfiReleaseLock (&Private->Lock);
    return EFI_UNSUPPORTED;
  }

  //
  // Setup shortcuts to SNP protocol and data structure.
  //
  SnpPtr      = Private->SimpleNetwork;
  SnpModePtr  = SnpPtr->Mode;

  //
  // Start and initialize SNP.
  //
  if (SnpModePtr->State == EfiSimpleNetworkStopped) {
    StatCode = (*SnpPtr->Start) (SnpPtr);

    if (SnpModePtr->State != EfiSimpleNetworkStarted) {
      DEBUG ((EFI_D_WARN, "\nBcStart()  Could not start SNP.\n"));
      EfiReleaseLock (&Private->Lock);
      return StatCode;
    }
  }
  //
  // acquire memory for mode and transmit/receive buffers
  //
  if (SnpModePtr->State == EfiSimpleNetworkStarted) {
    StatCode = (*SnpPtr->Initialize) (SnpPtr, 0, 0);

    if (SnpModePtr->State != EfiSimpleNetworkInitialized) {
      DEBUG ((EFI_D_WARN, "\nBcStart()  Could not initialize SNP."));
      EfiReleaseLock (&Private->Lock);
      return StatCode;
    }
  }
  //
  // Dump debug info.
  //
  DEBUG ((EFI_D_INFO, "\nBC Start()"));
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->State                    %Xh",
    SnpModePtr->State)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->HwAddressSize            %Xh",
    SnpModePtr->HwAddressSize)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->MediaHeaderSize          %Xh",
    SnpModePtr->MediaHeaderSize)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->MaxPacketSize            %Xh",
    SnpModePtr->MaxPacketSize)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->MacAddressChangeable     %Xh",
    SnpModePtr->MacAddressChangeable)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->MultipleTxSupported      %Xh",
    SnpModePtr->MultipleTxSupported)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->CurrentAddress           %Xh",
     *((UINTN *)&SnpModePtr->CurrentAddress))
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->BroadcastAddress         %Xh",
    *((UINTN *)&SnpModePtr->BroadcastAddress))
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->PermanentAddress         %Xh",
    *((UINTN *)&SnpModePtr->PermanentAddress))
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->NvRamSize                %Xh",
    SnpModePtr->NvRamSize)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->NvRamAccessSize          %Xh",
    SnpModePtr->NvRamAccessSize)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->ReceiveFilterMask        %Xh",
    SnpModePtr->ReceiveFilterMask)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->ReceiveFilterSetting     %Xh",
    SnpModePtr->ReceiveFilterSetting)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->MCastFilterCount         %Xh",
    SnpModePtr->MCastFilterCount)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->MCastFilter              %Xh",
    SnpModePtr->MCastFilter)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->IfType                   %Xh",
    SnpModePtr->IfType)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->MediaPresentSupported    %Xh",
    SnpModePtr->MediaPresentSupported)
    );
  DEBUG (
    (EFI_D_INFO,
    "\nSnpModePtr->MediaPresent             %Xh",
    SnpModePtr->MediaPresent)
    );

  //
  // If media check is supported and there is no media,
  // return error to caller.
  //
  if (SnpModePtr->MediaPresentSupported && !SnpModePtr->MediaPresent) {
    DEBUG ((EFI_D_WARN, "\nBcStart()  Media not present.\n"));
    EfiReleaseLock (&Private->Lock);
    return EFI_NO_MEDIA;
  }
  //
  // Allocate Tx/Rx buffers
  //
  Private->TransmitBufferPtr = AllocateZeroPool (BUFFER_ALLOCATE_SIZE);
  if (Private->TransmitBufferPtr == NULL) {
    DEBUG ((EFI_D_NET, "\nBcStart()  Could not alloc TxBuf.\n"));
    EfiReleaseLock (&Private->Lock);
    return EFI_OUT_OF_RESOURCES;
  }

  Private->ReceiveBufferPtr = AllocateZeroPool (BUFFER_ALLOCATE_SIZE);
  if (Private->ReceiveBufferPtr == NULL) {
    DEBUG ((EFI_D_NET, "\nBcStart()  Could not alloc RxBuf.\n"));
    FreePool (Private->TransmitBufferPtr);
    EfiReleaseLock (&Private->Lock);
    return EFI_OUT_OF_RESOURCES;
  }

  Private->TftpErrorBuffer = AllocatePool (256);
  if (Private->TftpErrorBuffer == NULL) {
    FreePool (Private->ReceiveBufferPtr);
    FreePool (Private->TransmitBufferPtr);
    EfiReleaseLock (&Private->Lock);
    return EFI_OUT_OF_RESOURCES;
  }

  Private->TftpAckBuffer = AllocatePool (256);
  if (Private->TftpAckBuffer == NULL) {
    FreePool (Private->TftpErrorBuffer);
    FreePool (Private->ReceiveBufferPtr);
    FreePool (Private->TransmitBufferPtr);
    EfiReleaseLock (&Private->Lock);
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Initialize private BaseCode instance data
  //
  do {
    Private->RandomPort = (UINT16) (Private->RandomPort + PXE_RND_PORT_LOW + Random (Private));
  } while (Private->RandomPort < PXE_RND_PORT_LOW);

  Private->Igmpv1TimeoutEvent = NULL;
  Private->UseIgmpv1Reporting = TRUE;
  Private->IpLength           = IP_ADDRESS_LENGTH (Private->EfiBc.Mode);

  //
  // Initialize Mode structure
  //
  //
  // check for callback protocol and set boolean
  //
  SetMakeCallback (Private);
  Private->EfiBc.Mode->Started              = TRUE;
  Private->EfiBc.Mode->TTL                  = DEFAULT_TTL;
  Private->EfiBc.Mode->ToS                  = DEFAULT_ToS;
  Private->EfiBc.Mode->UsingIpv6            = UseIPv6;
  Private->EfiBc.Mode->DhcpDiscoverValid    = FALSE;
  Private->EfiBc.Mode->DhcpAckReceived      = FALSE;
  Private->EfiBc.Mode->ProxyOfferReceived   = FALSE;
  Private->EfiBc.Mode->PxeDiscoverValid     = FALSE;
  Private->EfiBc.Mode->PxeReplyReceived     = FALSE;
  Private->EfiBc.Mode->PxeBisReplyReceived  = FALSE;
  Private->EfiBc.Mode->IcmpErrorReceived    = FALSE;
  Private->EfiBc.Mode->TftpErrorReceived    = FALSE;
  ZeroMem (&Private->EfiBc.Mode->StationIp, sizeof (EFI_IP_ADDRESS));
  ZeroMem (&Private->EfiBc.Mode->SubnetMask, sizeof (EFI_IP_ADDRESS));
  Private->EfiBc.Mode->IpFilter.Filters   = 0;
  Private->EfiBc.Mode->IpFilter.IpCnt     = 0;
  Private->EfiBc.Mode->ArpCacheEntries    = 0;
  Private->EfiBc.Mode->RouteTableEntries  = 0;
  ZeroMem (&Private->EfiBc.Mode->IcmpError, sizeof (EFI_PXE_BASE_CODE_ICMP_ERROR));
  ZeroMem (&Private->EfiBc.Mode->TftpError, sizeof (EFI_PXE_BASE_CODE_TFTP_ERROR));

  //
  // Set to PXE_TRUE by the BC constructor if this BC implementation
  // supports IPv6.
  //
  Private->EfiBc.Mode->Ipv6Supported = SUPPORT_IPV6;
  Private->EfiBc.Mode->Ipv6Available = FALSE;

  //
  // Set to TRUE by the BC constructor if this BC implementation
  // supports BIS.
  //
  Private->EfiBc.Mode->BisSupported = TRUE;
  Private->EfiBc.Mode->BisDetected  = PxebcBisDetect (Private);

  //
  // This field is set to PXE_TRUE by the BC Start() function.  When this
  // field is PXE_TRUE, ARP packets are sent as needed to get IP and MAC
  // addresses.  This can cause unexpected delays in the DHCP(), Discover()
  // and MTFTP() functions.  Setting this to PXE_FALSE will cause these
  // functions to fail if the required IP/MAC information is not in the
  // ARP cache.  The value of this field can be changed by an application
  // at any time.
  //
  Private->EfiBc.Mode->AutoArp = TRUE;

  //
  // Unlock the instance data
  //
  EfiReleaseLock (&Private->Lock);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BcStop (
  IN EFI_PXE_BASE_CODE_PROTOCOL *This
  )
/*++

  Routine Description:
    Stop the BaseCode protocol, Simple Network protocol and UNDI.

  Arguments:
    Private                - Pointer to Pxe BaseCode Protocol

  Returns:

    0                  - Successfully stopped
    !0                 - Failed
--*/
{
  //
  // Lock the instance data
  //
  EFI_SIMPLE_NETWORK_PROTOCOL *SnpPtr;
  EFI_SIMPLE_NETWORK_MODE     *SnpModePtr;
  EFI_STATUS                  StatCode;
  PXE_BASECODE_DEVICE         *Private;

  StatCode = EFI_SUCCESS;

  if (This == NULL) {
    DEBUG ((EFI_D_ERROR, "BC *This pointer == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  Private = CR (This, PXE_BASECODE_DEVICE, EfiBc, PXE_BASECODE_DEVICE_SIGNATURE);

  if (Private == NULL) {
    DEBUG ((EFI_D_ERROR, "PXE_BASECODE_DEVICE poiner == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  EfiAcquireLock (&Private->Lock);

  SnpPtr      = Private->SimpleNetwork;
  SnpModePtr  = SnpPtr->Mode;

  //
  // Issue BC command
  //
  StatCode = EFI_NOT_STARTED;

  if (SnpModePtr->State == EfiSimpleNetworkInitialized) {
    StatCode = (*SnpPtr->Shutdown) (SnpPtr);
  }

  if (SnpModePtr->State == EfiSimpleNetworkStarted) {
    StatCode = (*SnpPtr->Stop) (SnpPtr);
  }

  if (Private->TransmitBufferPtr != NULL) {
    FreePool (Private->TransmitBufferPtr);
    Private->TransmitBufferPtr = NULL;
  }

  if (Private->ReceiveBufferPtr != NULL) {
    FreePool (Private->ReceiveBufferPtr);
    Private->ReceiveBufferPtr = NULL;
  }

  if (Private->ArpBuffer != NULL) {
    FreePool (Private->ArpBuffer);
    Private->ArpBuffer = NULL;
  }

  if (Private->TftpErrorBuffer != NULL) {
    FreePool (Private->TftpErrorBuffer);
    Private->TftpErrorBuffer = NULL;
  }

  if (Private->TftpAckBuffer != NULL) {
    FreePool (Private->TftpAckBuffer);
    Private->TftpAckBuffer = NULL;
  }

  if (Private->Igmpv1TimeoutEvent != NULL) {
    gBS->CloseEvent (Private->Igmpv1TimeoutEvent);
    Private->Igmpv1TimeoutEvent = NULL;
  }

  Private->FileSize             = 0;
  Private->EfiBc.Mode->Started  = FALSE;

  //
  // Unlock the instance data
  //
  EfiReleaseLock (&Private->Lock);
  return StatCode;
}

const IPV4_ADDR AllSystemsGroup = { { 224, 0, 0, 1 } };

EFI_STATUS
IpFilter (
  IN PXE_BASECODE_DEVICE          *Private,
  IN EFI_PXE_BASE_CODE_IP_FILTER  *Filter
  )
/*++

  Routine Description:
    Set up the IP filter

  Arguments:
    Private                - Pointer to Pxe BaseCode Protocol
    Filter             - Pointer to the filter

  Returns:

    0                  - Successfully set the filter
    !0                 - Failed
--*/
{
  EFI_STATUS                  StatCode;
  EFI_MAC_ADDRESS             MACadds[PXE_IP_FILTER_SIZE];
  EFI_PXE_BASE_CODE_MODE      *PxebcMode;
  EFI_SIMPLE_NETWORK_PROTOCOL *SnpPtr;
  EFI_SIMPLE_NETWORK_MODE     *SnpModePtr;
  UINT32                      Enable;
  UINT32                      Disable;
  UINTN                       Index;
  UINTN                       Index2;

  PxebcMode   = Private->EfiBc.Mode;
  SnpPtr      = Private->SimpleNetwork;
  SnpModePtr  = SnpPtr->Mode;

  //
  // validate input parameters
  // must have a filter
  // must not have any extra filter bits set
  //
  if (Filter == NULL ||
      (Filter->Filters &~FILTER_BITS)
      //
      // must not have a count which is too large or with no IP list
      //
      ||
      (Filter->IpCnt && (!Filter->IpList || Filter->IpCnt > PXE_IP_FILTER_SIZE))
      //
      // must not have incompatible filters - promiscuous incompatible with anything else
      //
      ||
      (
        (Filter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS) &&
      ((Filter->Filters &~EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS) || Filter->IpCnt)
    )
      ) {
    DEBUG ((EFI_D_INFO, "\nIpFilter()  Exit #1"));
    return EFI_INVALID_PARAMETER;
  }
  //
  // promiscuous multicast incompatible with multicast in IP list
  //
  if (Filter->IpCnt && (Filter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS_MULTICAST)) {
    for (Index = 0; Index < Filter->IpCnt; ++Index) {
      if (IS_MULTICAST (&Filter->IpList[Index])) {
        DEBUG ((EFI_D_INFO, "\nIpFilter()  Exit #2"));
        return EFI_INVALID_PARAMETER;
      }
    }
  }
  //
  // leave groups for all those multicast which are no longer enabled
  //
  for (Index = 0; Index < PxebcMode->IpFilter.IpCnt; ++Index) {
    if (!IS_MULTICAST (&PxebcMode->IpFilter.IpList[Index])) {
      continue;
    }

    for (Index2 = 0; Index2 < Filter->IpCnt; ++Index2) {
      if (!CompareMem (&PxebcMode->IpFilter.IpList[Index], &Filter->IpList[Index2], IP_ADDRESS_LENGTH (PxebcMode))) {
        //
        // still enabled
        //
        break;
      }
    }
    //
    // if we didn't find it, remove from group
    //
    if (Index2 == Filter->IpCnt) {
      IgmpLeaveGroup (Private, &PxebcMode->IpFilter.IpList[Index]);
    }
  }
  //
  // set enable bits, convert multicast ip adds, join groups
  // allways leave receive broadcast enabled at hardware layer
  //
  Index2 = 0;

  if (Filter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS) {
    Enable = EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS;
  } else {
    if (Filter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS_MULTICAST) {
      Enable = EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;
    } else {
      Enable = EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST;

      for (Index = 0; Index < Filter->IpCnt; ++Index) {
        CopyMem (&(PxebcMode->IpFilter.IpList[Index]), &(Filter->IpList[Index]), sizeof (EFI_IP_ADDRESS));

        if (IS_MULTICAST (&Filter->IpList[Index])) {
          EFI_IP_ADDRESS  *TmpIp;

          Enable |= EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST;

          //
          // if this is the first group, add the all systems group to mcast list
          //
          if (!Index2)
          {
            TmpIp = (EFI_IP_ADDRESS *) &AllSystemsGroup;
            --Index;
          } else {
            TmpIp = (EFI_IP_ADDRESS *) &Filter->IpList[Index];
          }
          //
          // get MAC address of IP
          //
          StatCode = (*SnpPtr->MCastIpToMac) (SnpPtr, PxebcMode->UsingIpv6, TmpIp, &MACadds[Index2++]);

          if (EFI_ERROR (StatCode)) {
            DEBUG (
              (EFI_D_INFO,
              "\nIpFilter()  Exit #2  %Xh (%r)",
              StatCode,
              StatCode)
              );
            return StatCode;
          }
        } else {
          Enable |= EFI_SIMPLE_NETWORK_RECEIVE_UNICAST;
        }
      }
    }

    if (Filter->Filters & EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP) {
      Enable |= EFI_SIMPLE_NETWORK_RECEIVE_UNICAST;
    }
  }
  //
  // if nothing changed, just return
  //
  DEBUG (
    (EFI_D_INFO,
    "\nsnp->ReceiveFilterSetting == %Xh  Filter->IpCnt == %Xh",
    SnpModePtr->ReceiveFilterSetting,
    Filter->IpCnt)
    );

  if (SnpModePtr->ReceiveFilterSetting == Enable && !Filter->IpCnt) {
    DEBUG ((EFI_D_INFO, "\nIpFilter()  Exit #4"));
    return EFI_SUCCESS;
  }
  //
  // disable those currently set but not set in new filter
  //
  Disable                   = SnpModePtr->ReceiveFilterSetting &~Enable;

  StatCode                  = SnpPtr->ReceiveFilters (SnpPtr, Enable, Disable, FALSE, Index2, MACadds);

  PxebcMode->IpFilter.IpCnt = Filter->IpCnt;

  //
  // join groups for all multicast in list
  //
  for (Index = 0; Index < Filter->IpCnt; ++Index) {
    if (IS_MULTICAST (&Filter->IpList[Index])) {
      IgmpJoinGroup (Private, &Filter->IpList[Index]);
    }
  }

  DEBUG ((EFI_D_INFO, "\nIpFilter()  Exit #5  %Xh (%r)", StatCode, StatCode));

  return StatCode;
}

EFI_STATUS
EFIAPI
BcIpFilter (
  IN EFI_PXE_BASE_CODE_PROTOCOL  *This,
  IN EFI_PXE_BASE_CODE_IP_FILTER *Filter
  )
/*++

  Routine Description:
    Call the IP filter

  Arguments:
    Private                - Pointer to Pxe BaseCode Protocol
    Filter             - Pointer to the filter

  Returns:

    0                  - Successfully set the filter
    !0                 - Failed
--*/
{
  EFI_STATUS          StatCode;
  PXE_BASECODE_DEVICE *Private;

  //
  // Lock the instance data and make sure started
  //
  StatCode = EFI_SUCCESS;

  if (This == NULL) {
    DEBUG ((EFI_D_ERROR, "BC *This pointer == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  Private = CR (This, PXE_BASECODE_DEVICE, EfiBc, PXE_BASECODE_DEVICE_SIGNATURE);

  if (Private == NULL) {
    DEBUG ((EFI_D_ERROR, "PXE_BASECODE_DEVICE poiner == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  EfiAcquireLock (&Private->Lock);

  if (This->Mode == NULL || !This->Mode->Started) {
    DEBUG ((EFI_D_ERROR, "BC was not started."));
    EfiReleaseLock (&Private->Lock);
    return EFI_NOT_STARTED;
  }

  if (Filter == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Issue BC command
  //
  StatCode = IpFilter (Private, Filter);

  //
  // Unlock the instance data
  //
  EfiReleaseLock (&Private->Lock);
  return StatCode;
}

EFI_STATUS
EFIAPI
BcSetParameters (
  EFI_PXE_BASE_CODE_PROTOCOL  *This,
  BOOLEAN                     *AutoArpPtr,
  BOOLEAN                     *SendGuidPtr,
  UINT8                       *TimeToLivePtr,
  UINT8                       *TypeOfServicePtr,
  BOOLEAN                     *MakeCallbackPtr
  )
/*++

  Routine Description:
    Set the Base Code behavior parameters

  Arguments:
    This               - Pointer to Pxe BaseCode Protocol
    AutoArpPtr           - Boolean to do ARP stuff
    SendGuidPtr          - Boolean whether or not to send GUID info
    TimeToLivePtr               - Value for Total time to live
    TypeOfServicePtr               - Value for Type of Service
    MakeCallbackPtr      - Boolean to determine if we make callbacks

  Returns:

    0                  - Successfully set the parameters
    !0                 - Failed
--*/
{
  EFI_PXE_BASE_CODE_MODE  *PxebcMode;
  EFI_GUID                TmpGuid;
  CHAR8                   *SerialNumberPtr;
  EFI_STATUS              StatCode;
  PXE_BASECODE_DEVICE     *Private;

  //
  // Lock the instance data and make sure started
  //
  StatCode = EFI_SUCCESS;

  if (This == NULL) {
    DEBUG ((EFI_D_ERROR, "BC *This pointer == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  Private = CR (This, PXE_BASECODE_DEVICE, EfiBc, PXE_BASECODE_DEVICE_SIGNATURE);

  if (Private == NULL) {
    DEBUG ((EFI_D_ERROR, "PXE_BASECODE_DEVICE poiner == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  EfiAcquireLock (&Private->Lock);

  if (This->Mode == NULL || !This->Mode->Started) {
    DEBUG ((EFI_D_ERROR, "BC was not started."));
    EfiReleaseLock (&Private->Lock);
    return EFI_NOT_STARTED;
  }

  DEBUG ((EFI_D_INFO, "\nSetParameters()  Entry.  "));

  PxebcMode = Private->EfiBc.Mode;
  StatCode  = EFI_SUCCESS;

  if (SendGuidPtr != NULL) {
    if (*SendGuidPtr) {
      if (PxeBcLibGetSmbiosSystemGuidAndSerialNumber (&TmpGuid, &SerialNumberPtr) != EFI_SUCCESS) {
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  if (MakeCallbackPtr != NULL) {
    if (*MakeCallbackPtr) {
      if (!SetMakeCallback (Private)) {
        return EFI_INVALID_PARAMETER;
      }
    }

    PxebcMode->MakeCallbacks = *MakeCallbackPtr;
  }

  if (AutoArpPtr != NULL) {
    PxebcMode->AutoArp = *AutoArpPtr;
  }

  if (SendGuidPtr != NULL) {
    PxebcMode->SendGUID = *SendGuidPtr;
  }

  if (TimeToLivePtr != NULL) {
    PxebcMode->TTL = *TimeToLivePtr;
  }

  if (TypeOfServicePtr != NULL) {
    PxebcMode->ToS = *TypeOfServicePtr;
  }
  //
  // Unlock the instance data
  //
  DEBUG ((EFI_D_INFO, "\nSetparameters()  Exit = %xh  ", StatCode));

  EfiReleaseLock (&Private->Lock);
  return StatCode;
}
//
// //////////////////////////////////////////////////////////
//
//  BC Set Station IP Routine
//
EFI_STATUS
EFIAPI
BcSetStationIP (
  IN EFI_PXE_BASE_CODE_PROTOCOL *This,
  IN EFI_IP_ADDRESS             *StationIpPtr,
  IN EFI_IP_ADDRESS             *SubnetMaskPtr
  )
/*++

  Routine Description:
    Set the station IP address

  Arguments:
    This                 - Pointer to Pxe BaseCode Protocol
    StationIpPtr         - Pointer to the requested IP address to set in base code
    SubnetMaskPtr        - Pointer to the requested subnet mask for the base code

  Returns:

    EFI_SUCCESS          - Successfully set the parameters
    EFI_NOT_STARTED      - BC has not started
--*/
{
  EFI_PXE_BASE_CODE_MODE  *PxebcMode;
  EFI_STATUS              StatCode;
  PXE_BASECODE_DEVICE     *Private;
  UINT32                  SubnetMask;

  //
  // Lock the instance data and make sure started
  //
  StatCode = EFI_SUCCESS;

  if (This == NULL) {
    DEBUG ((EFI_D_ERROR, "BC *This pointer == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  Private = CR (This, PXE_BASECODE_DEVICE, EfiBc, PXE_BASECODE_DEVICE_SIGNATURE);

  if (Private == NULL) {
    DEBUG ((EFI_D_ERROR, "PXE_BASECODE_DEVICE poiner == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  EfiAcquireLock (&Private->Lock);

  if (This->Mode == NULL || !This->Mode->Started) {
    DEBUG ((EFI_D_ERROR, "BC was not started."));
    StatCode = EFI_NOT_STARTED;
    goto RELEASE_LOCK;
  }

  PxebcMode = Private->EfiBc.Mode;

  if (!Private->GoodStationIp && ((StationIpPtr == NULL) || (SubnetMaskPtr == NULL))) {
    //
    // It's not allowed to only set one of the two addresses while there isn't a previous
    // GOOD address configuration.
    //
    StatCode = EFI_INVALID_PARAMETER;
    goto RELEASE_LOCK;
  }

  if (SubnetMaskPtr != NULL) {
    SubnetMask = SubnetMaskPtr->Addr[0];

    if (SubnetMask & (SubnetMask + 1)) {
      //
      // the subnet mask is valid if it's with leading continuous 1 bits.
      //
      StatCode = EFI_INVALID_PARAMETER;
      goto RELEASE_LOCK;
    }
  } else {
    SubnetMaskPtr = &PxebcMode->SubnetMask;
    SubnetMask    = SubnetMaskPtr->Addr[0];
  }

  if (StationIpPtr == NULL) {
    StationIpPtr = &PxebcMode->StationIp;
  }

  if (!IS_INADDR_UNICAST (StationIpPtr) ||
      ((StationIpPtr->Addr[0] | SubnetMask) == BROADCAST_IPv4)) {
    //
    // The station IP is not a unicast address.
    //
    StatCode = EFI_INVALID_PARAMETER;
    goto RELEASE_LOCK;
  }

  CopyMem (&PxebcMode->StationIp, StationIpPtr, sizeof (EFI_IP_ADDRESS));
  CopyMem (&PxebcMode->SubnetMask, SubnetMaskPtr, sizeof (EFI_IP_ADDRESS));

  Private->GoodStationIp = TRUE;

RELEASE_LOCK:
  //
  // Unlock the instance data
  //
  EfiReleaseLock (&Private->Lock);

  return StatCode;
}

EFI_DRIVER_BINDING_PROTOCOL gPxeBcDriverBinding = {
  PxeBcDriverSupported,
  PxeBcDriverStart,
  PxeBcDriverStop,
  0xa,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
PxeBcDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

  Routine Description:
    Test to see if this driver supports Controller. Any Controller
    than contains a Snp protocol can be supported.

  Arguments:
    This                - Protocol instance pointer.
    Controller          - Handle of device to test.
    RemainingDevicePath - Not used.

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_ALREADY_STARTED - This driver is already running on this device.
    other               - This driver does not support this device.

--*/
{
  EFI_STATUS                  Status;
  EFI_SIMPLE_NETWORK_PROTOCOL *SnpPtr;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **) &SnpPtr,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiSimpleNetworkProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return Status;
}

EFI_STATUS
EFIAPI
PxeBcDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

  Routine Description:
    Start the Base code driver.

  Arguments:
    This                - Protocol instance pointer.
    Controller          - Handle of device to test.
    RemainingDevicePath - Not used.

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_ALREADY_STARTED - This driver is already running on this device.
    other               - This driver does not support this device.

--*/
{
  EFI_STATUS          Status;
  PXE_BASECODE_DEVICE *Private;
  LOADFILE_DEVICE     *pLF;

  //
  // Allocate structures needed by BaseCode and LoadFile protocols.
  //
  Private = AllocateZeroPool (sizeof (PXE_BASECODE_DEVICE));

  if (Private == NULL ) {
    DEBUG ((EFI_D_NET, "\nBcNotifySnp()  Could not alloc PXE_BASECODE_DEVICE structure.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  pLF = AllocateZeroPool (sizeof (LOADFILE_DEVICE));
  if (pLF == NULL) {
    DEBUG ((EFI_D_NET, "\nBcNotifySnp()  Could not alloc LOADFILE_DEVICE structure.\n"));
    FreePool (Private);
    return EFI_OUT_OF_RESOURCES;
  }

  Private->EfiBc.Mode = AllocateZeroPool (sizeof (EFI_PXE_BASE_CODE_MODE));
  if (Private->EfiBc.Mode == NULL) {
    DEBUG ((EFI_D_NET, "\nBcNotifySnp()  Could not alloc Mode structure.\n"));
    FreePool (Private);
    FreePool (pLF);
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Lock access, just in case
  //
  EfiInitializeLock (&Private->Lock, TPL_CALLBACK);
  EfiAcquireLock (&Private->Lock);

  EfiInitializeLock (&pLF->Lock, TPL_CALLBACK);
  EfiAcquireLock (&pLF->Lock);

  //
  // Initialize PXE structure
  //
  //
  // First initialize the internal 'private' data that the application
  // does not see.
  //
  Private->Signature  = PXE_BASECODE_DEVICE_SIGNATURE;
  Private->Handle     = Controller;

  //
  // Get the NII interface
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  (VOID **) &Private->NiiPtr,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiNetworkInterfaceIdentifierProtocolGuid,
                    (VOID **) &Private->NiiPtr,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );

    if (EFI_ERROR (Status)) {
      goto PxeBcError;
    }
  }
  //
  // Get the Snp interface
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **) &Private->SimpleNetwork,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    goto PxeBcError;
  }

  //
  // Next, initialize the external 'public' data that
  // the application does see.
  //
  Private->EfiBc.Revision       = EFI_PXE_BASE_CODE_INTERFACE_REVISION;
  Private->EfiBc.Start          = BcStart;
  Private->EfiBc.Stop           = BcStop;
  Private->EfiBc.Dhcp           = BcDhcp;
  Private->EfiBc.Discover       = BcDiscover;
  Private->EfiBc.Mtftp          = BcMtftp;
  Private->EfiBc.UdpWrite       = BcUdpWrite;
  Private->EfiBc.UdpRead        = BcUdpRead;
  Private->EfiBc.Arp            = BcArp;
  Private->EfiBc.SetIpFilter    = BcIpFilter;
  Private->EfiBc.SetParameters  = BcSetParameters;
  Private->EfiBc.SetStationIp   = BcSetStationIP;
  Private->EfiBc.SetPackets     = BcSetPackets;

  //
  // Initialize BaseCode Mode structure
  //
  Private->EfiBc.Mode->Started    = FALSE;
  Private->EfiBc.Mode->TTL        = DEFAULT_TTL;
  Private->EfiBc.Mode->ToS        = DEFAULT_ToS;
  Private->EfiBc.Mode->UsingIpv6  = FALSE;
  Private->EfiBc.Mode->AutoArp    = TRUE;

  //
  // Set to PXE_TRUE by the BC constructor if this BC
  // implementation supports IPv6.
  //
  Private->EfiBc.Mode->Ipv6Supported = SUPPORT_IPV6;
  Private->EfiBc.Mode->Ipv6Available = FALSE;

  //
  // Set to TRUE by the BC constructor if this BC
  // implementation supports BIS.
  //
  Private->EfiBc.Mode->BisSupported = TRUE;
  Private->EfiBc.Mode->BisDetected  = PxebcBisDetect (Private);

  //
  // Initialize LoadFile structure.
  //
  pLF->Signature          = LOADFILE_DEVICE_SIGNATURE;
  pLF->LoadFile.LoadFile  = LoadFile;
  pLF->Private            = Private;

  //
  // Install protocol interfaces.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiPxeBaseCodeProtocolGuid,
                  &Private->EfiBc,
                  &gEfiLoadFileProtocolGuid,
                  &pLF->LoadFile,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiSimpleNetworkProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );

    goto PxeBcError;
  }
  //
  // Release locks.
  //
  EfiReleaseLock (&pLF->Lock);
  EfiReleaseLock (&Private->Lock);
  return Status;

PxeBcError: ;
  FreePool (Private->EfiBc.Mode);
  FreePool (Private);
  FreePool (pLF);
  return Status;
}

EFI_STATUS
EFIAPI
PxeBcDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  )
/*++

  Routine Description:
    Stop the Base code driver.

  Arguments:
    This                - Protocol instance pointer.
    Controller          - Handle of device to test.
    NumberOfChildren    - Not used
    ChildHandleBuffer   - Not used

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_ALREADY_STARTED - This driver is already running on this device.
    other               - This driver does not support this device.

--*/
{
  EFI_STATUS              Status;
  EFI_LOAD_FILE_PROTOCOL  *LfProtocol;
  LOADFILE_DEVICE         *LoadDevice;

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiLoadFileProtocolGuid,
                  (VOID **) &LfProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  LoadDevice = EFI_LOAD_FILE_DEV_FROM_THIS (LfProtocol);

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gEfiLoadFileProtocolGuid,
                  &LoadDevice->LoadFile,
                  &gEfiPxeBaseCodeProtocolGuid,
                  &LoadDevice->Private->EfiBc,
                  NULL
                  );

  if (!EFI_ERROR (Status)) {

    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiSimpleNetworkProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );

    FreePool (LoadDevice->Private->EfiBc.Mode);
    FreePool (LoadDevice->Private);
    FreePool (LoadDevice);
  }

  return Status;
}

EFI_STATUS
EFIAPI
InitializeBCDriver (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
/*++

  Routine Description:
    Initialize the base code drivers and install the driver binding

  Arguments:
    Standard EFI Image Entry

  Returns:
    EFI_SUCCESS         - This driver was successfully bound

--*/
{
  InitArpHeader ();
  OptionsStrucInit ();

  return EFI_SUCCESS;
}

/* eof - bc.c */
