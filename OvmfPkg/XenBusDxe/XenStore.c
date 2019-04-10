/** @file
  Low-level kernel interface to the XenStore.

  The XenStore interface is a simple storage system that is a means of
  communicating state and configuration data between the Xen Domain 0
  and the various guest domains.  All configuration data other than
  a small amount of essential information required during the early
  boot process of launching a Xen aware guest, is managed using the
  XenStore.

  The XenStore is ASCII string based, and has a structure and semantics
  similar to a filesystem.  There are files and directories, the directories
  able to contain files or other directories.  The depth of the hierarchy
  is only limited by the XenStore's maximum path length.

  The communication channel between the XenStore service and other
  domains is via two, guest specific, ring buffers in a shared memory
  area.  One ring buffer is used for communicating in each direction.
  The grant table references for this shared memory are given to the
  guest either via the xen_start_info structure for a fully para-
  virtualized guest, or via HVM hypercalls for a hardware virtualized
  guest.

  The XenStore communication relies on an event channel and thus
  interrupts.  But under OVMF this XenStore client will pull the
  state of the event channel.

  Several Xen services depend on the XenStore, most notably the
  XenBus used to discover and manage Xen devices.

  Copyright (C) 2005 Rusty Russell, IBM Corporation
  Copyright (C) 2009,2010 Spectra Logic Corporation
  Copyright (C) 2014, Citrix Ltd.

  This file may be distributed separately from the Linux kernel, or
  incorporated into other software packages, subject to the following license:

  SPDX-License-Identifier: MIT
**/

#include "XenStore.h"

#include <Library/PrintLib.h>

#include <IndustryStandard/Xen/hvm/params.h>

#include "EventChannel.h"
#include <Library/XenHypercallLib.h>

//
// Private Data Structures
//

typedef struct {
  CONST VOID  *Data;
  UINT32      Len;
} WRITE_REQUEST;

/* Register callback to watch subtree (node) in the XenStore. */
#define XENSTORE_WATCH_SIGNATURE SIGNATURE_32 ('X','S','w','a')
struct _XENSTORE_WATCH
{
  UINT32      Signature;
  LIST_ENTRY  Link;

  /* Path being watched. */
  CHAR8       *Node;
};

#define XENSTORE_WATCH_FROM_LINK(l) \
  CR (l, XENSTORE_WATCH, Link, XENSTORE_WATCH_SIGNATURE)


/**
 * Structure capturing messages received from the XenStore service.
 */
#define XENSTORE_MESSAGE_SIGNATURE SIGNATURE_32 ('X', 'S', 's', 'm')
typedef struct {
  UINT32 Signature;
  LIST_ENTRY Link;

  struct xsd_sockmsg Header;

  union {
    /* Queued replies. */
    struct {
      CHAR8 *Body;
    } Reply;

    /* Queued watch events. */
    struct {
      XENSTORE_WATCH *Handle;
      CONST CHAR8 **Vector;
      UINT32 VectorSize;
    } Watch;
  } u;
} XENSTORE_MESSAGE;
#define XENSTORE_MESSAGE_FROM_LINK(r) \
  CR (r, XENSTORE_MESSAGE, Link, XENSTORE_MESSAGE_SIGNATURE)

/**
 * Container for all XenStore related state.
 */
typedef struct {
  /**
   * Pointer to shared memory communication structures allowing us
   * to communicate with the XenStore service.
   */
  struct xenstore_domain_interface *XenStore;

  XENBUS_DEVICE *Dev;

  /**
   * A list of replies to our requests.
   *
   * The reply list is filled by xs_rcv_thread().  It
   * is consumed by the context that issued the request
   * to which a reply is made.  The requester blocks in
   * XenStoreReadReply ().
   *
   * /note Only one requesting context can be active at a time.
   */
  LIST_ENTRY ReplyList;

  /** Lock protecting the reply list. */
  EFI_LOCK ReplyLock;

  /**
   * List of registered watches.
   */
  LIST_ENTRY RegisteredWatches;

  /** Lock protecting the registered watches list. */
  EFI_LOCK RegisteredWatchesLock;

  /**
   * List of pending watch callback events.
   */
  LIST_ENTRY WatchEvents;

  /** Lock protecting the watch calback list. */
  EFI_LOCK WatchEventsLock;

  /**
   * The event channel for communicating with the
   * XenStore service.
   */
  evtchn_port_t EventChannel;

  /** Handle for XenStore events. */
  EFI_EVENT EventChannelEvent;
} XENSTORE_PRIVATE;

//
// Global Data
//
static XENSTORE_PRIVATE xs;


//
// Private Utility Functions
//

/**
  Count and optionally record pointers to a number of NUL terminated
  strings in a buffer.

  @param Strings  A pointer to a contiguous buffer of NUL terminated strings.
  @param Len      The length of the buffer pointed to by strings.
  @param Dst      An array to store pointers to each string found in strings.

  @return  A count of the number of strings found.
**/
STATIC
UINT32
ExtractStrings (
  IN  CONST CHAR8 *Strings,
  IN  UINTN       Len,
  OUT CONST CHAR8 **Dst OPTIONAL
  )
{
  UINT32 Num = 0;
  CONST CHAR8 *Ptr;

  for (Ptr = Strings; Ptr < Strings + Len; Ptr += AsciiStrSize (Ptr)) {
    if (Dst != NULL) {
      *Dst++ = Ptr;
    }
    Num++;
  }

  return Num;
}

/**
  Convert a contiguous buffer containing a series of NUL terminated
  strings into an array of pointers to strings.

  The returned pointer references the array of string pointers which
  is followed by the storage for the string data.  It is the client's
  responsibility to free this storage.

  The storage addressed by Strings is free'd prior to Split returning.

  @param Strings  A pointer to a contiguous buffer of NUL terminated strings.
  @param Len      The length of the buffer pointed to by strings.
  @param NumPtr   The number of strings found and returned in the strings
                  array.

  @return  An array of pointers to the strings found in the input buffer.
**/
STATIC
CONST CHAR8 **
Split (
  IN  CHAR8   *Strings,
  IN  UINTN   Len,
  OUT UINT32  *NumPtr
  )
{
  CONST CHAR8 **Dst;

  ASSERT(NumPtr != NULL);
  ASSERT(Strings != NULL);

  /* Protect against unterminated buffers. */
  if (Len > 0) {
    Strings[Len - 1] = '\0';
  }

  /* Count the Strings. */
  *NumPtr = ExtractStrings (Strings, Len, NULL);

  /* Transfer to one big alloc for easy freeing by the caller. */
  Dst = AllocatePool (*NumPtr * sizeof (CHAR8 *) + Len);
  CopyMem ((VOID*)&Dst[*NumPtr], Strings, Len);
  FreePool (Strings);

  /* Extract pointers to newly allocated array. */
  Strings = (CHAR8 *) &Dst[*NumPtr];
  ExtractStrings (Strings, Len, Dst);

  return (Dst);
}

/**
  Convert from watch token (unique identifier) to the associated
  internal tracking structure for this watch.

  @param Tocken  The unique identifier for the watch to find.

  @return  A pointer to the found watch structure or NULL.
**/
STATIC
XENSTORE_WATCH *
XenStoreFindWatch (
  IN CONST CHAR8 *Token
  )
{
  XENSTORE_WATCH *Watch, *WantedWatch;
  LIST_ENTRY *Entry;

  WantedWatch = (VOID *) AsciiStrHexToUintn (Token);

  if (IsListEmpty (&xs.RegisteredWatches)) {
    return NULL;
  }
  for (Entry = GetFirstNode (&xs.RegisteredWatches);
       !IsNull (&xs.RegisteredWatches, Entry);
       Entry = GetNextNode (&xs.RegisteredWatches, Entry)) {
    Watch = XENSTORE_WATCH_FROM_LINK (Entry);
    if (Watch == WantedWatch)
      return Watch;
  }

  return NULL;
}

//
// Public Utility Functions
// API comments for these methods can be found in XenStore.h
//

CHAR8 *
XenStoreJoin (
  IN CONST CHAR8 *DirectoryPath,
  IN CONST CHAR8 *Node
  )
{
  CHAR8 *Buf;
  UINTN BufSize;

  /* +1 for '/' and +1 for '\0' */
  BufSize = AsciiStrLen (DirectoryPath) + AsciiStrLen (Node) + 2;
  Buf = AllocatePool (BufSize);
  ASSERT (Buf != NULL);

  if (Node[0] == '\0') {
    AsciiSPrint (Buf, BufSize, "%a", DirectoryPath);
  } else {
    AsciiSPrint (Buf, BufSize, "%a/%a", DirectoryPath, Node);
  }

  return Buf;
}

//
// Low Level Communication Management
//

/**
  Verify that the indexes for a ring are valid.

  The difference between the producer and consumer cannot
  exceed the size of the ring.

  @param Cons  The consumer index for the ring to test.
  @param Prod  The producer index for the ring to test.

  @retval TRUE   If indexes are in range.
  @retval FALSE  If the indexes are out of range.
**/
STATIC
BOOLEAN
XenStoreCheckIndexes (
  XENSTORE_RING_IDX Cons,
  XENSTORE_RING_IDX Prod
  )
{
  return ((Prod - Cons) <= XENSTORE_RING_SIZE);
}

/**
  Return a pointer to, and the length of, the contiguous
  free region available for output in a ring buffer.

  @param Cons    The consumer index for the ring.
  @param Prod    The producer index for the ring.
  @param Buffer  The base address of the ring's storage.
  @param LenPtr  The amount of contiguous storage available.

  @return  A pointer to the start location of the free region.
**/
STATIC
VOID *
XenStoreGetOutputChunk (
  IN  XENSTORE_RING_IDX Cons,
  IN  XENSTORE_RING_IDX Prod,
  IN  CHAR8             *Buffer,
  OUT UINT32            *LenPtr
  )
{
  UINT32 Len;
  Len = XENSTORE_RING_SIZE - MASK_XENSTORE_IDX (Prod);
  if ((XENSTORE_RING_SIZE - (Prod - Cons)) < Len) {
    Len = XENSTORE_RING_SIZE - (Prod - Cons);
  }
  *LenPtr = Len;
  return (Buffer + MASK_XENSTORE_IDX (Prod));
}

/**
  Return a pointer to, and the length of, the contiguous
  data available to read from a ring buffer.

  @param Cons    The consumer index for the ring.
  @param Prod    The producer index for the ring.
  @param Buffer  The base address of the ring's storage.
  @param LenPtr  The amount of contiguous data available to read.

  @return  A pointer to the start location of the available data.
**/
STATIC
CONST VOID *
XenStoreGetInputChunk (
  IN  XENSTORE_RING_IDX Cons,
  IN  XENSTORE_RING_IDX Prod,
  IN  CONST CHAR8       *Buffer,
  OUT UINT32            *LenPtr
  )
{
  UINT32 Len;

  Len = XENSTORE_RING_SIZE - MASK_XENSTORE_IDX (Cons);
  if ((Prod - Cons) < Len) {
    Len = Prod - Cons;
  }
  *LenPtr = Len;
  return (Buffer + MASK_XENSTORE_IDX (Cons));
}

/**
  Wait for an event or timeout.

  @param Event    Event to wait for.
  @param Timeout  A timeout value in 100ns units.

  @retval EFI_SUCCESS   Event have been triggered or the current TPL is not
                        TPL_APPLICATION.
  @retval EFI_TIMEOUT   Timeout have expired.
**/
STATIC
EFI_STATUS
XenStoreWaitForEvent (
  IN EFI_EVENT Event,
  IN UINT64    Timeout
  )
{
  UINTN Index;
  EFI_STATUS Status;
  EFI_EVENT TimerEvent;
  EFI_EVENT WaitList[2];

  gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);
  gBS->SetTimer (TimerEvent, TimerRelative, Timeout);

  WaitList[0] = xs.EventChannelEvent;
  WaitList[1] = TimerEvent;
  Status = gBS->WaitForEvent (2, WaitList, &Index);
  ASSERT (Status != EFI_INVALID_PARAMETER);
  gBS->CloseEvent (TimerEvent);
  if (Status == EFI_UNSUPPORTED) {
    return EFI_SUCCESS;
  }
  if (Index == 1) {
    return EFI_TIMEOUT;
  } else {
    return EFI_SUCCESS;
  }
}

/**
  Transmit data to the XenStore service.

  The buffer pointed to by DataPtr is at least Len bytes in length.

  @param DataPtr  A pointer to the contiguous data to send.
  @param Len      The amount of data to send.

  @return  On success 0, otherwise an errno value indicating the
           cause of failure.
**/
STATIC
XENSTORE_STATUS
XenStoreWriteStore (
  IN CONST VOID *DataPtr,
  IN UINT32     Len
  )
{
  XENSTORE_RING_IDX Cons, Prod;
  CONST CHAR8 *Data = (CONST CHAR8 *)DataPtr;

  while (Len != 0) {
    void *Dest;
    UINT32 Available;

    Cons = xs.XenStore->req_cons;
    Prod = xs.XenStore->req_prod;
    if ((Prod - Cons) == XENSTORE_RING_SIZE) {
      /*
       * Output ring is full. Wait for a ring event.
       *
       * Note that the events from both queues are combined, so being woken
       * does not guarantee that data exist in the read ring.
       */
      EFI_STATUS Status;

      Status = XenStoreWaitForEvent (xs.EventChannelEvent,
                                     EFI_TIMER_PERIOD_SECONDS (1));
      if (Status == EFI_TIMEOUT) {
        DEBUG ((EFI_D_WARN, "XenStore Write, waiting for a ring event.\n"));
      }
      continue;
    }

    /* Verify queue sanity. */
    if (!XenStoreCheckIndexes (Cons, Prod)) {
      xs.XenStore->req_cons = xs.XenStore->req_prod = 0;
      return XENSTORE_STATUS_EIO;
    }

    Dest = XenStoreGetOutputChunk (Cons, Prod, xs.XenStore->req, &Available);
    if (Available > Len) {
      Available = Len;
    }

    CopyMem (Dest, Data, Available);
    Data += Available;
    Len -= Available;

    /*
     * The store to the producer index, which indicates
     * to the other side that new data has arrived, must
     * be visible only after our copy of the data into the
     * ring has completed.
     */
    MemoryFence ();
    xs.XenStore->req_prod += Available;

    /*
     * The other side will see the change to req_prod at the time of the
     * interrupt.
     */
    MemoryFence ();
    XenEventChannelNotify (xs.Dev, xs.EventChannel);
  }

  return XENSTORE_STATUS_SUCCESS;
}

/**
  Receive data from the XenStore service.

  The buffer pointed to by DataPtr is at least Len bytes in length.

  @param DataPtr  A pointer to the contiguous buffer to receive the data.
  @param Len      The amount of data to receive.

  @return  On success 0, otherwise an errno value indicating the
           cause of failure.
**/
STATIC
XENSTORE_STATUS
XenStoreReadStore (
  OUT VOID *DataPtr,
  IN  UINT32 Len
  )
{
  XENSTORE_RING_IDX Cons, Prod;
  CHAR8 *Data = (CHAR8 *) DataPtr;

  while (Len != 0) {
    UINT32 Available;
    CONST CHAR8 *Src;

    Cons = xs.XenStore->rsp_cons;
    Prod = xs.XenStore->rsp_prod;
    if (Cons == Prod) {
      /*
       * Nothing to read. Wait for a ring event.
       *
       * Note that the events from both queues are combined, so being woken
       * does not guarantee that data exist in the read ring.
       */
      EFI_STATUS Status;

      Status = XenStoreWaitForEvent (xs.EventChannelEvent,
                                     EFI_TIMER_PERIOD_SECONDS (1));
      if (Status == EFI_TIMEOUT) {
        DEBUG ((EFI_D_WARN, "XenStore Read, waiting for a ring event.\n"));
      }
      continue;
    }

    /* Verify queue sanity. */
    if (!XenStoreCheckIndexes (Cons, Prod)) {
      xs.XenStore->rsp_cons = xs.XenStore->rsp_prod = 0;
      return XENSTORE_STATUS_EIO;
    }

    Src = XenStoreGetInputChunk (Cons, Prod, xs.XenStore->rsp, &Available);
    if (Available > Len) {
      Available = Len;
    }

    /*
     * Insure the data we read is related to the indexes
     * we read above.
     */
    MemoryFence ();

    CopyMem (Data, Src, Available);
    Data += Available;
    Len -= Available;

    /*
     * Insure that the producer of this ring does not see
     * the ring space as free until after we have copied it
     * out.
     */
    MemoryFence ();
    xs.XenStore->rsp_cons += Available;

    /*
     * The producer will see the updated consumer index when the event is
     * delivered.
     */
    MemoryFence ();
    XenEventChannelNotify (xs.Dev, xs.EventChannel);
  }

  return XENSTORE_STATUS_SUCCESS;
}

//
// Received Message Processing
//

/**
  Block reading the next message from the XenStore service and
  process the result.

  @return  XENSTORE_STATUS_SUCCESS on success.  Otherwise an errno value
           indicating the type of failure encountered.
**/
STATIC
XENSTORE_STATUS
XenStoreProcessMessage (
  VOID
  )
{
  XENSTORE_MESSAGE *Message;
  CHAR8 *Body;
  XENSTORE_STATUS Status;

  Message = AllocateZeroPool (sizeof (XENSTORE_MESSAGE));
  Message->Signature = XENSTORE_MESSAGE_SIGNATURE;
  Status = XenStoreReadStore (&Message->Header, sizeof (Message->Header));
  if (Status != XENSTORE_STATUS_SUCCESS) {
    FreePool (Message);
    DEBUG ((EFI_D_ERROR, "XenStore: Error read store (%d)\n", Status));
    return Status;
  }

  Body = AllocatePool (Message->Header.len + 1);
  Status = XenStoreReadStore (Body, Message->Header.len);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    FreePool (Body);
    FreePool (Message);
    DEBUG ((EFI_D_ERROR, "XenStore: Error read store (%d)\n", Status));
    return Status;
  }
  Body[Message->Header.len] = '\0';

  if (Message->Header.type == XS_WATCH_EVENT) {
    Message->u.Watch.Vector = Split(Body, Message->Header.len,
                                    &Message->u.Watch.VectorSize);

    EfiAcquireLock (&xs.RegisteredWatchesLock);
    Message->u.Watch.Handle =
      XenStoreFindWatch (Message->u.Watch.Vector[XS_WATCH_TOKEN]);
    DEBUG ((EFI_D_INFO, "XenStore: Watch event %a\n",
            Message->u.Watch.Vector[XS_WATCH_TOKEN]));
    if (Message->u.Watch.Handle != NULL) {
      EfiAcquireLock (&xs.WatchEventsLock);
      InsertHeadList (&xs.WatchEvents, &Message->Link);
      EfiReleaseLock (&xs.WatchEventsLock);
    } else {
      DEBUG ((EFI_D_WARN, "XenStore: Watch handle %a not found\n",
              Message->u.Watch.Vector[XS_WATCH_TOKEN]));
      FreePool((VOID*)Message->u.Watch.Vector);
      FreePool(Message);
    }
    EfiReleaseLock (&xs.RegisteredWatchesLock);
  } else {
    Message->u.Reply.Body = Body;
    EfiAcquireLock (&xs.ReplyLock);
    InsertTailList (&xs.ReplyList, &Message->Link);
    EfiReleaseLock (&xs.ReplyLock);
  }

  return XENSTORE_STATUS_SUCCESS;
}

//
// XenStore Message Request/Reply Processing
//

/**
  Convert a XenStore error string into an errno number.

  Unknown error strings are converted to EINVAL.

  @param errorstring  The error string to convert.

  @return  The errno best matching the input string.

**/
typedef struct {
  XENSTORE_STATUS Status;
  CONST CHAR8 *ErrorStr;
} XenStoreErrors;

static XenStoreErrors gXenStoreErrors[] = {
  { XENSTORE_STATUS_EINVAL, "EINVAL" },
  { XENSTORE_STATUS_EACCES, "EACCES" },
  { XENSTORE_STATUS_EEXIST, "EEXIST" },
  { XENSTORE_STATUS_EISDIR, "EISDIR" },
  { XENSTORE_STATUS_ENOENT, "ENOENT" },
  { XENSTORE_STATUS_ENOMEM, "ENOMEM" },
  { XENSTORE_STATUS_ENOSPC, "ENOSPC" },
  { XENSTORE_STATUS_EIO, "EIO" },
  { XENSTORE_STATUS_ENOTEMPTY, "ENOTEMPTY" },
  { XENSTORE_STATUS_ENOSYS, "ENOSYS" },
  { XENSTORE_STATUS_EROFS, "EROFS" },
  { XENSTORE_STATUS_EBUSY, "EBUSY" },
  { XENSTORE_STATUS_EAGAIN, "EAGAIN" },
  { XENSTORE_STATUS_EISCONN, "EISCONN" },
  { XENSTORE_STATUS_E2BIG, "E2BIG" }
};

STATIC
XENSTORE_STATUS
XenStoreGetError (
  CONST CHAR8 *ErrorStr
  )
{
  UINT32 Index;

  for (Index = 0; Index < ARRAY_SIZE(gXenStoreErrors); Index++) {
    if (!AsciiStrCmp (ErrorStr, gXenStoreErrors[Index].ErrorStr)) {
      return gXenStoreErrors[Index].Status;
    }
  }
  DEBUG ((EFI_D_WARN, "XenStore gave unknown error %a\n", ErrorStr));
  return XENSTORE_STATUS_EINVAL;
}

/**
  Block waiting for a reply to a message request.

  @param TypePtr The returned type of the reply.
  @param LenPtr  The returned body length of the reply.
  @param Result  The returned body of the reply.
**/
STATIC
XENSTORE_STATUS
XenStoreReadReply (
  OUT enum xsd_sockmsg_type *TypePtr,
  OUT UINT32 *LenPtr OPTIONAL,
  OUT VOID **Result
  )
{
  XENSTORE_MESSAGE *Message;
  LIST_ENTRY *Entry;
  CHAR8 *Body;

  while (IsListEmpty (&xs.ReplyList)) {
    XENSTORE_STATUS Status;
    Status = XenStoreProcessMessage ();
    if (Status != XENSTORE_STATUS_SUCCESS && Status != XENSTORE_STATUS_EAGAIN) {
      DEBUG ((EFI_D_ERROR, "XenStore, error while reading the ring (%d).",
              Status));
      return Status;
    }
  }
  EfiAcquireLock (&xs.ReplyLock);
  Entry = GetFirstNode (&xs.ReplyList);
  Message = XENSTORE_MESSAGE_FROM_LINK (Entry);
  RemoveEntryList (Entry);
  EfiReleaseLock (&xs.ReplyLock);

  *TypePtr = Message->Header.type;
  if (LenPtr != NULL) {
    *LenPtr = Message->Header.len;
  }
  Body = Message->u.Reply.Body;

  FreePool (Message);
  *Result = Body;
  return XENSTORE_STATUS_SUCCESS;
}

/**
  Send a message with an optionally muti-part body to the XenStore service.

  @param Transaction    The transaction to use for this request.
  @param RequestType    The type of message to send.
  @param WriteRequest   Pointers to the body sections of the request.
  @param NumRequests    The number of body sections in the request.
  @param LenPtr         The returned length of the reply.
  @param ResultPtr      The returned body of the reply.

  @return  XENSTORE_STATUS_SUCCESS on success.  Otherwise an errno indicating
           the cause of failure.
**/
STATIC
XENSTORE_STATUS
XenStoreTalkv (
  IN  CONST XENSTORE_TRANSACTION *Transaction,
  IN  enum xsd_sockmsg_type   RequestType,
  IN  CONST WRITE_REQUEST     *WriteRequest,
  IN  UINT32                  NumRequests,
  OUT UINT32                  *LenPtr OPTIONAL,
  OUT VOID                    **ResultPtr OPTIONAL
  )
{
  struct xsd_sockmsg Message;
  void *Return = NULL;
  UINT32 Index;
  XENSTORE_STATUS Status;

  if (Transaction == XST_NIL) {
    Message.tx_id = 0;
  } else {
    Message.tx_id = Transaction->Id;
  }
  Message.req_id = 0;
  Message.type = RequestType;
  Message.len = 0;
  for (Index = 0; Index < NumRequests; Index++) {
    Message.len += WriteRequest[Index].Len;
  }

  Status = XenStoreWriteStore (&Message, sizeof (Message));
  if (Status != XENSTORE_STATUS_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "XenStoreTalkv failed %d\n", Status));
    goto Error;
  }

  for (Index = 0; Index < NumRequests; Index++) {
    Status = XenStoreWriteStore (WriteRequest[Index].Data, WriteRequest[Index].Len);
    if (Status != XENSTORE_STATUS_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "XenStoreTalkv failed %d\n", Status));
      goto Error;
    }
  }

  Status = XenStoreReadReply ((enum xsd_sockmsg_type *)&Message.type, LenPtr, &Return);

Error:
  if (Status != XENSTORE_STATUS_SUCCESS) {
    return Status;
  }

  if (Message.type == XS_ERROR) {
    Status = XenStoreGetError (Return);
    FreePool (Return);
    return Status;
  }

  /* Reply is either error or an echo of our request message type. */
  ASSERT ((enum xsd_sockmsg_type)Message.type == RequestType);

  if (ResultPtr) {
    *ResultPtr = Return;
  } else {
    FreePool (Return);
  }

  return XENSTORE_STATUS_SUCCESS;
}

/**
  Wrapper for XenStoreTalkv allowing easy transmission of a message with
  a single, contiguous, message body.

  The returned result is provided in malloced storage and thus must be free'd
  by the caller.

  @param Transaction    The transaction to use for this request.
  @param RequestType    The type of message to send.
  @param Body           The body of the request.
  @param LenPtr         The returned length of the reply.
  @param Result         The returned body of the reply.

  @return  0 on success.  Otherwise an errno indicating
           the cause of failure.
**/
STATIC
XENSTORE_STATUS
XenStoreSingle (
  IN  CONST XENSTORE_TRANSACTION *Transaction,
  IN  enum xsd_sockmsg_type   RequestType,
  IN  CONST CHAR8             *Body,
  OUT UINT32                  *LenPtr OPTIONAL,
  OUT VOID                    **Result OPTIONAL
  )
{
  WRITE_REQUEST WriteRequest;

  WriteRequest.Data = (VOID *) Body;
  WriteRequest.Len = (UINT32)AsciiStrSize (Body);

  return XenStoreTalkv (Transaction, RequestType, &WriteRequest, 1,
                        LenPtr, Result);
}

//
// XenStore Watch Support
//

/**
  Transmit a watch request to the XenStore service.

  @param Path    The path in the XenStore to watch.
  @param Tocken  A unique identifier for this watch.

  @return  XENSTORE_STATUS_SUCCESS on success.  Otherwise an errno indicating the
           cause of failure.
**/
STATIC
XENSTORE_STATUS
XenStoreWatch (
  CONST CHAR8 *Path,
  CONST CHAR8 *Token
  )
{
  WRITE_REQUEST WriteRequest[2];

  WriteRequest[0].Data = (VOID *) Path;
  WriteRequest[0].Len = (UINT32)AsciiStrSize (Path);
  WriteRequest[1].Data = (VOID *) Token;
  WriteRequest[1].Len = (UINT32)AsciiStrSize (Token);

  return XenStoreTalkv (XST_NIL, XS_WATCH, WriteRequest, 2, NULL, NULL);
}

/**
  Transmit an uwatch request to the XenStore service.

  @param Path    The path in the XenStore to watch.
  @param Tocken  A unique identifier for this watch.

  @return  XENSTORE_STATUS_SUCCESS on success.  Otherwise an errno indicating
           the cause of failure.
**/
STATIC
XENSTORE_STATUS
XenStoreUnwatch (
  CONST CHAR8 *Path,
  CONST CHAR8 *Token
  )
{
  WRITE_REQUEST WriteRequest[2];

  WriteRequest[0].Data = (VOID *) Path;
  WriteRequest[0].Len = (UINT32)AsciiStrSize (Path);
  WriteRequest[1].Data = (VOID *) Token;
  WriteRequest[1].Len = (UINT32)AsciiStrSize (Token);

  return XenStoreTalkv (XST_NIL, XS_UNWATCH, WriteRequest, 2, NULL, NULL);
}

STATIC
XENSTORE_STATUS
XenStoreWaitWatch (
  VOID *Token
  )
{
  XENSTORE_MESSAGE *Message;
  LIST_ENTRY *Entry = NULL;
  LIST_ENTRY *Last = NULL;
  XENSTORE_STATUS Status;

  while (TRUE) {
    EfiAcquireLock (&xs.WatchEventsLock);
    if (IsListEmpty (&xs.WatchEvents) ||
        Last == GetFirstNode (&xs.WatchEvents)) {
      EfiReleaseLock (&xs.WatchEventsLock);
      Status = XenStoreProcessMessage ();
      if (Status != XENSTORE_STATUS_SUCCESS && Status != XENSTORE_STATUS_EAGAIN) {
        return Status;
      }
      continue;
    }

    for (Entry = GetFirstNode (&xs.WatchEvents);
         Entry != Last && !IsNull (&xs.WatchEvents, Entry);
         Entry = GetNextNode (&xs.WatchEvents, Entry)) {
      Message = XENSTORE_MESSAGE_FROM_LINK (Entry);
      if (Message->u.Watch.Handle == Token) {
        RemoveEntryList (Entry);
        EfiReleaseLock (&xs.WatchEventsLock);
        FreePool((VOID*)Message->u.Watch.Vector);
        FreePool(Message);
        return XENSTORE_STATUS_SUCCESS;
      }
    }
    Last = GetFirstNode (&xs.WatchEvents);
    EfiReleaseLock (&xs.WatchEventsLock);
  }
}

VOID
EFIAPI
NotifyEventChannelCheckForEvent (
  IN EFI_EVENT Event,
  IN VOID *Context
  )
{
  XENSTORE_PRIVATE *xsp;
  xsp = (XENSTORE_PRIVATE *)Context;
  if (TestAndClearBit (xsp->EventChannel, xsp->Dev->SharedInfo->evtchn_pending)) {
    gBS->SignalEvent (Event);
  }
}

/**
  Setup communication channels with the XenStore service.

  @retval EFI_SUCCESS if everything went well.
**/
STATIC
EFI_STATUS
XenStoreInitComms (
  XENSTORE_PRIVATE *xsp
  )
{
  EFI_STATUS Status;
  EFI_EVENT TimerEvent;
  struct xenstore_domain_interface *XenStore = xsp->XenStore;

  Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);
  Status = gBS->SetTimer (TimerEvent, TimerRelative,
                          EFI_TIMER_PERIOD_SECONDS (5));
  while (XenStore->rsp_prod != XenStore->rsp_cons) {
    Status = gBS->CheckEvent (TimerEvent);
    if (!EFI_ERROR (Status)) {
      DEBUG ((EFI_D_WARN, "XENSTORE response ring is not quiescent "
              "(%08x:%08x): fixing up\n",
              XenStore->rsp_cons, XenStore->rsp_prod));
      XenStore->rsp_cons = XenStore->rsp_prod;
    }
  }
  gBS->CloseEvent (TimerEvent);

  Status = gBS->CreateEvent (EVT_NOTIFY_WAIT, TPL_NOTIFY,
                             NotifyEventChannelCheckForEvent, xsp,
                             &xsp->EventChannelEvent);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Initialize XenStore.

  @param Dev  A XENBUS_DEVICE instance.

  @retval EFI_SUCCESS if everything went well.
**/
EFI_STATUS
XenStoreInit (
  XENBUS_DEVICE *Dev
  )
{
  EFI_STATUS Status;
  /**
   * The HVM guest pseudo-physical frame number.  This is Xen's mapping
   * of the true machine frame number into our "physical address space".
   */
  UINTN XenStoreGpfn;

  xs.Dev = Dev;

  xs.EventChannel = (evtchn_port_t)XenHypercallHvmGetParam (HVM_PARAM_STORE_EVTCHN);
  XenStoreGpfn = (UINTN)XenHypercallHvmGetParam (HVM_PARAM_STORE_PFN);
  xs.XenStore = (VOID *) (XenStoreGpfn << EFI_PAGE_SHIFT);
  DEBUG ((EFI_D_INFO, "XenBusInit: XenBus rings @%p, event channel %x\n",
          xs.XenStore, xs.EventChannel));

  InitializeListHead (&xs.ReplyList);
  InitializeListHead (&xs.WatchEvents);
  InitializeListHead (&xs.RegisteredWatches);

  EfiInitializeLock (&xs.ReplyLock, TPL_NOTIFY);
  EfiInitializeLock (&xs.RegisteredWatchesLock, TPL_NOTIFY);
  EfiInitializeLock (&xs.WatchEventsLock, TPL_NOTIFY);

  /* Initialize the shared memory rings to talk to xenstored */
  Status = XenStoreInitComms (&xs);

  return Status;
}

VOID
XenStoreDeinit (
  IN XENBUS_DEVICE *Dev
  )
{
  //
  // Emptying the list RegisteredWatches, but this list should already be
  // empty. Every driver that is using Watches should unregister them when
  // it is stopped.
  //
  if (!IsListEmpty (&xs.RegisteredWatches)) {
    XENSTORE_WATCH *Watch;
    LIST_ENTRY *Entry;
    DEBUG ((EFI_D_WARN, "XenStore: RegisteredWatches is not empty, cleaning up..."));
    Entry = GetFirstNode (&xs.RegisteredWatches);
    while (!IsNull (&xs.RegisteredWatches, Entry)) {
      Watch = XENSTORE_WATCH_FROM_LINK (Entry);
      Entry = GetNextNode (&xs.RegisteredWatches, Entry);

      XenStoreUnregisterWatch (Watch);
    }
  }

  //
  // Emptying the list WatchEvents, but this list should already be empty after
  // having cleanup the list RegisteredWatches.
  //
  if (!IsListEmpty (&xs.WatchEvents)) {
    LIST_ENTRY *Entry;
    DEBUG ((EFI_D_WARN, "XenStore: WatchEvents is not empty, cleaning up..."));
    Entry = GetFirstNode (&xs.WatchEvents);
    while (!IsNull (&xs.WatchEvents, Entry)) {
      XENSTORE_MESSAGE *Message = XENSTORE_MESSAGE_FROM_LINK (Entry);
      Entry = GetNextNode (&xs.WatchEvents, Entry);
      RemoveEntryList (&Message->Link);
      FreePool ((VOID*)Message->u.Watch.Vector);
      FreePool (Message);
    }
  }

  if (!IsListEmpty (&xs.ReplyList)) {
    XENSTORE_MESSAGE *Message;
    LIST_ENTRY *Entry;
    Entry = GetFirstNode (&xs.ReplyList);
    while (!IsNull (&xs.ReplyList, Entry)) {
      Message = XENSTORE_MESSAGE_FROM_LINK (Entry);
      Entry = GetNextNode (&xs.ReplyList, Entry);
      RemoveEntryList (&Message->Link);
      FreePool (Message->u.Reply.Body);
      FreePool (Message);
    }
  }

  gBS->CloseEvent (xs.EventChannelEvent);

  if (xs.XenStore->server_features & XENSTORE_SERVER_FEATURE_RECONNECTION) {
    xs.XenStore->connection = XENSTORE_RECONNECT;
    XenEventChannelNotify (xs.Dev, xs.EventChannel);
    while (*(volatile UINT32*)&xs.XenStore->connection == XENSTORE_RECONNECT) {
      XenStoreWaitForEvent (xs.EventChannelEvent, EFI_TIMER_PERIOD_MILLISECONDS (100));
    }
  } else {
    /* If the backend reads the state while we're erasing it then the
     * ring state will become corrupted, preventing guest frontends from
     * connecting. This is rare. To help diagnose the failure, we fill
     * the ring with XS_INVALID packets. */
    SetMem (xs.XenStore->req, XENSTORE_RING_SIZE, 0xff);
    SetMem (xs.XenStore->rsp, XENSTORE_RING_SIZE, 0xff);
    xs.XenStore->req_cons = xs.XenStore->req_prod = 0;
    xs.XenStore->rsp_cons = xs.XenStore->rsp_prod = 0;
  }
  xs.XenStore = NULL;
}

//
// Public API
// API comments for these methods can be found in XenStore.h
//

XENSTORE_STATUS
XenStoreListDirectory (
  IN  CONST XENSTORE_TRANSACTION *Transaction,
  IN  CONST CHAR8           *DirectoryPath,
  IN  CONST CHAR8           *Node,
  OUT UINT32                *DirectoryCountPtr,
  OUT CONST CHAR8           ***DirectoryListPtr
  )
{
  CHAR8 *Path;
  CHAR8 *TempStr;
  UINT32 Len = 0;
  XENSTORE_STATUS Status;

  Path = XenStoreJoin (DirectoryPath, Node);
  Status = XenStoreSingle (Transaction, XS_DIRECTORY, Path, &Len,
                           (VOID **) &TempStr);
  FreePool (Path);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    return Status;
  }

  *DirectoryListPtr = Split (TempStr, Len, DirectoryCountPtr);

  return XENSTORE_STATUS_SUCCESS;
}

BOOLEAN
XenStorePathExists (
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8           *Directory,
  IN CONST CHAR8           *Node
  )
{
  CONST CHAR8 **TempStr;
  XENSTORE_STATUS Status;
  UINT32 TempNum;

  Status = XenStoreListDirectory (Transaction, Directory, Node,
                                  &TempNum, &TempStr);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    return FALSE;
  }
  FreePool ((VOID*)TempStr);
  return TRUE;
}

XENSTORE_STATUS
XenStoreRead (
  IN  CONST XENSTORE_TRANSACTION *Transaction,
  IN  CONST CHAR8             *DirectoryPath,
  IN  CONST CHAR8             *Node,
  OUT UINT32                  *LenPtr OPTIONAL,
  OUT VOID                    **Result
  )
{
  CHAR8 *Path;
  VOID *Value;
  XENSTORE_STATUS Status;

  Path = XenStoreJoin (DirectoryPath, Node);
  Status = XenStoreSingle (Transaction, XS_READ, Path, LenPtr, &Value);
  FreePool (Path);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    return Status;
  }

  *Result = Value;
  return XENSTORE_STATUS_SUCCESS;
}

XENSTORE_STATUS
XenStoreWrite (
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8           *DirectoryPath,
  IN CONST CHAR8           *Node,
  IN CONST CHAR8           *Str
  )
{
  CHAR8 *Path;
  WRITE_REQUEST WriteRequest[2];
  XENSTORE_STATUS Status;

  Path = XenStoreJoin (DirectoryPath, Node);

  WriteRequest[0].Data = (VOID *) Path;
  WriteRequest[0].Len = (UINT32)AsciiStrSize (Path);
  WriteRequest[1].Data = (VOID *) Str;
  WriteRequest[1].Len = (UINT32)AsciiStrLen (Str);

  Status = XenStoreTalkv (Transaction, XS_WRITE, WriteRequest, 2, NULL, NULL);
  FreePool (Path);

  return Status;
}

XENSTORE_STATUS
XenStoreRemove (
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8            *DirectoryPath,
  IN CONST CHAR8            *Node
  )
{
  CHAR8 *Path;
  XENSTORE_STATUS Status;

  Path = XenStoreJoin (DirectoryPath, Node);
  Status = XenStoreSingle (Transaction, XS_RM, Path, NULL, NULL);
  FreePool (Path);

  return Status;
}

XENSTORE_STATUS
XenStoreTransactionStart (
  OUT XENSTORE_TRANSACTION  *Transaction
  )
{
  CHAR8 *IdStr;
  XENSTORE_STATUS Status;

  Status = XenStoreSingle (XST_NIL, XS_TRANSACTION_START, "", NULL,
                           (VOID **) &IdStr);
  if (Status == XENSTORE_STATUS_SUCCESS) {
    Transaction->Id = (UINT32)AsciiStrDecimalToUintn (IdStr);
    FreePool (IdStr);
  }

  return Status;
}

XENSTORE_STATUS
XenStoreTransactionEnd (
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN BOOLEAN                Abort
  )
{
  CHAR8 AbortStr[2];

  AbortStr[0] = Abort ? 'F' : 'T';
  AbortStr[1] = '\0';

  return XenStoreSingle (Transaction, XS_TRANSACTION_END, AbortStr, NULL, NULL);
}

XENSTORE_STATUS
EFIAPI
XenStoreVSPrint (
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8           *DirectoryPath,
  IN CONST CHAR8           *Node,
  IN CONST CHAR8           *FormatString,
  IN VA_LIST               Marker
  )
{
  CHAR8 *Buf;
  XENSTORE_STATUS Status;
  UINTN BufSize;
  VA_LIST Marker2;

  VA_COPY (Marker2, Marker);
  BufSize = SPrintLengthAsciiFormat (FormatString, Marker2) + 1;
  VA_END (Marker2);
  Buf = AllocateZeroPool (BufSize);
  AsciiVSPrint (Buf, BufSize, FormatString, Marker);
  Status = XenStoreWrite (Transaction, DirectoryPath, Node, Buf);
  FreePool (Buf);

  return Status;
}

XENSTORE_STATUS
EFIAPI
XenStoreSPrint (
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8            *DirectoryPath,
  IN CONST CHAR8            *Node,
  IN CONST CHAR8            *FormatString,
  ...
  )
{
  VA_LIST Marker;
  XENSTORE_STATUS Status;

  VA_START (Marker, FormatString);
  Status = XenStoreVSPrint (Transaction, DirectoryPath, Node, FormatString, Marker);
  VA_END (Marker);

  return Status;
}

XENSTORE_STATUS
XenStoreRegisterWatch (
  IN CONST CHAR8      *DirectoryPath,
  IN CONST CHAR8      *Node,
  OUT XENSTORE_WATCH  **WatchPtr
  )
{
  /* Pointer in ascii is the token. */
  CHAR8 Token[sizeof (XENSTORE_WATCH) * 2 + 1];
  XENSTORE_STATUS Status;
  XENSTORE_WATCH *Watch;

  Watch = AllocateZeroPool (sizeof (XENSTORE_WATCH));
  Watch->Signature = XENSTORE_WATCH_SIGNATURE;
  Watch->Node = XenStoreJoin (DirectoryPath, Node);

  EfiAcquireLock (&xs.RegisteredWatchesLock);
  InsertTailList (&xs.RegisteredWatches, &Watch->Link);
  EfiReleaseLock (&xs.RegisteredWatchesLock);

  AsciiSPrint (Token, sizeof (Token), "%p", (VOID*) Watch);
  Status = XenStoreWatch (Watch->Node, Token);

  /* Ignore errors due to multiple registration. */
  if (Status == XENSTORE_STATUS_EEXIST) {
    Status = XENSTORE_STATUS_SUCCESS;
  }

  if (Status == XENSTORE_STATUS_SUCCESS) {
    *WatchPtr = Watch;
  } else {
    EfiAcquireLock (&xs.RegisteredWatchesLock);
    RemoveEntryList (&Watch->Link);
    EfiReleaseLock (&xs.RegisteredWatchesLock);
    FreePool (Watch->Node);
    FreePool (Watch);
  }

  return Status;
}

VOID
XenStoreUnregisterWatch (
  IN XENSTORE_WATCH *Watch
  )
{
  CHAR8 Token[sizeof (Watch) * 2 + 1];
  LIST_ENTRY *Entry;

  ASSERT (Watch->Signature == XENSTORE_WATCH_SIGNATURE);

  AsciiSPrint (Token, sizeof (Token), "%p", (VOID *) Watch);
  if (XenStoreFindWatch (Token) == NULL) {
    return;
  }

  EfiAcquireLock (&xs.RegisteredWatchesLock);
  RemoveEntryList (&Watch->Link);
  EfiReleaseLock (&xs.RegisteredWatchesLock);

  XenStoreUnwatch (Watch->Node, Token);

  /* Cancel pending watch events. */
  EfiAcquireLock (&xs.WatchEventsLock);
  Entry = GetFirstNode (&xs.WatchEvents);
  while (!IsNull (&xs.WatchEvents, Entry)) {
    XENSTORE_MESSAGE *Message = XENSTORE_MESSAGE_FROM_LINK (Entry);
    Entry = GetNextNode (&xs.WatchEvents, Entry);
    if (Message->u.Watch.Handle == Watch) {
      RemoveEntryList (&Message->Link);
      FreePool ((VOID*)Message->u.Watch.Vector);
      FreePool (Message);
    }
  }
  EfiReleaseLock (&xs.WatchEventsLock);

  FreePool (Watch->Node);
  FreePool (Watch);
}


//
// XENBUS protocol
//

XENSTORE_STATUS
EFIAPI
XenBusWaitForWatch (
  IN XENBUS_PROTOCOL *This,
  IN VOID *Token
  )
{
  return XenStoreWaitWatch (Token);
}

XENSTORE_STATUS
EFIAPI
XenBusXenStoreRead (
  IN  XENBUS_PROTOCOL       *This,
  IN  CONST XENSTORE_TRANSACTION *Transaction,
  IN  CONST CHAR8           *Node,
  OUT VOID                  **Value
  )
{
  return XenStoreRead (Transaction, This->Node, Node, NULL, Value);
}

XENSTORE_STATUS
EFIAPI
XenBusXenStoreBackendRead (
  IN  XENBUS_PROTOCOL       *This,
  IN  CONST XENSTORE_TRANSACTION *Transaction,
  IN  CONST CHAR8           *Node,
  OUT VOID                  **Value
  )
{
  return XenStoreRead (Transaction, This->Backend, Node, NULL, Value);
}

XENSTORE_STATUS
EFIAPI
XenBusXenStoreRemove (
  IN XENBUS_PROTOCOL        *This,
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN const char             *Node
  )
{
  return XenStoreRemove (Transaction, This->Node, Node);
}

XENSTORE_STATUS
EFIAPI
XenBusXenStoreTransactionStart (
  IN  XENBUS_PROTOCOL       *This,
  OUT XENSTORE_TRANSACTION  *Transaction
  )
{
  return XenStoreTransactionStart (Transaction);
}

XENSTORE_STATUS
EFIAPI
XenBusXenStoreTransactionEnd (
  IN XENBUS_PROTOCOL        *This,
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN BOOLEAN                Abort
  )
{
  return XenStoreTransactionEnd (Transaction, Abort);
}

XENSTORE_STATUS
EFIAPI
XenBusXenStoreSPrint (
  IN XENBUS_PROTOCOL        *This,
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8            *DirectoryPath,
  IN CONST CHAR8            *Node,
  IN CONST CHAR8            *FormatString,
  ...
  )
{
  VA_LIST Marker;
  XENSTORE_STATUS Status;

  VA_START (Marker, FormatString);
  Status = XenStoreVSPrint (Transaction, DirectoryPath, Node, FormatString, Marker);
  VA_END (Marker);

  return Status;
}

XENSTORE_STATUS
EFIAPI
XenBusRegisterWatch (
  IN  XENBUS_PROTOCOL *This,
  IN  CONST CHAR8     *Node,
  OUT VOID            **Token
  )
{
  return XenStoreRegisterWatch (This->Node, Node, (XENSTORE_WATCH **) Token);
}

XENSTORE_STATUS
EFIAPI
XenBusRegisterWatchBackend (
  IN  XENBUS_PROTOCOL *This,
  IN  CONST CHAR8     *Node,
  OUT VOID            **Token
  )
{
  return XenStoreRegisterWatch (This->Backend, Node, (XENSTORE_WATCH **) Token);
}

VOID
EFIAPI
XenBusUnregisterWatch (
  IN XENBUS_PROTOCOL  *This,
  IN VOID             *Token
  )
{
  XenStoreUnregisterWatch ((XENSTORE_WATCH *) Token);
}
