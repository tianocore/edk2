/** @file
  Member functions of EFI_SHELL_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PROTOCOL.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Shell.h"

#define INIT_NAME_BUFFER_SIZE  128

/**
  Close an open file handle.

  This function closes a specified file handle. All "dirty" cached file data is
  flushed to the device, and the file is closed. In all cases the handle is
  closed.

  @param[in] FileHandle           The file handle to close.

  @retval EFI_SUCCESS             The file handle was closed successfully.
**/
EFI_STATUS
EFIAPI
EfiShellClose (
  IN SHELL_FILE_HANDLE  FileHandle
  )
{
  ShellFileHandleRemove (FileHandle);
  return (FileHandleClose (ConvertShellHandleToEfiFileProtocol (FileHandle)));
}

/**
  Internal worker to determine whether there is a BlockIo somewhere
  upon the device path specified.

  @param[in] DevicePath    The device path to test.

  @retval TRUE      gEfiBlockIoProtocolGuid was installed on a handle with this device path
  @retval FALSE     gEfiBlockIoProtocolGuid was not found.
**/
BOOLEAN
InternalShellProtocolIsBlockIoPresent (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathCopy;
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;

  Handle = NULL;

  DevicePathCopy = (EFI_DEVICE_PATH_PROTOCOL *)DevicePath;
  Status         = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &DevicePathCopy, &Handle);

  if ((Handle != NULL) && (!EFI_ERROR (Status))) {
    return (TRUE);
  }

  return (FALSE);
}

/**
  Internal worker to determine whether there is a file system somewhere
  upon the device path specified.

  @param[in] DevicePath    The device path to test.

  @retval TRUE      gEfiSimpleFileSystemProtocolGuid was installed on a handle with this device path
  @retval FALSE     gEfiSimpleFileSystemProtocolGuid was not found.
**/
BOOLEAN
InternalShellProtocolIsSimpleFileSystemPresent (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathCopy;
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;

  Handle = NULL;

  DevicePathCopy = (EFI_DEVICE_PATH_PROTOCOL *)DevicePath;
  Status         = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &DevicePathCopy, &Handle);

  if ((Handle != NULL) && (!EFI_ERROR (Status))) {
    return (TRUE);
  }

  return (FALSE);
}

/**
  This function creates a mapping for a device path.

  If both DevicePath and Mapping are NULL, this will reset the mapping to default values.

  @param DevicePath             Points to the device path. If this is NULL and Mapping points to a valid mapping,
                                then the mapping will be deleted.
  @param Mapping                Points to the NULL-terminated mapping for the device path.  Must end with a ':'

  @retval EFI_SUCCESS           Mapping created or deleted successfully.
  @retval EFI_NO_MAPPING        There is no handle that corresponds exactly to DevicePath. See the
                                boot service function LocateDevicePath().
  @retval EFI_ACCESS_DENIED     The mapping is a built-in alias.
  @retval EFI_INVALID_PARAMETER Mapping was NULL
  @retval EFI_INVALID_PARAMETER Mapping did not end with a ':'
  @retval EFI_INVALID_PARAMETER DevicePath was not pointing at a device that had a SIMPLE_FILE_SYSTEM_PROTOCOL installed.
  @retval EFI_NOT_FOUND         There was no mapping found to delete
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed
**/
EFI_STATUS
EFIAPI
EfiShellSetMap (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath OPTIONAL,
  IN CONST CHAR16                    *Mapping
  )
{
  EFI_STATUS      Status;
  SHELL_MAP_LIST  *MapListNode;

  if (Mapping == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  if (Mapping[StrLen (Mapping)-1] != ':') {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // Delete the mapping
  //
  if (DevicePath == NULL) {
    if (IsListEmpty (&gShellMapList.Link)) {
      return (EFI_NOT_FOUND);
    }

    for ( MapListNode = (SHELL_MAP_LIST *)GetFirstNode (&gShellMapList.Link)
          ; !IsNull (&gShellMapList.Link, &MapListNode->Link)
          ; MapListNode = (SHELL_MAP_LIST *)GetNextNode (&gShellMapList.Link, &MapListNode->Link)
          )
    {
      if (StringNoCaseCompare (&MapListNode->MapName, &Mapping) == 0) {
        RemoveEntryList (&MapListNode->Link);
        SHELL_FREE_NON_NULL (MapListNode->DevicePath);
        SHELL_FREE_NON_NULL (MapListNode->MapName);
        SHELL_FREE_NON_NULL (MapListNode->CurrentDirectoryPath);
        FreePool (MapListNode);
        return (EFI_SUCCESS);
      }
    } // for loop

    //
    // We didn't find one to delete
    //
    return (EFI_NOT_FOUND);
  }

  //
  // make sure this is a valid to add device path
  //
  /// @todo add BlockIo to this test...
  if (  !InternalShellProtocolIsSimpleFileSystemPresent (DevicePath)
     && !InternalShellProtocolIsBlockIoPresent (DevicePath))
  {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // First make sure there is no old mapping
  //
  Status = EfiShellSetMap (NULL, Mapping);
  if ((Status != EFI_SUCCESS) && (Status != EFI_NOT_FOUND)) {
    return (Status);
  }

  //
  // now add the new one.
  //
  Status = ShellCommandAddMapItemAndUpdatePath (Mapping, DevicePath, 0, FALSE);

  return (Status);
}

/**
  Gets the device path from the mapping.

  This function gets the device path associated with a mapping.

  @param Mapping                A pointer to the mapping

  @retval !=NULL                Pointer to the device path that corresponds to the
                                device mapping. The returned pointer does not need
                                to be freed.
  @retval NULL                  There is no device path associated with the
                                specified mapping.
**/
CONST EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
EfiShellGetDevicePathFromMap (
  IN CONST CHAR16  *Mapping
  )
{
  SHELL_MAP_LIST  *MapListItem;
  CHAR16          *NewName;
  UINTN           Size;

  NewName = NULL;
  Size    = 0;

  StrnCatGrow (&NewName, &Size, Mapping, 0);
  if (Mapping[StrLen (Mapping)-1] != L':') {
    StrnCatGrow (&NewName, &Size, L":", 0);
  }

  MapListItem = ShellCommandFindMapItem (NewName);

  FreePool (NewName);

  if (MapListItem != NULL) {
    return (MapListItem->DevicePath);
  }

  return (NULL);
}

/**
  Gets the mapping(s) that most closely matches the device path.

  This function gets the mapping which corresponds to the device path *DevicePath. If
  there is no exact match, then the mapping which most closely matches *DevicePath
  is returned, and *DevicePath is updated to point to the remaining portion of the
  device path. If there is an exact match, the mapping is returned and *DevicePath
  points to the end-of-device-path node.

  If there are multiple map names they will be semi-colon separated in the
  NULL-terminated string.

  @param DevicePath             On entry, points to a device path pointer. On
                                exit, updates the pointer to point to the
                                portion of the device path after the mapping.

  @retval NULL                  No mapping was found.
  @return !=NULL                Pointer to NULL-terminated mapping. The buffer
                                is callee allocated and should be freed by the caller.
**/
CONST CHAR16 *
EFIAPI
EfiShellGetMapFromDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  )
{
  SHELL_MAP_LIST  *Node;
  CHAR16          *PathForReturn;
  UINTN           PathSize;

  //  EFI_HANDLE                  PathHandle;
  //  EFI_HANDLE                  MapHandle;
  //  EFI_STATUS                  Status;
  //  EFI_DEVICE_PATH_PROTOCOL    *DevicePathCopy;
  //  EFI_DEVICE_PATH_PROTOCOL    *MapPathCopy;

  if ((DevicePath == NULL) || (*DevicePath == NULL)) {
    return (NULL);
  }

  PathForReturn = NULL;
  PathSize      = 0;

  for ( Node = (SHELL_MAP_LIST *)GetFirstNode (&gShellMapList.Link)
        ; !IsNull (&gShellMapList.Link, &Node->Link)
        ; Node = (SHELL_MAP_LIST *)GetNextNode (&gShellMapList.Link, &Node->Link)
        )
  {
    //
    // check for exact match
    //
    if (DevicePathCompare (DevicePath, &Node->DevicePath) == 0) {
      ASSERT ((PathForReturn == NULL && PathSize == 0) || (PathForReturn != NULL));
      if (PathSize != 0) {
        PathForReturn = StrnCatGrow (&PathForReturn, &PathSize, L";", 0);
      }

      PathForReturn = StrnCatGrow (&PathForReturn, &PathSize, Node->MapName, 0);
    }
  }

  if (PathForReturn != NULL) {
    while (!IsDevicePathEndType (*DevicePath)) {
      *DevicePath = NextDevicePathNode (*DevicePath);
    }

    SetDevicePathEndNode (*DevicePath);
  }

  /*
    ///@todo finish code for inexact matches.
    if (PathForReturn == NULL) {
      PathSize = 0;

      DevicePathCopy = DuplicateDevicePath(*DevicePath);
      ASSERT(DevicePathCopy != NULL);
      Status = gBS->LocateDevicePath(&gEfiSimpleFileSystemProtocolGuid, &DevicePathCopy, &PathHandle);
      ASSERT_EFI_ERROR(Status);
      //
      //  check each of the device paths we have to get the root of the path for consist mappings
      //
      for ( Node = (SHELL_MAP_LIST *)GetFirstNode(&gShellMapList.Link)
          ; !IsNull(&gShellMapList.Link, &Node->Link)
          ; Node = (SHELL_MAP_LIST *)GetNextNode(&gShellMapList.Link, &Node->Link)
         ){
        if ((Node->Flags & SHELL_MAP_FLAGS_CONSIST) == 0) {
          continue;
        }
        MapPathCopy = DuplicateDevicePath(Node->DevicePath);
        ASSERT(MapPathCopy != NULL);
        Status = gBS->LocateDevicePath(&gEfiSimpleFileSystemProtocolGuid, &MapPathCopy, &MapHandle);
        if (MapHandle == PathHandle) {

          *DevicePath = DevicePathCopy;

          MapPathCopy = NULL;
          DevicePathCopy = NULL;
          PathForReturn = StrnCatGrow(&PathForReturn, &PathSize, Node->MapName, 0);
          PathForReturn = StrnCatGrow(&PathForReturn, &PathSize, L";", 0);
          break;
        }
      }
      //
      // now add on the non-consistent mappings
      //
      for ( Node = (SHELL_MAP_LIST *)GetFirstNode(&gShellMapList.Link)
          ; !IsNull(&gShellMapList.Link, &Node->Link)
          ; Node = (SHELL_MAP_LIST *)GetNextNode(&gShellMapList.Link, &Node->Link)
         ){
        if ((Node->Flags & SHELL_MAP_FLAGS_CONSIST) != 0) {
          continue;
        }
        MapPathCopy = Node->DevicePath;
        ASSERT(MapPathCopy != NULL);
        Status = gBS->LocateDevicePath(&gEfiSimpleFileSystemProtocolGuid, &MapPathCopy, &MapHandle);
        if (MapHandle == PathHandle) {
          PathForReturn = StrnCatGrow(&PathForReturn, &PathSize, Node->MapName, 0);
          PathForReturn = StrnCatGrow(&PathForReturn, &PathSize, L";", 0);
          break;
        }
      }
    }
  */

  return (AddBufferToFreeList (PathForReturn));
}

/**
  Converts a device path to a file system-style path.

  This function converts a device path to a file system path by replacing part, or all, of
  the device path with the file-system mapping. If there are more than one application
  file system mappings, the one that most closely matches Path will be used.

  @param Path                   The pointer to the device path

  @retval NULL                  the device path could not be found.
  @return all                   The pointer of the NULL-terminated file path. The path
                                is callee-allocated and should be freed by the caller.
**/
CHAR16 *
EFIAPI
EfiShellGetFilePathFromDevicePath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *Path
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathCopy;
  EFI_DEVICE_PATH_PROTOCOL  *MapPathCopy;
  SHELL_MAP_LIST            *MapListItem;
  CHAR16                    *PathForReturn;
  UINTN                     PathSize;
  EFI_HANDLE                PathHandle;
  EFI_HANDLE                MapHandle;
  EFI_STATUS                Status;
  FILEPATH_DEVICE_PATH      *FilePath;
  FILEPATH_DEVICE_PATH      *AlignedNode;

  PathForReturn = NULL;
  PathSize      = 0;

  DevicePathCopy = (EFI_DEVICE_PATH_PROTOCOL *)Path;
  ASSERT (DevicePathCopy != NULL);
  if (DevicePathCopy == NULL) {
    return (NULL);
  }

  /// @todo BlockIo?
  Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &DevicePathCopy, &PathHandle);

  if (EFI_ERROR (Status)) {
    return (NULL);
  }

  //
  //  check each of the device paths we have to get the root of the path
  //
  for ( MapListItem = (SHELL_MAP_LIST *)GetFirstNode (&gShellMapList.Link)
        ; !IsNull (&gShellMapList.Link, &MapListItem->Link)
        ; MapListItem = (SHELL_MAP_LIST *)GetNextNode (&gShellMapList.Link, &MapListItem->Link)
        )
  {
    MapPathCopy = (EFI_DEVICE_PATH_PROTOCOL *)MapListItem->DevicePath;
    ASSERT (MapPathCopy != NULL);
    /// @todo BlockIo?
    Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &MapPathCopy, &MapHandle);
    if (MapHandle == PathHandle) {
      ASSERT ((PathForReturn == NULL && PathSize == 0) || (PathForReturn != NULL));
      PathForReturn = StrnCatGrow (&PathForReturn, &PathSize, MapListItem->MapName, 0);
      //
      // go through all the remaining nodes in the device path
      //
      for ( FilePath = (FILEPATH_DEVICE_PATH *)DevicePathCopy
            ; !IsDevicePathEnd (&FilePath->Header)
            ; FilePath = (FILEPATH_DEVICE_PATH *)NextDevicePathNode (&FilePath->Header)
            )
      {
        //
        // If any node is not a file path node, then the conversion can not be completed
        //
        if ((DevicePathType (&FilePath->Header) != MEDIA_DEVICE_PATH) ||
            (DevicePathSubType (&FilePath->Header) != MEDIA_FILEPATH_DP))
        {
          FreePool (PathForReturn);
          return NULL;
        }

        //
        // append the path part onto the filepath.
        //
        ASSERT ((PathForReturn == NULL && PathSize == 0) || (PathForReturn != NULL));

        AlignedNode = AllocateCopyPool (DevicePathNodeLength (FilePath), FilePath);
        if (AlignedNode == NULL) {
          FreePool (PathForReturn);
          return NULL;
        }

        // File Path Device Path Nodes 'can optionally add a "\" separator to
        //  the beginning and/or the end of the Path Name string.'
        // (UEFI Spec 2.4 section 9.3.6.4).
        // If necessary, add a "\", but otherwise don't
        // (This is specified in the above section, and also implied by the
        //  UEFI Shell spec section 3.7)
        if ((PathSize != 0)                        &&
            (PathForReturn != NULL)                &&
            (PathForReturn[PathSize / sizeof (CHAR16) - 1] != L'\\') &&
            (AlignedNode->PathName[0]    != L'\\'))
        {
          PathForReturn = StrnCatGrow (&PathForReturn, &PathSize, L"\\", 1);
        }

        PathForReturn = StrnCatGrow (&PathForReturn, &PathSize, AlignedNode->PathName, 0);
        FreePool (AlignedNode);
      } // for loop of remaining nodes
    }

    if (PathForReturn != NULL) {
      break;
    }
  } // for loop of paths to check

  return (PathForReturn);
}

/**
  Converts a file system style name to a device path.

  This function converts a file system style name to a device path, by replacing any
  mapping references to the associated device path.

  @param[in] Path               The pointer to the path.

  @return                       The pointer of the file path. The file path is callee
                                allocated and should be freed by the caller.
  @retval NULL                  The path could not be found.
  @retval NULL                  There was not enough available memory.
**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
EfiShellGetDevicePathFromFilePath (
  IN CONST CHAR16  *Path
  )
{
  CHAR16                          *MapName;
  CHAR16                          *NewPath;
  CONST CHAR16                    *Cwd;
  UINTN                           Size;
  CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePathCopy;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePathCopyForFree;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePathForReturn;
  EFI_HANDLE                      Handle;
  EFI_STATUS                      Status;

  if (Path == NULL) {
    return (NULL);
  }

  MapName = NULL;
  NewPath = NULL;

  if (StrStr (Path, L":") == NULL) {
    Cwd = EfiShellGetCurDir (NULL);
    if (Cwd == NULL) {
      return (NULL);
    }

    Size    = StrSize (Cwd) + StrSize (Path);
    NewPath = AllocateZeroPool (Size);
    if (NewPath == NULL) {
      return (NULL);
    }

    StrCpyS (NewPath, Size/sizeof (CHAR16), Cwd);
    StrCatS (NewPath, Size/sizeof (CHAR16), L"\\");
    if (*Path == L'\\') {
      Path++;
      while (PathRemoveLastItem (NewPath)) {
      }
    }

    StrCatS (NewPath, Size/sizeof (CHAR16), Path);
    DevicePathForReturn = EfiShellGetDevicePathFromFilePath (NewPath);
    FreePool (NewPath);
    return (DevicePathForReturn);
  }

  Size = 0;
  //
  // find the part before (but including) the : for the map name
  //
  ASSERT ((MapName == NULL && Size == 0) || (MapName != NULL));
  MapName = StrnCatGrow (&MapName, &Size, Path, (StrStr (Path, L":")-Path+1));
  if ((MapName == NULL) || (MapName[StrLen (MapName)-1] != L':')) {
    return (NULL);
  }

  //
  // look up the device path in the map
  //
  DevicePath = EfiShellGetDevicePathFromMap (MapName);
  if (DevicePath == NULL) {
    //
    // Must have been a bad Mapname
    //
    return (NULL);
  }

  //
  // make a copy for LocateDevicePath to modify (also save a pointer to call FreePool with)
  //
  DevicePathCopyForFree = DevicePathCopy = DuplicateDevicePath (DevicePath);
  if (DevicePathCopy == NULL) {
    FreePool (MapName);
    return (NULL);
  }

  //
  // get the handle
  //
  /// @todo BlockIo?
  Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &DevicePathCopy, &Handle);
  if (EFI_ERROR (Status)) {
    if (DevicePathCopyForFree != NULL) {
      FreePool (DevicePathCopyForFree);
    }

    FreePool (MapName);
    return (NULL);
  }

  //
  // build the full device path
  //
  if ((*(Path+StrLen (MapName)) != CHAR_NULL) &&
      (*(Path+StrLen (MapName)+1) == CHAR_NULL))
  {
    DevicePathForReturn = FileDevicePath (Handle, L"\\");
  } else {
    DevicePathForReturn = FileDevicePath (Handle, Path+StrLen (MapName));
  }

  FreePool (MapName);
  if (DevicePathCopyForFree != NULL) {
    FreePool (DevicePathCopyForFree);
  }

  return (DevicePathForReturn);
}

/**
  Gets the name of the device specified by the device handle.

  This function gets the user-readable name of the device specified by the device
  handle. If no user-readable name could be generated, then *BestDeviceName will be
  NULL and EFI_NOT_FOUND will be returned.

  If EFI_DEVICE_NAME_USE_COMPONENT_NAME is set, then the function will return the
  device's name using the EFI_COMPONENT_NAME2_PROTOCOL, if present on
  DeviceHandle.

  If EFI_DEVICE_NAME_USE_DEVICE_PATH is set, then the function will return the
  device's name using the EFI_DEVICE_PATH_PROTOCOL, if present on DeviceHandle.
  If both EFI_DEVICE_NAME_USE_COMPONENT_NAME and
  EFI_DEVICE_NAME_USE_DEVICE_PATH are set, then
  EFI_DEVICE_NAME_USE_COMPONENT_NAME will have higher priority.

  @param DeviceHandle           The handle of the device.
  @param Flags                  Determines the possible sources of component names.
                                Valid bits are:
                                  EFI_DEVICE_NAME_USE_COMPONENT_NAME
                                  EFI_DEVICE_NAME_USE_DEVICE_PATH
  @param Language               A pointer to the language specified for the device
                                name, in the same format as described in the UEFI
                                specification, Appendix M
  @param BestDeviceName         On return, points to the callee-allocated NULL-
                                terminated name of the device. If no device name
                                could be found, points to NULL. The name must be
                                freed by the caller...

  @retval EFI_SUCCESS           Get the name successfully.
  @retval EFI_NOT_FOUND         Fail to get the device name.
  @retval EFI_INVALID_PARAMETER Flags did not have a valid bit set.
  @retval EFI_INVALID_PARAMETER BestDeviceName was NULL
  @retval EFI_INVALID_PARAMETER DeviceHandle was NULL
**/
EFI_STATUS
EFIAPI
EfiShellGetDeviceName (
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_SHELL_DEVICE_NAME_FLAGS  Flags,
  IN CHAR8                        *Language,
  OUT CHAR16                      **BestDeviceName
  )
{
  EFI_STATUS                    Status;
  EFI_COMPONENT_NAME2_PROTOCOL  *CompName2;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_HANDLE                    *HandleList;
  UINTN                         HandleCount;
  UINTN                         LoopVar;
  CHAR16                        *DeviceNameToReturn;
  CHAR8                         *Lang;
  UINTN                         ParentControllerCount;
  EFI_HANDLE                    *ParentControllerBuffer;
  UINTN                         ParentDriverCount;
  EFI_HANDLE                    *ParentDriverBuffer;

  if ((BestDeviceName == NULL) ||
      (DeviceHandle   == NULL)
      )
  {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // make sure one of the 2 supported bits is on
  //
  if (((Flags & EFI_DEVICE_NAME_USE_COMPONENT_NAME) == 0) &&
      ((Flags & EFI_DEVICE_NAME_USE_DEVICE_PATH) == 0))
  {
    return (EFI_INVALID_PARAMETER);
  }

  DeviceNameToReturn = NULL;
  *BestDeviceName    = NULL;
  HandleList         = NULL;
  HandleCount        = 0;
  Lang               = NULL;

  if ((Flags & EFI_DEVICE_NAME_USE_COMPONENT_NAME) != 0) {
    Status = ParseHandleDatabaseByRelationship (
               NULL,
               DeviceHandle,
               HR_DRIVER_BINDING_HANDLE|HR_DEVICE_DRIVER,
               &HandleCount,
               &HandleList
               );
    for (LoopVar = 0; LoopVar < HandleCount; LoopVar++) {
      //
      // Go through those handles until we get one that passes for GetComponentName
      //
      Status = gBS->OpenProtocol (
                      HandleList[LoopVar],
                      &gEfiComponentName2ProtocolGuid,
                      (VOID **)&CompName2,
                      gImageHandle,
                      NULL,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (EFI_ERROR (Status)) {
        Status = gBS->OpenProtocol (
                        HandleList[LoopVar],
                        &gEfiComponentNameProtocolGuid,
                        (VOID **)&CompName2,
                        gImageHandle,
                        NULL,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );
      }

      if (EFI_ERROR (Status)) {
        continue;
      }

      Lang   = GetBestLanguageForDriver (CompName2->SupportedLanguages, Language, FALSE);
      Status = CompName2->GetControllerName (CompName2, DeviceHandle, NULL, Lang, &DeviceNameToReturn);
      FreePool (Lang);
      Lang = NULL;
      if (!EFI_ERROR (Status) && (DeviceNameToReturn != NULL)) {
        break;
      }
    }

    if (HandleList != NULL) {
      FreePool (HandleList);
    }

    //
    // Now check the parent controller using this as the child.
    //
    if (DeviceNameToReturn == NULL) {
      PARSE_HANDLE_DATABASE_PARENTS (DeviceHandle, &ParentControllerCount, &ParentControllerBuffer);
      for (LoopVar = 0; LoopVar < ParentControllerCount; LoopVar++) {
        PARSE_HANDLE_DATABASE_UEFI_DRIVERS (ParentControllerBuffer[LoopVar], &ParentDriverCount, &ParentDriverBuffer);
        for (HandleCount = 0; HandleCount < ParentDriverCount; HandleCount++) {
          //
          // try using that driver's component name with controller and our driver as the child.
          //
          Status = gBS->OpenProtocol (
                          ParentDriverBuffer[HandleCount],
                          &gEfiComponentName2ProtocolGuid,
                          (VOID **)&CompName2,
                          gImageHandle,
                          NULL,
                          EFI_OPEN_PROTOCOL_GET_PROTOCOL
                          );
          if (EFI_ERROR (Status)) {
            Status = gBS->OpenProtocol (
                            ParentDriverBuffer[HandleCount],
                            &gEfiComponentNameProtocolGuid,
                            (VOID **)&CompName2,
                            gImageHandle,
                            NULL,
                            EFI_OPEN_PROTOCOL_GET_PROTOCOL
                            );
          }

          if (EFI_ERROR (Status)) {
            continue;
          }

          Lang   = GetBestLanguageForDriver (CompName2->SupportedLanguages, Language, FALSE);
          Status = CompName2->GetControllerName (CompName2, ParentControllerBuffer[LoopVar], DeviceHandle, Lang, &DeviceNameToReturn);
          FreePool (Lang);
          Lang = NULL;
          if (!EFI_ERROR (Status) && (DeviceNameToReturn != NULL)) {
            break;
          }
        }

        SHELL_FREE_NON_NULL (ParentDriverBuffer);
        if (!EFI_ERROR (Status) && (DeviceNameToReturn != NULL)) {
          break;
        }
      }

      SHELL_FREE_NON_NULL (ParentControllerBuffer);
    }

    //
    // dont return on fail since we will try device path if that bit is on
    //
    if (DeviceNameToReturn != NULL) {
      ASSERT (BestDeviceName != NULL);
      StrnCatGrow (BestDeviceName, NULL, DeviceNameToReturn, 0);
      return (EFI_SUCCESS);
    }
  }

  if ((Flags & EFI_DEVICE_NAME_USE_DEVICE_PATH) != 0) {
    Status = gBS->OpenProtocol (
                    DeviceHandle,
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      //
      // use device path to text on the device path
      //
      *BestDeviceName = ConvertDevicePathToText (DevicePath, TRUE, TRUE);
      return (EFI_SUCCESS);
    }
  }

  //
  // none of the selected bits worked.
  //
  return (EFI_NOT_FOUND);
}

/**
  Opens the root directory of a device on a handle

  This function opens the root directory of a device and returns a file handle to it.

  @param DeviceHandle           The handle of the device that contains the volume.
  @param FileHandle             On exit, points to the file handle corresponding to the root directory on the
                                device.

  @retval EFI_SUCCESS           Root opened successfully.
  @retval EFI_NOT_FOUND         EFI_SIMPLE_FILE_SYSTEM could not be found or the root directory
                                could not be opened.
  @retval EFI_VOLUME_CORRUPTED  The data structures in the volume were corrupted.
  @retval EFI_DEVICE_ERROR      The device had an error.
  @retval Others                Error status returned from EFI_SIMPLE_FILE_SYSTEM_PROTOCOL->OpenVolume().
**/
EFI_STATUS
EFIAPI
EfiShellOpenRootByHandle (
  IN EFI_HANDLE          DeviceHandle,
  OUT SHELL_FILE_HANDLE  *FileHandle
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFileSystem;
  EFI_FILE_PROTOCOL                *RealFileHandle;
  EFI_DEVICE_PATH_PROTOCOL         *DevPath;

  //
  // get the simple file system interface
  //
  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&SimpleFileSystem,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return (EFI_NOT_FOUND);
  }

  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevPath,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return (EFI_NOT_FOUND);
  }

  //
  // Open the root volume now...
  //
  Status = SimpleFileSystem->OpenVolume (SimpleFileSystem, &RealFileHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *FileHandle = ConvertEfiFileProtocolToShellHandle (RealFileHandle, EfiShellGetMapFromDevicePath (&DevPath));
  return (EFI_SUCCESS);
}

/**
  Opens the root directory of a device.

  This function opens the root directory of a device and returns a file handle to it.

  @param DevicePath             Points to the device path corresponding to the device where the
                                EFI_SIMPLE_FILE_SYSTEM_PROTOCOL is installed.
  @param FileHandle             On exit, points to the file handle corresponding to the root directory on the
                                device.

  @retval EFI_SUCCESS           Root opened successfully.
  @retval EFI_NOT_FOUND         EFI_SIMPLE_FILE_SYSTEM could not be found or the root directory
                                could not be opened.
  @retval EFI_VOLUME_CORRUPTED  The data structures in the volume were corrupted.
  @retval EFI_DEVICE_ERROR      The device had an error
  @retval EFI_INVALID_PARAMETER FileHandle is NULL.
**/
EFI_STATUS
EFIAPI
EfiShellOpenRoot (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT SHELL_FILE_HANDLE        *FileHandle
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  if (FileHandle == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // find the handle of the device with that device handle and the file system
  //
  /// @todo BlockIo?
  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  &DevicePath,
                  &Handle
                  );
  if (EFI_ERROR (Status)) {
    return (EFI_NOT_FOUND);
  }

  return (EfiShellOpenRootByHandle (Handle, FileHandle));
}

/**
  Returns whether any script files are currently being processed.

  @retval TRUE                 There is at least one script file active.
  @retval FALSE                No script files are active now.

**/
BOOLEAN
EFIAPI
EfiShellBatchIsActive (
  VOID
  )
{
  if (ShellCommandGetCurrentScriptFile () == NULL) {
    return (FALSE);
  }

  return (TRUE);
}

/**
  Worker function to open a file based on a device path.  this will open the root
  of the volume and then traverse down to the file itself.

  @param DevicePath               Device Path of the file.
  @param FileHandle               Pointer to the file upon a successful return.
  @param OpenMode                 mode to open file in.
  @param Attributes               the File Attributes to use when creating a new file.

  @retval EFI_SUCCESS             the file is open and FileHandle is valid
  @retval EFI_UNSUPPORTED         the device path contained non-path elements
  @retval other                   an error occurred.
**/
EFI_STATUS
InternalOpenFileDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT SHELL_FILE_HANDLE            *FileHandle,
  IN UINT64                        OpenMode,
  IN UINT64                        Attributes OPTIONAL
  )
{
  EFI_STATUS            Status;
  FILEPATH_DEVICE_PATH  *FilePathNode;
  EFI_HANDLE            Handle;
  SHELL_FILE_HANDLE     ShellHandle;
  EFI_FILE_PROTOCOL     *Handle1;
  EFI_FILE_PROTOCOL     *Handle2;
  FILEPATH_DEVICE_PATH  *AlignedNode;

  if (FileHandle == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  *FileHandle  = NULL;
  Handle1      = NULL;
  Handle2      = NULL;
  Handle       = NULL;
  ShellHandle  = NULL;
  FilePathNode = NULL;
  AlignedNode  = NULL;

  Status = EfiShellOpenRoot (DevicePath, &ShellHandle);

  if (!EFI_ERROR (Status)) {
    Handle1 = ConvertShellHandleToEfiFileProtocol (ShellHandle);
    if (Handle1 != NULL) {
      //
      // chop off the beginning part before the file system part...
      //
      /// @todo BlockIo?
      Status = gBS->LocateDevicePath (
                      &gEfiSimpleFileSystemProtocolGuid,
                      &DevicePath,
                      &Handle
                      );
      if (!EFI_ERROR (Status)) {
        //
        // To access as a file system, the file path should only
        // contain file path components.  Follow the file path nodes
        // and find the target file
        //
        for ( FilePathNode = (FILEPATH_DEVICE_PATH *)DevicePath
              ; !IsDevicePathEnd (&FilePathNode->Header)
              ; FilePathNode = (FILEPATH_DEVICE_PATH *)NextDevicePathNode (&FilePathNode->Header)
              )
        {
          SHELL_FREE_NON_NULL (AlignedNode);
          AlignedNode = AllocateCopyPool (DevicePathNodeLength (FilePathNode), FilePathNode);
          //
          // For file system access each node should be a file path component
          //
          if ((DevicePathType (&FilePathNode->Header) != MEDIA_DEVICE_PATH) ||
              (DevicePathSubType (&FilePathNode->Header) != MEDIA_FILEPATH_DP)
              )
          {
            Status = EFI_UNSUPPORTED;
            break;
          }

          //
          // Open this file path node
          //
          Handle2 = Handle1;
          Handle1 = NULL;

          //
          // if this is the last node in the DevicePath always create (if that was requested).
          //
          if (IsDevicePathEnd ((NextDevicePathNode (&FilePathNode->Header)))) {
            Status = Handle2->Open (
                                Handle2,
                                &Handle1,
                                AlignedNode->PathName,
                                OpenMode,
                                Attributes
                                );
          } else {
            //
            //  This is not the last node and we dont want to 'create' existing
            //  directory entries...
            //

            //
            // open without letting it create
            // prevents error on existing files/directories
            //
            Status = Handle2->Open (
                                Handle2,
                                &Handle1,
                                AlignedNode->PathName,
                                OpenMode &~EFI_FILE_MODE_CREATE,
                                Attributes
                                );
            //
            // if above failed now open and create the 'item'
            // if OpenMode EFI_FILE_MODE_CREATE bit was on (but disabled above)
            //
            if ((EFI_ERROR (Status)) && ((OpenMode & EFI_FILE_MODE_CREATE) != 0)) {
              Status = Handle2->Open (
                                  Handle2,
                                  &Handle1,
                                  AlignedNode->PathName,
                                  OpenMode,
                                  Attributes
                                  );
            }
          }

          //
          // Close the last node
          //
          ShellInfoObject.NewEfiShellProtocol->CloseFile (Handle2);

          //
          // If there's been an error, stop
          //
          if (EFI_ERROR (Status)) {
            break;
          }
        } // for loop
      }
    }
  }

  SHELL_FREE_NON_NULL (AlignedNode);
  if (EFI_ERROR (Status)) {
    if (Handle1 != NULL) {
      ShellInfoObject.NewEfiShellProtocol->CloseFile (Handle1);
    }
  } else {
    *FileHandle = ConvertEfiFileProtocolToShellHandle (Handle1, ShellFileHandleGetPath (ShellHandle));
  }

  return (Status);
}

/**
  Creates a file or directory by name.

  This function creates an empty new file or directory with the specified attributes and
  returns the new file's handle. If the file already exists and is read-only, then
  EFI_INVALID_PARAMETER will be returned.

  If the file already existed, it is truncated and its attributes updated. If the file is
  created successfully, the FileHandle is the file's handle, else, the FileHandle is NULL.

  If the file name begins with >v, then the file handle which is returned refers to the
  shell environment variable with the specified name. If the shell environment variable
  already exists and is non-volatile then EFI_INVALID_PARAMETER is returned.

  @param FileName           Pointer to NULL-terminated file path
  @param FileAttribs        The new file's attributes.  the different attributes are
                            described in EFI_FILE_PROTOCOL.Open().
  @param FileHandle         On return, points to the created file handle or directory's handle

  @retval EFI_SUCCESS       The file was opened.  FileHandle points to the new file's handle.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED   could not open the file path
  @retval EFI_NOT_FOUND     the specified file could not be found on the device, or could not
                            file the file system on the device.
  @retval EFI_NO_MEDIA      the device has no medium.
  @retval EFI_MEDIA_CHANGED The device has a different medium in it or the medium is no
                            longer supported.
  @retval EFI_DEVICE_ERROR The device reported an error or can't get the file path according
                            the DirName.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED An attempt was made to create a file, or open a file for write
                            when the media is write-protected.
  @retval EFI_ACCESS_DENIED The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES Not enough resources were available to open the file.
  @retval EFI_VOLUME_FULL   The volume is full.
**/
EFI_STATUS
EFIAPI
EfiShellCreateFile (
  IN CONST CHAR16        *FileName,
  IN UINT64              FileAttribs,
  OUT SHELL_FILE_HANDLE  *FileHandle
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_STATUS                Status;
  BOOLEAN                   Volatile;

  //
  // Is this for an environment variable
  // do we start with >v
  //
  if (StrStr (FileName, L">v") == FileName) {
    Status = IsVolatileEnv (FileName + 2, &Volatile);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (!Volatile) {
      return (EFI_INVALID_PARAMETER);
    }

    *FileHandle = CreateFileInterfaceEnv (FileName+2);
    return (EFI_SUCCESS);
  }

  //
  // We are opening a regular file.
  //
  DevicePath = EfiShellGetDevicePathFromFilePath (FileName);
  if (DevicePath == NULL) {
    return (EFI_NOT_FOUND);
  }

  Status = InternalOpenFileDevicePath (DevicePath, FileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, FileAttribs);
  FreePool (DevicePath);

  return (Status);
}

/**
  Register a GUID and a localized human readable name for it.

  If Guid is not assigned a name, then assign GuidName to Guid.  This list of GUID
  names must be used whenever a shell command outputs GUID information.

  This function is only available when the major and minor versions in the
  EfiShellProtocol are greater than or equal to 2 and 1, respectively.

  @param[in] Guid       A pointer to the GUID being registered.
  @param[in] GuidName   A pointer to the localized name for the GUID being registered.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_INVALID_PARAMETER   Guid was NULL.
  @retval EFI_INVALID_PARAMETER   GuidName was NULL.
  @retval EFI_ACCESS_DENIED       Guid already is assigned a name.
**/
EFI_STATUS
EFIAPI
EfiShellRegisterGuidName (
  IN CONST EFI_GUID  *Guid,
  IN CONST CHAR16    *GuidName
  )
{
  return (AddNewGuidNameMapping (Guid, GuidName, NULL));
}

/**
  Opens a file or a directory by file name.

  This function opens the specified file in the specified OpenMode and returns a file
  handle.
  If the file name begins with >v, then the file handle which is returned refers to the
  shell environment variable with the specified name. If the shell environment variable
  exists, is non-volatile and the OpenMode indicates EFI_FILE_MODE_WRITE, then
  EFI_INVALID_PARAMETER is returned.

  If the file name is >i, then the file handle which is returned refers to the standard
  input. If the OpenMode indicates EFI_FILE_MODE_WRITE, then EFI_INVALID_PARAMETER
  is returned.

  If the file name is >o, then the file handle which is returned refers to the standard
  output. If the OpenMode indicates EFI_FILE_MODE_READ, then EFI_INVALID_PARAMETER
  is returned.

  If the file name is >e, then the file handle which is returned refers to the standard
  error. If the OpenMode indicates EFI_FILE_MODE_READ, then EFI_INVALID_PARAMETER
  is returned.

  If the file name is NUL, then the file handle that is returned refers to the standard NUL
  file. If the OpenMode indicates EFI_FILE_MODE_READ, then EFI_INVALID_PARAMETER is
  returned.

  If return EFI_SUCCESS, the FileHandle is the opened file's handle, else, the
  FileHandle is NULL.

  @param FileName               Points to the NULL-terminated UCS-2 encoded file name.
  @param FileHandle             On return, points to the file handle.
  @param OpenMode               File open mode. Either EFI_FILE_MODE_READ or
                                EFI_FILE_MODE_WRITE from section 12.4 of the UEFI
                                Specification.
  @retval EFI_SUCCESS           The file was opened. FileHandle has the opened file's handle.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value. FileHandle is NULL.
  @retval EFI_UNSUPPORTED       Could not open the file path. FileHandle is NULL.
  @retval EFI_NOT_FOUND         The specified file could not be found on the device or the file
                                system could not be found on the device. FileHandle is NULL.
  @retval EFI_NO_MEDIA          The device has no medium. FileHandle is NULL.
  @retval EFI_MEDIA_CHANGED     The device has a different medium in it or the medium is no
                                longer supported. FileHandle is NULL.
  @retval EFI_DEVICE_ERROR      The device reported an error or can't get the file path according
                                the FileName. FileHandle is NULL.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted. FileHandle is NULL.
  @retval EFI_WRITE_PROTECTED   An attempt was made to create a file, or open a file for write
                                when the media is write-protected. FileHandle is NULL.
  @retval EFI_ACCESS_DENIED     The service denied access to the file. FileHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  Not enough resources were available to open the file. FileHandle
                                is NULL.
  @retval EFI_VOLUME_FULL       The volume is full. FileHandle is NULL.
**/
EFI_STATUS
EFIAPI
EfiShellOpenFileByName (
  IN CONST CHAR16        *FileName,
  OUT SHELL_FILE_HANDLE  *FileHandle,
  IN UINT64              OpenMode
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_STATUS                Status;
  BOOLEAN                   Volatile;

  *FileHandle = NULL;

  //
  // Is this for StdIn
  //
  if (StrCmp (FileName, L">i") == 0) {
    //
    // make sure not writing to StdIn
    //
    if ((OpenMode & EFI_FILE_MODE_WRITE) != 0) {
      return (EFI_INVALID_PARAMETER);
    }

    *FileHandle = ShellInfoObject.NewShellParametersProtocol->StdIn;
    ASSERT (*FileHandle != NULL);
    return (EFI_SUCCESS);
  }

  //
  // Is this for StdOut
  //
  if (StrCmp (FileName, L">o") == 0) {
    //
    // make sure not writing to StdIn
    //
    if ((OpenMode & EFI_FILE_MODE_READ) != 0) {
      return (EFI_INVALID_PARAMETER);
    }

    *FileHandle = &FileInterfaceStdOut;
    return (EFI_SUCCESS);
  }

  //
  // Is this for NUL / NULL file
  //
  if ((gUnicodeCollation->StriColl (gUnicodeCollation, (CHAR16 *)FileName, L"NUL") == 0) ||
      (gUnicodeCollation->StriColl (gUnicodeCollation, (CHAR16 *)FileName, L"NULL") == 0))
  {
    *FileHandle = &FileInterfaceNulFile;
    return (EFI_SUCCESS);
  }

  //
  // Is this for StdErr
  //
  if (StrCmp (FileName, L">e") == 0) {
    //
    // make sure not writing to StdIn
    //
    if ((OpenMode & EFI_FILE_MODE_READ) != 0) {
      return (EFI_INVALID_PARAMETER);
    }

    *FileHandle = &FileInterfaceStdErr;
    return (EFI_SUCCESS);
  }

  //
  // Is this for an environment variable
  // do we start with >v
  //
  if (StrStr (FileName, L">v") == FileName) {
    Status = IsVolatileEnv (FileName + 2, &Volatile);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (!Volatile &&
        ((OpenMode & EFI_FILE_MODE_WRITE) != 0))
    {
      return (EFI_INVALID_PARAMETER);
    }

    *FileHandle = CreateFileInterfaceEnv (FileName+2);
    return (EFI_SUCCESS);
  }

  //
  // We are opening a regular file.
  //
  DevicePath = EfiShellGetDevicePathFromFilePath (FileName);

  if (DevicePath == NULL) {
    return (EFI_NOT_FOUND);
  }

  //
  // Copy the device path, open the file, then free the memory
  //
  Status = InternalOpenFileDevicePath (DevicePath, FileHandle, OpenMode, 0); // 0 = no specific file attributes
  FreePool (DevicePath);

  return (Status);
}

/**
  Deletes the file specified by the file name.

  This function deletes a file.

  @param FileName                 Points to the NULL-terminated file name.

  @retval EFI_SUCCESS             The file was closed and deleted, and the handle was closed.
  @retval EFI_WARN_DELETE_FAILURE The handle was closed but the file was not deleted.
  @sa EfiShellCreateFile
**/
EFI_STATUS
EFIAPI
EfiShellDeleteFileByName (
  IN CONST CHAR16  *FileName
  )
{
  SHELL_FILE_HANDLE  FileHandle;
  EFI_STATUS         Status;

  FileHandle = NULL;

  //
  // get a handle to the file
  //
  Status = EfiShellCreateFile (
             FileName,
             0,
             &FileHandle
             );
  if (EFI_ERROR (Status)) {
    return (Status);
  }

  //
  // now delete the file
  //
  ShellFileHandleRemove (FileHandle);
  return (ShellInfoObject.NewEfiShellProtocol->DeleteFile (FileHandle));
}

/**
  Disables the page break output mode.
**/
VOID
EFIAPI
EfiShellDisablePageBreak (
  VOID
  )
{
  ShellInfoObject.PageBreakEnabled = FALSE;
}

/**
  Enables the page break output mode.
**/
VOID
EFIAPI
EfiShellEnablePageBreak (
  VOID
  )
{
  ShellInfoObject.PageBreakEnabled = TRUE;
}

/**
  internal worker function to load and run an image via device path.

  @param ParentImageHandle      A handle of the image that is executing the specified
                                command line.
  @param DevicePath             device path of the file to execute
  @param CommandLine            Points to the NULL-terminated UCS-2 encoded string
                                containing the command line. If NULL then the command-
                                line will be empty.
  @param Environment            Points to a NULL-terminated array of environment
                                variables with the format 'x=y', where x is the
                                environment variable name and y is the value. If this
                                is NULL, then the current shell environment is used.

  @param[out] StartImageStatus  Returned status from gBS->StartImage.

  @retval EFI_SUCCESS       The command executed successfully. The  status code
                            returned by the command is pointed to by StatusCode.
  @retval EFI_INVALID_PARAMETER The parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES Out of resources.
  @retval EFI_UNSUPPORTED   Nested shell invocations are not allowed.
**/
EFI_STATUS
InternalShellExecuteDevicePath (
  IN CONST EFI_HANDLE                *ParentImageHandle,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN CONST CHAR16                    *CommandLine OPTIONAL,
  IN CONST CHAR16                    **Environment OPTIONAL,
  OUT EFI_STATUS                     *StartImageStatus OPTIONAL
  )
{
  EFI_STATUS                     Status;
  EFI_STATUS                     StartStatus;
  EFI_STATUS                     CleanupStatus;
  EFI_HANDLE                     NewHandle;
  EFI_LOADED_IMAGE_PROTOCOL      *LoadedImage;
  LIST_ENTRY                     OrigEnvs;
  EFI_SHELL_PARAMETERS_PROTOCOL  ShellParamsProtocol;
  CHAR16                         *ImagePath;
  UINTN                          Index;
  CHAR16                         *Walker;
  CHAR16                         *NewCmdLine;

  if (ParentImageHandle == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  InitializeListHead (&OrigEnvs);
  ZeroMem (&ShellParamsProtocol, sizeof (EFI_SHELL_PARAMETERS_PROTOCOL));

  NewHandle = NULL;

  NewCmdLine = AllocateCopyPool (StrSize (CommandLine), CommandLine);
  if (NewCmdLine == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Walker = NewCmdLine; Walker != NULL && *Walker != CHAR_NULL; Walker++) {
    if ((*Walker == L'^') && (*(Walker+1) == L'#')) {
      CopyMem (Walker, Walker+1, StrSize (Walker) - sizeof (Walker[0]));
    }
  }

  //
  // Load the image with:
  // FALSE - not from boot manager and NULL, 0 being not already in memory
  //
  Status = gBS->LoadImage (
                  FALSE,
                  *ParentImageHandle,
                  (EFI_DEVICE_PATH_PROTOCOL *)DevicePath,
                  NULL,
                  0,
                  &NewHandle
                  );

  if (EFI_ERROR (Status)) {
    if (NewHandle != NULL) {
      gBS->UnloadImage (NewHandle);
    }

    FreePool (NewCmdLine);
    return (Status);
  }

  Status = gBS->OpenProtocol (
                  NewHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    //
    // If the image is not an app abort it.
    //
    if (LoadedImage->ImageCodeType != EfiLoaderCode) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_SHELL_IMAGE_NOT_APP),
        ShellInfoObject.HiiHandle
        );
      goto UnloadImage;
    }

    ASSERT (LoadedImage->LoadOptionsSize == 0);
    if (NewCmdLine != NULL) {
      LoadedImage->LoadOptionsSize = (UINT32)StrSize (NewCmdLine);
      LoadedImage->LoadOptions     = (VOID *)NewCmdLine;
    }

    //
    // Save our current environment settings for later restoration if necessary
    //
    if (Environment != NULL) {
      Status = GetEnvironmentVariableList (&OrigEnvs);
      if (!EFI_ERROR (Status)) {
        Status = SetEnvironmentVariables (Environment);
      }
    }

    //
    // Initialize and install a shell parameters protocol on the image.
    //
    ShellParamsProtocol.StdIn  = ShellInfoObject.NewShellParametersProtocol->StdIn;
    ShellParamsProtocol.StdOut = ShellInfoObject.NewShellParametersProtocol->StdOut;
    ShellParamsProtocol.StdErr = ShellInfoObject.NewShellParametersProtocol->StdErr;
    Status                     = UpdateArgcArgv (&ShellParamsProtocol, NewCmdLine, Efi_Application, NULL, NULL);
    if (EFI_ERROR (Status)) {
      goto UnloadImage;
    }

    //
    // Replace Argv[0] with the full path of the binary we're executing:
    // If the command line was "foo", the binary might be called "foo.efi".
    // "The first entry in [Argv] is always the full file path of the
    //  executable" - UEFI Shell Spec section 2.3
    //
    ImagePath = EfiShellGetFilePathFromDevicePath (DevicePath);
    // The image we're executing isn't necessarily in a filesystem - it might
    // be memory mapped. In this case EfiShellGetFilePathFromDevicePath will
    // return NULL, and we'll leave Argv[0] as UpdateArgcArgv set it.
    if (ImagePath != NULL) {
      if (ShellParamsProtocol.Argv == NULL) {
        // Command line was empty or null.
        // (UpdateArgcArgv sets Argv to NULL when CommandLine is "" or NULL)
        ShellParamsProtocol.Argv = AllocatePool (sizeof (CHAR16 *));
        if (ShellParamsProtocol.Argv == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto UnloadImage;
        }

        ShellParamsProtocol.Argc = 1;
      } else {
        // Free the string UpdateArgcArgv put in Argv[0];
        FreePool (ShellParamsProtocol.Argv[0]);
      }

      ShellParamsProtocol.Argv[0] = ImagePath;
    }

    Status = gBS->InstallProtocolInterface (&NewHandle, &gEfiShellParametersProtocolGuid, EFI_NATIVE_INTERFACE, &ShellParamsProtocol);
    ASSERT_EFI_ERROR (Status);

    /// @todo initialize and install ShellInterface protocol on the new image for compatibility if - PcdGetBool(PcdShellSupportOldProtocols)

    //
    // now start the image and if the caller wanted the return code pass it to them...
    //
    if (!EFI_ERROR (Status)) {
      StartStatus = gBS->StartImage (
                           NewHandle,
                           0,
                           NULL
                           );
      if (StartImageStatus != NULL) {
        *StartImageStatus = StartStatus;
      }

      CleanupStatus = gBS->UninstallProtocolInterface (
                             NewHandle,
                             &gEfiShellParametersProtocolGuid,
                             &ShellParamsProtocol
                             );
      ASSERT_EFI_ERROR (CleanupStatus);

      goto FreeAlloc;
    }

UnloadImage:
    // Unload image - We should only get here if we didn't call StartImage
    gBS->UnloadImage (NewHandle);

FreeAlloc:
    // Free Argv (Allocated in UpdateArgcArgv)
    if (ShellParamsProtocol.Argv != NULL) {
      for (Index = 0; Index < ShellParamsProtocol.Argc; Index++) {
        if (ShellParamsProtocol.Argv[Index] != NULL) {
          FreePool (ShellParamsProtocol.Argv[Index]);
        }
      }

      FreePool (ShellParamsProtocol.Argv);
    }
  }

  // Restore environment variables
  if (!IsListEmpty (&OrigEnvs)) {
    CleanupStatus = SetEnvironmentVariableList (&OrigEnvs);
    ASSERT_EFI_ERROR (CleanupStatus);
  }

  FreePool (NewCmdLine);

  return (Status);
}

/**
  internal worker function to load and run an image in the current shell.

  @param CommandLine            Points to the NULL-terminated UCS-2 encoded string
                                containing the command line. If NULL then the command-
                                line will be empty.
  @param Environment            Points to a NULL-terminated array of environment
                                variables with the format 'x=y', where x is the
                                environment variable name and y is the value. If this
                                is NULL, then the current shell environment is used.

  @param[out] StartImageStatus  Returned status from the command line.

  @retval EFI_SUCCESS       The command executed successfully. The  status code
                            returned by the command is pointed to by StatusCode.
  @retval EFI_INVALID_PARAMETER The parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES Out of resources.
  @retval EFI_UNSUPPORTED   Nested shell invocations are not allowed.
**/
EFI_STATUS
InternalShellExecute (
  IN CONST CHAR16  *CommandLine OPTIONAL,
  IN CONST CHAR16  **Environment OPTIONAL,
  OUT EFI_STATUS   *StartImageStatus OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  CleanupStatus;
  LIST_ENTRY  OrigEnvs;

  InitializeListHead (&OrigEnvs);

  //
  // Save our current environment settings for later restoration if necessary
  //
  if (Environment != NULL) {
    Status = GetEnvironmentVariableList (&OrigEnvs);
    if (!EFI_ERROR (Status)) {
      Status = SetEnvironmentVariables (Environment);
    } else {
      return Status;
    }
  }

  Status = RunShellCommand (CommandLine, StartImageStatus);

  // Restore environment variables
  if (!IsListEmpty (&OrigEnvs)) {
    CleanupStatus = SetEnvironmentVariableList (&OrigEnvs);
    ASSERT_EFI_ERROR (CleanupStatus);
  }

  return (Status);
}

/**
  Determine if the UEFI Shell is currently running with nesting enabled or disabled.

  @retval FALSE   nesting is required
  @retval other   nesting is enabled
**/
STATIC
BOOLEAN
NestingEnabled (
  VOID
  )
{
  EFI_STATUS  Status;
  CHAR16      *Temp;
  CHAR16      *Temp2;
  UINTN       TempSize;
  BOOLEAN     RetVal;

  RetVal = TRUE;
  Temp   = NULL;
  Temp2  = NULL;

  if (ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoNest) {
    TempSize = 0;
    Temp     = NULL;
    Status   = SHELL_GET_ENVIRONMENT_VARIABLE (mNoNestingEnvVarName, &TempSize, Temp);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Temp = AllocateZeroPool (TempSize + sizeof (CHAR16));
      if (Temp != NULL) {
        Status = SHELL_GET_ENVIRONMENT_VARIABLE (mNoNestingEnvVarName, &TempSize, Temp);
      }
    }

    Temp2 = StrnCatGrow (&Temp2, NULL, mNoNestingTrue, 0);
    if ((Temp != NULL) && (Temp2 != NULL) && (StringNoCaseCompare (&Temp, &Temp2) == 0)) {
      //
      // Use the no nesting method.
      //
      RetVal = FALSE;
    }
  }

  SHELL_FREE_NON_NULL (Temp);
  SHELL_FREE_NON_NULL (Temp2);
  return (RetVal);
}

/**
  Execute the command line.

  This function creates a nested instance of the shell and executes the specified
  command (CommandLine) with the specified environment (Environment). Upon return,
  the status code returned by the specified command is placed in StatusCode.

  If Environment is NULL, then the current environment is used and all changes made
  by the commands executed will be reflected in the current environment. If the
  Environment is non-NULL, then the changes made will be discarded.

  The CommandLine is executed from the current working directory on the current
  device.

  @param ParentImageHandle  A handle of the image that is executing the specified
                            command line.
  @param CommandLine        Points to the NULL-terminated UCS-2 encoded string
                            containing the command line. If NULL then the command-
                            line will be empty.
  @param Environment        Points to a NULL-terminated array of environment
                            variables with the format 'x=y', where x is the
                            environment variable name and y is the value. If this
                            is NULL, then the current shell environment is used.
  @param StatusCode         Points to the status code returned by the CommandLine.

  @retval EFI_SUCCESS       The command executed successfully. The  status code
                            returned by the command is pointed to by StatusCode.
  @retval EFI_INVALID_PARAMETER The parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES Out of resources.
  @retval EFI_UNSUPPORTED   Nested shell invocations are not allowed.
  @retval EFI_UNSUPPORTED   The support level required for this function is not present.

  @sa InternalShellExecuteDevicePath
**/
EFI_STATUS
EFIAPI
EfiShellExecute (
  IN EFI_HANDLE   *ParentImageHandle,
  IN CHAR16       *CommandLine OPTIONAL,
  IN CHAR16       **Environment OPTIONAL,
  OUT EFI_STATUS  *StatusCode OPTIONAL
  )
{
  EFI_STATUS                Status;
  CHAR16                    *Temp;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  UINTN                     Size;

  if ((PcdGet8 (PcdShellSupportLevel) < 1)) {
    return (EFI_UNSUPPORTED);
  }

  if (NestingEnabled ()) {
    DevPath = AppendDevicePath (ShellInfoObject.ImageDevPath, ShellInfoObject.FileDevPath);

    DEBUG_CODE_BEGIN ();
    Temp = ConvertDevicePathToText (ShellInfoObject.FileDevPath, TRUE, TRUE);
    FreePool (Temp);
    Temp = ConvertDevicePathToText (ShellInfoObject.ImageDevPath, TRUE, TRUE);
    FreePool (Temp);
    Temp = ConvertDevicePathToText (DevPath, TRUE, TRUE);
    FreePool (Temp);
    DEBUG_CODE_END ();

    Temp = NULL;
    Size = 0;
    ASSERT ((Temp == NULL && Size == 0) || (Temp != NULL));
    StrnCatGrow (&Temp, &Size, L"Shell.efi -exit ", 0);
    StrnCatGrow (&Temp, &Size, CommandLine, 0);

    Status = InternalShellExecuteDevicePath (
               ParentImageHandle,
               DevPath,
               Temp,
               (CONST CHAR16 **)Environment,
               StatusCode
               );

    //
    // de-allocate and return
    //
    FreePool (DevPath);
    FreePool (Temp);
  } else {
    Status = InternalShellExecute (
               (CONST CHAR16 *)CommandLine,
               (CONST CHAR16 **)Environment,
               StatusCode
               );
  }

  return (Status);
}

/**
  Utility cleanup function for EFI_SHELL_FILE_INFO objects.

  1) frees all pointers (non-NULL)
  2) Closes the SHELL_FILE_HANDLE

  @param FileListNode     pointer to the list node to free
**/
VOID
InternalFreeShellFileInfoNode (
  IN EFI_SHELL_FILE_INFO  *FileListNode
  )
{
  if (FileListNode->Info != NULL) {
    FreePool ((VOID *)FileListNode->Info);
  }

  if (FileListNode->FileName != NULL) {
    FreePool ((VOID *)FileListNode->FileName);
  }

  if (FileListNode->FullName != NULL) {
    FreePool ((VOID *)FileListNode->FullName);
  }

  if (FileListNode->Handle != NULL) {
    ShellInfoObject.NewEfiShellProtocol->CloseFile (FileListNode->Handle);
  }

  FreePool (FileListNode);
}

/**
  Frees the file list.

  This function cleans up the file list and any related data structures. It has no
  impact on the files themselves.

  @param FileList               The file list to free. Type EFI_SHELL_FILE_INFO is
                                defined in OpenFileList()

  @retval EFI_SUCCESS           Free the file list successfully.
  @retval EFI_INVALID_PARAMETER FileList was NULL or *FileList was NULL;
**/
EFI_STATUS
EFIAPI
EfiShellFreeFileList (
  IN EFI_SHELL_FILE_INFO  **FileList
  )
{
  EFI_SHELL_FILE_INFO  *ShellFileListItem;

  if ((FileList == NULL) || (*FileList == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  for ( ShellFileListItem = (EFI_SHELL_FILE_INFO *)GetFirstNode (&(*FileList)->Link)
        ; !IsListEmpty (&(*FileList)->Link)
        ; ShellFileListItem = (EFI_SHELL_FILE_INFO *)GetFirstNode (&(*FileList)->Link)
        )
  {
    RemoveEntryList (&ShellFileListItem->Link);
    InternalFreeShellFileInfoNode (ShellFileListItem);
  }

  InternalFreeShellFileInfoNode (*FileList);
  *FileList = NULL;
  return (EFI_SUCCESS);
}

/**
  Deletes the duplicate file names files in the given file list.

  This function deletes the reduplicate files in the given file list.

  @param FileList               A pointer to the first entry in the file list.

  @retval EFI_SUCCESS           Always success.
  @retval EFI_INVALID_PARAMETER FileList was NULL or *FileList was NULL;
**/
EFI_STATUS
EFIAPI
EfiShellRemoveDupInFileList (
  IN EFI_SHELL_FILE_INFO  **FileList
  )
{
  EFI_STATUS           Status;
  EFI_SHELL_FILE_INFO  *Duplicates;
  EFI_SHELL_FILE_INFO  *ShellFileListItem;
  EFI_SHELL_FILE_INFO  *ShellFileListItem2;
  EFI_SHELL_FILE_INFO  *TempNode;

  if ((FileList == NULL) || (*FileList == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  Status = ShellSortFileList (
             FileList,
             &Duplicates,
             ShellSortFileListByFullName
             );
  if (!EFI_ERROR (Status)) {
    EfiShellFreeFileList (&Duplicates);
    return EFI_SUCCESS;
  }

  //
  // Fall back to the slow method that needs no extra memory, and so cannot
  // fail.
  //
  for ( ShellFileListItem = (EFI_SHELL_FILE_INFO *)GetFirstNode (&(*FileList)->Link)
        ; !IsNull (&(*FileList)->Link, &ShellFileListItem->Link)
        ; ShellFileListItem = (EFI_SHELL_FILE_INFO *)GetNextNode (&(*FileList)->Link, &ShellFileListItem->Link)
        )
  {
    for ( ShellFileListItem2 = (EFI_SHELL_FILE_INFO *)GetNextNode (&(*FileList)->Link, &ShellFileListItem->Link)
          ; !IsNull (&(*FileList)->Link, &ShellFileListItem2->Link)
          ; ShellFileListItem2 = (EFI_SHELL_FILE_INFO *)GetNextNode (&(*FileList)->Link, &ShellFileListItem2->Link)
          )
    {
      if (gUnicodeCollation->StriColl (
                               gUnicodeCollation,
                               (CHAR16 *)ShellFileListItem->FullName,
                               (CHAR16 *)ShellFileListItem2->FullName
                               ) == 0
          )
      {
        TempNode = (EFI_SHELL_FILE_INFO *)GetPreviousNode (
                                            &(*FileList)->Link,
                                            &ShellFileListItem2->Link
                                            );
        RemoveEntryList (&ShellFileListItem2->Link);
        InternalFreeShellFileInfoNode (ShellFileListItem2);
        // Set ShellFileListItem2 to PreviousNode so we don't access Freed
        // memory in GetNextNode in the loop expression above.
        ShellFileListItem2 = TempNode;
      }
    }
  }

  return (EFI_SUCCESS);
}

//
// This is the same structure as the external version, but it has no CONST qualifiers.
//
typedef struct {
  LIST_ENTRY           Link;      ///< Linked list members.
  EFI_STATUS           Status;    ///< Status of opening the file.  Valid only if Handle != NULL.
  CHAR16               *FullName; ///< Fully qualified filename.
  CHAR16               *FileName; ///< name of this file.
  SHELL_FILE_HANDLE    Handle;    ///< Handle for interacting with the opened file or NULL if closed.
  EFI_FILE_INFO        *Info;     ///< Pointer to the FileInfo struct for this file or NULL.
} EFI_SHELL_FILE_INFO_NO_CONST;

/**
  Allocates and duplicates a EFI_SHELL_FILE_INFO node.

  @param[in] Node     The node to copy from.
  @param[in] Save     TRUE to set Node->Handle to NULL, FALSE otherwise.

  @retval NULL        a memory allocation error occurred
  @return != NULL     a pointer to the new node
**/
EFI_SHELL_FILE_INFO *
InternalDuplicateShellFileInfo (
  IN       EFI_SHELL_FILE_INFO  *Node,
  IN BOOLEAN                    Save
  )
{
  EFI_SHELL_FILE_INFO_NO_CONST  *NewNode;

  //
  // try to confirm that the objects are in sync
  //
  ASSERT (sizeof (EFI_SHELL_FILE_INFO_NO_CONST) == sizeof (EFI_SHELL_FILE_INFO));

  NewNode = AllocateZeroPool (sizeof (EFI_SHELL_FILE_INFO));
  if (NewNode == NULL) {
    return (NULL);
  }

  NewNode->FullName = AllocateCopyPool (StrSize (Node->FullName), Node->FullName);
  NewNode->FileName = AllocateCopyPool (StrSize (Node->FileName), Node->FileName);
  NewNode->Info     = AllocateCopyPool ((UINTN)Node->Info->Size, Node->Info);
  if (  (NewNode->FullName == NULL)
     || (NewNode->FileName == NULL)
     || (NewNode->Info == NULL)
        )
  {
    SHELL_FREE_NON_NULL (NewNode->FullName);
    SHELL_FREE_NON_NULL (NewNode->FileName);
    SHELL_FREE_NON_NULL (NewNode->Info);
    SHELL_FREE_NON_NULL (NewNode);
    return (NULL);
  }

  NewNode->Status = Node->Status;
  NewNode->Handle = Node->Handle;
  if (!Save) {
    Node->Handle = NULL;
  }

  return ((EFI_SHELL_FILE_INFO *)NewNode);
}

/**
  Allocates and populates a EFI_SHELL_FILE_INFO structure.  if any memory operation
  failed it will return NULL.

  @param[in] BasePath         the Path to prepend onto filename for FullPath
  @param[in] Status           Status member initial value.
  @param[in] FileName         FileName member initial value.
  @param[in] Handle           Handle member initial value.
  @param[in] Info             Info struct to copy.

  @retval NULL                An error occurred.
  @return                     a pointer to the newly allocated structure.
**/
EFI_SHELL_FILE_INFO *
CreateAndPopulateShellFileInfo (
  IN CONST CHAR16             *BasePath,
  IN CONST EFI_STATUS         Status,
  IN CONST CHAR16             *FileName,
  IN CONST SHELL_FILE_HANDLE  Handle,
  IN CONST EFI_FILE_INFO      *Info
  )
{
  EFI_SHELL_FILE_INFO  *ShellFileListItem;
  CHAR16               *TempString;
  UINTN                Size;

  TempString = NULL;
  Size       = 0;

  ShellFileListItem = AllocateZeroPool (sizeof (EFI_SHELL_FILE_INFO));
  if (ShellFileListItem == NULL) {
    return (NULL);
  }

  if ((Info != NULL) && (Info->Size != 0)) {
    ShellFileListItem->Info = AllocateZeroPool ((UINTN)Info->Size);
    if (ShellFileListItem->Info == NULL) {
      FreePool (ShellFileListItem);
      return (NULL);
    }

    CopyMem (ShellFileListItem->Info, Info, (UINTN)Info->Size);
  } else {
    ShellFileListItem->Info = NULL;
  }

  if (FileName != NULL) {
    ASSERT (TempString == NULL);
    ShellFileListItem->FileName = StrnCatGrow (&TempString, 0, FileName, 0);
    if (ShellFileListItem->FileName == NULL) {
      FreePool (ShellFileListItem->Info);
      FreePool (ShellFileListItem);
      return (NULL);
    }
  } else {
    ShellFileListItem->FileName = NULL;
  }

  Size       = 0;
  TempString = NULL;
  if (BasePath != NULL) {
    ASSERT ((TempString == NULL && Size == 0) || (TempString != NULL));
    TempString = StrnCatGrow (&TempString, &Size, BasePath, 0);
    if (TempString == NULL) {
      FreePool ((VOID *)ShellFileListItem->FileName);
      SHELL_FREE_NON_NULL (ShellFileListItem->Info);
      FreePool (ShellFileListItem);
      return (NULL);
    }
  }

  if (ShellFileListItem->FileName != NULL) {
    ASSERT ((TempString == NULL && Size == 0) || (TempString != NULL));
    TempString = StrnCatGrow (&TempString, &Size, ShellFileListItem->FileName, 0);
    if (TempString == NULL) {
      FreePool ((VOID *)ShellFileListItem->FileName);
      FreePool (ShellFileListItem->Info);
      FreePool (ShellFileListItem);
      return (NULL);
    }
  }

  TempString = PathCleanUpDirectories (TempString);

  ShellFileListItem->FullName = TempString;
  ShellFileListItem->Status   = Status;
  ShellFileListItem->Handle   = Handle;

  return (ShellFileListItem);
}

/**
  Find all files in a specified directory.

  @param FileDirHandle          Handle of the directory to search.
  @param FileList               On return, points to the list of files in the directory
                                or NULL if there are no files in the directory.

  @retval EFI_SUCCESS           File information was returned successfully.
  @retval EFI_VOLUME_CORRUPTED  The file system structures have been corrupted.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_NO_MEDIA          The device media is not present.
  @retval EFI_INVALID_PARAMETER The FileDirHandle was not a directory.
  @return                       An error from FileHandleGetFileName().
**/
EFI_STATUS
EFIAPI
EfiShellFindFilesInDir (
  IN SHELL_FILE_HANDLE     FileDirHandle,
  OUT EFI_SHELL_FILE_INFO  **FileList
  )
{
  EFI_SHELL_FILE_INFO  *ShellFileList;
  EFI_SHELL_FILE_INFO  *ShellFileListItem;
  EFI_FILE_INFO        *FileInfo;
  EFI_STATUS           Status;
  BOOLEAN              NoFile;
  CHAR16               *TempString;
  CHAR16               *BasePath;
  UINTN                Size;
  CHAR16               *TempSpot;

  BasePath = NULL;
  Status   = FileHandleGetFileName (FileDirHandle, &BasePath);
  if (EFI_ERROR (Status)) {
    return (Status);
  }

  if (ShellFileHandleGetPath (FileDirHandle) != NULL) {
    TempString = NULL;
    Size       = 0;
    TempString = StrnCatGrow (&TempString, &Size, ShellFileHandleGetPath (FileDirHandle), 0);
    if (TempString == NULL) {
      SHELL_FREE_NON_NULL (BasePath);
      return (EFI_OUT_OF_RESOURCES);
    }

    TempSpot = StrStr (TempString, L";");

    if (TempSpot != NULL) {
      *TempSpot = CHAR_NULL;
    }

    TempString = StrnCatGrow (&TempString, &Size, BasePath, 0);
    if (TempString == NULL) {
      SHELL_FREE_NON_NULL (BasePath);
      return (EFI_OUT_OF_RESOURCES);
    }

    SHELL_FREE_NON_NULL (BasePath);
    BasePath = TempString;
  }

  NoFile            = FALSE;
  ShellFileList     = NULL;
  ShellFileListItem = NULL;
  FileInfo          = NULL;
  Status            = EFI_SUCCESS;

  for ( Status = FileHandleFindFirstFile (FileDirHandle, &FileInfo)
        ; !EFI_ERROR (Status) && !NoFile
        ; Status = FileHandleFindNextFile (FileDirHandle, FileInfo, &NoFile)
        )
  {
    if (ShellFileList == NULL) {
      ShellFileList = (EFI_SHELL_FILE_INFO *)AllocateZeroPool (sizeof (EFI_SHELL_FILE_INFO));
      if (ShellFileList == NULL) {
        SHELL_FREE_NON_NULL (BasePath);
        return EFI_OUT_OF_RESOURCES;
      }

      InitializeListHead (&ShellFileList->Link);
    }

    //
    // allocate a new EFI_SHELL_FILE_INFO and populate it...
    //
    ShellFileListItem = CreateAndPopulateShellFileInfo (
                          BasePath,
                          EFI_SUCCESS, // success since we didn't fail to open it...
                          FileInfo->FileName,
                          NULL, // no handle since not open
                          FileInfo
                          );
    if (ShellFileListItem == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      //
      // Free resources outside the loop.
      //
      break;
    }

    InsertTailList (&ShellFileList->Link, &ShellFileListItem->Link);
  }

  if (EFI_ERROR (Status)) {
    EfiShellFreeFileList (&ShellFileList);
    *FileList = NULL;
  } else {
    *FileList = ShellFileList;
  }

  SHELL_FREE_NON_NULL (BasePath);
  return (Status);
}

/**
  Get the GUID value from a human readable name.

  If GuidName is a known GUID name, then update Guid to have the correct value for
  that GUID.

  This function is only available when the major and minor versions in the
  EfiShellProtocol are greater than or equal to 2 and 1, respectively.

  @param[in]  GuidName   A pointer to the localized name for the GUID being queried.
  @param[out] Guid       A pointer to the GUID structure to be filled in.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_INVALID_PARAMETER   Guid was NULL.
  @retval EFI_INVALID_PARAMETER   GuidName was NULL.
  @retval EFI_NOT_FOUND           GuidName is not a known GUID Name.
**/
EFI_STATUS
EFIAPI
EfiShellGetGuidFromName (
  IN  CONST CHAR16    *GuidName,
  OUT       EFI_GUID  *Guid
  )
{
  EFI_GUID    *NewGuid;
  EFI_STATUS  Status;

  if ((Guid == NULL) || (GuidName == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  Status = GetGuidFromStringName (GuidName, NULL, &NewGuid);

  if (!EFI_ERROR (Status)) {
    CopyGuid (Guid, NewGuid);
  }

  return (Status);
}

/**
  Get the human readable name for a GUID from the value.

  If Guid is assigned a name, then update *GuidName to point to the name. The callee
  should not modify the value.

  This function is only available when the major and minor versions in the
  EfiShellProtocol are greater than or equal to 2 and 1, respectively.

  @param[in]  Guid       A pointer to the GUID being queried.
  @param[out] GuidName   A pointer to a pointer the localized to name for the GUID being requested

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_INVALID_PARAMETER   Guid was NULL.
  @retval EFI_INVALID_PARAMETER   GuidName was NULL.
  @retval EFI_NOT_FOUND           Guid is not assigned a name.
**/
EFI_STATUS
EFIAPI
EfiShellGetGuidName (
  IN  CONST EFI_GUID  *Guid,
  OUT CONST CHAR16    **GuidName
  )
{
  CHAR16  *Name;

  if ((Guid == NULL) || (GuidName == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  Name = GetStringNameFromGuid (Guid, NULL);
  if ((Name == NULL) || (StrLen (Name) == 0)) {
    SHELL_FREE_NON_NULL (Name);
    return (EFI_NOT_FOUND);
  }

  *GuidName = AddBufferToFreeList (Name);

  return (EFI_SUCCESS);
}

/**
  If FileHandle is a directory then the function reads from FileHandle and reads in
  each of the FileInfo structures.  If one of them matches the Pattern's first
  "level" then it opens that handle and calls itself on that handle.

  If FileHandle is a file and matches all of the remaining Pattern (which would be
  on its last node), then add a EFI_SHELL_FILE_INFO object for this file to fileList.

  Upon a EFI_SUCCESS return fromt he function any the caller is responsible to call
  FreeFileList with FileList.

  @param[in] FilePattern         The FilePattern to check against.
  @param[in] UnicodeCollation    The pointer to EFI_UNICODE_COLLATION_PROTOCOL structure
  @param[in] FileHandle          The FileHandle to start with
  @param[in, out] FileList       pointer to pointer to list of found files.
  @param[in] ParentNode          The node for the parent. Same file as identified by HANDLE.
  @param[in] MapName             The file system name this file is on.

  @retval EFI_SUCCESS           all files were found and the FileList contains a list.
  @retval EFI_NOT_FOUND         no files were found
  @retval EFI_OUT_OF_RESOURCES  a memory allocation failed
**/
EFI_STATUS
ShellSearchHandle (
  IN     CONST CHAR16                          *FilePattern,
  IN           EFI_UNICODE_COLLATION_PROTOCOL  *UnicodeCollation,
  IN           SHELL_FILE_HANDLE               FileHandle,
  IN OUT       EFI_SHELL_FILE_INFO             **FileList,
  IN     CONST EFI_SHELL_FILE_INFO             *ParentNode OPTIONAL,
  IN     CONST CHAR16                          *MapName
  )
{
  EFI_STATUS           Status;
  CONST CHAR16         *NextFilePatternStart;
  CHAR16               *CurrentFilePattern;
  EFI_SHELL_FILE_INFO  *ShellInfo;
  EFI_SHELL_FILE_INFO  *ShellInfoNode;
  EFI_SHELL_FILE_INFO  *NewShellNode;
  EFI_FILE_INFO        *FileInfo;
  BOOLEAN              Directory;
  CHAR16               *NewFullName;
  UINTN                Size;

  if (  (FilePattern      == NULL)
     || (UnicodeCollation == NULL)
     || (FileList         == NULL)
        )
  {
    return (EFI_INVALID_PARAMETER);
  }

  ShellInfo          = NULL;
  CurrentFilePattern = NULL;

  if (*FilePattern == L'\\') {
    FilePattern++;
  }

  for ( NextFilePatternStart = FilePattern
        ; *NextFilePatternStart != CHAR_NULL && *NextFilePatternStart != L'\\'
        ; NextFilePatternStart++)
  {
  }

  CurrentFilePattern = AllocateZeroPool ((NextFilePatternStart-FilePattern+1)*sizeof (CHAR16));
  if (CurrentFilePattern == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StrnCpyS (CurrentFilePattern, NextFilePatternStart-FilePattern+1, FilePattern, NextFilePatternStart-FilePattern);

  if (  (CurrentFilePattern[0]   == CHAR_NULL)
     && (NextFilePatternStart[0] == CHAR_NULL)
        )
  {
    //
    // we want the parent or root node (if no parent)
    //
    if (ParentNode == NULL) {
      //
      // We want the root node.  create the node.
      //
      FileInfo     = FileHandleGetInfo (FileHandle);
      NewShellNode = CreateAndPopulateShellFileInfo (
                       MapName,
                       EFI_SUCCESS,
                       L"\\",
                       FileHandle,
                       FileInfo
                       );
      SHELL_FREE_NON_NULL (FileInfo);
    } else {
      //
      // Add the current parameter FileHandle to the list, then end...
      //
      NewShellNode = InternalDuplicateShellFileInfo ((EFI_SHELL_FILE_INFO *)ParentNode, TRUE);
    }

    if (NewShellNode == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      NewShellNode->Handle = NULL;
      if (*FileList == NULL) {
        *FileList = AllocateZeroPool (sizeof (EFI_SHELL_FILE_INFO));
        InitializeListHead (&((*FileList)->Link));
      }

      //
      // Add to the returning to use list
      //
      InsertTailList (&(*FileList)->Link, &NewShellNode->Link);

      Status = EFI_SUCCESS;
    }
  } else {
    Status = EfiShellFindFilesInDir (FileHandle, &ShellInfo);

    if (!EFI_ERROR (Status)) {
      if (StrStr (NextFilePatternStart, L"\\") != NULL) {
        Directory = TRUE;
      } else {
        Directory = FALSE;
      }

      for ( ShellInfoNode = (EFI_SHELL_FILE_INFO *)GetFirstNode (&ShellInfo->Link)
            ; !IsNull (&ShellInfo->Link, &ShellInfoNode->Link)
            ; ShellInfoNode = (EFI_SHELL_FILE_INFO *)GetNextNode (&ShellInfo->Link, &ShellInfoNode->Link)
            )
      {
        if (UnicodeCollation->MetaiMatch (UnicodeCollation, (CHAR16 *)ShellInfoNode->FileName, CurrentFilePattern)) {
          if ((ShellInfoNode->FullName != NULL) && (StrStr (ShellInfoNode->FullName, L":") == NULL)) {
            Size        = StrSize (ShellInfoNode->FullName) + StrSize (MapName);
            NewFullName = AllocateZeroPool (Size);
            if (NewFullName == NULL) {
              Status = EFI_OUT_OF_RESOURCES;
            } else {
              StrCpyS (NewFullName, Size / sizeof (CHAR16), MapName);
              StrCatS (NewFullName, Size / sizeof (CHAR16), ShellInfoNode->FullName);
              FreePool ((VOID *)ShellInfoNode->FullName);
              ShellInfoNode->FullName = NewFullName;
            }
          }

          if (Directory && !EFI_ERROR (Status) && (ShellInfoNode->FullName != NULL) && (ShellInfoNode->FileName != NULL)) {
            //
            // should be a directory
            //

            //
            // don't open the . and .. directories
            //
            if (  (StrCmp (ShellInfoNode->FileName, L".") != 0)
               && (StrCmp (ShellInfoNode->FileName, L"..") != 0)
                  )
            {
              //
              //
              //
              if (EFI_ERROR (Status)) {
                break;
              }

              //
              // Open the directory since we need that handle in the next recursion.
              //
              ShellInfoNode->Status = EfiShellOpenFileByName (ShellInfoNode->FullName, &ShellInfoNode->Handle, EFI_FILE_MODE_READ);

              //
              // recurse with the next part of the pattern
              //
              Status = ShellSearchHandle (NextFilePatternStart, UnicodeCollation, ShellInfoNode->Handle, FileList, ShellInfoNode, MapName);
              EfiShellClose (ShellInfoNode->Handle);
              ShellInfoNode->Handle = NULL;
            }
          } else if (!EFI_ERROR (Status)) {
            //
            // should be a file
            //

            //
            // copy the information we need into a new Node
            //
            NewShellNode = InternalDuplicateShellFileInfo (ShellInfoNode, FALSE);
            if (NewShellNode == NULL) {
              Status = EFI_OUT_OF_RESOURCES;
            }

            if (*FileList == NULL) {
              *FileList = AllocateZeroPool (sizeof (EFI_SHELL_FILE_INFO));
              InitializeListHead (&((*FileList)->Link));
            }

            //
            // Add to the returning to use list
            //
            InsertTailList (&(*FileList)->Link, &NewShellNode->Link);
          }
        }

        if (EFI_ERROR (Status)) {
          break;
        }
      }

      if (EFI_ERROR (Status)) {
        EfiShellFreeFileList (&ShellInfo);
      } else {
        Status = EfiShellFreeFileList (&ShellInfo);
      }
    }
  }

  if ((*FileList == NULL) || ((*FileList != NULL) && IsListEmpty (&(*FileList)->Link))) {
    Status = EFI_NOT_FOUND;
  }

  FreePool (CurrentFilePattern);
  return (Status);
}

/**
  Find files that match a specified pattern.

  This function searches for all files and directories that match the specified
  FilePattern. The FilePattern can contain wild-card characters. The resulting file
  information is placed in the file list FileList.

  Wildcards are processed
  according to the rules specified in UEFI Shell 2.0 spec section 3.7.1.

  The files in the file list are not opened. The OpenMode field is set to 0 and the FileInfo
  field is set to NULL.

  if *FileList is not NULL then it must be a pre-existing and properly initialized list.

  @param FilePattern      Points to a NULL-terminated shell file path, including wildcards.
  @param FileList         On return, points to the start of a file list containing the names
                          of all matching files or else points to NULL if no matching files
                          were found.  only on a EFI_SUCCESS return will; this be non-NULL.

  @retval EFI_SUCCESS           Files found.  FileList is a valid list.
  @retval EFI_NOT_FOUND         No files found.
  @retval EFI_NO_MEDIA          The device has no media
  @retval EFI_DEVICE_ERROR      The device reported an error
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted
**/
EFI_STATUS
EFIAPI
EfiShellFindFiles (
  IN CONST CHAR16          *FilePattern,
  OUT EFI_SHELL_FILE_INFO  **FileList
  )
{
  EFI_STATUS                Status;
  CHAR16                    *PatternCopy;
  CHAR16                    *PatternCurrentLocation;
  EFI_DEVICE_PATH_PROTOCOL  *RootDevicePath;
  SHELL_FILE_HANDLE         RootFileHandle;
  CHAR16                    *MapName;
  UINTN                     Count;

  if (  (FilePattern      == NULL)
     || (FileList         == NULL)
     || (StrStr (FilePattern, L":") == NULL)
        )
  {
    return (EFI_INVALID_PARAMETER);
  }

  Status         = EFI_SUCCESS;
  RootDevicePath = NULL;
  RootFileHandle = NULL;
  MapName        = NULL;
  PatternCopy    = AllocateCopyPool (StrSize (FilePattern), FilePattern);
  if (PatternCopy == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  PatternCopy = PathCleanUpDirectories (PatternCopy);

  Count = StrStr (PatternCopy, L":") - PatternCopy + 1;
  ASSERT (Count <= StrLen (PatternCopy));

  ASSERT (MapName == NULL);
  MapName = StrnCatGrow (&MapName, NULL, PatternCopy, Count);
  if (MapName == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    RootDevicePath = EfiShellGetDevicePathFromFilePath (PatternCopy);
    if (RootDevicePath == NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      Status = EfiShellOpenRoot (RootDevicePath, &RootFileHandle);
      if (!EFI_ERROR (Status)) {
        for ( PatternCurrentLocation = PatternCopy
              ; *PatternCurrentLocation != ':'
              ; PatternCurrentLocation++)
        {
        }

        PatternCurrentLocation++;
        Status = ShellSearchHandle (PatternCurrentLocation, gUnicodeCollation, RootFileHandle, FileList, NULL, MapName);
        EfiShellClose (RootFileHandle);
      }

      FreePool (RootDevicePath);
    }
  }

  SHELL_FREE_NON_NULL (PatternCopy);
  SHELL_FREE_NON_NULL (MapName);

  return (Status);
}

/**
  Opens the files that match the path specified.

  This function opens all of the files specified by Path. Wildcards are processed
  according to the rules specified in UEFI Shell 2.0 spec section 3.7.1. Each
  matching file has an EFI_SHELL_FILE_INFO structure created in a linked list.

  @param Path                   A pointer to the path string.
  @param OpenMode               Specifies the mode used to open each file, EFI_FILE_MODE_READ or
                                EFI_FILE_MODE_WRITE.
  @param FileList               Points to the start of a list of files opened.

  @retval EFI_SUCCESS           Create the file list successfully.
  @return Others                Can't create the file list.
**/
EFI_STATUS
EFIAPI
EfiShellOpenFileList (
  IN CHAR16                   *Path,
  IN UINT64                   OpenMode,
  IN OUT EFI_SHELL_FILE_INFO  **FileList
  )
{
  EFI_STATUS           Status;
  EFI_SHELL_FILE_INFO  *ShellFileListItem;
  CHAR16               *Path2;
  UINTN                Path2Size;
  CONST CHAR16         *CurDir;
  BOOLEAN              Found;

  PathCleanUpDirectories (Path);

  Path2Size = 0;
  Path2     = NULL;

  if ((FileList == NULL) || (*FileList == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  if ((*Path == L'.') && (*(Path+1) == L'\\')) {
    Path += 2;
  }

  //
  // convert a local path to an absolute path
  //
  if (StrStr (Path, L":") == NULL) {
    CurDir = EfiShellGetCurDir (NULL);
    ASSERT ((Path2 == NULL && Path2Size == 0) || (Path2 != NULL));
    StrnCatGrow (&Path2, &Path2Size, CurDir, 0);
    StrnCatGrow (&Path2, &Path2Size, L"\\", 0);
    if (*Path == L'\\') {
      Path++;
      while (PathRemoveLastItem (Path2)) {
      }
    }

    ASSERT ((Path2 == NULL && Path2Size == 0) || (Path2 != NULL));
    StrnCatGrow (&Path2, &Path2Size, Path, 0);
  } else {
    ASSERT (Path2 == NULL);
    StrnCatGrow (&Path2, NULL, Path, 0);
  }

  PathCleanUpDirectories (Path2);

  //
  // do the search
  //
  Status = EfiShellFindFiles (Path2, FileList);

  FreePool (Path2);

  if (EFI_ERROR (Status)) {
    return (Status);
  }

  Found = FALSE;
  //
  // We had no errors so open all the files (that are not already opened...)
  //
  for ( ShellFileListItem = (EFI_SHELL_FILE_INFO *)GetFirstNode (&(*FileList)->Link)
        ; !IsNull (&(*FileList)->Link, &ShellFileListItem->Link)
        ; ShellFileListItem = (EFI_SHELL_FILE_INFO *)GetNextNode (&(*FileList)->Link, &ShellFileListItem->Link)
        )
  {
    if ((ShellFileListItem->Status == 0) && (ShellFileListItem->Handle == NULL)) {
      ShellFileListItem->Status = EfiShellOpenFileByName (ShellFileListItem->FullName, &ShellFileListItem->Handle, OpenMode);
      Found                     = TRUE;
    }
  }

  if (!Found) {
    return (EFI_NOT_FOUND);
  }

  return (EFI_SUCCESS);
}

/**
  Gets the environment variable and Attributes, or list of environment variables.  Can be
  used instead of GetEnv().

  This function returns the current value of the specified environment variable and
  the Attributes. If no variable name was specified, then all of the known
  variables will be returned.

  @param[in] Name               A pointer to the environment variable name. If Name is NULL,
                                then the function will return all of the defined shell
                                environment variables. In the case where multiple environment
                                variables are being returned, each variable will be terminated
                                by a NULL, and the list will be terminated by a double NULL.
  @param[out] Attributes        If not NULL, a pointer to the returned attributes bitmask for
                                the environment variable. In the case where Name is NULL, and
                                multiple environment variables are being returned, Attributes
                                is undefined.

  @retval NULL                  The environment variable doesn't exist.
  @return                       A non-NULL value points to the variable's value. The returned
                                pointer does not need to be freed by the caller.
**/
CONST CHAR16 *
EFIAPI
EfiShellGetEnvEx (
  IN  CONST CHAR16  *Name,
  OUT       UINT32  *Attributes OPTIONAL
  )
{
  EFI_STATUS    Status;
  VOID          *Buffer;
  UINTN         Size;
  ENV_VAR_LIST  *Node;
  CHAR16        *CurrentWriteLocation;

  Size   = 0;
  Buffer = NULL;

  if (Name == NULL) {
    //
    // Build the semi-colon delimited list. (2 passes)
    //
    for ( Node = (ENV_VAR_LIST *)GetFirstNode (&gShellEnvVarList.Link)
          ; !IsNull (&gShellEnvVarList.Link, &Node->Link)
          ; Node = (ENV_VAR_LIST *)GetNextNode (&gShellEnvVarList.Link, &Node->Link)
          )
    {
      ASSERT (Node->Key != NULL);
      Size += StrSize (Node->Key);
    }

    Size += 2*sizeof (CHAR16);

    Buffer = AllocateZeroPool (Size);
    if (Buffer == NULL) {
      return (NULL);
    }

    CurrentWriteLocation = (CHAR16 *)Buffer;

    for ( Node = (ENV_VAR_LIST *)GetFirstNode (&gShellEnvVarList.Link)
          ; !IsNull (&gShellEnvVarList.Link, &Node->Link)
          ; Node = (ENV_VAR_LIST *)GetNextNode (&gShellEnvVarList.Link, &Node->Link)
          )
    {
      ASSERT (Node->Key != NULL);
      StrCpyS (
        CurrentWriteLocation,
        (Size)/sizeof (CHAR16) - (CurrentWriteLocation - ((CHAR16 *)Buffer)),
        Node->Key
        );
      CurrentWriteLocation += StrLen (CurrentWriteLocation) + 1;
    }
  } else {
    //
    // We are doing a specific environment variable
    //
    Status = ShellFindEnvVarInList (Name, (CHAR16 **)&Buffer, &Size, Attributes);

    if (EFI_ERROR (Status)) {
      //
      // get the size we need for this EnvVariable
      //
      Status = SHELL_GET_ENVIRONMENT_VARIABLE_AND_ATTRIBUTES (Name, Attributes, &Size, Buffer);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        //
        // Allocate the space and recall the get function
        //
        Buffer = AllocateZeroPool (Size);
        Status = SHELL_GET_ENVIRONMENT_VARIABLE_AND_ATTRIBUTES (Name, Attributes, &Size, Buffer);
      }

      //
      // we didn't get it (might not exist)
      // free the memory if we allocated any and return NULL
      //
      if (EFI_ERROR (Status)) {
        if (Buffer != NULL) {
          FreePool (Buffer);
        }

        return (NULL);
      } else {
        //
        // If we did not find the environment variable in the gShellEnvVarList
        // but get it from UEFI variable storage successfully then we need update
        // the gShellEnvVarList.
        //
        ShellFreeEnvVarList ();
        Status = ShellInitEnvVarList ();
        ASSERT (Status == EFI_SUCCESS);
      }
    }
  }

  //
  // return the buffer
  //
  return (AddBufferToFreeList (Buffer));
}

/**
  Gets either a single or list of environment variables.

  If name is not NULL then this function returns the current value of the specified
  environment variable.

  If Name is NULL, then a list of all environment variable names is returned.  Each is a
  NULL terminated string with a double NULL terminating the list.

  @param Name                   A pointer to the environment variable name.  If
                                Name is NULL, then the function will return all
                                of the defined shell environment variables.  In
                                the case where multiple environment variables are
                                being returned, each variable will be terminated by
                                a NULL, and the list will be terminated by a double
                                NULL.

  @retval !=NULL                A pointer to the returned string.
                                The returned pointer does not need to be freed by the caller.

  @retval NULL                  The environment variable doesn't exist or there are
                                no environment variables.
**/
CONST CHAR16 *
EFIAPI
EfiShellGetEnv (
  IN CONST CHAR16  *Name
  )
{
  return (EfiShellGetEnvEx (Name, NULL));
}

/**
  Internal variable setting function.  Allows for setting of the read only variables.

  @param Name                   Points to the NULL-terminated environment variable name.
  @param Value                  Points to the NULL-terminated environment variable value. If the value is an
                                empty string then the environment variable is deleted.
  @param Volatile               Indicates whether the variable is non-volatile (FALSE) or volatile (TRUE).

  @retval EFI_SUCCESS           The environment variable was successfully updated.
**/
EFI_STATUS
InternalEfiShellSetEnv (
  IN CONST CHAR16  *Name,
  IN CONST CHAR16  *Value,
  IN BOOLEAN       Volatile
  )
{
  EFI_STATUS  Status;

  if ((Value == NULL) || (StrLen (Value) == 0)) {
    Status = SHELL_DELETE_ENVIRONMENT_VARIABLE (Name);
    if (!EFI_ERROR (Status)) {
      ShellRemvoeEnvVarFromList (Name);
    }
  } else {
    SHELL_DELETE_ENVIRONMENT_VARIABLE (Name);
    Status = ShellAddEnvVarToList (
               Name,
               Value,
               StrSize (Value),
               EFI_VARIABLE_BOOTSERVICE_ACCESS | (Volatile ? 0 : EFI_VARIABLE_NON_VOLATILE)
               );
    if (!EFI_ERROR (Status)) {
      Status = Volatile
             ? SHELL_SET_ENVIRONMENT_VARIABLE_V (Name, StrSize (Value) - sizeof (CHAR16), Value)
             : SHELL_SET_ENVIRONMENT_VARIABLE_NV (Name, StrSize (Value) - sizeof (CHAR16), Value);
      if (EFI_ERROR (Status)) {
        ShellRemvoeEnvVarFromList (Name);
      }
    }
  }

  return Status;
}

/**
  Sets the environment variable.

  This function changes the current value of the specified environment variable. If the
  environment variable exists and the Value is an empty string, then the environment
  variable is deleted. If the environment variable exists and the Value is not an empty
  string, then the value of the environment variable is changed. If the environment
  variable does not exist and the Value is an empty string, there is no action. If the
  environment variable does not exist and the Value is a non-empty string, then the
  environment variable is created and assigned the specified value.

  For a description of volatile and non-volatile environment variables, see UEFI Shell
  2.0 specification section 3.6.1.

  @param Name                   Points to the NULL-terminated environment variable name.
  @param Value                  Points to the NULL-terminated environment variable value. If the value is an
                                empty string then the environment variable is deleted.
  @param Volatile               Indicates whether the variable is non-volatile (FALSE) or volatile (TRUE).

  @retval EFI_SUCCESS           The environment variable was successfully updated.
**/
EFI_STATUS
EFIAPI
EfiShellSetEnv (
  IN CONST CHAR16  *Name,
  IN CONST CHAR16  *Value,
  IN BOOLEAN       Volatile
  )
{
  if ((Name == NULL) || (*Name == CHAR_NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // Make sure we dont 'set' a predefined read only variable
  //
  if ((StrCmp (Name, L"cwd") == 0) ||
      (StrCmp (Name, L"lasterror") == 0) ||
      (StrCmp (Name, L"profiles") == 0) ||
      (StrCmp (Name, L"uefishellsupport") == 0) ||
      (StrCmp (Name, L"uefishellversion") == 0) ||
      (StrCmp (Name, L"uefiversion") == 0) ||
      (!ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoNest &&
       (StrCmp (Name, mNoNestingEnvVarName) == 0))
      )
  {
    return (EFI_INVALID_PARAMETER);
  }

  return (InternalEfiShellSetEnv (Name, Value, Volatile));
}

/**
  Returns the current directory on the specified device.

  If FileSystemMapping is NULL, it returns the current working directory. If the
  FileSystemMapping is not NULL, it returns the current directory associated with the
  FileSystemMapping. In both cases, the returned name includes the file system
  mapping (i.e. fs0:\current-dir).

  Note that the current directory string should exclude the tailing backslash character.

  @param FileSystemMapping      A pointer to the file system mapping. If NULL,
                                then the current working directory is returned.

  @retval !=NULL                The current directory.
  @retval NULL                  Current directory does not exist.
**/
CONST CHAR16 *
EFIAPI
EfiShellGetCurDir (
  IN CONST CHAR16  *FileSystemMapping OPTIONAL
  )
{
  CHAR16          *PathToReturn;
  UINTN           Size;
  SHELL_MAP_LIST  *MapListItem;

  if (!IsListEmpty (&gShellMapList.Link)) {
    //
    // if parameter is NULL, use current
    //
    if (FileSystemMapping == NULL) {
      return (EfiShellGetEnv (L"cwd"));
    } else {
      Size         = 0;
      PathToReturn = NULL;
      MapListItem  = ShellCommandFindMapItem (FileSystemMapping);
      if (MapListItem != NULL) {
        ASSERT ((PathToReturn == NULL && Size == 0) || (PathToReturn != NULL));
        PathToReturn = StrnCatGrow (&PathToReturn, &Size, MapListItem->MapName, 0);
        PathToReturn = StrnCatGrow (&PathToReturn, &Size, MapListItem->CurrentDirectoryPath, 0);
      }
    }

    return (AddBufferToFreeList (PathToReturn));
  } else {
    return (NULL);
  }
}

/**
  Changes the current directory on the specified device.

  If the FileSystem is NULL, and the directory Dir does not contain a file system's
  mapped name, this function changes the current working directory.

  If the FileSystem is NULL and the directory Dir contains a mapped name, then the
  current file system and the current directory on that file system are changed.

  If FileSystem is NULL, and Dir is not NULL, then this changes the current working file
  system.

  If FileSystem is not NULL and Dir is not NULL, then this function changes the current
  directory on the specified file system.

  If the current working directory or the current working file system is changed then the
  %cwd% environment variable will be updated

  Note that the current directory string should exclude the tailing backslash character.

  @param FileSystem             A pointer to the file system's mapped name. If NULL, then the current working
                                directory is changed.
  @param Dir                    Points to the NULL-terminated directory on the device specified by FileSystem.

  @retval EFI_SUCCESS           The operation was successful
  @retval EFI_NOT_FOUND         The file system could not be found
**/
EFI_STATUS
EFIAPI
EfiShellSetCurDir (
  IN CONST CHAR16  *FileSystem OPTIONAL,
  IN CONST CHAR16  *Dir
  )
{
  CHAR16          *MapName;
  SHELL_MAP_LIST  *MapListItem;
  UINTN           Size;
  EFI_STATUS      Status;
  CHAR16          *TempString;
  CHAR16          *DirectoryName;
  UINTN           TempLen;

  Size          = 0;
  MapName       = NULL;
  MapListItem   = NULL;
  TempString    = NULL;
  DirectoryName = NULL;

  if (((FileSystem == NULL) && (Dir == NULL)) || (Dir == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  if (IsListEmpty (&gShellMapList.Link)) {
    return (EFI_NOT_FOUND);
  }

  DirectoryName = StrnCatGrow (&DirectoryName, NULL, Dir, 0);
  ASSERT (DirectoryName != NULL);

  PathCleanUpDirectories (DirectoryName);

  if (FileSystem == NULL) {
    //
    // determine the file system mapping to use
    //
    if (StrStr (DirectoryName, L":") != NULL) {
      ASSERT (MapName == NULL);
      MapName = StrnCatGrow (&MapName, NULL, DirectoryName, (StrStr (DirectoryName, L":")-DirectoryName+1));
    }

    //
    // find the file system mapping's entry in the list
    // or use current
    //
    if (MapName != NULL) {
      MapListItem = ShellCommandFindMapItem (MapName);

      //
      // make that the current file system mapping
      //
      if (MapListItem != NULL) {
        gShellCurMapping = MapListItem;
      }
    } else {
      MapListItem = gShellCurMapping;
    }

    if (MapListItem == NULL) {
      FreePool (DirectoryName);
      SHELL_FREE_NON_NULL (MapName);
      return (EFI_NOT_FOUND);
    }

    //
    // now update the MapListItem's current directory
    //
    if ((MapListItem->CurrentDirectoryPath != NULL) && (DirectoryName[StrLen (DirectoryName) - 1] != L':')) {
      FreePool (MapListItem->CurrentDirectoryPath);
      MapListItem->CurrentDirectoryPath = NULL;
    }

    if (MapName != NULL) {
      TempLen = StrLen (MapName);
      if (TempLen != StrLen (DirectoryName)) {
        ASSERT ((MapListItem->CurrentDirectoryPath == NULL && Size == 0) || (MapListItem->CurrentDirectoryPath != NULL));
        MapListItem->CurrentDirectoryPath = StrnCatGrow (&MapListItem->CurrentDirectoryPath, &Size, DirectoryName+StrLen (MapName), 0);
      }

      FreePool (MapName);
    } else {
      ASSERT ((MapListItem->CurrentDirectoryPath == NULL && Size == 0) || (MapListItem->CurrentDirectoryPath != NULL));
      MapListItem->CurrentDirectoryPath = StrnCatGrow (&MapListItem->CurrentDirectoryPath, &Size, DirectoryName, 0);
    }

    if (((MapListItem->CurrentDirectoryPath != NULL) && (MapListItem->CurrentDirectoryPath[StrLen (MapListItem->CurrentDirectoryPath)-1] == L'\\')) || (MapListItem->CurrentDirectoryPath == NULL)) {
      ASSERT ((MapListItem->CurrentDirectoryPath == NULL && Size == 0) || (MapListItem->CurrentDirectoryPath != NULL));
      if (MapListItem->CurrentDirectoryPath != NULL) {
        MapListItem->CurrentDirectoryPath[StrLen (MapListItem->CurrentDirectoryPath)-1] = CHAR_NULL;
      }
    }
  } else {
    //
    // cant have a mapping in the directory...
    //
    if (StrStr (DirectoryName, L":") != NULL) {
      FreePool (DirectoryName);
      return (EFI_INVALID_PARAMETER);
    }

    //
    // FileSystem != NULL
    //
    MapListItem = ShellCommandFindMapItem (FileSystem);
    if (MapListItem == NULL) {
      FreePool (DirectoryName);
      return (EFI_INVALID_PARAMETER);
    }

    //    gShellCurMapping = MapListItem;
    if (DirectoryName != NULL) {
      //
      // change current dir on that file system
      //

      if (MapListItem->CurrentDirectoryPath != NULL) {
        FreePool (MapListItem->CurrentDirectoryPath);
        DEBUG_CODE (
          MapListItem->CurrentDirectoryPath = NULL;
          );
      }

      //      ASSERT((MapListItem->CurrentDirectoryPath == NULL && Size == 0) || (MapListItem->CurrentDirectoryPath != NULL));
      //      MapListItem->CurrentDirectoryPath = StrnCatGrow(&MapListItem->CurrentDirectoryPath, &Size, FileSystem, 0);
      ASSERT ((MapListItem->CurrentDirectoryPath == NULL && Size == 0) || (MapListItem->CurrentDirectoryPath != NULL));
      MapListItem->CurrentDirectoryPath = StrnCatGrow (&MapListItem->CurrentDirectoryPath, &Size, L"\\", 0);
      ASSERT ((MapListItem->CurrentDirectoryPath == NULL && Size == 0) || (MapListItem->CurrentDirectoryPath != NULL));
      MapListItem->CurrentDirectoryPath = StrnCatGrow (&MapListItem->CurrentDirectoryPath, &Size, DirectoryName, 0);
      if ((MapListItem->CurrentDirectoryPath != NULL) && (MapListItem->CurrentDirectoryPath[StrLen (MapListItem->CurrentDirectoryPath)-1] == L'\\')) {
        ASSERT ((MapListItem->CurrentDirectoryPath == NULL && Size == 0) || (MapListItem->CurrentDirectoryPath != NULL));
        MapListItem->CurrentDirectoryPath[StrLen (MapListItem->CurrentDirectoryPath)-1] = CHAR_NULL;
      }
    }
  }

  FreePool (DirectoryName);
  //
  // if updated the current directory then update the environment variable
  //
  if (MapListItem == gShellCurMapping) {
    Size = 0;
    ASSERT ((TempString == NULL && Size == 0) || (TempString != NULL));
    StrnCatGrow (&TempString, &Size, MapListItem->MapName, 0);
    ASSERT ((TempString == NULL && Size == 0) || (TempString != NULL));
    StrnCatGrow (&TempString, &Size, MapListItem->CurrentDirectoryPath, 0);
    Status =  InternalEfiShellSetEnv (L"cwd", TempString, TRUE);
    FreePool (TempString);
    return (Status);
  }

  return (EFI_SUCCESS);
}

/**
  Return help information about a specific command.

  This function returns the help information for the specified command. The help text
  can be internal to the shell or can be from a UEFI Shell manual page.

  If Sections is specified, then each section name listed will be compared in a casesensitive
  manner, to the section names described in Appendix B. If the section exists,
  it will be appended to the returned help text. If the section does not exist, no
  information will be returned. If Sections is NULL, then all help text information
  available will be returned.

  @param Command                Points to the NULL-terminated UEFI Shell command name.
  @param Sections               Points to the NULL-terminated comma-delimited
                                section names to return. If NULL, then all
                                sections will be returned.
  @param HelpText               On return, points to a callee-allocated buffer
                                containing all specified help text.

  @retval EFI_SUCCESS           The help text was returned.
  @retval EFI_OUT_OF_RESOURCES  The necessary buffer could not be allocated to hold the
                                returned help text.
  @retval EFI_INVALID_PARAMETER HelpText is NULL
  @retval EFI_NOT_FOUND         There is no help text available for Command.
**/
EFI_STATUS
EFIAPI
EfiShellGetHelpText (
  IN CONST CHAR16  *Command,
  IN CONST CHAR16  *Sections OPTIONAL,
  OUT CHAR16       **HelpText
  )
{
  CONST CHAR16  *ManFileName;
  CHAR16        *FixCommand;
  EFI_STATUS    Status;

  ASSERT (HelpText != NULL);
  FixCommand = NULL;

  ManFileName = ShellCommandGetManFileNameHandler (Command);

  if (ManFileName != NULL) {
    return (ProcessManFile (ManFileName, Command, Sections, NULL, HelpText));
  } else {
    if (  (StrLen (Command) > 4)
       && ((Command[StrLen (Command)-1] == L'i') || (Command[StrLen (Command)-1] == L'I'))
       && ((Command[StrLen (Command)-2] == L'f') || (Command[StrLen (Command)-2] == L'F'))
       && ((Command[StrLen (Command)-3] == L'e') || (Command[StrLen (Command)-3] == L'E'))
       && (Command[StrLen (Command)-4] == L'.')
          )
    {
      FixCommand = AllocateZeroPool (StrSize (Command) - 4 * sizeof (CHAR16));
      if (FixCommand == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      StrnCpyS (
        FixCommand,
        (StrSize (Command) - 4 * sizeof (CHAR16))/sizeof (CHAR16),
        Command,
        StrLen (Command)-4
        );
      Status = ProcessManFile (FixCommand, FixCommand, Sections, NULL, HelpText);
      FreePool (FixCommand);
      return Status;
    } else {
      return (ProcessManFile (Command, Command, Sections, NULL, HelpText));
    }
  }
}

/**
  Gets the enable status of the page break output mode.

  User can use this function to determine current page break mode.

  @retval TRUE                  The page break output mode is enabled.
  @retval FALSE                 The page break output mode is disabled.
**/
BOOLEAN
EFIAPI
EfiShellGetPageBreak (
  VOID
  )
{
  return (ShellInfoObject.PageBreakEnabled);
}

/**
  Judges whether the active shell is the root shell.

  This function makes the user to know that whether the active Shell is the root shell.

  @retval TRUE                  The active Shell is the root Shell.
  @retval FALSE                 The active Shell is NOT the root Shell.
**/
BOOLEAN
EFIAPI
EfiShellIsRootShell (
  VOID
  )
{
  return (ShellInfoObject.RootShellInstance);
}

/**
  function to return a semi-colon delimited list of all alias' in the current shell

  up to caller to free the memory.

  @retval NULL    No alias' were found
  @retval NULL    An error occurred getting alias'
  @return !NULL   a list of all alias'
**/
CHAR16 *
InternalEfiShellGetListAlias (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_GUID    Guid;
  CHAR16      *VariableName;
  UINTN       NameSize;
  UINTN       NameBufferSize;
  CHAR16      *RetVal;
  UINTN       RetSize;

  NameBufferSize = INIT_NAME_BUFFER_SIZE;
  VariableName   = AllocateZeroPool (NameBufferSize);
  RetSize        = 0;
  RetVal         = NULL;

  if (VariableName == NULL) {
    return (NULL);
  }

  VariableName[0] = CHAR_NULL;

  while (TRUE) {
    NameSize = NameBufferSize;
    Status   = gRT->GetNextVariableName (&NameSize, VariableName, &Guid);
    if (Status == EFI_NOT_FOUND) {
      break;
    } else if (Status == EFI_BUFFER_TOO_SMALL) {
      NameBufferSize = NameSize > NameBufferSize * 2 ? NameSize : NameBufferSize * 2;
      SHELL_FREE_NON_NULL (VariableName);
      VariableName = AllocateZeroPool (NameBufferSize);
      if (VariableName == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        SHELL_FREE_NON_NULL (RetVal);
        RetVal = NULL;
        break;
      }

      NameSize = NameBufferSize;
      Status   = gRT->GetNextVariableName (&NameSize, VariableName, &Guid);
    }

    if (EFI_ERROR (Status)) {
      SHELL_FREE_NON_NULL (RetVal);
      RetVal = NULL;
      break;
    }

    if (CompareGuid (&Guid, &gShellAliasGuid)) {
      ASSERT ((RetVal == NULL && RetSize == 0) || (RetVal != NULL));
      RetVal = StrnCatGrow (&RetVal, &RetSize, VariableName, 0);
      RetVal = StrnCatGrow (&RetVal, &RetSize, L";", 0);
    } // compare guid
  } // while

  SHELL_FREE_NON_NULL (VariableName);

  return (RetVal);
}

/**
  Convert a null-terminated unicode string, in-place, to all lowercase.
  Then return it.

  @param  Str    The null-terminated string to be converted to all lowercase.

  @return        The null-terminated string converted into all lowercase.
**/
CHAR16 *
ToLower (
  CHAR16  *Str
  )
{
  UINTN  Index;

  for (Index = 0; Str[Index] != L'\0'; Index++) {
    if ((Str[Index] >= L'A') && (Str[Index] <= L'Z')) {
      Str[Index] -= (CHAR16)(L'A' - L'a');
    }
  }

  return Str;
}

/**
  This function returns the command associated with a alias or a list of all
  alias'.

  @param[in] Alias              Points to the NULL-terminated shell alias.
                                If this parameter is NULL, then all
                                aliases will be returned in ReturnedData.
  @param[out] Volatile          upon return of a single command if TRUE indicates
                                this is stored in a volatile fashion.  FALSE otherwise.

  @return                        If Alias is not NULL, it will return a pointer to
                                the NULL-terminated command for that alias.
                                If Alias is NULL, ReturnedData points to a ';'
                                delimited list of alias (e.g.
                                ReturnedData = "dir;del;copy;mfp") that is NULL-terminated.
  @retval NULL                  an error occurred
  @retval NULL                  Alias was not a valid Alias
**/
CONST CHAR16 *
EFIAPI
EfiShellGetAlias (
  IN  CONST CHAR16  *Alias,
  OUT BOOLEAN       *Volatile OPTIONAL
  )
{
  CHAR16      *RetVal;
  UINTN       RetSize;
  UINT32      Attribs;
  EFI_STATUS  Status;
  CHAR16      *AliasLower;
  CHAR16      *AliasVal;

  // Convert to lowercase to make aliases case-insensitive
  if (Alias != NULL) {
    AliasLower = AllocateCopyPool (StrSize (Alias), Alias);
    if (AliasLower == NULL) {
      return NULL;
    }

    ToLower (AliasLower);

    if (Volatile == NULL) {
      GetVariable2 (AliasLower, &gShellAliasGuid, (VOID **)&AliasVal, NULL);
      FreePool (AliasLower);
      return (AddBufferToFreeList (AliasVal));
    }

    RetSize = 0;
    RetVal  = NULL;
    Status  = gRT->GetVariable (AliasLower, &gShellAliasGuid, &Attribs, &RetSize, RetVal);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      RetVal = AllocateZeroPool (RetSize);
      Status = gRT->GetVariable (AliasLower, &gShellAliasGuid, &Attribs, &RetSize, RetVal);
    }

    if (EFI_ERROR (Status)) {
      if (RetVal != NULL) {
        FreePool (RetVal);
      }

      FreePool (AliasLower);
      return (NULL);
    }

    if ((EFI_VARIABLE_NON_VOLATILE & Attribs) == EFI_VARIABLE_NON_VOLATILE) {
      *Volatile = FALSE;
    } else {
      *Volatile = TRUE;
    }

    FreePool (AliasLower);
    return (AddBufferToFreeList (RetVal));
  }

  return (AddBufferToFreeList (InternalEfiShellGetListAlias ()));
}

/**
  Changes a shell command alias.

  This function creates an alias for a shell command or if Alias is NULL it will delete an existing alias.

  this function does not check for built in alias'.

  @param[in] Command            Points to the NULL-terminated shell command or existing alias.
  @param[in] Alias              Points to the NULL-terminated alias for the shell command. If this is NULL, and
                                Command refers to an alias, that alias will be deleted.
  @param[in] Volatile           if TRUE the Alias being set will be stored in a volatile fashion.  if FALSE the
                                Alias being set will be stored in a non-volatile fashion.

  @retval EFI_SUCCESS           Alias created or deleted successfully.
  @retval EFI_NOT_FOUND         the Alias intended to be deleted was not found
**/
EFI_STATUS
InternalSetAlias (
  IN CONST CHAR16  *Command,
  IN CONST CHAR16  *Alias,
  IN BOOLEAN       Volatile
  )
{
  EFI_STATUS  Status;
  CHAR16      *AliasLower;
  BOOLEAN     DeleteAlias;

  DeleteAlias = FALSE;
  if (Alias == NULL) {
    //
    // We must be trying to remove one if Alias is NULL
    // remove an alias (but passed in COMMAND parameter)
    //
    Alias       = Command;
    DeleteAlias = TRUE;
  }

  ASSERT (Alias != NULL);

  //
  // Convert to lowercase to make aliases case-insensitive
  //
  AliasLower = AllocateCopyPool (StrSize (Alias), Alias);
  if (AliasLower == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ToLower (AliasLower);

  if (DeleteAlias) {
    Status = gRT->SetVariable (AliasLower, &gShellAliasGuid, 0, 0, NULL);
  } else {
    Status = gRT->SetVariable (
                    AliasLower,
                    &gShellAliasGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | (Volatile ? 0 : EFI_VARIABLE_NON_VOLATILE),
                    StrSize (Command),
                    (VOID *)Command
                    );
  }

  FreePool (AliasLower);

  return Status;
}

/**
  Changes a shell command alias.

  This function creates an alias for a shell command or if Alias is NULL it will delete an existing alias.


  @param[in] Command            Points to the NULL-terminated shell command or existing alias.
  @param[in] Alias              Points to the NULL-terminated alias for the shell command. If this is NULL, and
                                Command refers to an alias, that alias will be deleted.
  @param[in] Replace            If TRUE and the alias already exists, then the existing alias will be replaced. If
                                FALSE and the alias already exists, then the existing alias is unchanged and
                                EFI_ACCESS_DENIED is returned.
  @param[in] Volatile           if TRUE the Alias being set will be stored in a volatile fashion.  if FALSE the
                                Alias being set will be stored in a non-volatile fashion.

  @retval EFI_SUCCESS           Alias created or deleted successfully.
  @retval EFI_NOT_FOUND         the Alias intended to be deleted was not found
  @retval EFI_ACCESS_DENIED     The alias is a built-in alias or already existed and Replace was set to
                                FALSE.
  @retval EFI_INVALID_PARAMETER Command is null or the empty string.
**/
EFI_STATUS
EFIAPI
EfiShellSetAlias (
  IN CONST CHAR16  *Command,
  IN CONST CHAR16  *Alias,
  IN BOOLEAN       Replace,
  IN BOOLEAN       Volatile
  )
{
  if (ShellCommandIsOnAliasList ((Alias == NULL) ? Command : Alias)) {
    //
    // cant set over a built in alias
    //
    return (EFI_ACCESS_DENIED);
  } else if ((Command == NULL) || (*Command == CHAR_NULL) || (StrLen (Command) == 0)) {
    //
    // Command is null or empty
    //
    return (EFI_INVALID_PARAMETER);
  } else if ((EfiShellGetAlias (Command, NULL) != NULL) && !Replace) {
    //
    // Alias already exists, Replace not set
    //
    return (EFI_ACCESS_DENIED);
  } else {
    return (InternalSetAlias (Command, Alias, Volatile));
  }
}

// Pure FILE_HANDLE operations are passed to FileHandleLib
// these functions are indicated by the *
EFI_SHELL_PROTOCOL  mShellProtocol = {
  EfiShellExecute,
  EfiShellGetEnv,
  EfiShellSetEnv,
  EfiShellGetAlias,
  EfiShellSetAlias,
  EfiShellGetHelpText,
  EfiShellGetDevicePathFromMap,
  EfiShellGetMapFromDevicePath,
  EfiShellGetDevicePathFromFilePath,
  EfiShellGetFilePathFromDevicePath,
  EfiShellSetMap,
  EfiShellGetCurDir,
  EfiShellSetCurDir,
  EfiShellOpenFileList,
  EfiShellFreeFileList,
  EfiShellRemoveDupInFileList,
  EfiShellBatchIsActive,
  EfiShellIsRootShell,
  EfiShellEnablePageBreak,
  EfiShellDisablePageBreak,
  EfiShellGetPageBreak,
  EfiShellGetDeviceName,
  (EFI_SHELL_GET_FILE_INFO)FileHandleGetInfo,         // *
  (EFI_SHELL_SET_FILE_INFO)FileHandleSetInfo,         // *
  EfiShellOpenFileByName,
  EfiShellClose,
  EfiShellCreateFile,
  (EFI_SHELL_READ_FILE)FileHandleRead,                // *
  (EFI_SHELL_WRITE_FILE)FileHandleWrite,              // *
  (EFI_SHELL_DELETE_FILE)FileHandleDelete,            // *
  EfiShellDeleteFileByName,
  (EFI_SHELL_GET_FILE_POSITION)FileHandleGetPosition, // *
  (EFI_SHELL_SET_FILE_POSITION)FileHandleSetPosition, // *
  (EFI_SHELL_FLUSH_FILE)FileHandleFlush,              // *
  EfiShellFindFiles,
  EfiShellFindFilesInDir,
  (EFI_SHELL_GET_FILE_SIZE)FileHandleGetSize,         // *
  EfiShellOpenRoot,
  EfiShellOpenRootByHandle,
  NULL,
  SHELL_MAJOR_VERSION,
  SHELL_MINOR_VERSION,

  // New for UEFI Shell 2.1
  EfiShellRegisterGuidName,
  EfiShellGetGuidName,
  EfiShellGetGuidFromName,
  EfiShellGetEnvEx
};

/**
  Function to create and install on the current handle.

  Will overwrite any existing ShellProtocols in the system to be sure that
  the current shell is in control.

  This must be removed via calling CleanUpShellProtocol().

  @param[in, out] NewShell   The pointer to the pointer to the structure
  to install.

  @retval EFI_SUCCESS     The operation was successful.
  @return                 An error from LocateHandle, CreateEvent, or other core function.
**/
EFI_STATUS
CreatePopulateInstallShellProtocol (
  IN OUT EFI_SHELL_PROTOCOL  **NewShell
  )
{
  EFI_STATUS                  Status;
  UINTN                       BufferSize;
  EFI_HANDLE                  *Buffer;
  UINTN                       HandleCounter;
  SHELL_PROTOCOL_HANDLE_LIST  *OldProtocolNode;
  EFI_SHELL_PROTOCOL          *OldShell;

  if (NewShell == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  BufferSize      = 0;
  Buffer          = NULL;
  OldProtocolNode = NULL;
  InitializeListHead (&ShellInfoObject.OldShellList.Link);

  //
  // Initialize EfiShellProtocol object...
  //
  Status = gBS->CreateEvent (
                  0,
                  0,
                  NULL,
                  NULL,
                  &mShellProtocol.ExecutionBreak
                  );
  if (EFI_ERROR (Status)) {
    return (Status);
  }

  //
  // Get the size of the buffer we need.
  //
  Status = gBS->LocateHandle (
                  ByProtocol,
                  &gEfiShellProtocolGuid,
                  NULL,
                  &BufferSize,
                  Buffer
                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Allocate and recall with buffer of correct size
    //
    Buffer = AllocateZeroPool (BufferSize);
    if (Buffer == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }

    Status = gBS->LocateHandle (
                    ByProtocol,
                    &gEfiShellProtocolGuid,
                    NULL,
                    &BufferSize,
                    Buffer
                    );
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      return (Status);
    }

    //
    // now overwrite each of them, but save the info to restore when we end.
    //
    for (HandleCounter = 0; HandleCounter < (BufferSize/sizeof (EFI_HANDLE)); HandleCounter++) {
      Status = gBS->OpenProtocol (
                      Buffer[HandleCounter],
                      &gEfiShellProtocolGuid,
                      (VOID **)&OldShell,
                      gImageHandle,
                      NULL,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (!EFI_ERROR (Status)) {
        OldProtocolNode = AllocateZeroPool (sizeof (SHELL_PROTOCOL_HANDLE_LIST));
        if (OldProtocolNode == NULL) {
          if (!IsListEmpty (&ShellInfoObject.OldShellList.Link)) {
            CleanUpShellProtocol (&mShellProtocol);
          }

          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        //
        // reinstall over the old one...
        //
        OldProtocolNode->Handle    = Buffer[HandleCounter];
        OldProtocolNode->Interface = OldShell;
        Status                     = gBS->ReinstallProtocolInterface (
                                            OldProtocolNode->Handle,
                                            &gEfiShellProtocolGuid,
                                            OldProtocolNode->Interface,
                                            (VOID *)(&mShellProtocol)
                                            );
        if (!EFI_ERROR (Status)) {
          //
          // we reinstalled successfully.  log this so we can reverse it later.
          //

          //
          // add to the list for subsequent...
          //
          InsertTailList (&ShellInfoObject.OldShellList.Link, &OldProtocolNode->Link);
        }
      }
    }

    FreePool (Buffer);
  } else if (Status == EFI_NOT_FOUND) {
    ASSERT (IsListEmpty (&ShellInfoObject.OldShellList.Link));
    //
    // no one else published yet.  just publish it ourselves.
    //
    Status = gBS->InstallProtocolInterface (
                    &gImageHandle,
                    &gEfiShellProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    (VOID *)(&mShellProtocol)
                    );
  }

  if (PcdGetBool (PcdShellSupportOldProtocols)) {
    /// @todo support ShellEnvironment2
    /// @todo do we need to support ShellEnvironment (not ShellEnvironment2) also?
  }

  if (!EFI_ERROR (Status)) {
    *NewShell = &mShellProtocol;
  }

  return (Status);
}

/**
  Opposite of CreatePopulateInstallShellProtocol.

  Free all memory and restore the system to the state it was in before calling
  CreatePopulateInstallShellProtocol.

  @param[in, out] NewShell   The pointer to the new shell protocol structure.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
CleanUpShellProtocol (
  IN OUT EFI_SHELL_PROTOCOL  *NewShell
  )
{
  SHELL_PROTOCOL_HANDLE_LIST  *Node2;

  //
  // if we need to restore old protocols...
  //
  if (!IsListEmpty (&ShellInfoObject.OldShellList.Link)) {
    for (Node2 = (SHELL_PROTOCOL_HANDLE_LIST *)GetFirstNode (&ShellInfoObject.OldShellList.Link)
         ; !IsListEmpty (&ShellInfoObject.OldShellList.Link)
         ; Node2 = (SHELL_PROTOCOL_HANDLE_LIST *)GetFirstNode (&ShellInfoObject.OldShellList.Link)
         )
    {
      RemoveEntryList (&Node2->Link);
      gBS->ReinstallProtocolInterface (Node2->Handle, &gEfiShellProtocolGuid, NewShell, Node2->Interface);
      FreePool (Node2);
    }
  } else {
    //
    // no need to restore
    //
    gBS->UninstallProtocolInterface (gImageHandle, &gEfiShellProtocolGuid, NewShell);
  }

  return EFI_SUCCESS;
}

/**
  Cleanup the shell environment.

  @param[in, out] NewShell   The pointer to the new shell protocol structure.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
CleanUpShellEnvironment (
  IN OUT EFI_SHELL_PROTOCOL  *NewShell
  )
{
  EFI_STATUS                         Status;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *SimpleEx;

  CleanUpShellProtocol (NewShell);

  Status                   = gBS->CloseEvent (NewShell->ExecutionBreak);
  NewShell->ExecutionBreak = NULL;

  Status = gBS->OpenProtocol (
                  gST->ConsoleInHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&SimpleEx,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellInfoObject.CtrlCNotifyHandle1);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellInfoObject.CtrlCNotifyHandle2);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellInfoObject.CtrlCNotifyHandle3);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellInfoObject.CtrlCNotifyHandle4);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellInfoObject.CtrlSNotifyHandle1);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellInfoObject.CtrlSNotifyHandle2);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellInfoObject.CtrlSNotifyHandle3);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellInfoObject.CtrlSNotifyHandle4);
  }

  return (Status);
}

/**
  Notification function for keystrokes.

  @param[in] KeyData    The key that was pressed.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
NotificationFunction (
  IN EFI_KEY_DATA  *KeyData
  )
{
  if (((KeyData->Key.UnicodeChar == L'c') &&
       ((KeyData->KeyState.KeyShiftState == (EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED)) || (KeyData->KeyState.KeyShiftState  == (EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED)))) ||
      (KeyData->Key.UnicodeChar == 3)
      )
  {
    if (ShellInfoObject.NewEfiShellProtocol->ExecutionBreak == NULL) {
      return (EFI_UNSUPPORTED);
    }

    return (gBS->SignalEvent (ShellInfoObject.NewEfiShellProtocol->ExecutionBreak));
  } else if ((KeyData->Key.UnicodeChar == L's') &&
             ((KeyData->KeyState.KeyShiftState  == (EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED)) || (KeyData->KeyState.KeyShiftState  == (EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED)))
             )
  {
    ShellInfoObject.HaltOutput = TRUE;
  }

  return (EFI_SUCCESS);
}

/**
  Function to start monitoring for CTRL-C using SimpleTextInputEx.  This
  feature's enabled state was not known when the shell initially launched.

  @retval EFI_SUCCESS           The feature is enabled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory available.
**/
EFI_STATUS
InernalEfiShellStartMonitor (
  VOID
  )
{
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *SimpleEx;
  EFI_KEY_DATA                       KeyData;
  EFI_STATUS                         Status;

  Status = gBS->OpenProtocol (
                  gST->ConsoleInHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&SimpleEx,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_SHELL_NO_IN_EX),
      ShellInfoObject.HiiHandle
      );
    return (EFI_SUCCESS);
  }

  if (ShellInfoObject.NewEfiShellProtocol->ExecutionBreak == NULL) {
    return (EFI_UNSUPPORTED);
  }

  KeyData.KeyState.KeyToggleState = 0;
  KeyData.Key.ScanCode            = 0;
  KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED;
  KeyData.Key.UnicodeChar         = L'c';

  Status = SimpleEx->RegisterKeyNotify (
                       SimpleEx,
                       &KeyData,
                       NotificationFunction,
                       &ShellInfoObject.CtrlCNotifyHandle1
                       );

  KeyData.KeyState.KeyShiftState = EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED;
  if (!EFI_ERROR (Status)) {
    Status = SimpleEx->RegisterKeyNotify (
                         SimpleEx,
                         &KeyData,
                         NotificationFunction,
                         &ShellInfoObject.CtrlCNotifyHandle2
                         );
  }

  KeyData.KeyState.KeyShiftState = EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED;
  KeyData.Key.UnicodeChar        = 3;
  if (!EFI_ERROR (Status)) {
    Status = SimpleEx->RegisterKeyNotify (
                         SimpleEx,
                         &KeyData,
                         NotificationFunction,
                         &ShellInfoObject.CtrlCNotifyHandle3
                         );
  }

  KeyData.KeyState.KeyShiftState = EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED;
  if (!EFI_ERROR (Status)) {
    Status = SimpleEx->RegisterKeyNotify (
                         SimpleEx,
                         &KeyData,
                         NotificationFunction,
                         &ShellInfoObject.CtrlCNotifyHandle4
                         );
  }

  return (Status);
}
