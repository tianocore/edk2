/** @file
  Locate handle functions

  Copyright (c) 2009 - 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2021, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmCore.h"

//
// ProtocolRequest - Last LocateHandle request ID
//
UINTN mEfiLocateHandleRequest = 0;

//
// Internal prototypes
//

typedef struct {
  EFI_GUID        *Protocol;
  VOID            *SearchKey;
  LIST_ENTRY      *Position;
  PROTOCOL_ENTRY  *ProtEntry;
} LOCATE_POSITION;

typedef
IHANDLE *
(* CORE_GET_NEXT) (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
  );

/**
  Routine to get the next Handle, when you are searching for all handles.

  @param  Position               Information about which Handle to seach for.
  @param  Interface              Return the interface structure for the matching
                                 protocol.

  @return An pointer to IHANDLE if the next Position is not the end of the list.
          Otherwise,NULL is returned.

**/
IHANDLE *
MmGetNextLocateAllHandles (
  IN OUT LOCATE_POSITION  *Position,
  OUT    VOID             **Interface
  )
{
  IHANDLE     *Handle;

  //
  // Next handle
  //
  Position->Position = Position->Position->ForwardLink;

  //
  // If not at the end of the list, get the handle
  //
  Handle      = NULL;
  *Interface  = NULL;
  if (Position->Position != &gHandleList) {
    Handle = CR (Position->Position, IHANDLE, AllHandles, EFI_HANDLE_SIGNATURE);
  }
  return Handle;
}

/**
  Routine to get the next Handle, when you are searching for register protocol
  notifies.

  @param  Position               Information about which Handle to seach for.
  @param  Interface              Return the interface structure for the matching
                                 protocol.

  @return An pointer to IHANDLE if the next Position is not the end of the list.
          Otherwise,NULL is returned.

**/
IHANDLE *
MmGetNextLocateByRegisterNotify (
  IN OUT LOCATE_POSITION  *Position,
  OUT    VOID             **Interface
  )
{
  IHANDLE             *Handle;
  PROTOCOL_NOTIFY     *ProtNotify;
  PROTOCOL_INTERFACE  *Prot;
  LIST_ENTRY          *Link;

  Handle      = NULL;
  *Interface  = NULL;
  ProtNotify = Position->SearchKey;

  //
  // If this is the first request, get the next handle
  //
  if (ProtNotify != NULL) {
    ASSERT (ProtNotify->Signature == PROTOCOL_NOTIFY_SIGNATURE);
    Position->SearchKey = NULL;

    //
    // If not at the end of the list, get the next handle
    //
    Link = ProtNotify->Position->ForwardLink;
    if (Link != &ProtNotify->Protocol->Protocols) {
      Prot = CR (Link, PROTOCOL_INTERFACE, ByProtocol, PROTOCOL_INTERFACE_SIGNATURE);
      Handle = Prot->Handle;
      *Interface = Prot->Interface;
    }
  }
  return Handle;
}

/**
  Routine to get the next Handle, when you are searching for a given protocol.

  @param  Position               Information about which Handle to seach for.
  @param  Interface              Return the interface structure for the matching
                                 protocol.

  @return An pointer to IHANDLE if the next Position is not the end of the list.
          Otherwise,NULL is returned.

**/
IHANDLE *
MmGetNextLocateByProtocol (
  IN OUT LOCATE_POSITION  *Position,
  OUT    VOID             **Interface
  )
{
  IHANDLE             *Handle;
  LIST_ENTRY          *Link;
  PROTOCOL_INTERFACE  *Prot;

  Handle      = NULL;
  *Interface  = NULL;
  for (; ;) {
    //
    // Next entry
    //
    Link = Position->Position->ForwardLink;
    Position->Position = Link;

    //
    // If not at the end, return the handle
    //
    if (Link == &Position->ProtEntry->Protocols) {
      Handle = NULL;
      break;
    }

    //
    // Get the handle
    //
    Prot = CR (Link, PROTOCOL_INTERFACE, ByProtocol, PROTOCOL_INTERFACE_SIGNATURE);
    Handle = Prot->Handle;
    *Interface = Prot->Interface;

    //
    // If this handle has not been returned this request, then
    // return it now
    //
    if (Handle->LocateRequest != mEfiLocateHandleRequest) {
      Handle->LocateRequest = mEfiLocateHandleRequest;
      break;
    }
  }
  return Handle;
}

/**
  Return the first Protocol Interface that matches the Protocol GUID. If
  Registration is passed in return a Protocol Instance that was just add
  to the system. If Registration is NULL return the first Protocol Interface
  you find.

  @param  Protocol               The protocol to search for
  @param  Registration           Optional Registration Key returned from
                                 RegisterProtocolNotify()
  @param  Interface              Return the Protocol interface (instance).

  @retval EFI_SUCCESS            If a valid Interface is returned
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Protocol interface not found

**/
EFI_STATUS
EFIAPI
MmLocateProtocol (
  IN  EFI_GUID  *Protocol,
  IN  VOID      *Registration OPTIONAL,
  OUT VOID      **Interface
  )
{
  EFI_STATUS              Status;
  LOCATE_POSITION         Position;
  PROTOCOL_NOTIFY         *ProtNotify;
  IHANDLE                 *Handle;

  if ((Interface == NULL) || (Protocol == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Interface = NULL;
  Status = EFI_SUCCESS;

  //
  // Set initial position
  //
  Position.Protocol  = Protocol;
  Position.SearchKey = Registration;
  Position.Position  = &gHandleList;

  mEfiLocateHandleRequest += 1;

  if (Registration == NULL) {
    //
    // Look up the protocol entry and set the head pointer
    //
    Position.ProtEntry = MmFindProtocolEntry (Protocol, FALSE);
    if (Position.ProtEntry == NULL) {
      return EFI_NOT_FOUND;
    }
    Position.Position = &Position.ProtEntry->Protocols;

    Handle = MmGetNextLocateByProtocol (&Position, Interface);
  } else {
    Handle = MmGetNextLocateByRegisterNotify (&Position, Interface);
  }

  if (Handle == NULL) {
    Status = EFI_NOT_FOUND;
  } else if (Registration != NULL) {
    //
    // If this is a search by register notify and a handle was
    // returned, update the register notification position
    //
    ProtNotify = Registration;
    ProtNotify->Position = ProtNotify->Position->ForwardLink;
  }

  return Status;
}

/**
  Locates the requested handle(s) and returns them in Buffer.

  @param  SearchType             The type of search to perform to locate the
                                 handles
  @param  Protocol               The protocol to search for
  @param  SearchKey              Dependant on SearchType
  @param  BufferSize             On input the size of Buffer.  On output the
                                 size of data returned.
  @param  Buffer                 The buffer to return the results in

  @retval EFI_BUFFER_TOO_SMALL   Buffer too small, required buffer size is
                                 returned in BufferSize.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCCESS            Successfully found the requested handle(s) and
                                 returns them in Buffer.

**/
EFI_STATUS
EFIAPI
MmLocateHandle (
  IN     EFI_LOCATE_SEARCH_TYPE  SearchType,
  IN     EFI_GUID                *Protocol   OPTIONAL,
  IN     VOID                    *SearchKey  OPTIONAL,
  IN OUT UINTN                   *BufferSize,
  OUT    EFI_HANDLE              *Buffer
  )
{
  EFI_STATUS       Status;
  LOCATE_POSITION  Position;
  PROTOCOL_NOTIFY  *ProtNotify;
  CORE_GET_NEXT    GetNext;
  UINTN            ResultSize;
  IHANDLE          *Handle;
  IHANDLE          **ResultBuffer;
  VOID             *Interface;

  if (BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*BufferSize > 0) && (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  GetNext = NULL;

  //
  // Set initial position
  //
  Position.Protocol  = Protocol;
  Position.SearchKey = SearchKey;
  Position.Position  = &gHandleList;

  ResultSize = 0;
  ResultBuffer = (IHANDLE **) Buffer;
  Status = EFI_SUCCESS;

  //
  // Get the search function based on type
  //
  switch (SearchType) {
  case AllHandles:
    GetNext = MmGetNextLocateAllHandles;
    break;

  case ByRegisterNotify:
    GetNext = MmGetNextLocateByRegisterNotify;
    //
    // Must have SearchKey for locate ByRegisterNotify
    //
    if (SearchKey == NULL) {
      Status = EFI_INVALID_PARAMETER;
    }
    break;

  case ByProtocol:
    GetNext = MmGetNextLocateByProtocol;
    if (Protocol == NULL) {
      Status = EFI_INVALID_PARAMETER;
      break;
    }
    //
    // Look up the protocol entry and set the head pointer
    //
    Position.ProtEntry = MmFindProtocolEntry (Protocol, FALSE);
    if (Position.ProtEntry == NULL) {
      Status = EFI_NOT_FOUND;
      break;
    }
    Position.Position = &Position.ProtEntry->Protocols;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Enumerate out the matching handles
  //
  mEfiLocateHandleRequest += 1;
  for (; ;) {
    //
    // Get the next handle.  If no more handles, stop
    //
    Handle = GetNext (&Position, &Interface);
    if (NULL == Handle) {
      break;
    }

    //
    // Increase the resulting buffer size, and if this handle
    // fits return it
    //
    ResultSize += sizeof (Handle);
    if (ResultSize <= *BufferSize) {
        *ResultBuffer = Handle;
        ResultBuffer += 1;
    }
  }

  //
  // If the result is a zero length buffer, then there were no
  // matching handles
  //
  if (ResultSize == 0) {
    Status = EFI_NOT_FOUND;
  } else {
    //
    // Return the resulting buffer size.  If it's larger than what
    // was passed, then set the error code
    //
    if (ResultSize > *BufferSize) {
      Status = EFI_BUFFER_TOO_SMALL;
    }

    *BufferSize = ResultSize;

    if (SearchType == ByRegisterNotify && !EFI_ERROR (Status)) {
      ASSERT (SearchKey != NULL);
      //
      // If this is a search by register notify and a handle was
      // returned, update the register notification position
      //
      ProtNotify = SearchKey;
      ProtNotify->Position = ProtNotify->Position->ForwardLink;
    }
  }

  return Status;
}

/**
  Function returns an array of handles that support the requested protocol
  in a buffer allocated from pool. This is a version of MmLocateHandle()
  that allocates a buffer for the caller.

  @param  SearchType             Specifies which handle(s) are to be returned.
  @param  Protocol               Provides the protocol to search by.    This
                                 parameter is only valid for SearchType
                                 ByProtocol.
  @param  SearchKey              Supplies the search key depending on the
                                 SearchType.
  @param  NumberHandles          The number of handles returned in Buffer.
  @param  Buffer                 A pointer to the buffer to return the requested
                                 array of  handles that support Protocol.

  @retval EFI_SUCCESS            The result array of handles was returned.
  @retval EFI_NOT_FOUND          No handles match the search.
  @retval EFI_OUT_OF_RESOURCES   There is not enough pool memory to store the
                                 matching results.
  @retval EFI_INVALID_PARAMETER  One or more parameters are not valid.

**/
EFI_STATUS
EFIAPI
MmLocateHandleBuffer (
  IN     EFI_LOCATE_SEARCH_TYPE  SearchType,
  IN     EFI_GUID                *Protocol OPTIONAL,
  IN     VOID                    *SearchKey OPTIONAL,
  IN OUT UINTN                   *NumberHandles,
  OUT    EFI_HANDLE              **Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;

  if (NumberHandles == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BufferSize = 0;
  *NumberHandles = 0;
  *Buffer = NULL;
  Status = MmLocateHandle (
             SearchType,
             Protocol,
             SearchKey,
             &BufferSize,
             *Buffer
             );
  //
  // LocateHandleBuffer() returns incorrect status code if SearchType is
  // invalid.
  //
  // Add code to correctly handle expected errors from MmLocateHandle().
  //
  if (EFI_ERROR (Status) && Status != EFI_BUFFER_TOO_SMALL) {
    if (Status != EFI_INVALID_PARAMETER) {
      Status = EFI_NOT_FOUND;
    }
    return Status;
  }

  *Buffer = AllocatePool (BufferSize);
  if (*Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = MmLocateHandle (
             SearchType,
             Protocol,
             SearchKey,
             &BufferSize,
             *Buffer
             );

  *NumberHandles = BufferSize / sizeof(EFI_HANDLE);
  if (EFI_ERROR (Status)) {
    *NumberHandles = 0;
  }

  return Status;
}
