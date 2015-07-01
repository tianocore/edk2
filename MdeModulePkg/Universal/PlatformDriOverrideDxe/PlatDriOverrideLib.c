/** @file
  Implementation of the shared functions to do the platform driver vverride mapping.

  Copyright (c) 2007 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalPlatDriOverrideDxe.h"

#define PLATFORM_OVERRIDE_ITEM_SIGNATURE      SIGNATURE_32('p','d','o','i')
 typedef struct _PLATFORM_OVERRIDE_ITEM {
  UINTN                                 Signature;
  LIST_ENTRY                            Link;
  UINT32                                DriverInfoNum;
  EFI_DEVICE_PATH_PROTOCOL              *ControllerDevicePath;
  ///
  /// List of DRIVER_IMAGE_INFO
  ///
  LIST_ENTRY                            DriverInfoList;
  EFI_HANDLE                            LastReturnedImageHandle;
} PLATFORM_OVERRIDE_ITEM;

#define DRIVER_IMAGE_INFO_SIGNATURE           SIGNATURE_32('p','d','i','i')
typedef struct _DRIVER_IMAGE_INFO {
  UINTN                                 Signature;
  LIST_ENTRY                            Link;
  EFI_HANDLE                            ImageHandle;
  EFI_DEVICE_PATH_PROTOCOL              *DriverImagePath;
  BOOLEAN                               UnLoadable;
  BOOLEAN                               UnStartable;
} DRIVER_IMAGE_INFO;

#define DEVICE_PATH_STACK_ITEM_SIGNATURE      SIGNATURE_32('d','p','s','i')
typedef struct _DEVICE_PATH_STACK_ITEM{
  UINTN                                 Signature;
  LIST_ENTRY                            Link;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
} DEVICE_PATH_STACK_ITEM;


LIST_ENTRY   mDevicePathStack = INITIALIZE_LIST_HEAD_VARIABLE (mDevicePathStack);

/**
  Push a controller device path into a globle device path list.

  @param  DevicePath     The controller device path to push into stack

  @retval EFI_SUCCESS    Device path successfully pushed into the stack.

**/
EFI_STATUS
EFIAPI
PushDevPathStack (
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  )
{
  DEVICE_PATH_STACK_ITEM  *DevicePathStackItem;

  DevicePathStackItem = AllocateZeroPool (sizeof (DEVICE_PATH_STACK_ITEM));
  ASSERT (DevicePathStackItem != NULL);
  DevicePathStackItem->Signature = DEVICE_PATH_STACK_ITEM_SIGNATURE;
  DevicePathStackItem->DevicePath = DuplicateDevicePath (DevicePath);
  InsertTailList (&mDevicePathStack, &DevicePathStackItem->Link);
  return EFI_SUCCESS;
}


/**
  Pop a controller device path from a globle device path list

  @param  DevicePath     The controller device path popped from stack

  @retval EFI_SUCCESS    Controller device path successfully popped.
  @retval EFI_NOT_FOUND  Stack is empty.

**/
EFI_STATUS
EFIAPI
PopDevPathStack (
  OUT  EFI_DEVICE_PATH_PROTOCOL    **DevicePath
  )
{
  DEVICE_PATH_STACK_ITEM  *DevicePathStackItem;
  LIST_ENTRY              *ItemListIndex;

  ItemListIndex = mDevicePathStack.BackLink;
  //
  // Check if the stack is empty
  //
  if (ItemListIndex != &mDevicePathStack){
    DevicePathStackItem = CR(ItemListIndex, DEVICE_PATH_STACK_ITEM, Link, DEVICE_PATH_STACK_ITEM_SIGNATURE);
    if (DevicePath != NULL) {
      *DevicePath = DuplicateDevicePath (DevicePathStackItem->DevicePath);
    }
    FreePool (DevicePathStackItem->DevicePath);
    RemoveEntryList (&DevicePathStackItem->Link);
    FreePool (DevicePathStackItem);
    return EFI_SUCCESS;
  }
  return EFI_NOT_FOUND;
}


/**
  Check whether a controller device path is in a globle device path list

  @param  DevicePath     The controller device path to check

  @retval TRUE           DevicePath exists in the stack.
  @retval FALSE          DevicePath does not exist in the stack.

**/
BOOLEAN
EFIAPI
CheckExistInStack (
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  )
{
  DEVICE_PATH_STACK_ITEM  *DevicePathStackItem;
  LIST_ENTRY              *ItemListIndex;
  UINTN                   DevicePathSize;

  ItemListIndex = mDevicePathStack.BackLink;
  while (ItemListIndex != &mDevicePathStack){
    DevicePathStackItem = CR(ItemListIndex, DEVICE_PATH_STACK_ITEM, Link, DEVICE_PATH_STACK_ITEM_SIGNATURE);
    DevicePathSize = GetDevicePathSize (DevicePath);
    if (DevicePathSize == GetDevicePathSize (DevicePathStackItem->DevicePath)) {
      if (CompareMem (DevicePath, DevicePathStackItem->DevicePath, DevicePathSize) == 0) {
        return TRUE;
      }
    }
    ItemListIndex = ItemListIndex->BackLink;
  }

  return FALSE;
}

/**
  Update the FV file device path if it is not valid.

  According to a file GUID, check a Fv file device path is valid. If it is invalid,
  try to return the valid device path.
  FV address maybe changes for memory layout adjust from time to time, use this funciton
  could promise the Fv file device path is right.

  @param  DevicePath               On input, the FV file device path to check
                                   On output, the updated valid FV file device path
  @param  FileGuid                 The FV file GUID
  @param  CallerImageHandle        Image handle of the caller

  @retval EFI_INVALID_PARAMETER    the input DevicePath or FileGuid is invalid
                                   parameter
  @retval EFI_UNSUPPORTED          the input DevicePath does not contain FV file
                                   GUID at all
  @retval EFI_ALREADY_STARTED      the input DevicePath has pointed to FV file, it
                                   is valid
  @retval EFI_SUCCESS              Successfully updated the invalid DevicePath,
                                   and return the updated device path in DevicePath

**/
EFI_STATUS
EFIAPI
UpdateFvFileDevicePath (
  IN  OUT EFI_DEVICE_PATH_PROTOCOL      **DevicePath,
  IN  EFI_GUID                          *FileGuid,
  IN  EFI_HANDLE                        CallerImageHandle
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *LastDeviceNode;
  EFI_STATUS                    Status;
  EFI_GUID                      *GuidPoint;
  UINTN                         Index;
  UINTN                         FvHandleCount;
  EFI_HANDLE                    *FvHandleBuffer;
  EFI_FV_FILETYPE               Type;
  UINTN                         Size;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINT32                        AuthenticationStatus;
  BOOLEAN                       FindFvFile;
  EFI_LOADED_IMAGE_PROTOCOL     *LoadedImage;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH FvFileNode;
  EFI_HANDLE                    FoundFvHandle;
  EFI_DEVICE_PATH_PROTOCOL      *NewDevicePath;
  BOOLEAN                       HasFvNode;

  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether the device path points to the default the input FV file
  //
  TempDevicePath = *DevicePath;
  LastDeviceNode = TempDevicePath;
  while (!IsDevicePathEnd (TempDevicePath)) {
     LastDeviceNode = TempDevicePath;
     TempDevicePath = NextDevicePathNode (TempDevicePath);
  }
  GuidPoint = EfiGetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) LastDeviceNode);
  if (GuidPoint == NULL) {
    //
    // If this option does not point to a FV file, just return EFI_UNSUPPORTED.
    //
    return EFI_UNSUPPORTED;
  }

  if (FileGuid != NULL) {
    if (!CompareGuid (GuidPoint, FileGuid)) {
      //
      // If the FV file is not the input file GUID, just return EFI_UNSUPPORTED
      //
      return EFI_UNSUPPORTED;
    }
  } else {
    FileGuid = GuidPoint;
  }

  //
  // Check to see if the device path contains memory map node
  //
  TempDevicePath = *DevicePath;
  HasFvNode = FALSE;
  while (!IsDevicePathEnd (TempDevicePath)) {
    //
    // Use old Device Path
    //
    if (DevicePathType (TempDevicePath) == HARDWARE_DEVICE_PATH &&
        DevicePathSubType (TempDevicePath) == HW_MEMMAP_DP) {
      HasFvNode = TRUE;
      break;
    }
    TempDevicePath = NextDevicePathNode (TempDevicePath);
  }

  if (!HasFvNode) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check whether the input Fv file device path is valid
  //
  TempDevicePath = *DevicePath;
  FoundFvHandle = NULL;
  Status = gBS->LocateDevicePath (
                  &gEfiFirmwareVolume2ProtocolGuid,
                  &TempDevicePath,
                  &FoundFvHandle
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (
                    FoundFvHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **) &Fv
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Set FV ReadFile Buffer as NULL, only need to check whether input Fv file exist there
      //
      Status = Fv->ReadFile (
                     Fv,
                     FileGuid,
                     NULL,
                     &Size,
                     &Type,
                     &Attributes,
                     &AuthenticationStatus
                     );
      if (!EFI_ERROR (Status)) {
        return EFI_ALREADY_STARTED;
      }
    }
  }

  //
  // Look for the input wanted FV file in current FV
  // First, try to look for in Caller own FV. Caller and input wanted FV file usually are in the same FV
  //
  FindFvFile = FALSE;
  FoundFvHandle = NULL;
  Status = gBS->HandleProtocol (
                  CallerImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **) &LoadedImage
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (
                    LoadedImage->DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **) &Fv
                    );
    if (!EFI_ERROR (Status)) {
      Status = Fv->ReadFile (
                     Fv,
                     FileGuid,
                     NULL,
                     &Size,
                     &Type,
                     &Attributes,
                     &AuthenticationStatus
                     );
      if (!EFI_ERROR (Status)) {
        FindFvFile = TRUE;
        FoundFvHandle = LoadedImage->DeviceHandle;
      }
    }
  }
  //
  // Second, if fail to find, try to enumerate all FV
  //
  if (!FindFvFile) {
    gBS->LocateHandleBuffer (
           ByProtocol,
           &gEfiFirmwareVolume2ProtocolGuid,
           NULL,
           &FvHandleCount,
           &FvHandleBuffer
           );
    for (Index = 0; Index < FvHandleCount; Index++) {
      gBS->HandleProtocol (
             FvHandleBuffer[Index],
             &gEfiFirmwareVolume2ProtocolGuid,
             (VOID **) &Fv
             );

      Status = Fv->ReadFile (
                     Fv,
                     FileGuid,
                     NULL,
                     &Size,
                     &Type,
                     &Attributes,
                     &AuthenticationStatus
                     );
      if (EFI_ERROR (Status)) {
        //
        // Skip if input Fv file not in the FV
        //
        continue;
      }
      FindFvFile = TRUE;
      FoundFvHandle = FvHandleBuffer[Index];
      break;
    }
  }

  if (FindFvFile) {
    //
    // Build the shell device path
    //
    NewDevicePath = DevicePathFromHandle (FoundFvHandle);
    EfiInitializeFwVolDevicepathNode (&FvFileNode, FileGuid);
    NewDevicePath = AppendDevicePathNode (NewDevicePath, (EFI_DEVICE_PATH_PROTOCOL *) &FvFileNode);
    *DevicePath = NewDevicePath;
    return EFI_SUCCESS;
  }
  return EFI_NOT_FOUND;
}

/**
  Gets the data and size of a variable.

  Read the EFI variable (VendorGuid/Name) and return a dynamically allocated
  buffer, and the size of the buffer. If failure return NULL.

  @param  Name                     String part of EFI variable name
  @param  VendorGuid               GUID part of EFI variable name
  @param  VariableSize             Returns the size of the EFI variable that was
                                   read

  @return Dynamically allocated memory that contains a copy of the EFI variable.
          Caller is responsible freeing the buffer.
  @retval NULL                     Variable was not read

**/
VOID *
EFIAPI
GetVariableAndSize (
  IN  CHAR16              *Name,
  IN  EFI_GUID            *VendorGuid,
  OUT UINTN               *VariableSize
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;
  VOID        *Buffer;

  Buffer = NULL;

  //
  // Pass in a zero size buffer to find the required buffer size.
  //
  BufferSize  = 0;
  Status      = gRT->GetVariable (Name, VendorGuid, NULL, &BufferSize, Buffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Allocate the buffer to return
    //
    Buffer = AllocateZeroPool (BufferSize);
    if (Buffer == NULL) {
      return NULL;
    }
    //
    // Read variable into the allocated buffer.
    //
    Status = gRT->GetVariable (Name, VendorGuid, NULL, &BufferSize, Buffer);
    if (EFI_ERROR (Status)) {
      BufferSize = 0;
    }
  }

  *VariableSize = BufferSize;
  return Buffer;
}

/**
  Connect to the handle to a device on the device path.

  This function will create all handles associate with every device
  path node. If the handle associate with one device path node can not
  be created success, then still give one chance to do the dispatch,
  which load the missing drivers if possible.

  @param  DevicePathToConnect      The device path which will be connected, it can
                                   be a multi-instance device path

  @retval EFI_SUCCESS              All handles associate with every device path
                                   node have been created
  @retval EFI_OUT_OF_RESOURCES     There is no resource to create new handles
  @retval EFI_NOT_FOUND            Create the handle associate with one device
                                   path node failed

**/
EFI_STATUS
EFIAPI
ConnectDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *CopyOfDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *Next;
  EFI_HANDLE                Handle;
  EFI_HANDLE                PreviousHandle;
  UINTN                     Size;

  if (DevicePathToConnect == NULL) {
    return EFI_SUCCESS;
  }

  DevicePath        = DuplicateDevicePath (DevicePathToConnect);
  CopyOfDevicePath  = DevicePath;
  if (DevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  do {
    //
    // The outer loop handles multi instance device paths.
    // Only console variables contain multiple instance device paths.
    //
    // After this call DevicePath points to the next Instance
    //
    Instance  = GetNextDevicePathInstance (&DevicePath, &Size);
    ASSERT (Instance != NULL);

    Next      = Instance;
    while (!IsDevicePathEndType (Next)) {
      Next = NextDevicePathNode (Next);
    }

    SetDevicePathEndNode (Next);

    //
    // Start the real work of connect with RemainingDevicePath
    //
    PreviousHandle = NULL;
    do {
      //
      // Find the handle that best matches the Device Path. If it is only a
      // partial match the remaining part of the device path is returned in
      // RemainingDevicePath.
      //
      RemainingDevicePath = Instance;
      Status              = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemainingDevicePath, &Handle);

      if (!EFI_ERROR (Status)) {
        if (Handle == PreviousHandle) {
          //
          // If no forward progress is made try invoking the Dispatcher.
          // A new FV may have been added to the system an new drivers
          // may now be found.
          // Status == EFI_SUCCESS means a driver was dispatched
          // Status == EFI_NOT_FOUND means no new drivers were dispatched
          //
          Status = gDS->Dispatch ();
        }

        if (!EFI_ERROR (Status)) {
          PreviousHandle = Handle;
          //
          // Connect all drivers that apply to Handle and RemainingDevicePath,
          // the Recursive flag is FALSE so only one level will be expanded.
          //
          // Do not check the connect status here, if the connect controller fail,
          // then still give the chance to do dispatch, because partial
          // RemainingDevicepath may be in the new FV
          //
          // 1. If the connect fails, RemainingDevicepath and handle will not
          //    change, so next time will do the dispatch, then dispatch's status
          //    will take effect
          // 2. If the connect succeeds, the RemainingDevicepath and handle will
          //    change, then avoid the dispatch, we have chance to continue the
          //    next connection
          //
          gBS->ConnectController (Handle, NULL, RemainingDevicePath, FALSE);
        }
      }
      //
      // Loop until RemainingDevicePath is an empty device path
      //
    } while (!EFI_ERROR (Status) && !IsDevicePathEnd (RemainingDevicePath));

  } while (DevicePath != NULL);

  if (CopyOfDevicePath != NULL) {
    FreePool (CopyOfDevicePath);
  }
  //
  // All handle with DevicePath exists in the handle database
  //
  return Status;
}

/**
  Free all the mapping database memory resource and initialize the mapping list entry.

  @param  MappingDataBase          Mapping database list entry pointer

  @retval EFI_SUCCESS              Mapping database successfully freed
  @retval EFI_INVALID_PARAMETER    MappingDataBase is NULL

**/
EFI_STATUS
EFIAPI
FreeMappingDatabase (
  IN  OUT  LIST_ENTRY            *MappingDataBase
  )
{
  LIST_ENTRY                  *OverrideItemListIndex;
  LIST_ENTRY                  *ImageInfoListIndex;
  PLATFORM_OVERRIDE_ITEM      *OverrideItem;
  DRIVER_IMAGE_INFO           *DriverImageInfo;

  if (MappingDataBase == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OverrideItemListIndex = GetFirstNode (MappingDataBase);
  while (!IsNull (MappingDataBase, OverrideItemListIndex)) {
    OverrideItem = CR(OverrideItemListIndex, PLATFORM_OVERRIDE_ITEM, Link, PLATFORM_OVERRIDE_ITEM_SIGNATURE);
    //
    // Free PLATFORM_OVERRIDE_ITEM.ControllerDevicePath[]
    //
    if (OverrideItem->ControllerDevicePath != NULL){
      FreePool (OverrideItem->ControllerDevicePath);
    }

    ImageInfoListIndex = GetFirstNode (&OverrideItem->DriverInfoList);
    while (!IsNull (&OverrideItem->DriverInfoList, ImageInfoListIndex)) {
      //
      // Free DRIVER_IMAGE_INFO.DriverImagePath[]
      //
      DriverImageInfo = CR(ImageInfoListIndex, DRIVER_IMAGE_INFO, Link, DRIVER_IMAGE_INFO_SIGNATURE);
      if (DriverImageInfo->DriverImagePath != NULL) {
        FreePool(DriverImageInfo->DriverImagePath);
      }
      //
      // Free DRIVER_IMAGE_INFO itself
      //
      ImageInfoListIndex = GetNextNode (&OverrideItem->DriverInfoList, ImageInfoListIndex);
      RemoveEntryList (&DriverImageInfo->Link);
      FreePool (DriverImageInfo);
    }
    //
    // Free PLATFORM_OVERRIDE_ITEM itself
    //
    OverrideItemListIndex = GetNextNode (MappingDataBase, OverrideItemListIndex);
    RemoveEntryList (&OverrideItem->Link);
    FreePool (OverrideItem);
  }

  InitializeListHead (MappingDataBase);
  return EFI_SUCCESS;
}


/**
  Create the mapping database according to variable.

  Read the environment variable(s) that contain the override mappings from Controller Device Path to
  a set of Driver Device Paths, and create the mapping database in memory with those variable info.
  VariableLayout{
  //
  // NotEnd indicate whether the variable is the last one, and has no subsequent variable need to load.
  // Each variable has MaximumVariableSize limitation, so we maybe need multiple variables to store
  // large mapping infos.
  // The variable(s) name rule is PlatDriOver, PlatDriOver1, PlatDriOver2, ....
  //
  UINT32                         NotEnd;               //Zero is the last one.
  //
  // The entry which contains the mapping that Controller Device Path to a set of Driver Device Paths
  // There are often multi mapping entries in a variable.
  //
  UINT32                         SIGNATURE;            //SIGNATURE_32('p','d','o','i')
  UINT32                         DriverNum;
  EFI_DEVICE_PATH_PROTOCOL       ControllerDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  ......
  UINT32                         NotEnd;                //Zero is the last one.
  UINT32                         SIGNATURE;
  UINT32                         DriverNum;
  EFI_DEVICE_PATH_PROTOCOL       ControllerDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  ......
  }

  @param  MappingDataBase          Mapping database list entry pointer

  @retval EFI_SUCCESS              Create the mapping database in memory successfully
  @retval EFI_INVALID_PARAMETER    MappingDataBase pointer is null
  @retval EFI_NOT_FOUND            Cannot find the 'PlatDriOver' NV variable
  @retval EFI_VOLUME_CORRUPTED     The found NV variable is corrupted

**/
EFI_STATUS
EFIAPI
InitOverridesMapping (
  OUT  LIST_ENTRY            *MappingDataBase
  )
{
  UINTN                       BufferSize;
  VOID                        *VariableBuffer;
  UINT8                       *VariableIndex;
  UINTN                       VariableNum;
  CHAR16                      OverrideVariableName[40];
  UINT32                      NotEnd;
  UINT32                      DriverNumber;
  PLATFORM_OVERRIDE_ITEM      *OverrideItem;
  DRIVER_IMAGE_INFO           *DriverImageInfo;
  BOOLEAN                     Corrupted;
  UINT32                      Signature;
  EFI_DEVICE_PATH_PROTOCOL    *ControllerDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    *DriverDevicePath;
  UINTN                       Index;

  if (MappingDataBase == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check the environment variable(s) that contain the override mappings .
  //
  VariableBuffer = GetVariableAndSize (L"PlatDriOver", &gEfiCallerIdGuid, &BufferSize);
  ASSERT ((UINTN) VariableBuffer % sizeof(UINTN) == 0);
  if (VariableBuffer == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Traverse all variables.
  //
  VariableNum = 1;
  Corrupted = FALSE;
  NotEnd = 0;
  do {
    VariableIndex = VariableBuffer;
    if (VariableIndex + sizeof (UINT32) > (UINT8 *) VariableBuffer + BufferSize) {
      Corrupted = TRUE;
    } else {
      //
      // End flag
      //
      NotEnd = *(UINT32*) VariableIndex;
    }
    //
    // Traverse the entries containing the mapping that Controller Device Path
    // to a set of Driver Device Paths within this variable.
    //
    VariableIndex = VariableIndex + sizeof (UINT32);
    while (VariableIndex < ((UINT8 *)VariableBuffer + BufferSize)) {
      //
      // Check signature of this entry
      //
      if (VariableIndex + sizeof (UINT32) > (UINT8 *) VariableBuffer + BufferSize) {
        Corrupted = TRUE;
        break;
      }
      Signature = *(UINT32 *) VariableIndex;
      if (Signature != PLATFORM_OVERRIDE_ITEM_SIGNATURE) {
        Corrupted = TRUE;
        break;
      }
      //
      // Create PLATFORM_OVERRIDE_ITEM for this mapping
      //
      OverrideItem = AllocateZeroPool (sizeof (PLATFORM_OVERRIDE_ITEM));
      ASSERT (OverrideItem != NULL);
      OverrideItem->Signature = PLATFORM_OVERRIDE_ITEM_SIGNATURE;
      InitializeListHead (&OverrideItem->DriverInfoList);
      VariableIndex = VariableIndex + sizeof (UINT32);
      //
      // Get DriverNum
      //
      if (VariableIndex + sizeof (UINT32) >= (UINT8 *) VariableBuffer + BufferSize) {
        Corrupted = TRUE;
        break;
      }
      DriverNumber = *(UINT32*) VariableIndex;
      OverrideItem->DriverInfoNum = DriverNumber;
      VariableIndex = VariableIndex + sizeof (UINT32);
      //
      // Get ControllerDevicePath[]
      //
      ControllerDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) VariableIndex;
      OverrideItem->ControllerDevicePath = DuplicateDevicePath (ControllerDevicePath);
      VariableIndex = VariableIndex + GetDevicePathSize (ControllerDevicePath);
      //
      // Align the VariableIndex since the controller device path may not be aligned, refer to the SaveOverridesMapping()
      //
      VariableIndex += ((sizeof(UINT32) - ((UINTN) (VariableIndex))) & (sizeof(UINT32) - 1));
      //
      // Check buffer overflow.
      //
      if ((OverrideItem->ControllerDevicePath == NULL) || (VariableIndex < (UINT8 *) ControllerDevicePath) || 
          (VariableIndex > (UINT8 *) VariableBuffer + BufferSize)) {
        Corrupted = TRUE;
        break;
      }

      //
      // Get all DriverImageDevicePath[]
      //
      for (Index = 0; Index < DriverNumber; Index++) {
        //
        // Create DRIVER_IMAGE_INFO for this DriverDevicePath[]
        //
        DriverImageInfo = AllocateZeroPool (sizeof (DRIVER_IMAGE_INFO));
        ASSERT (DriverImageInfo != NULL);
        DriverImageInfo->Signature = DRIVER_IMAGE_INFO_SIGNATURE;

        DriverDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) VariableIndex;
        DriverImageInfo->DriverImagePath = DuplicateDevicePath (DriverDevicePath);
        VariableIndex = VariableIndex + GetDevicePathSize (DriverDevicePath);
        //
        // Align the VariableIndex since the driver image device path may not be aligned, refer to the SaveOverridesMapping()
        //
        VariableIndex += ((sizeof(UINT32) - ((UINTN) (VariableIndex))) & (sizeof(UINT32) - 1));

        InsertTailList (&OverrideItem->DriverInfoList, &DriverImageInfo->Link);

        //
        // Check buffer overflow
        //
        if ((DriverImageInfo->DriverImagePath == NULL) || (VariableIndex < (UINT8 *) DriverDevicePath) || 
            (VariableIndex < (UINT8 *) VariableBuffer + BufferSize)) {
          Corrupted = TRUE;
          break;
        }
      }
      InsertTailList (MappingDataBase, &OverrideItem->Link);
      if (Corrupted) {
        break;
      }
    }

    FreePool (VariableBuffer);
    if (Corrupted) {
      FreeMappingDatabase (MappingDataBase);
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // If there are additional variables (PlatDriOver1, PlatDriOver2, PlatDriOver3.....), get them.
    // NotEnd indicates whether current variable is the end variable.
    //
    if (NotEnd != 0) {
      UnicodeSPrint (OverrideVariableName, sizeof (OverrideVariableName), L"PlatDriOver%d", VariableNum++);
      VariableBuffer = GetVariableAndSize (OverrideVariableName, &gEfiCallerIdGuid, &BufferSize);
      ASSERT ((UINTN) VariableBuffer % sizeof(UINTN) == 0);
      if (VariableBuffer == NULL) {
        FreeMappingDatabase (MappingDataBase);
        return EFI_VOLUME_CORRUPTED;
      }
    }

  } while (NotEnd != 0);

  return EFI_SUCCESS;
}


/**
  Calculate the needed size in NV variable for recording a specific PLATFORM_OVERRIDE_ITEM info.

  @param  OverrideItemListIndex    Pointer to the list of a specific PLATFORM_OVERRIDE_ITEM

  @return The needed size number

**/
UINTN
EFIAPI
GetOneItemNeededSize (
  IN  LIST_ENTRY            *OverrideItemListIndex
  )
{
  UINTN                       NeededSize;
  PLATFORM_OVERRIDE_ITEM      *OverrideItem;
  LIST_ENTRY                  *ImageInfoListIndex;
  DRIVER_IMAGE_INFO           *DriverImageInfo;
  UINTN                       DevicePathSize;

  NeededSize = 0;
  OverrideItem = CR(OverrideItemListIndex, PLATFORM_OVERRIDE_ITEM, Link, PLATFORM_OVERRIDE_ITEM_SIGNATURE);
  NeededSize += sizeof (UINT32); //UINT32  SIGNATURE;
  NeededSize += sizeof (UINT32); //UINT32  DriverNum;
  DevicePathSize = GetDevicePathSize (OverrideItem->ControllerDevicePath);
  NeededSize += DevicePathSize; // ControllerDevicePath
  //
  // Align the controller device path
  //
  NeededSize += ((sizeof(UINT32) - DevicePathSize) & (sizeof(UINT32) - 1));
  //
  // Traverse the Driver Info List of this Override Item
  //
  ImageInfoListIndex = GetFirstNode (&OverrideItem->DriverInfoList);
  while (!IsNull (&OverrideItem->DriverInfoList, ImageInfoListIndex)) {
    DriverImageInfo = CR(ImageInfoListIndex, DRIVER_IMAGE_INFO, Link, DRIVER_IMAGE_INFO_SIGNATURE);
    DevicePathSize = GetDevicePathSize (DriverImageInfo->DriverImagePath);
    NeededSize += DevicePathSize; //DriverDevicePath
    //
    // Align the driver image device path
    //
    NeededSize += ((sizeof(UINT32) - DevicePathSize) & (sizeof(UINT32) - 1));
    ImageInfoListIndex = GetNextNode (&OverrideItem->DriverInfoList, ImageInfoListIndex);
  }

  return NeededSize;
}

/**
  Deletes all environment variable(s) that contain the override mappings from Controller Device Path to
  a set of Driver Device Paths.

  @retval EFI_SUCCESS  Delete all variable(s) successfully.

**/
EFI_STATUS
EFIAPI
DeleteOverridesVariables (
  VOID
  )
{
  EFI_STATUS                  Status;
  VOID                        *VariableBuffer;
  UINTN                       VariableNum;
  UINTN                       BufferSize;
  UINTN                       Index;
  CHAR16                      OverrideVariableName[40];

  //
  // Get environment variable(s) number
  //
  VariableNum = 0;
  VariableBuffer = GetVariableAndSize (L"PlatDriOver", &gEfiCallerIdGuid, &BufferSize);
  VariableNum++;
  if (VariableBuffer == NULL) {
    return EFI_NOT_FOUND;
  }
  //
  // Check NotEnd to get all PlatDriOverX variable(s)
  //
  while ((VariableBuffer != NULL) && ((*(UINT32*)VariableBuffer) != 0)) {
    FreePool (VariableBuffer);
    UnicodeSPrint (OverrideVariableName, sizeof (OverrideVariableName), L"PlatDriOver%d", VariableNum);
    VariableBuffer = GetVariableAndSize (OverrideVariableName, &gEfiCallerIdGuid, &BufferSize);
    VariableNum++;
  }

  //
  // Delete PlatDriOver and all additional variables, if exist.
  //
  Status = gRT->SetVariable (
                  L"PlatDriOver",
                  &gEfiCallerIdGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  0,
                  NULL
                  );
  ASSERT (!EFI_ERROR (Status));
  for (Index = 1; Index < VariableNum; Index++) {
    UnicodeSPrint (OverrideVariableName, sizeof (OverrideVariableName), L"PlatDriOver%d", Index);
    Status = gRT->SetVariable (
                    OverrideVariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    0,
                    NULL
                    );
    ASSERT (!EFI_ERROR (Status));
  }
  return EFI_SUCCESS;
}


/**
  Save the memory mapping database into NV environment variable(s).

  @param  MappingDataBase          Mapping database list entry pointer

  @retval EFI_SUCCESS              Save memory mapping database successfully
  @retval EFI_INVALID_PARAMETER    MappingDataBase pointer is null

**/
EFI_STATUS
EFIAPI
SaveOverridesMapping (
  IN  LIST_ENTRY              *MappingDataBase
  )
{
  EFI_STATUS                  Status;
  VOID                        *VariableBuffer;
  UINT8                       *VariableIndex;
  UINTN                       NumIndex;
  CHAR16                      OverrideVariableName[40];
  UINT32                      NotEnd;
  PLATFORM_OVERRIDE_ITEM      *OverrideItem;
  DRIVER_IMAGE_INFO           *DriverImageInfo;
  LIST_ENTRY                  *OverrideItemListIndex;
  LIST_ENTRY                  *ItemIndex;
  LIST_ENTRY                  *ImageInfoListIndex;
  UINTN                       VariableNeededSize;
  UINT64                      MaximumVariableStorageSize;
  UINT64                      RemainingVariableStorageSize;
  UINT64                      MaximumVariableSize;
  UINTN                       OneItemNeededSize;

  if (MappingDataBase == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsListEmpty (MappingDataBase)) {
    Status = DeleteOverridesVariables ();
    return EFI_SUCCESS;
  }

  //
  // Get the the maximum size of an individual EFI variable in current system
  //
  gRT->QueryVariableInfo (
          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
          &MaximumVariableStorageSize,
          &RemainingVariableStorageSize,
          &MaximumVariableSize
          );

  NumIndex = 0;
  OverrideItemListIndex = GetFirstNode (MappingDataBase);
  while (!IsNull (MappingDataBase, OverrideItemListIndex)) {
    //
    // Try to find the most proper variable size which <= MaximumVariableSize,
    // but can contain mapping info as much as possible
    //
    VariableNeededSize = sizeof (UINT32); // NotEnd;
    ItemIndex = OverrideItemListIndex;
    NotEnd = FALSE;
    //
    // Traverse all PLATFORM_OVERRIDE_ITEMs and get the total size.
    //
    while (!IsNull (MappingDataBase, ItemIndex)) {
      OneItemNeededSize = GetOneItemNeededSize (ItemIndex);
      //
      // If the total size exceeds the MaximumVariableSize, then we must use
      // multiple variables.
      //
      if ((VariableNeededSize +
           OneItemNeededSize +
           StrSize (L"PlatDriOver ")
           ) >= MaximumVariableSize
          ) {
        NotEnd = TRUE;
        break;
      }

      VariableNeededSize += OneItemNeededSize;
      ItemIndex = GetNextNode (MappingDataBase, ItemIndex);
    }

    if (NotEnd != 0) {
      if (VariableNeededSize == sizeof (UINT32)) {
        //
        // If an individual EFI variable cannot contain a single Item, return error
        //
        return EFI_OUT_OF_RESOURCES;
      }
    }

    //
    // VariableNeededSize is the most proper variable size, allocate variable buffer
    // ItemIndex now points to the next PLATFORM_OVERRIDE_ITEM which is not covered by VariableNeededSize
    //
    VariableBuffer = AllocateZeroPool (VariableNeededSize);
    ASSERT (VariableBuffer != NULL);
    ASSERT ((UINTN) VariableBuffer % sizeof(UINTN) == 0);

    //
    // Fill the variable buffer according to MappingDataBase
    //
    VariableIndex = VariableBuffer;
    *(UINT32 *) VariableIndex = NotEnd;
    VariableIndex += sizeof (UINT32); // pass NotEnd
    //
    // ItemIndex points to the next PLATFORM_OVERRIDE_ITEM which is not covered by VariableNeededSize
    //
    while (OverrideItemListIndex != ItemIndex){
      *(UINT32 *) VariableIndex = PLATFORM_OVERRIDE_ITEM_SIGNATURE;
      VariableIndex += sizeof (UINT32); // pass SIGNATURE

      OverrideItem = CR(OverrideItemListIndex, PLATFORM_OVERRIDE_ITEM, Link, PLATFORM_OVERRIDE_ITEM_SIGNATURE);
      *(UINT32 *) VariableIndex = OverrideItem->DriverInfoNum;
      VariableIndex += sizeof (UINT32); // pass DriverNum

      CopyMem (VariableIndex, OverrideItem->ControllerDevicePath, GetDevicePathSize (OverrideItem->ControllerDevicePath));
      VariableIndex += GetDevicePathSize (OverrideItem->ControllerDevicePath); // pass ControllerDevicePath

      //
      // Align the VariableIndex since the controller device path may not be aligned
      //
      VariableIndex += ((sizeof(UINT32) - ((UINTN) (VariableIndex))) & (sizeof(UINT32) - 1));
      //
      // Save the Driver Info List of this PLATFORM_OVERRIDE_ITEM
      //
      ImageInfoListIndex = GetFirstNode (&OverrideItem->DriverInfoList);
      while (!IsNull (&OverrideItem->DriverInfoList, ImageInfoListIndex)) {
        DriverImageInfo = CR(ImageInfoListIndex, DRIVER_IMAGE_INFO, Link, DRIVER_IMAGE_INFO_SIGNATURE);
        CopyMem (VariableIndex, DriverImageInfo->DriverImagePath, GetDevicePathSize (DriverImageInfo->DriverImagePath));
        VariableIndex += GetDevicePathSize (DriverImageInfo->DriverImagePath); // pass DriverImageDevicePath
        //
        // Align the VariableIndex since the driver image device path may not be aligned
        //
        VariableIndex += ((sizeof(UINT32) - ((UINTN) (VariableIndex))) & (sizeof(UINT32) - 1));
        ImageInfoListIndex = GetNextNode (&OverrideItem->DriverInfoList, ImageInfoListIndex);
      }

      OverrideItemListIndex =  GetNextNode (MappingDataBase, OverrideItemListIndex);
    }

    ASSERT (((UINTN)VariableIndex - (UINTN)VariableBuffer) == VariableNeededSize);

    if (NumIndex == 0) {
      UnicodeSPrint (OverrideVariableName, sizeof (OverrideVariableName), L"PlatDriOver");
    } else {
      UnicodeSPrint (OverrideVariableName, sizeof (OverrideVariableName), L"PlatDriOver%d", NumIndex );
    }

    Status = gRT->SetVariable (
                    OverrideVariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    VariableNeededSize,
                    VariableBuffer
                    );
    FreePool (VariableBuffer);

    if (EFI_ERROR (Status)) {
      if (NumIndex > 0) {
        //
        // Delete all PlatDriOver variables when full mapping can't be set.  
        //
        DeleteOverridesVariables ();
      }
      return Status;
    }

    NumIndex ++;
  }

  return EFI_SUCCESS;
}

/**
  Get the first Binding protocol which has the specific image handle.

  @param  ImageHandle          The Image handle
  @param  BindingHandle        The BindingHandle of the found Driver Binding protocol.
                               If Binding protocol is not found, it is set to NULL. 

  @return                      Pointer into the Binding Protocol interface
  @retval NULL                 The paramter is not valid or the binding protocol is not found.

**/
EFI_DRIVER_BINDING_PROTOCOL *
EFIAPI
GetBindingProtocolFromImageHandle (
  IN  EFI_HANDLE   ImageHandle,
  OUT EFI_HANDLE   *BindingHandle
  )
{
  EFI_STATUS                        Status;
  UINTN                             Index;
  UINTN                             DriverBindingHandleCount;
  EFI_HANDLE                        *DriverBindingHandleBuffer;
  EFI_DRIVER_BINDING_PROTOCOL       *DriverBindingInterface;

  if (BindingHandle == NULL || ImageHandle == NULL) {
    return NULL;
  }
  //
  // Get all drivers which support driver binding protocol
  //
  DriverBindingHandleCount  = 0;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDriverBindingProtocolGuid,
                  NULL,
                  &DriverBindingHandleCount,
                  &DriverBindingHandleBuffer
                  );
  if (EFI_ERROR (Status) || (DriverBindingHandleCount == 0)) {
    return NULL;
  }

  for (Index = 0; Index < DriverBindingHandleCount; Index++) {
    DriverBindingInterface = NULL;
    Status = gBS->OpenProtocol (
                    DriverBindingHandleBuffer[Index],
                    &gEfiDriverBindingProtocolGuid,
                    (VOID **) &DriverBindingInterface,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (DriverBindingInterface->ImageHandle == ImageHandle) {
      *BindingHandle = DriverBindingHandleBuffer[Index];
      FreePool (DriverBindingHandleBuffer);
      return DriverBindingInterface;
    }
  }

  //
  // If no Driver Binding Protocol instance is found
  //
  FreePool (DriverBindingHandleBuffer);
  *BindingHandle = NULL;
  return NULL;
}

/**
  Return the current TPL.

  @return Current TPL

**/
EFI_TPL
GetCurrentTpl (
  VOID
  )
{
  EFI_TPL                 Tpl;

  Tpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (Tpl);

  return Tpl;
}


/**
  Retrieves the image handle of the platform override driver for a controller in
  the system from the memory mapping database.

  @param  ControllerHandle         The device handle of the controller to check if
                                   a driver override exists.
  @param  DriverImageHandle        On input, the previously returnd driver image handle.
                                   On output, a pointer to the next driver handle.
                                   Passing in a pointer to NULL, will return the
                                   first driver handle for ControllerHandle.
  @param  MappingDataBase          Mapping database list entry pointer
  @param  CallerImageHandle        The caller driver's image handle, for
                                   UpdateFvFileDevicePath use.

  @retval EFI_INVALID_PARAMETER    The handle specified by ControllerHandle is not
                                   a valid handle.  Or DriverImagePath is not a
                                   device path that was returned on a previous call
                                   to GetDriverPath().
  @retval EFI_NOT_FOUND            A driver override for ControllerHandle was not
                                   found.
  @retval EFI_UNSUPPORTED          The operation is not supported.
  @retval EFI_SUCCESS              The driver override for ControllerHandle was
                                   returned in DriverImagePath.

**/
EFI_STATUS
EFIAPI
GetDriverFromMapping (
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_HANDLE                                     *DriverImageHandle,
  IN     LIST_ENTRY                                     *MappingDataBase,
  IN     EFI_HANDLE                                     CallerImageHandle
  )
{
  EFI_STATUS                  Status;
  EFI_DEVICE_PATH_PROTOCOL    *ControllerDevicePath;
  BOOLEAN                     ControllerFound;
  BOOLEAN                     ImageFound;
  EFI_HANDLE                  *ImageHandleBuffer;
  UINTN                       ImageHandleCount;
  UINTN                       Index;
  EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;
  EFI_HANDLE                  DriverBindingHandle;
  BOOLEAN                     FoundLastReturned;
  PLATFORM_OVERRIDE_ITEM      *OverrideItem;
  DRIVER_IMAGE_INFO           *DriverImageInfo;
  LIST_ENTRY                  *OverrideItemListIndex;
  LIST_ENTRY                  *ImageInfoListIndex;
  EFI_DEVICE_PATH_PROTOCOL    *TempDriverImagePath;
  EFI_HANDLE                  ImageHandle;
  EFI_HANDLE                  Handle;
  EFI_DEVICE_PATH_PROTOCOL    *LoadedImageDevicePath;
  EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL  *BusSpecificDriverOverride;
  UINTN                       DevicePathSize;

  //
  // Check that ControllerHandle is a valid handle
  //
  if (ControllerHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Get the device path of ControllerHandle
  //
  Status = gBS->HandleProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ControllerDevicePath
                  );
  if (EFI_ERROR (Status) || ControllerDevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Search ControllerDevicePath in MappingDataBase
  //
  OverrideItem = NULL;
  ControllerFound = FALSE;
  DevicePathSize = GetDevicePathSize (ControllerDevicePath);

  OverrideItemListIndex = GetFirstNode (MappingDataBase);
  while (!IsNull (MappingDataBase, OverrideItemListIndex)) {
    OverrideItem = CR(OverrideItemListIndex, PLATFORM_OVERRIDE_ITEM, Link, PLATFORM_OVERRIDE_ITEM_SIGNATURE);
    if (DevicePathSize == GetDevicePathSize (OverrideItem->ControllerDevicePath)) {
      if (CompareMem (
            ControllerDevicePath,
            OverrideItem->ControllerDevicePath,
            DevicePathSize
            ) == 0
          ) {
        ControllerFound = TRUE;
        break;
      }
    }
    OverrideItemListIndex = GetNextNode (MappingDataBase, OverrideItemListIndex);
  }

  if (!ControllerFound) {
    return EFI_NOT_FOUND;
  }
  //
  // Passing in a pointer to NULL, will return the first driver device path for ControllerHandle.
  // Check whether the driverImagePath is not a device path that was returned on a previous call to GetDriverPath().
  //
  if (*DriverImageHandle != NULL) {
    if (*DriverImageHandle != OverrideItem->LastReturnedImageHandle) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // The GetDriverPath() may be called recursively, because it use ConnectDevicePath() internally,
  //  so should check whether there is a dead loop.
  //  Here use a controller device path stack to record all processed controller device path during a GetDriverPath() call,
  //  and check the controller device path whether appear again during the GetDriverPath() call.
  //
  if (CheckExistInStack (OverrideItem->ControllerDevicePath)) {
    //
    // There is a dependecy dead loop if the ControllerDevicePath appear in stack twice
    //
    return EFI_UNSUPPORTED;
  }
  PushDevPathStack (OverrideItem->ControllerDevicePath);

  //
  // Check every override driver, try to load and start them
  //
  ImageInfoListIndex = GetFirstNode (&OverrideItem->DriverInfoList);
  while (!IsNull (&OverrideItem->DriverInfoList, ImageInfoListIndex)) {
    DriverImageInfo = CR(ImageInfoListIndex, DRIVER_IMAGE_INFO, Link, DRIVER_IMAGE_INFO_SIGNATURE);
    if (DriverImageInfo->ImageHandle == NULL) {
      //
      // Skip if the image is unloadable or unstartable
      //
      if ((!DriverImageInfo->UnLoadable) && ((!DriverImageInfo->UnStartable))) {
        TempDriverImagePath = DriverImageInfo->DriverImagePath;
        //
        // If the image device path contains an FV node, check the FV file device path is valid.
        // If it is invalid, try to return the valid device path.
        // FV address maybe changes for memory layout adjust from time to time,
        // use this funciton could promise the FV file device path is right.
        //
        Status = UpdateFvFileDevicePath (&TempDriverImagePath, NULL, CallerImageHandle);
        if (!EFI_ERROR (Status)) {
          FreePool (DriverImageInfo->DriverImagePath);
          DriverImageInfo->DriverImagePath = TempDriverImagePath;
        }
        //
        // Get all Loaded Image protocol to check whether the driver image has been loaded and started
        //
        ImageFound = FALSE;
        ImageHandleCount  = 0;
        Status = gBS->LocateHandleBuffer (
                        ByProtocol,
                        &gEfiLoadedImageProtocolGuid,
                        NULL,
                        &ImageHandleCount,
                        &ImageHandleBuffer
                        );
        if (EFI_ERROR (Status) || (ImageHandleCount == 0)) {
          return EFI_NOT_FOUND;
        }

        for(Index = 0; Index < ImageHandleCount; Index ++) {
          //
          // Get the EFI Loaded Image Device Path Protocol
          //
          LoadedImageDevicePath = NULL;
          Status = gBS->HandleProtocol (
                          ImageHandleBuffer[Index],
                          &gEfiLoadedImageDevicePathProtocolGuid,
                          (VOID **) &LoadedImageDevicePath
                          );
          if (EFI_ERROR (Status)) {
            //
            // Maybe not all EFI_LOADED_IMAGE_DEVICE_PATH_PROTOCOL existed.
            //
            continue;
          }

          DevicePathSize = GetDevicePathSize (DriverImageInfo->DriverImagePath);
          if (DevicePathSize == GetDevicePathSize (LoadedImageDevicePath)) {
            if (CompareMem (
                  DriverImageInfo->DriverImagePath,
                  LoadedImageDevicePath,
                  GetDevicePathSize (LoadedImageDevicePath)
                  ) == 0
                ) {
              ImageFound = TRUE;
              break;
            }
          }
        }

        if (ImageFound) {
          //
          // Find its related driver binding protocol
          // Driver binding handle may be different with its driver's Image Handle.
          //
          DriverBindingHandle = NULL;
          DriverBinding = GetBindingProtocolFromImageHandle (
                            ImageHandleBuffer[Index],
                            &DriverBindingHandle
                            );
          ASSERT (DriverBinding != NULL);
          DriverImageInfo->ImageHandle = ImageHandleBuffer[Index];
        } else if (GetCurrentTpl() <= TPL_CALLBACK){
          //
          // The driver image has not been loaded and started. Try to load and start it now.
          // Try to connect all device in the driver image path.
          //
          // Note: LoadImage() and  StartImage() should be called under CALLBACK TPL in theory, but
          // since many device need to be connected in  CALLBACK level environment( e.g. Usb devices )
          // and the Fat and Patition driver can endure executing in CALLBACK level in fact, so here permit
          // to use LoadImage() and  StartImage() in CALLBACK TPL.
          //
          Status = ConnectDevicePath (DriverImageInfo->DriverImagePath);
          //
          // check whether it points to a PCI Option Rom image,
          // and try to use bus override protocol to get its first option rom image driver
          //
          TempDriverImagePath = DriverImageInfo->DriverImagePath;
          gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &TempDriverImagePath, &Handle);
          //
          // Get the Bus Specific Driver Override Protocol instance on the Controller Handle
          //
          Status = gBS->HandleProtocol(
                          Handle,
                          &gEfiBusSpecificDriverOverrideProtocolGuid,
                          (VOID **) &BusSpecificDriverOverride
                          );
          if (!EFI_ERROR (Status) && (BusSpecificDriverOverride != NULL)) {
            ImageHandle = NULL;
            Status = BusSpecificDriverOverride->GetDriver (
                                                  BusSpecificDriverOverride,
                                                  &ImageHandle
                                                  );
            if (!EFI_ERROR (Status)) {
              //
              // Find its related driver binding protocol
              // Driver binding handle may be different with its driver's Image handle
              //
              DriverBindingHandle = NULL;
              DriverBinding = GetBindingProtocolFromImageHandle (
                                ImageHandle,
                                &DriverBindingHandle
                                );
              ASSERT (DriverBinding != NULL);
              DriverImageInfo->ImageHandle = ImageHandle;
            }
          }
          //
          // Skip if any device cannot be connected now, future passes through GetDriver() may be able to load that driver.
          // Only file path media or FwVol Device Path Node remain if all device is connected
          //
          TempDriverImagePath = DriverImageInfo->DriverImagePath;
          gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &TempDriverImagePath, &Handle);
          if (((DevicePathType (TempDriverImagePath) == MEDIA_DEVICE_PATH) &&
               (DevicePathSubType (TempDriverImagePath) == MEDIA_FILEPATH_DP)) ||
              (EfiGetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) TempDriverImagePath) != NULL)
             ) {
            //
            // Try to load the driver
            //
            TempDriverImagePath = DriverImageInfo->DriverImagePath;
            Status = gBS->LoadImage (
                            FALSE,
                            CallerImageHandle,
                            TempDriverImagePath,
                            NULL,
                            0,
                            &ImageHandle
                            );
            if (!EFI_ERROR (Status)) {
              //
              // Try to start the driver
              //
              Status = gBS->StartImage (ImageHandle, NULL, NULL);
              if (EFI_ERROR (Status)){
                DriverImageInfo->UnStartable = TRUE;
                DriverImageInfo->ImageHandle = NULL;
              } else {
                //
                // Find its related driver binding protocol
                // Driver binding handle may be different with its driver's Image handle
                //
                DriverBindingHandle = NULL;
                DriverBinding = GetBindingProtocolFromImageHandle (
                                   ImageHandle,
                                   &DriverBindingHandle
                                   );
                ASSERT (DriverBinding != NULL);
                DriverImageInfo->ImageHandle = ImageHandle;
              }
            } else {
              DriverImageInfo->UnLoadable = TRUE;
              DriverImageInfo->ImageHandle = NULL;
            }
          }
        }
        FreePool (ImageHandleBuffer);
      }
    }
    ImageInfoListIndex = GetNextNode (&OverrideItem->DriverInfoList, ImageInfoListIndex);
  }
  //
  // Finish try to load and start the override driver of a controller, popup the controller's device path
  //
  PopDevPathStack (NULL);

  //
  // return the DriverImageHandle for ControllerHandle
  //
  FoundLastReturned = FALSE;
  ImageInfoListIndex = GetFirstNode (&OverrideItem->DriverInfoList);
  while (!IsNull (&OverrideItem->DriverInfoList, ImageInfoListIndex)) {
    DriverImageInfo = CR(ImageInfoListIndex, DRIVER_IMAGE_INFO, Link, DRIVER_IMAGE_INFO_SIGNATURE);
    if (DriverImageInfo->ImageHandle != NULL) {
      if ((*DriverImageHandle == NULL) || FoundLastReturned) {
        //
        // If DriverImageHandle is NULL, then we just need to return the first driver.
        // If FoundLastReturned, this means we have just encountered the previously returned driver.
        // For both cases, we just return the image handle of this driver.
        //
        OverrideItem->LastReturnedImageHandle = DriverImageInfo->ImageHandle;
        *DriverImageHandle = DriverImageInfo->ImageHandle;
        return EFI_SUCCESS;
      } else if (*DriverImageHandle == DriverImageInfo->ImageHandle){
        //
        // We have found the previously returned driver.
        //
        FoundLastReturned = TRUE;
      }
    }
    ImageInfoListIndex = GetNextNode (&OverrideItem->DriverInfoList, ImageInfoListIndex);
  }

  return EFI_NOT_FOUND;
}


/**
  Check mapping database whether already has the mapping info which
  records the input Controller to input DriverImage.

  @param  ControllerDevicePath     The controller device path is to be check.
  @param  DriverImageDevicePath    The driver image device path is to be check.
  @param  MappingDataBase          Mapping database list entry pointer
  @param  DriverInfoNum            the controller's total override driver number
  @param  DriverImageNO            The driver order number for the input DriverImage.
                                   If the DriverImageDevicePath is NULL, DriverImageNO is not set.

  @retval EFI_INVALID_PARAMETER    ControllerDevicePath or MappingDataBase is NULL.
  @retval EFI_NOT_FOUND            ControllerDevicePath is not found in MappingDataBase or
                                   DriverImageDevicePath is not found in the found DriverImage Info list. 
  @retval EFI_SUCCESS              The controller's total override driver number and 
                                   input DriverImage's order number is correctly return.
**/
EFI_STATUS
EFIAPI
CheckMapping (
  IN     EFI_DEVICE_PATH_PROTOCOL                       *ControllerDevicePath,
  IN     EFI_DEVICE_PATH_PROTOCOL                       *DriverImageDevicePath  OPTIONAL,
  IN     LIST_ENTRY                                     *MappingDataBase,
  OUT    UINT32                                         *DriverInfoNum  OPTIONAL,
  OUT    UINT32                                         *DriverImageNO  OPTIONAL
  )
{
  LIST_ENTRY                  *OverrideItemListIndex;
  PLATFORM_OVERRIDE_ITEM      *OverrideItem;
  LIST_ENTRY                  *ImageInfoListIndex;
  DRIVER_IMAGE_INFO           *DriverImageInfo;
  BOOLEAN                     Found;
  UINT32                      ImageNO;
  UINTN                       DevicePathSize;

  if (ControllerDevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (MappingDataBase == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Search ControllerDevicePath in MappingDataBase
  //
  Found = FALSE;
  OverrideItem = NULL;
  OverrideItemListIndex = GetFirstNode (MappingDataBase);
  while (!IsNull (MappingDataBase, OverrideItemListIndex)) {
    OverrideItem = CR(OverrideItemListIndex, PLATFORM_OVERRIDE_ITEM, Link, PLATFORM_OVERRIDE_ITEM_SIGNATURE);
    DevicePathSize = GetDevicePathSize (ControllerDevicePath);
    if (DevicePathSize == GetDevicePathSize (OverrideItem->ControllerDevicePath)) {
      if (CompareMem (
            ControllerDevicePath,
            OverrideItem->ControllerDevicePath,
            DevicePathSize
            ) == 0
          ) {
        Found = TRUE;
        break;
      }
    }
    OverrideItemListIndex = GetNextNode (MappingDataBase, OverrideItemListIndex);
  }

  if (!Found) {
    //
    // ControllerDevicePath is not in MappingDataBase
    //
    return EFI_NOT_FOUND;
  }

  ASSERT (OverrideItem->DriverInfoNum != 0);
  if (DriverInfoNum != NULL) {
    *DriverInfoNum = OverrideItem->DriverInfoNum;
  }

  //
  // If DriverImageDevicePath is NULL, skip checking DriverImageDevicePath
  // in the controller's Driver Image Info List
  //
  if (DriverImageDevicePath == NULL) {
    return EFI_SUCCESS;
  }
  //
  // return the DriverImageHandle for ControllerHandle
  //
  ImageNO = 0;
  Found = FALSE;
  ImageInfoListIndex = GetFirstNode (&OverrideItem->DriverInfoList);
  while (!IsNull (&OverrideItem->DriverInfoList, ImageInfoListIndex)) {
    DriverImageInfo = CR(ImageInfoListIndex, DRIVER_IMAGE_INFO, Link, DRIVER_IMAGE_INFO_SIGNATURE);
    ImageNO++;
    DevicePathSize = GetDevicePathSize (DriverImageDevicePath);
    if (DevicePathSize == GetDevicePathSize (DriverImageInfo->DriverImagePath)) {
      if (CompareMem (
            DriverImageDevicePath,
            DriverImageInfo->DriverImagePath,
            GetDevicePathSize (DriverImageInfo->DriverImagePath)
            ) == 0
          ) {
        Found = TRUE;
        break;
      }
    }
    ImageInfoListIndex = GetNextNode (&OverrideItem->DriverInfoList, ImageInfoListIndex);
  }

  if (!Found) {
    //
    // DriverImageDevicePath is not found in the controller's Driver Image Info List
    //
    return EFI_NOT_FOUND;
  } else {
    if (DriverImageNO != NULL) {
      *DriverImageNO = ImageNO;
    }
    return EFI_SUCCESS;
  }
}


/**
  Insert a driver image as a controller's override driver into the mapping database.
  The driver image's order number is indicated by DriverImageNO.

  @param  ControllerDevicePath     The controller device path need to add a
                                   override driver image item
  @param  DriverImageDevicePath    The driver image device path need to be insert
  @param  MappingDataBase          Mapping database list entry pointer
  @param  DriverImageNO            The inserted order number. If this number is taken, 
                                   the larger available number will be used.

  @retval EFI_INVALID_PARAMETER    ControllerDevicePath is NULL, or DriverImageDevicePath is NULL
                                   or MappingDataBase is NULL
  @retval EFI_ALREADY_STARTED      The input Controller to input DriverImage has been 
                                   recorded into the mapping database.
  @retval EFI_SUCCESS              The Controller and DriverImage are inserted into 
                                   the mapping database successfully.

**/
EFI_STATUS
EFIAPI
InsertDriverImage (
  IN     EFI_DEVICE_PATH_PROTOCOL                       *ControllerDevicePath,
  IN     EFI_DEVICE_PATH_PROTOCOL                       *DriverImageDevicePath,
  IN     LIST_ENTRY                                     *MappingDataBase,
  IN     UINT32                                         DriverImageNO
  )
{
  EFI_STATUS                  Status;
  LIST_ENTRY                  *OverrideItemListIndex;
  PLATFORM_OVERRIDE_ITEM      *OverrideItem;
  LIST_ENTRY                  *ImageInfoListIndex;
  DRIVER_IMAGE_INFO           *DriverImageInfo;
  BOOLEAN                     Found;
  UINT32                      ImageNO;
  UINTN                       DevicePathSize;

  if (ControllerDevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (DriverImageDevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (MappingDataBase == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If the driver is already in the controller's Driver Image Info List,
  // just return EFI_ALREADY_STARTED.
  //
  Status = CheckMapping (
             ControllerDevicePath,
             DriverImageDevicePath,
             MappingDataBase,
             NULL,
             NULL
             );
  if (Status == EFI_SUCCESS) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Search the input ControllerDevicePath in MappingDataBase
  //
  Found = FALSE;
  OverrideItem = NULL;
  OverrideItemListIndex = GetFirstNode (MappingDataBase);
  while (!IsNull (MappingDataBase, OverrideItemListIndex)) {
    OverrideItem = CR(OverrideItemListIndex, PLATFORM_OVERRIDE_ITEM, Link, PLATFORM_OVERRIDE_ITEM_SIGNATURE);
    DevicePathSize = GetDevicePathSize (ControllerDevicePath);
    if (DevicePathSize == GetDevicePathSize (OverrideItem->ControllerDevicePath)) {
      if (CompareMem (
            ControllerDevicePath,
            OverrideItem->ControllerDevicePath,
            DevicePathSize
            ) == 0
          ) {
        Found = TRUE;
        break;
      }
    }
    OverrideItemListIndex = GetNextNode (MappingDataBase, OverrideItemListIndex);
  }
  //
  // If cannot find, this is a new controller item
  // Add the Controller related PLATFORM_OVERRIDE_ITEM structrue in mapping data base
  //
  if (!Found) {
    OverrideItem = AllocateZeroPool (sizeof (PLATFORM_OVERRIDE_ITEM));
    ASSERT (OverrideItem != NULL);
    OverrideItem->Signature = PLATFORM_OVERRIDE_ITEM_SIGNATURE;
    OverrideItem->ControllerDevicePath = DuplicateDevicePath (ControllerDevicePath);
    InitializeListHead (&OverrideItem->DriverInfoList);
    InsertTailList (MappingDataBase, &OverrideItem->Link);
  }

  //
  // Prepare the driver image related DRIVER_IMAGE_INFO structure.
  //
  DriverImageInfo = AllocateZeroPool (sizeof (DRIVER_IMAGE_INFO));
  ASSERT (DriverImageInfo != NULL);
  DriverImageInfo->Signature = DRIVER_IMAGE_INFO_SIGNATURE;
  DriverImageInfo->DriverImagePath = DuplicateDevicePath (DriverImageDevicePath);
  //
  // Find the driver image wanted order location
  //
  ImageNO = 0;
  Found = FALSE;
  ImageInfoListIndex = GetFirstNode (&OverrideItem->DriverInfoList);
  while (!IsNull (&OverrideItem->DriverInfoList, ImageInfoListIndex)) {
    if (ImageNO == (DriverImageNO - 1)) {
      //
      // find the wanted order location, insert it
      //
      InsertTailList (ImageInfoListIndex, &DriverImageInfo->Link);
      OverrideItem->DriverInfoNum ++;
      Found = TRUE;
      break;
    }
    ImageNO++;
    ImageInfoListIndex = GetNextNode (&OverrideItem->DriverInfoList, ImageInfoListIndex);
  }

  if (!Found) {
    //
    // if not find the wanted order location, add it as last item of the controller mapping item
    //
    InsertTailList (&OverrideItem->DriverInfoList, &DriverImageInfo->Link);
    OverrideItem->DriverInfoNum ++;
  }

  return EFI_SUCCESS;
}


/**
  Delete a controller's override driver from the mapping database.

  @param  ControllerDevicePath     The controller device path will be deleted 
                                   when all drivers images on it are removed.
  @param  DriverImageDevicePath    The driver image device path will be delete.
                                   If NULL, all driver image will be delete.
  @param  MappingDataBase          Mapping database list entry pointer

  @retval EFI_INVALID_PARAMETER    ControllerDevicePath is NULL, or MappingDataBase is NULL
  @retval EFI_NOT_FOUND            ControllerDevicePath is not found in MappingDataBase or
                                   DriverImageDevicePath is not found in the found DriverImage Info list. 
  @retval EFI_SUCCESS              Delete the specified driver successfully.

**/
EFI_STATUS
EFIAPI
DeleteDriverImage (
  IN     EFI_DEVICE_PATH_PROTOCOL                       *ControllerDevicePath,
  IN     EFI_DEVICE_PATH_PROTOCOL                       *DriverImageDevicePath,
  IN     LIST_ENTRY                                     *MappingDataBase
  )
{
  EFI_STATUS                  Status;
  LIST_ENTRY                  *OverrideItemListIndex;
  PLATFORM_OVERRIDE_ITEM      *OverrideItem;
  LIST_ENTRY                  *ImageInfoListIndex;
  DRIVER_IMAGE_INFO           *DriverImageInfo;
  BOOLEAN                     Found;
  UINTN                       DevicePathSize;

  if (ControllerDevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (MappingDataBase == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If ControllerDevicePath is not found in mapping database, return EFI_NOT_FOUND.
  //
  Status = CheckMapping (
             ControllerDevicePath,
             DriverImageDevicePath,
             MappingDataBase,
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Search ControllerDevicePath in MappingDataBase
  //
  Found = FALSE;
  OverrideItem = NULL;
  OverrideItemListIndex = GetFirstNode (MappingDataBase);
  while (!IsNull (MappingDataBase, OverrideItemListIndex)) {
    OverrideItem = CR(OverrideItemListIndex, PLATFORM_OVERRIDE_ITEM, Link, PLATFORM_OVERRIDE_ITEM_SIGNATURE);
    DevicePathSize = GetDevicePathSize (ControllerDevicePath);
    if (DevicePathSize == GetDevicePathSize (OverrideItem->ControllerDevicePath)) {
      if (CompareMem (
            ControllerDevicePath,
            OverrideItem->ControllerDevicePath,
            DevicePathSize
            ) == 0
          ) {
        Found = TRUE;
        break;
      }
    }
    OverrideItemListIndex = GetNextNode (MappingDataBase, OverrideItemListIndex);
  }

  ASSERT (Found);
  ASSERT (OverrideItem->DriverInfoNum != 0);

  Found = FALSE;
  ImageInfoListIndex = GetFirstNode (&OverrideItem->DriverInfoList);
  while (!IsNull (&OverrideItem->DriverInfoList, ImageInfoListIndex)) {
    DriverImageInfo = CR(ImageInfoListIndex, DRIVER_IMAGE_INFO, Link, DRIVER_IMAGE_INFO_SIGNATURE);
    ImageInfoListIndex = GetNextNode (&OverrideItem->DriverInfoList, ImageInfoListIndex);
    if (DriverImageDevicePath != NULL) {
      //
      // Search for the specified DriverImageDevicePath and remove it, then break.
      //
      DevicePathSize = GetDevicePathSize (DriverImageDevicePath);
      if (DevicePathSize == GetDevicePathSize (DriverImageInfo->DriverImagePath)) {
        if (CompareMem (
              DriverImageDevicePath,
              DriverImageInfo->DriverImagePath,
              GetDevicePathSize (DriverImageInfo->DriverImagePath)
              ) == 0
            ) {
          Found = TRUE;
          FreePool(DriverImageInfo->DriverImagePath);
          RemoveEntryList (&DriverImageInfo->Link);
          OverrideItem->DriverInfoNum --;
          break;
        }
      }
    } else {
      //
      // Remove all existing driver image info entries, so no break here.
      //
      Found = TRUE;
      FreePool(DriverImageInfo->DriverImagePath);
      RemoveEntryList (&DriverImageInfo->Link);
      OverrideItem->DriverInfoNum --;
    }
  }

  //
  // Confirm all driver image info entries have been removed,
  // if DriverImageDevicePath is NULL.
  //
  if (DriverImageDevicePath == NULL) {
    ASSERT (OverrideItem->DriverInfoNum == 0);
  }
  //
  // If Override Item has no driver image info entry, then delete this item.
  //
  if (OverrideItem->DriverInfoNum == 0) {
    FreePool(OverrideItem->ControllerDevicePath);
    RemoveEntryList (&OverrideItem->Link);
    FreePool (OverrideItem);
  }

  if (!Found) {
    //
    // DriverImageDevicePath is not NULL and cannot be found in the controller's
    // driver image info list.
    //
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
