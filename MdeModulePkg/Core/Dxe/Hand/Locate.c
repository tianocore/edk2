/** @file
  Locate handle functions

Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeMain.h"
#include "Handle.h"

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
CoreGetNextLocateAllHandles (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
  );

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
CoreGetNextLocateByRegisterNotify (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
  );

/**
  Routine to get the next Handle, when you are searching for a given protocol.

  @param  Position               Information about which Handle to seach for.
  @param  Interface              Return the interface structure for the matching
                                 protocol.

  @return An pointer to IHANDLE if the next Position is not the end of the list.
          Otherwise,NULL is returned.

**/
IHANDLE *
CoreGetNextLocateByProtocol (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
  );


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
CoreLocateHandle (
  IN EFI_LOCATE_SEARCH_TYPE   SearchType,
  IN EFI_GUID                 *Protocol   OPTIONAL,
  IN VOID                     *SearchKey  OPTIONAL,
  IN OUT UINTN                *BufferSize,
  OUT EFI_HANDLE              *Buffer
  )
{
  EFI_STATUS          Status;
  LOCATE_POSITION     Position;
  PROTOCOL_NOTIFY     *ProtNotify;
  CORE_GET_NEXT       GetNext;
  UINTN               ResultSize;
  IHANDLE             *Handle;
  IHANDLE             **ResultBuffer;
  VOID                *Interface;

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
  // Lock the protocol database
  //
  CoreAcquireProtocolLock ();

  //
  // Get the search function based on type
  //
  switch (SearchType) {
  case AllHandles:
    GetNext = CoreGetNextLocateAllHandles;
    break;

  case ByRegisterNotify:
    //
    // Must have SearchKey for locate ByRegisterNotify
    //
    if (SearchKey == NULL) {
      Status = EFI_INVALID_PARAMETER;
      break;
    }
    GetNext = CoreGetNextLocateByRegisterNotify;
    break;

  case ByProtocol:
    GetNext = CoreGetNextLocateByProtocol;
    if (Protocol == NULL) {
      Status = EFI_INVALID_PARAMETER;
      break;
    }
    //
    // Look up the protocol entry and set the head pointer
    //
    Position.ProtEntry = CoreFindProtocolEntry (Protocol, FALSE);
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

  if (EFI_ERROR(Status)) {
    CoreReleaseProtocolLock ();
    return Status;
  }

  ASSERT (GetNext != NULL);
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
    ResultSize += sizeof(Handle);
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

    if (SearchType == ByRegisterNotify && !EFI_ERROR(Status)) {
      //
      // If this is a search by register notify and a handle was
      // returned, update the register notification position
      //
      ASSERT (SearchKey != NULL);
      ProtNotify = SearchKey;
      ProtNotify->Position = ProtNotify->Position->ForwardLink;
    }
  }

  CoreReleaseProtocolLock ();
  return Status;
}



/**
  Routine to get the next Handle, when you are searching for all handles.

  @param  Position               Information about which Handle to seach for.
  @param  Interface              Return the interface structure for the matching
                                 protocol.

  @return An pointer to IHANDLE if the next Position is not the end of the list.
          Otherwise,NULL is returned.

**/
IHANDLE *
CoreGetNextLocateAllHandles (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
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
CoreGetNextLocateByRegisterNotify (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
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
    ASSERT(ProtNotify->Signature == PROTOCOL_NOTIFY_SIGNATURE);
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
CoreGetNextLocateByProtocol (
  IN OUT LOCATE_POSITION    *Position,
  OUT VOID                  **Interface
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
    Prot = CR(Link, PROTOCOL_INTERFACE, ByProtocol, PROTOCOL_INTERFACE_SIGNATURE);
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
  Locates the handle to a device on the device path that supports the specified protocol.

  @param  Protocol              Specifies the protocol to search for.
  @param  DevicePath            On input, a pointer to a pointer to the device path. On output, the device
                                path pointer is modified to point to the remaining part of the device
                                path.
  @param  Device                A pointer to the returned device handle.

  @retval EFI_SUCCESS           The resulting handle was returned.
  @retval EFI_NOT_FOUND         No handles match the search.
  @retval EFI_INVALID_PARAMETER Protocol is NULL.
  @retval EFI_INVALID_PARAMETER DevicePath is NULL.
  @retval EFI_INVALID_PARAMETER A handle matched the search and Device is NULL.

**/
EFI_STATUS
EFIAPI
CoreLocateDevicePath (
  IN EFI_GUID                       *Protocol,
  IN OUT EFI_DEVICE_PATH_PROTOCOL   **DevicePath,
  OUT EFI_HANDLE                    *Device
  )
{
  INTN                        SourceSize;
  INTN                        Size;
  INTN                        BestMatch;
  UINTN                       HandleCount;
  UINTN                       Index;
  EFI_STATUS                  Status;
  EFI_HANDLE                  *Handles;
  EFI_HANDLE                  Handle;
  EFI_HANDLE                  BestDevice;
  EFI_DEVICE_PATH_PROTOCOL    *SourcePath;
  EFI_DEVICE_PATH_PROTOCOL    *TmpDevicePath;

  if (Protocol == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DevicePath == NULL) || (*DevicePath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Handles = NULL;
  BestDevice = NULL;
  SourcePath = *DevicePath;
  TmpDevicePath = SourcePath;
  while (!IsDevicePathEnd (TmpDevicePath)) {
    if (IsDevicePathEndInstance (TmpDevicePath)) {
      //
      // If DevicePath is a multi-instance device path,
      // the function will operate on the first instance 
      //
      break;
    }
    TmpDevicePath = NextDevicePathNode (TmpDevicePath);
  }

  SourceSize = (UINTN) TmpDevicePath - (UINTN) SourcePath;

  //
  // Get a list of all handles that support the requested protocol
  //
  Status = CoreLocateHandleBuffer (ByProtocol, Protocol, NULL, &HandleCount, &Handles);
  if (EFI_ERROR (Status) || HandleCount == 0) {
    return EFI_NOT_FOUND;
  }

  BestMatch = -1;
  for(Index = 0; Index < HandleCount; Index += 1) {
    Handle = Handles[Index];
    Status = CoreHandleProtocol (Handle, &gEfiDevicePathProtocolGuid, (VOID **)&TmpDevicePath);
    if (EFI_ERROR (Status)) {
      //
      // If this handle doesn't support device path, then skip it
      //
      continue;
    }

    //
    // Check if DevicePath is first part of SourcePath
    //
    Size = GetDevicePathSize (TmpDevicePath) - sizeof(EFI_DEVICE_PATH_PROTOCOL);
    ASSERT (Size >= 0);
    if ((Size <= SourceSize) && CompareMem (SourcePath, TmpDevicePath, (UINTN) Size) == 0) {
      //
      // If the size is equal to the best match, then we
      // have a duplicate device path for 2 different device
      // handles
      //
      ASSERT (Size != BestMatch);

      //
      // We've got a match, see if it's the best match so far
      //
      if (Size > BestMatch) {
        BestMatch = Size;
        BestDevice = Handle;
      }
    }
  }

  CoreFreePool (Handles);

  //
  // If there wasn't any match, then no parts of the device path was found.
  // Which is strange since there is likely a "root level" device path in the system.
  //
  if (BestMatch == -1) {
    return EFI_NOT_FOUND;
  }

  if (Device == NULL) {
    return  EFI_INVALID_PARAMETER;
  }
  *Device = BestDevice;
  
  //
  // Return the remaining part of the device path
  //
  *DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) (((UINT8 *) SourcePath) + BestMatch);
  return EFI_SUCCESS;
}


/**
  Return the first Protocol Interface that matches the Protocol GUID. If
  Registration is passed in, return a Protocol Instance that was just add
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
CoreLocateProtocol (
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

  //
  // Lock the protocol database
  //
  Status = CoreAcquireLockOrFail (&gProtocolDatabaseLock);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  mEfiLocateHandleRequest += 1;

  if (Registration == NULL) {
    //
    // Look up the protocol entry and set the head pointer
    //
    Position.ProtEntry = CoreFindProtocolEntry (Protocol, FALSE);
    if (Position.ProtEntry == NULL) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }
    Position.Position = &Position.ProtEntry->Protocols;

    Handle = CoreGetNextLocateByProtocol (&Position, Interface);
  } else {
    Handle = CoreGetNextLocateByRegisterNotify (&Position, Interface);
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

Done:
  CoreReleaseProtocolLock ();
  return Status;
}


/**
  Function returns an array of handles that support the requested protocol
  in a buffer allocated from pool. This is a version of CoreLocateHandle()
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
CoreLocateHandleBuffer (
  IN EFI_LOCATE_SEARCH_TYPE       SearchType,
  IN EFI_GUID                     *Protocol OPTIONAL,
  IN VOID                         *SearchKey OPTIONAL,
  IN OUT UINTN                    *NumberHandles,
  OUT EFI_HANDLE                  **Buffer
  )
{
  EFI_STATUS          Status;
  UINTN               BufferSize;

  if (NumberHandles == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BufferSize = 0;
  *NumberHandles = 0;
  *Buffer = NULL;
  Status = CoreLocateHandle (
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
  // Add code to correctly handle expected errors from CoreLocateHandle().
  //
  if (EFI_ERROR(Status) && Status != EFI_BUFFER_TOO_SMALL) {
    if (Status != EFI_INVALID_PARAMETER) {
      Status = EFI_NOT_FOUND;
    }
    return Status;
  }

  *Buffer = AllocatePool (BufferSize);
  if (*Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = CoreLocateHandle (
             SearchType,
             Protocol,
             SearchKey,
             &BufferSize,
             *Buffer
             );

  *NumberHandles = BufferSize / sizeof(EFI_HANDLE);
  if (EFI_ERROR(Status)) {
    *NumberHandles = 0;
  }

  return Status;
}



