/** @file
#
#  Copyright (c) 2014, ARM Ltd. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#
#**/

#include <Protocol/AndroidFastbootTransport.h>
#include <Protocol/Dhcp4.h>
#include <Protocol/Tcp4.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/SimpleTextOut.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#define IP4_ADDR_TO_STRING(IpAddr, IpAddrString)  UnicodeSPrint (      \
                                                   IpAddrString,       \
                                                   16 * 2,             \
                                                   L"%d.%d.%d.%d",     \
                                                   IpAddr.Addr[0],     \
                                                   IpAddr.Addr[1],     \
                                                   IpAddr.Addr[2],     \
                                                   IpAddr.Addr[3]      \
                                                   );

// Fastboot says max packet size is 512, but FASTBOOT_TRANSPORT_PROTOCOL
// doesn't place a limit on the size of buffers returned by Receive.
// (This isn't actually a packet size - it's just the size of the buffers we
//  pass to the TCP driver to fill with received data.)
// We can achieve much better performance by doing this in larger chunks.
#define RX_FRAGMENT_SIZE  2048

STATIC EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *mTextOut;

STATIC EFI_TCP4_PROTOCOL  *mTcpConnection;
STATIC EFI_TCP4_PROTOCOL  *mTcpListener;

STATIC EFI_EVENT  mReceiveEvent;

STATIC EFI_SERVICE_BINDING_PROTOCOL  *mTcpServiceBinding;
STATIC EFI_HANDLE                    mTcpHandle = NULL;

// We only ever use one IO token for receive and one for transmit. To save
// repeatedly allocating and freeing, just allocate statically and re-use.
#define NUM_RX_TOKENS  16
#define TOKEN_NEXT(Index)  (((Index) + 1) % NUM_RX_TOKENS)

STATIC UINTN                   mNextSubmitIndex;
STATIC UINTN                   mNextReceiveIndex;
STATIC EFI_TCP4_IO_TOKEN       mReceiveToken[NUM_RX_TOKENS];
STATIC EFI_TCP4_RECEIVE_DATA   mRxData[NUM_RX_TOKENS];
STATIC EFI_TCP4_IO_TOKEN       mTransmitToken;
STATIC EFI_TCP4_TRANSMIT_DATA  mTxData;
// We also reuse the accept token
STATIC EFI_TCP4_LISTEN_TOKEN  mAcceptToken;
// .. and the close token
STATIC EFI_TCP4_CLOSE_TOKEN  mCloseToken;

// List type for queued received packets
typedef struct _FASTBOOT_TCP_PACKET_LIST {
  LIST_ENTRY    Link;
  VOID          *Buffer;
  UINTN         BufferSize;
} FASTBOOT_TCP_PACKET_LIST;

STATIC LIST_ENTRY  mPacketListHead;

STATIC
VOID
EFIAPI
DataReceived (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/*
  Helper function to set up a receive IO token and call Tcp->Receive
*/
STATIC
EFI_STATUS
SubmitRecieveToken (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *FragmentBuffer;

  Status = EFI_SUCCESS;

  FragmentBuffer = AllocatePool (RX_FRAGMENT_SIZE);
  ASSERT (FragmentBuffer != NULL);
  if (FragmentBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "TCP Fastboot out of resources"));
    return EFI_OUT_OF_RESOURCES;
  }

  mRxData[mNextSubmitIndex].DataLength                      = RX_FRAGMENT_SIZE;
  mRxData[mNextSubmitIndex].FragmentTable[0].FragmentLength = RX_FRAGMENT_SIZE;
  mRxData[mNextSubmitIndex].FragmentTable[0].FragmentBuffer = FragmentBuffer;

  Status = mTcpConnection->Receive (mTcpConnection, &mReceiveToken[mNextSubmitIndex]);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TCP Receive: %r\n", Status));
    FreePool (FragmentBuffer);
  }

  mNextSubmitIndex = TOKEN_NEXT (mNextSubmitIndex);
  return Status;
}

/*
  Event notify function for when we have closed our TCP connection.
  We can now start listening for another connection.
*/
STATIC
VOID
ConnectionClosed (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  // Possible bug in EDK2 TCP4 driver: closing a connection doesn't remove its
  // PCB from the list of live connections. Subsequent attempts to Configure()
  // a TCP instance with the same local port will fail with INVALID_PARAMETER.
  // Calling Configure with NULL is a workaround for this issue.
  Status = mTcpConnection->Configure (mTcpConnection, NULL);

  mTcpConnection = NULL;

  Status = mTcpListener->Accept (mTcpListener, &mAcceptToken);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TCP Accept: %r\n", Status));
  }
}

STATIC
VOID
CloseReceiveEvents (
  VOID
  )
{
  UINTN  Index;

  for (Index = 0; Index < NUM_RX_TOKENS; Index++) {
    gBS->CloseEvent (mReceiveToken[Index].CompletionToken.Event);
  }
}

/*
  Event notify function to be called when we receive TCP data.
*/
STATIC
VOID
EFIAPI
DataReceived (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                Status;
  FASTBOOT_TCP_PACKET_LIST  *NewEntry;
  EFI_TCP4_IO_TOKEN         *ReceiveToken;

  ReceiveToken = &mReceiveToken[mNextReceiveIndex];

  Status = ReceiveToken->CompletionToken.Status;

  if (Status == EFI_CONNECTION_FIN) {
    //
    // Remote host closed connection. Close our end.
    //

    CloseReceiveEvents ();

    Status = mTcpConnection->Close (mTcpConnection, &mCloseToken);
    ASSERT_EFI_ERROR (Status);

    return;
  }

  //
  // Add an element to the receive queue
  //

  NewEntry = AllocatePool (sizeof (FASTBOOT_TCP_PACKET_LIST));
  if (NewEntry == NULL) {
    DEBUG ((DEBUG_ERROR, "TCP Fastboot: Out of resources\n"));
    return;
  }

  mNextReceiveIndex = TOKEN_NEXT (mNextReceiveIndex);

  if (!EFI_ERROR (Status)) {
    NewEntry->Buffer
      = ReceiveToken->Packet.RxData->FragmentTable[0].FragmentBuffer;
    NewEntry->BufferSize
      = ReceiveToken->Packet.RxData->FragmentTable[0].FragmentLength;

    // Prepare to receive more data
    SubmitRecieveToken ();
  } else {
    // Fatal receive error. Put an entry with NULL in the queue, signifying
    // to return EFI_DEVICE_ERROR from TcpFastbootTransportReceive.
    NewEntry->Buffer     = NULL;
    NewEntry->BufferSize = 0;

    DEBUG ((DEBUG_ERROR, "\nTCP Fastboot Receive error: %r\n", Status));
  }

  InsertTailList (&mPacketListHead, &NewEntry->Link);

  Status = gBS->SignalEvent (mReceiveEvent);
  ASSERT_EFI_ERROR (Status);
}

/*
  Event notify function to be called when we accept an incoming TCP connection.
*/
STATIC
VOID
EFIAPI
ConnectionAccepted (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_TCP4_LISTEN_TOKEN  *AcceptToken;
  EFI_STATUS             Status;
  UINTN                  Index;

  AcceptToken = (EFI_TCP4_LISTEN_TOKEN *)Context;
  Status      = AcceptToken->CompletionToken.Status;

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TCP Fastboot: Connection Error: %r\n", Status));
    return;
  }

  DEBUG ((DEBUG_ERROR, "TCP Fastboot: Connection Received.\n"));

  //
  // Accepting a new TCP connection creates a new instance of the TCP protocol.
  // Open it and prepare to receive on it.
  //

  Status = gBS->OpenProtocol (
                  AcceptToken->NewChildHandle,
                  &gEfiTcp4ProtocolGuid,
                  (VOID **)&mTcpConnection,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Open TCP Connection: %r\n", Status));
    return;
  }

  mNextSubmitIndex  = 0;
  mNextReceiveIndex = 0;

  for (Index = 0; Index < NUM_RX_TOKENS; Index++) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    DataReceived,
                    NULL,
                    &(mReceiveToken[Index].CompletionToken.Event)
                    );
    ASSERT_EFI_ERROR (Status);
  }

  for (Index = 0; Index < NUM_RX_TOKENS; Index++) {
    SubmitRecieveToken ();
  }
}

/*
  Set up TCP Fastboot transport: Configure the network device via DHCP then
  start waiting for a TCP connection on the Fastboot port.
*/
EFI_STATUS
TcpFastbootTransportStart (
  EFI_EVENT  ReceiveEvent
  )
{
  EFI_STATUS         Status;
  EFI_HANDLE         NetDeviceHandle;
  EFI_HANDLE         *HandleBuffer;
  EFI_IP4_MODE_DATA  Ip4ModeData;
  UINTN              NumHandles;
  CHAR16             IpAddrString[16];
  UINTN              Index;

  EFI_TCP4_CONFIG_DATA  TcpConfigData = {
    0x00,                                           // IPv4 Type of Service
    255,                                            // IPv4 Time to Live
    {                                               // AccessPoint:
      TRUE,                                         // Use default address
      {
        { 0, 0, 0, 0 }
      },                                            // IP Address  (ignored - use default)
      {
        { 0, 0, 0, 0 }
      },                                            // Subnet mask (ignored - use default)
      FixedPcdGet32 (PcdAndroidFastbootTcpPort),    // Station port
      {
        { 0, 0, 0, 0 }
      },                                            // Remote address: accept any
      0,                                            // Remote Port: accept any
      FALSE                                         // ActiveFlag: be a "server"
    },
    NULL                                            // Default advanced TCP options
  };

  mReceiveEvent = ReceiveEvent;
  InitializeListHead (&mPacketListHead);

  mTextOut->OutputString (mTextOut, L"Initialising TCP Fastboot transport...\r\n");

  //
  // Open a passive TCP instance
  //

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiTcp4ServiceBindingProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Find TCP Service Binding: %r\n", Status));
    return Status;
  }

  // We just use the first network device
  NetDeviceHandle = HandleBuffer[0];

  Status =  gBS->OpenProtocol (
                   NetDeviceHandle,
                   &gEfiTcp4ServiceBindingProtocolGuid,
                   (VOID **)&mTcpServiceBinding,
                   gImageHandle,
                   NULL,
                   EFI_OPEN_PROTOCOL_GET_PROTOCOL
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Open TCP Service Binding: %r\n", Status));
    return Status;
  }

  Status = mTcpServiceBinding->CreateChild (mTcpServiceBinding, &mTcpHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TCP ServiceBinding Create: %r\n", Status));
    return Status;
  }

  Status =  gBS->OpenProtocol (
                   mTcpHandle,
                   &gEfiTcp4ProtocolGuid,
                   (VOID **)&mTcpListener,
                   gImageHandle,
                   NULL,
                   EFI_OPEN_PROTOCOL_GET_PROTOCOL
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Open TCP Protocol: %r\n", Status));
  }

  //
  // Set up re-usable tokens
  //

  for (Index = 0; Index < NUM_RX_TOKENS; Index++) {
    mRxData[Index].UrgentFlag          = FALSE;
    mRxData[Index].FragmentCount       = 1;
    mReceiveToken[Index].Packet.RxData = &mRxData[Index];
  }

  mTxData.Push                 = TRUE;
  mTxData.Urgent               = FALSE;
  mTxData.FragmentCount        = 1;
  mTransmitToken.Packet.TxData = &mTxData;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  ConnectionAccepted,
                  &mAcceptToken,
                  &mAcceptToken.CompletionToken.Event
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  ConnectionClosed,
                  &mCloseToken,
                  &mCloseToken.CompletionToken.Event
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Configure the TCP instance
  //

  Status = mTcpListener->Configure (mTcpListener, &TcpConfigData);
  if (Status == EFI_NO_MAPPING) {
    // Wait until the IP configuration process (probably DHCP) has finished
    do {
      Status = mTcpListener->GetModeData (
                               mTcpListener,
                               NULL,
                               NULL,
                               &Ip4ModeData,
                               NULL,
                               NULL
                               );
      ASSERT_EFI_ERROR (Status);
    } while (!Ip4ModeData.IsConfigured);

    Status = mTcpListener->Configure (mTcpListener, &TcpConfigData);
  } else if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TCP Configure: %r\n", Status));
    return Status;
  }

  //
  // Tell the user our address and hostname
  //
  IP4_ADDR_TO_STRING (Ip4ModeData.ConfigData.StationAddress, IpAddrString);

  mTextOut->OutputString (mTextOut, L"TCP Fastboot transport configured.");
  mTextOut->OutputString (mTextOut, L"\r\nIP address: ");
  mTextOut->OutputString (mTextOut, IpAddrString);
  mTextOut->OutputString (mTextOut, L"\r\n");

  //
  // Start listening for a connection
  //

  Status = mTcpListener->Accept (mTcpListener, &mAcceptToken);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TCP Accept: %r\n", Status));
    return Status;
  }

  mTextOut->OutputString (mTextOut, L"TCP Fastboot transport initialised.\r\n");

  FreePool (HandleBuffer);

  return EFI_SUCCESS;
}

EFI_STATUS
TcpFastbootTransportStop (
  VOID
  )
{
  EFI_TCP4_CLOSE_TOKEN      CloseToken;
  EFI_STATUS                Status;
  UINTN                     EventIndex;
  FASTBOOT_TCP_PACKET_LIST  *Entry;
  FASTBOOT_TCP_PACKET_LIST  *NextEntry;

  // Close any existing TCP connection, blocking until it's done.
  if (mTcpConnection != NULL) {
    CloseReceiveEvents ();

    CloseToken.AbortOnClose = FALSE;

    Status = gBS->CreateEvent (0, 0, NULL, NULL, &CloseToken.CompletionToken.Event);
    ASSERT_EFI_ERROR (Status);

    Status = mTcpConnection->Close (mTcpConnection, &CloseToken);
    ASSERT_EFI_ERROR (Status);

    Status = gBS->WaitForEvent (
                    1,
                    &CloseToken.CompletionToken.Event,
                    &EventIndex
                    );
    ASSERT_EFI_ERROR (Status);

    ASSERT_EFI_ERROR (CloseToken.CompletionToken.Status);

    // Possible bug in EDK2 TCP4 driver: closing a connection doesn't remove its
    // PCB from the list of live connections. Subsequent attempts to Configure()
    // a TCP instance with the same local port will fail with INVALID_PARAMETER.
    // Calling Configure with NULL is a workaround for this issue.
    Status = mTcpConnection->Configure (mTcpConnection, NULL);
    ASSERT_EFI_ERROR (Status);
  }

  gBS->CloseEvent (mAcceptToken.CompletionToken.Event);

  // Stop listening for connections.
  // Ideally we would do this with Cancel, but it isn't implemented by EDK2.
  // So we just "reset this TCPv4 instance brutally".
  Status = mTcpListener->Configure (mTcpListener, NULL);
  ASSERT_EFI_ERROR (Status);

  Status = mTcpServiceBinding->DestroyChild (mTcpServiceBinding, mTcpHandle);

  // Free any data the user didn't pick up
  Entry = (FASTBOOT_TCP_PACKET_LIST *)GetFirstNode (&mPacketListHead);
  while (!IsNull (&mPacketListHead, &Entry->Link)) {
    NextEntry = (FASTBOOT_TCP_PACKET_LIST *)GetNextNode (&mPacketListHead, &Entry->Link);

    RemoveEntryList (&Entry->Link);
    if (Entry->Buffer) {
      FreePool (Entry->Buffer);
    }

    FreePool (Entry);

    Entry = NextEntry;
  }

  return EFI_SUCCESS;
}

/*
  Event notify function for when data has been sent. Free resources and report
  errors.
  Context should point to the transmit IO token passed to
  TcpConnection->Transmit.
*/
STATIC
VOID
DataSent (
  EFI_EVENT  Event,
  VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = mTransmitToken.CompletionToken.Status;
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TCP Fastboot transmit result: %r\n", Status));
    gBS->SignalEvent (*(EFI_EVENT *)Context);
  }

  FreePool (mTransmitToken.Packet.TxData->FragmentTable[0].FragmentBuffer);
}

EFI_STATUS
TcpFastbootTransportSend (
  IN        UINTN      BufferSize,
  IN  CONST VOID       *Buffer,
  IN        EFI_EVENT  *FatalErrorEvent
  )
{
  EFI_STATUS  Status;

  if (BufferSize > 512) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Build transmit IO token
  //

  // Create an event so we are notified when a transmission is complete.
  // We use this to free resources and report errors.
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  DataSent,
                  FatalErrorEvent,
                  &mTransmitToken.CompletionToken.Event
                  );
  ASSERT_EFI_ERROR (Status);

  mTxData.DataLength = BufferSize;

  mTxData.FragmentTable[0].FragmentLength = BufferSize;
  mTxData.FragmentTable[0].FragmentBuffer = AllocateCopyPool (
                                              BufferSize,
                                              Buffer
                                              );

  Status = mTcpConnection->Transmit (mTcpConnection, &mTransmitToken);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TCP Transmit: %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
TcpFastbootTransportReceive (
  OUT UINTN  *BufferSize,
  OUT VOID   **Buffer
  )
{
  FASTBOOT_TCP_PACKET_LIST  *Entry;

  if (IsListEmpty (&mPacketListHead)) {
    return EFI_NOT_READY;
  }

  Entry = (FASTBOOT_TCP_PACKET_LIST *)GetFirstNode (&mPacketListHead);

  if (Entry->Buffer == NULL) {
    // There was an error receiving this packet.
    return EFI_DEVICE_ERROR;
  }

  *Buffer     = Entry->Buffer;
  *BufferSize = Entry->BufferSize;

  RemoveEntryList (&Entry->Link);
  FreePool (Entry);

  return EFI_SUCCESS;
}

FASTBOOT_TRANSPORT_PROTOCOL  mTransportProtocol = {
  TcpFastbootTransportStart,
  TcpFastbootTransportStop,
  TcpFastbootTransportSend,
  TcpFastbootTransportReceive
};

EFI_STATUS
TcpFastbootTransportEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
                  &gEfiSimpleTextOutProtocolGuid,
                  NULL,
                  (VOID **)&mTextOut
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fastboot: Open Text Output Protocol: %r\n", Status));
    return Status;
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gAndroidFastbootTransportProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mTransportProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fastboot: Install transport Protocol: %r\n", Status));
  }

  return Status;
}
