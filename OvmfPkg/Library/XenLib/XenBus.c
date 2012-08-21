/**@file
  This file holds the Xenbus methods, which can setup Xenbus, write and read xenstore entry.
  Before all the Xenbus operations, XenbusSetup() should execute firstly. 
  Don't forget to close the Xenbus using XenbusShutdown() after the all the operations done.

  Copyright (c) 2011-2012, Bei Guan <gbtju85@gmail.com>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>

#include <Library/XenHypercallLib.h>
#include <Library/XenLib.h>

//
// Shared ring with Domain0
//
EFI_XENSTORE_DOMAIN_INTERFACE    *Rings = NULL;
//
// Event channel to Domain0
//
EFI_EVENT_CHANNEL_PORT            Event;
static CHAR8                      Payload[XENSTORE_PAYLOAD_MAX + 1];
static CHAR8                      *PayloadPtr[XENSTORE_PAYLOAD_MAX + 1];


/**
  Helper functions: print a string.
**/
UINTN
EFIAPI
PrintStr (
  IN      CONST CHAR8               *FirstString,
  IN      UINTN                     Length
  )
{
  ASSERT (AsciiStrSize (FirstString));

  while ((*FirstString != '\0') && (Length > 0)) {
    DEBUG ((EFI_D_INFO, "%c", *FirstString));
    FirstString++;
    Length--;
  }

  DEBUG ((EFI_D_INFO, "\n"));
  return EFI_SUCCESS;
}

/**
  Connect to the backend Xenbus daemon. 
  This method must be called first before any Xenbus operation.

  @return              EFI_STATUS.

**/
EFI_STATUS
EFIAPI
XenbusSetup (
  VOID
  )
{
  EFI_XEN_HVM_PARAM  Param;

  //
  // Get Xenbus shared page.
  //
  Param.DomId = DOMID_SELF;
  Param.Index = HVM_PARAM_STORE_PFN;
  if ( HypervisorHvmOp(HVMOP_GET_PARAM, &Param) ) {
    DEBUG ((EFI_D_ERROR, "Hypercall Error: can't get xenbus shared page.\n"));
  }
  Rings = (VOID *)(UINTN)(Param.Value << EFI_PAGE_SHIFT);

  //
  // Get Xenbus event channel.
  //
  Param.DomId = DOMID_SELF;
  Param.Index = HVM_PARAM_STORE_EVTCHN;
  if ( HypervisorHvmOp(HVMOP_GET_PARAM, &Param) ) {
    DEBUG ((EFI_D_ERROR, "Hypercall Error: can't get xenbus event channel.\n"));
  }
  Event = Param.Value;

  DEBUG ((EFI_D_INFO, "Xenbus rings @0x%x, event channel %d\n", (UINTN) Rings, (UINTN) Event));
  return EFI_SUCCESS;
}

/**
  Reset the Xenbus connection.

  @return              EFI_STATUS.

**/
EFI_STATUS
EFIAPI
XenbusShutdown (
  VOID
  )
{
  ASSERT(Rings);

  //
  // Clear the shared ring.
  //
  ZeroMem (Rings, sizeof (EFI_XENSTORE_DOMAIN_INTERFACE));  

  //
  // Clear the event channel state
  //
  ZeroMem(GetSharedInfo (), sizeof (EFI_XEN_SHARED_INFO));

  Rings = NULL;
  return EFI_SUCCESS;
}

/**
  Shared Ring operation methods.
**/
VOID
EFIAPI
RingWait (
  VOID
  )
{
  EFI_XEN_SHARED_INFO      *SharedInfoPtr;
  SCHED_POLL               Poll;

  SharedInfoPtr = (EFI_XEN_SHARED_INFO *) GetSharedInfo ();

  ZeroMem (&Poll, sizeof (SCHED_POLL));
  Poll.Ports.Ptr = &Event;
  Poll.NrPorts   = 1;

  while ( !SyncTestClearBit(Event, SharedInfoPtr->EventChannelPending) ) {
    HypervisorSchedOp (SCHEDOP_POLL, &Poll);
  }
}

/**
  Copy data in and out of the ring.

  @param IN Data      The data to write into the shared ring.
  @param IN Length    The length of the data.

**/
VOID
EFIAPI
RingWrite (
  IN     CHAR8                    *Data,
  IN     UINTN                    Length
  )
{
  UINT32           TotleOff;
  UINT32           RingIndex;

  //
  // Check the parameters
  //
  ASSERT(Length <= XENSTORE_PAYLOAD_MAX);

  RingIndex = XENSTORE_RING_SIZE - 1;
  while (Length) {
    //
    // Keep don't overrun the consumer pointer.
    //
    while ((TotleOff = RingIndex - ((Rings->ReqProd - Rings->ReqCons) & RingIndex)) == 0) {
      RingWait();
    }

    //
    // Keep don't overrun the end of the shared ring.
    //
    if (TotleOff > (XENSTORE_RING_SIZE - (Rings->ReqProd & RingIndex))) {
      TotleOff = XENSTORE_RING_SIZE - (Rings->ReqProd & RingIndex);
    }

    if (TotleOff > Length) {
      TotleOff = Length;
    }

    //
    // Copy the request data to the ring.
    //
    CopyMem (
      Rings->Req + (Rings->ReqProd & RingIndex),
      Data,
      TotleOff
      );
    MemoryFence();
    Rings->ReqProd += TotleOff;
    Length -= TotleOff;
  }
}

/**
  Copy data in and out of the ring.

  @param OUT Data      The data read from the shared ring.
  @param OUT Length    The length of the data.

**/
VOID
EFIAPI
RingRead (
  OUT    CHAR8                    *Data,
  OUT    UINTN                    Length
  )
{
  UINT32           TotleOff;
  UINT32           RingIndex;

  //
  // Check the parameters
  //
  ASSERT(Length <= XENSTORE_PAYLOAD_MAX);

  RingIndex = XENSTORE_RING_SIZE - 1;
  while (Length) {
    //
    // Keep don't overrun the productor pointer.
    //
    while ((TotleOff = ((Rings->RspProd - Rings->RspCons) & RingIndex)) == 0) {
      RingWait();
    }

    //
    // Keep don't overrun the end of the shared ring.
    //
    if (TotleOff > (XENSTORE_RING_SIZE - (Rings->RspCons & RingIndex))) {
      TotleOff = XENSTORE_RING_SIZE - (Rings->RspCons & RingIndex);
    }

    if (TotleOff > Length) {
      TotleOff = Length;
    }

    //
    // Copy the reply data from the ring.
    //
    CopyMem (
      Data,
      Rings->Rsp + (Rings->RspCons & RingIndex),
      TotleOff
      );
    MemoryFence();
    Rings->RspCons += TotleOff;
    Length -= TotleOff;
  }
}

/**
  Send and receive the request to and from the ring.

  @param IN  Type            The operation type of Xenstore.
  @param IN  Request         The requests need to send through shared ring.
  @param IN  ReqNum          The number of the requests.
  @param OUT ReplyLength     The length of reply data.
  @param OUT ReplyData       The reply data.

  @return EFI_SUCCESS        Successly.
  @return Other              Error.

**/
EFI_STATUS
EFIAPI
XenbusMsgReply (
  IN     EFI_XENSTORE_MSG_TYPE    Type,
  IN     EFI_XENSTORE_REQUEST     *Request,
  IN     UINTN                    ReqNum,
  OUT    UINTN                    *ReplyLength,
  OUT    CHAR8                    **ReplyData
  )
{
  EFI_XENSTORE_MSG              XenstoreMsg;
  EVENT_CHANNEL_SEND            Send;
  UINTN                         Index;
  UINTN                         Length;

  //
  // Check the shared ring.
  //
  ASSERT(Rings);

  Length = 0;
  for (Index = 0; Index < ReqNum; Index++) {
    Length += Request[Index].Length;
  }

  //
  // Put the request on the shared ring.
  //
  XenstoreMsg.Type              = Type;
  XenstoreMsg.RequestId         = 0;
  XenstoreMsg.TransactionId     = 0;
  XenstoreMsg.Length            = Length;

  RingWrite ((CHAR8 *) &XenstoreMsg, sizeof (EFI_XENSTORE_MSG));
  for (Index = 0; Index < ReqNum; Index++) {
    RingWrite (Request[Index].Data, Request[Index].Length);
  }

  //
  // Tell Xen (Dom0) about the request.
  //
  Send.Port = Event;
  HypervisorEventChannelOp(EVTCHNOP_SEND, &Send);

  //
  // Get the reply from the ring.
  //
  RingRead ((CHAR8 *) &XenstoreMsg, sizeof (EFI_XENSTORE_MSG));
  RingRead (Payload, XenstoreMsg.Length);
  Payload[XenstoreMsg.Length] = '\0';

  if (XenstoreMsg.Type == XSError) {
    //
    // If error, there is a Xen error message in Payload.
    //
    DEBUG ((EFI_D_ERROR, "Error from Xen shared ring. Error Message: "));
    PrintStr(Payload, XenstoreMsg.Length);

    *ReplyData    = NULL;
    *ReplyLength  = 0;
    return EFI_ABORTED;
  }

  *ReplyData    = Payload;
  *ReplyLength  = XenstoreMsg.Length;

  return EFI_SUCCESS;
}

/**
  Read a Xenstore key. Returns a nul-terminated string or NULL.

  @param IN  Path            The Xenstore key path.
  @param OUT Data            The value to of the key.

  @return EFI_SUCCESS        Successly.
  @return Other              Error.

**/
EFI_STATUS
EFIAPI
XenbusRead (
  IN     CHAR8                    *Path,
  OUT    CHAR8                    **Data
  )
{
  EFI_STATUS                    Status;
  UINTN                         ReplyLength;
  CHAR8                         *ReplyData;

  EFI_XENSTORE_REQUEST Request[] = {
    //
    // Path info
    //
    {
      Path,
      AsciiStrLen (Path) + 1
    }
  };

  //
  // Send request and get the reply. 
  //
  Status = XenbusMsgReply (XSRead, Request, ARRAY_SIZE(Request), &ReplyLength, &ReplyData);

  if (EFI_ERROR(Status)) {
    return Status;
  }

  *Data = ReplyData;
  return EFI_SUCCESS;
}

/**
  Create or modify a Xenstore key.

  @param IN  Path            The Xenstore key path.
  @param IN  Data            The value to of the key.

  @return EFI_SUCCESS        Successly.
  @return Other              Error.

**/
EFI_STATUS
EFIAPI
XenbusWrite (
  IN     CHAR8                    *Path,
  IN     CHAR8                    *Data
  )
{
  EFI_STATUS                    Status;
  UINTN                         ReplyLength;
  CHAR8                         *ReplyData;

  EFI_XENSTORE_REQUEST Request[] = {
    //
    // Path info
    //
    {
      Path,
      AsciiStrLen (Path) + 1
    },
    //
    // Value info
    //
    {
      Data,
      AsciiStrLen (Data) + 1
    }
  };

  //
  // Send request and get the reply. 
  //
  Status = XenbusMsgReply (XSWrite, Request, ARRAY_SIZE(Request), &ReplyLength, &ReplyData);

  if (EFI_ERROR(Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  List a Xenstore directory.

  @param IN  Path            The Xenstore key path.
  @param OUT DataArray       A array of pointers to the key under the directory.

  @return EFI_SUCCESS        Successly.
  @return Other              Error.

**/
EFI_STATUS
EFIAPI
XenbusLs (
  IN     CHAR8                    *Path,
  OUT    CHAR8                    ***DataArray
  )
{
  EFI_STATUS                    Status;
  UINTN                         ReplyLength;
  CHAR8                         *ReplyData;
  UINTN                         Index;
  CHAR8                         *StrStart;
  UINTN                         KeyNum;

  EFI_XENSTORE_REQUEST Request[] = {
    //
    // Path info
    //
    {
      Path,
      AsciiStrLen (Path) + 1
    }
  };

  //
  // Send request and get the reply. 
  //
  Status = XenbusMsgReply (XSDirectory, Request, ARRAY_SIZE(Request), &ReplyLength, &ReplyData);

  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Split out each key
  //
  KeyNum   = 0;
  StrStart = ReplyData;
  for (Index = 0; Index < ReplyLength; Index++) {
    if (ReplyData[Index] == '\0') {
      PayloadPtr[KeyNum++] = StrStart;
      StrStart = (CHAR8 *) (ReplyData + Index + 1);
    }
  }

  *DataArray = PayloadPtr;
  return EFI_SUCCESS;
}

/**
  Get domain ID from xenstore.

  @return  The ID of surrent Domain.

**/
DOMID
EFIAPI
XenbusGetDomId (
  VOID
  )
{
  EFI_STATUS                    Status;
  CHAR8                         *Data;

  Status = XenbusRead("domid", &Data);

  if (EFI_ERROR(Status)) {
    return -1;
  }
  
  return (DOMID) AsciiStrDecimalToUintn(Data);
}

/**
  Read a Xenstore key as an integer.

  @param IN  Path            The Xenstore key path.

  @return The value to of the key as an integer.
  @return Other negative number (Error Number).

**/
UINTN
EFIAPI
XenbusReadInteger (
  IN     CHAR8                    *Path
  )
{
  EFI_STATUS                    Status;
  CHAR8                         *Buf;

  Status = XenbusRead (Path, &Buf);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  return AsciiStrDecimalToUintn(Buf);
}

/**
  Write a Xenstore key.

  @param IN  Node            The Xenstore key to write.
  @param IN  Path            The Xenstore key path.
  @param IN  FormatString    A Null-terminated Unicode format string.

  @return EFI_SUCCESS        Successly.
  @return Other              Error.

**/
EFI_STATUS
EFIAPI
XenbusPrintf (
  IN     CHAR8                    *Node,
  IN     CHAR8                    *Path,
  IN     CHAR8                    *FormatString,
  ...
  )
{
  CHAR8                 FullPath[BUFFER_SIZE];
  CHAR8                 Value[BUFFER_SIZE];
  VA_LIST               Marker;

  ASSERT (AsciiStrLen (Node) + AsciiStrLen (Path) + 1 < BUFFER_SIZE);
  AsciiSPrint (FullPath, sizeof (FullPath), "%a/%a", Node, Path); 
  VA_START (Marker, FormatString);
  AsciiVSPrint (Value, sizeof (Value), FormatString, Marker); 
  VA_END (Marker);

  return XenbusWrite (FullPath, Value);
}

/**
  Change the Xenbus State in Xenstore.

  @param IN  Path            The Xenstore key path.
  @param IN  State           The new state.

  @return EFI_SUCCESS        Successly.
  @return Other              Error.

**/
EFI_STATUS
EFIAPI
XenbusSwitchState (
  IN     CHAR8                    *Path,
  IN     EFI_XENBUS_STATE         State
  )
{
  EFI_STATUS                      Status;
  CHAR8                           *CurrentState;
  EFI_XENBUS_STATE                XenbusState;
  CHAR8                           Value[2];

  Status = XenbusRead (Path, &CurrentState);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  XenbusState = (EFI_XENBUS_STATE) (CurrentState[0] - '0');
  if (XenbusState == State) {
    return EFI_SUCCESS;
  }

  AsciiSPrint (Value, sizeof (Value), "%d", State);
  return XenbusWrite (Path, Value);
}


