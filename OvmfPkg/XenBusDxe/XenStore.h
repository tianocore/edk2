/** @file
  Method declarations and structures for accessing the XenStore

  Copyright (C) 2005 Rusty Russell, IBM Corporation
  Copyright (C) 2005 XenSource Ltd.
  Copyright (C) 2009,2010 Spectra Logic Corporation
  Copyright (C) 2014, Citrix Ltd.

  This file may be distributed separately from the Linux kernel, or
  incorporated into other software packages, subject to the following license:

  SPDX-License-Identifier: MIT
**/

#ifndef _XEN_XENSTORE_XENSTOREVAR_H
#define _XEN_XENSTORE_XENSTOREVAR_H

#include "XenBusDxe.h"

#include <IndustryStandard/Xen/io/xs_wire.h>

typedef struct _XENSTORE_WATCH XENSTORE_WATCH;

/**
  Fetch the contents of a directory in the XenStore.

  @param Transaction        The XenStore transaction covering this request.
  @param DirectoryPath      The dirname of the path to read.
  @param Node               The basename of the path to read.
  @param DirectoryCountPtr  The returned number of directory entries.
  @param DirectoryListPtr   An array of directory entry strings.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.

  @note The results buffer is alloced and should be free'd by the
        caller.
**/
XENSTORE_STATUS
XenStoreListDirectory (
  IN  CONST XENSTORE_TRANSACTION *Transaction,
  IN  CONST CHAR8           *DirectoryPath,
  IN  CONST CHAR8           *Node,
  OUT UINT32                *DirectoryCountPtr,
  OUT CONST CHAR8           ***DirectoryListPtr
  );

/**
  Determine if a path exists in the XenStore.

  @param Transaction  The XenStore transaction covering this request.
  @param Directory    The dirname of the path to read.
  @param Node         The basename of the path to read.

  @retval TRUE  The path exists.
  @retval FALSE The path does not exist or an error occurred attempting
                to make that determination.
**/
BOOLEAN
XenStorePathExists (
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8 *Directory,
  IN CONST CHAR8 *Node
  );

/**
  Get the contents of a single "file".  Returns the contents in *Result which
  should be freed after use.  The length of the value in bytes is returned in
  *LenPtr.

  @param Transaction    The XenStore transaction covering this request.
  @param DirectoryPath  The dirname of the file to read.
  @param Node           The basename of the file to read.
  @param LenPtr         The amount of data read.
  @param Result         The returned contents from this file.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.

  @note The results buffer is malloced and should be free'd by the
        caller.
**/
XENSTORE_STATUS
XenStoreRead (
  IN  CONST XENSTORE_TRANSACTION *Transaction,
  IN  CONST CHAR8             *DirectoryPath,
  IN  CONST CHAR8             *Node,
  OUT UINT32                  *LenPtr OPTIONAL,
  OUT VOID                    **Result
  );

/**
  Write to a single file.

  @param Transaction    The XenStore transaction covering this request.
  @param DirectoryPath  The dirname of the file to write.
  @param Node           The basename of the file to write.
  @param Str            The NUL terminated string of data to write.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.
**/
XENSTORE_STATUS
XenStoreWrite (
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8           *DirectoryPath,
  IN CONST CHAR8           *Node,
  IN CONST CHAR8           *Str
  );

/**
  Remove a file or directory (directories must be empty).

  @param Transaction    The XenStore transaction covering this request.
  @param DirectoryPath  The dirname of the directory to remove.
  @param Node           The basename of the directory to remove.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.
**/
XENSTORE_STATUS
XenStoreRemove (
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8            *DirectoryPath,
  IN CONST CHAR8            *Node
  );

/**
  Start a transaction.

  Changes by others will not be seen during the lifetime of this
  transaction, and changes will not be visible to others until it
  is committed (XenStoreTransactionEnd).

  @param Transaction  The returned transaction.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.
**/
XENSTORE_STATUS
XenStoreTransactionStart (
  OUT XENSTORE_TRANSACTION *Transaction
  );

/**
  End a transaction.

  @param Transaction  The transaction to end/commit.
  @param Abort        If TRUE, the transaction is discarded
                      instead of committed.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.
**/
XENSTORE_STATUS
XenStoreTransactionEnd (
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN BOOLEAN                Abort
  );

/**
  Printf formatted write to a XenStore file.

  @param Transaction      The XenStore transaction covering this request.
  @param DirectoryPath    The dirname of the path to read.
  @param Node             The basename of the path to read.
  @param FormatString     AsciiSPrint format string followed by a variable number
                          of arguments.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of write failure.
**/
XENSTORE_STATUS
EFIAPI
XenStoreSPrint (
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8            *DirectoryPath,
  IN CONST CHAR8            *Node,
  IN CONST CHAR8            *FormatString,
  ...
  );

/**
  VA_LIST version of XenStoreSPrint().

  @param Transaction    The XenStore transaction covering this request.
  @param DirectoryPath  The dirname of the path to read.
  @param Node           The basename of the path to read.
  @param FormatString   Printf format string.
  @param Marker         VA_LIST of printf arguments.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of write failure.
**/
XENSTORE_STATUS
EFIAPI
XenStoreVSPrint (
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8           *DirectoryPath,
  IN CONST CHAR8           *Node,
  IN CONST CHAR8           *FormatString,
  IN VA_LIST               Marker
  );

/**
  Register a XenStore watch.

  XenStore watches allow a client to be notified via a callback (embedded
  within the watch object) of changes to an object in the XenStore.

  @param DirectoryPath  The dirname of the path to watch.
  @param Node           The basename of the path to watch.
  @param WatchPtr       A returned XENSTORE_WATCH pointer.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of write failure.  EEXIST errors from the
           XenStore are suppressed, allowing multiple, physically different,
           xenbus_watch objects, to watch the same path in the XenStore.
**/
XENSTORE_STATUS
XenStoreRegisterWatch (
  IN  CONST CHAR8     *DirectoryPath,
  IN  CONST CHAR8     *Node,
  OUT XENSTORE_WATCH  **WatchPtr
  );

/**
  Unregister a XenStore watch.

  @param Watch  An XENSTORE_WATCH object previously returned by a successful
                call to XenStoreRegisterWatch ().
**/
VOID
XenStoreUnregisterWatch (
  IN XENSTORE_WATCH *Watch
  );

/**
  Allocate and return the XenStore path string <DirectoryPath>/<Node>.  If name
  is the NUL string, the returned value contains the path string
  <DirectoryPath>.

  @param DirectoryPath	The NUL terminated directory prefix for new path.
  @param Node           The NUL terminated basename for the new path.

  @return  A buffer containing the joined path.
 */
CHAR8 *
XenStoreJoin (
  IN CONST CHAR8 *DirectoryPath,
  IN CONST CHAR8 *Node
  );


/**
  Initialize the XenStore states and rings.

  @param Dev  A pointer to a XENBUS_DEVICE instance.

  @return     EFI_SUCCESS if everything went smoothly.
**/
EFI_STATUS
XenStoreInit (
  XENBUS_DEVICE *Dev
  );

/**
  Deinitialize the XenStore states and rings.

  @param Dev  A pointer to a XENBUS_DEVICE instance.
**/
VOID
XenStoreDeinit (
  IN XENBUS_DEVICE *Dev
  );


//
// XENBUS protocol
//

XENSTORE_STATUS
EFIAPI
XenBusWaitForWatch (
  IN XENBUS_PROTOCOL *This,
  IN VOID *Token
  );

XENSTORE_STATUS
EFIAPI
XenBusXenStoreRead (
  IN  XENBUS_PROTOCOL       *This,
  IN  CONST XENSTORE_TRANSACTION *Transaction,
  IN  CONST CHAR8           *Node,
  OUT VOID                  **Value
  );

XENSTORE_STATUS
EFIAPI
XenBusXenStoreBackendRead (
  IN  XENBUS_PROTOCOL       *This,
  IN  CONST XENSTORE_TRANSACTION *Transaction,
  IN  CONST CHAR8           *Node,
  OUT VOID                  **Value
  );

XENSTORE_STATUS
EFIAPI
XenBusXenStoreRemove (
  IN XENBUS_PROTOCOL        *This,
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8            *Node
  );

XENSTORE_STATUS
EFIAPI
XenBusXenStoreTransactionStart (
  IN  XENBUS_PROTOCOL       *This,
  OUT XENSTORE_TRANSACTION  *Transaction
  );

XENSTORE_STATUS
EFIAPI
XenBusXenStoreTransactionEnd (
  IN XENBUS_PROTOCOL        *This,
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN BOOLEAN                Abort
  );

XENSTORE_STATUS
EFIAPI
XenBusXenStoreSPrint (
  IN XENBUS_PROTOCOL        *This,
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8            *DirectoryPath,
  IN CONST CHAR8            *Node,
  IN CONST CHAR8            *FormatString,
  ...
  );

XENSTORE_STATUS
EFIAPI
XenBusRegisterWatch (
  IN  XENBUS_PROTOCOL *This,
  IN  CONST CHAR8     *Node,
  OUT VOID            **Token
  );

XENSTORE_STATUS
EFIAPI
XenBusRegisterWatchBackend (
  IN  XENBUS_PROTOCOL *This,
  IN  CONST CHAR8     *Node,
  OUT VOID            **Token
  );

VOID
EFIAPI
XenBusUnregisterWatch (
  IN XENBUS_PROTOCOL  *This,
  IN VOID             *Token
  );

#endif /* _XEN_XENSTORE_XENSTOREVAR_H */
