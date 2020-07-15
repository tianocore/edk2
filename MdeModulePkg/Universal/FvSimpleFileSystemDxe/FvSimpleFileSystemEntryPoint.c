/** @file
  This driver uses the EFI_FIRMWARE_VOLUME2_PROTOCOL to expose files in firmware
  volumes via the the EFI_SIMPLE_FILESYSTEM_PROTOCOL and EFI_FILE_PROTOCOL.

  It will expose a single directory, containing one file for each file in the firmware
  volume. If a file has a UI section, its contents will be used as a filename.
  Otherwise, a string representation of the GUID will be used.
  Files of an executable type (That is PEIM, DRIVER, COMBINED_PEIM_DRIVER and APPLICATION)
  will have ".efi" added to their filename.

  Its primary intended use is to be able to start EFI applications embedded in FVs
  from the UEFI shell. It is entirely read-only.

Copyright (c) 2014, ARM Limited. All rights reserved.
Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FvSimpleFileSystemInternal.h"

EFI_UNICODE_COLLATION_PROTOCOL          *mUnicodeCollation = NULL;

//
// A Guid string is 32 hex characters with 4 hyphens and a NULL-terminated char: 37 characters total
//
#define GUID_STRING_SIZE                (37 * sizeof (CHAR16))

#define FVFS_VOLUME_LABEL_PREFIX        L"Firmware Volume: "
#define FVFS_VOLUME_LABEL_SIZE          (sizeof (FVFS_VOLUME_LABEL_PREFIX) + GUID_STRING_SIZE - sizeof (CHAR16))
#define FVFS_FALLBACK_VOLUME_LABEL      L"Firmware Volume"

//
// Template for EFI_SIMPLE_FILE_SYSTEM_PROTOCOL data structure.
//
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL mSimpleFsTemplate = {
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION,
  FvSimpleFileSystemOpenVolume
};

//
// Template for EFI_DRIVER_BINDING_PROTOCOL data structure.
//
EFI_DRIVER_BINDING_PROTOCOL mDriverBinding = {
  FvSimpleFileSystemDriverSupported,
  FvSimpleFileSystemDriverStart,
  FvSimpleFileSystemDriverStop,
  0,
  NULL,
  NULL
};

/**
  Open the root directory on a volume.

  @param  This     A pointer to the volume to open the root directory.
  @param  RootFile A pointer to the location to return the opened file handle for the
                   root directory.

  @retval EFI_SUCCESS          The device was opened.
  @retval EFI_UNSUPPORTED      This volume does not support the requested file system type.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_ACCESS_DENIED    The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES The volume was not opened due to lack of resources.
  @retval EFI_MEDIA_CHANGED    The device has a different medium in it or the medium is no
                               longer supported. Any existing file handles for this volume are
                               no longer valid. To access the files on the new medium, the
                               volume must be reopened with OpenVolume().

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemOpenVolume (
  IN     EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
     OUT EFI_FILE_PROTOCOL               **RootFile
  )
{
  EFI_STATUS                      Status;
  FV_FILESYSTEM_FILE              *Root;
  CHAR16                          *UiSection;
  EFI_GUID                        NameGuid;
  EFI_FV_FILE_ATTRIBUTES          Attributes;
  UINT32                          Authentication;
  UINTN                           Key;
  EFI_FV_FILETYPE                 FileType;
  UINTN                           Size;
  FV_FILESYSTEM_INSTANCE          *Instance;
  FV_FILESYSTEM_FILE_INFO         *FvFileInfo;
  EFI_FIRMWARE_VOLUME2_PROTOCOL   *FvProtocol;
  CHAR16                          *Name;
  UINTN                           NameLen;
  UINTN                           NumChars;
  UINTN                           DestMax;

  Instance = FVFS_INSTANCE_FROM_SIMPLE_FS_THIS (This);
  Status = EFI_SUCCESS;

  if (Instance->Root == NULL) {
    //
    // Allocate file structure for root file
    //
    Root = AllocateZeroPool (sizeof (FV_FILESYSTEM_FILE));
    if (Root == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Instance->Root  = Root;
    Root->Instance  = Instance;
    Root->Signature = FVFS_FILE_SIGNATURE;
    CopyMem (&Root->FileProtocol, &mFileSystemTemplate, sizeof (mFileSystemTemplate));
    Root->FvFileInfo = AllocateZeroPool (sizeof (FV_FILESYSTEM_FILE_INFO));
    if (Root->FvFileInfo == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }
    Root->FvFileInfo->FileInfo.Size      = sizeof (EFI_FILE_INFO);
    Root->FvFileInfo->FileInfo.Attribute = EFI_FILE_DIRECTORY | EFI_FILE_READ_ONLY;

    //
    // Populate the instance's list of files. We consider anything a file that
    // has a UI_SECTION, which we consider to be its filename.
    //
    FvProtocol = Instance->FvProtocol;
    //
    // Allocate Key
    //
    Key = 0;

    do {
      FileType = EFI_FV_FILETYPE_ALL;

      Status = FvProtocol->GetNextFile (
                             FvProtocol,
                             &Key,
                             &FileType,
                             &NameGuid,
                             &Attributes,
                             &Size
                             );
      if (EFI_ERROR (Status)) {
        ASSERT (Status == EFI_NOT_FOUND);
        break;
      }

      //
      // Get a file's name: If it has a UI section, use that, otherwise use
      // its NameGuid.
      //
      UiSection = NULL;
      Status = FvProtocol->ReadSection (
                             FvProtocol,
                             &NameGuid,
                             EFI_SECTION_USER_INTERFACE,
                             0,
                             (VOID **)&UiSection,
                             &Size,
                             &Authentication
                             );
      if (!EFI_ERROR (Status)) {
        Name = UiSection;
      } else {
        Name = AllocateZeroPool (GUID_STRING_SIZE);
        if (Name == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }
        NumChars = UnicodeSPrint (Name, GUID_STRING_SIZE, L"%g", &NameGuid);
        ASSERT ((NumChars + 1) * sizeof (CHAR16) == GUID_STRING_SIZE);
      }

      //
      // Found a file.
      // Allocate a file structure and populate it.
      //
      NameLen = StrSize (Name);
      if (FV_FILETYPE_IS_EXECUTABLE (FileType)) {
        NameLen += StrSize (L".efi") - sizeof (CHAR16);
      }

      FvFileInfo = AllocateZeroPool (sizeof (FV_FILESYSTEM_FILE_INFO) + NameLen - sizeof (CHAR16));
      if (FvFileInfo == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      FvFileInfo->Signature = FVFS_FILE_INFO_SIGNATURE;
      InitializeListHead (&FvFileInfo->Link);
      CopyMem (&FvFileInfo->NameGuid, &NameGuid, sizeof (EFI_GUID));
      FvFileInfo->Type = FileType;

      //
      // Add ".efi" to filenames of drivers and applications.
      //
      DestMax = NameLen / sizeof (CHAR16);
      Status  = StrnCpyS (&FvFileInfo->FileInfo.FileName[0], DestMax, Name, StrLen (Name));
      ASSERT_EFI_ERROR (Status);

      if (FV_FILETYPE_IS_EXECUTABLE (FileType)) {
        Status  = StrnCatS (&FvFileInfo->FileInfo.FileName[0], DestMax, L".efi", StrLen (L".efi"));
        ASSERT_EFI_ERROR (Status);
      }

      FvFileInfo->FileInfo.Size     = sizeof (EFI_FILE_INFO) + NameLen - sizeof (CHAR16);
      Status = FvFsGetFileSize (FvProtocol, FvFileInfo);
      ASSERT_EFI_ERROR (Status);
      FvFileInfo->FileInfo.PhysicalSize = FvFileInfo->FileInfo.FileSize;
      FvFileInfo->FileInfo.Attribute    = EFI_FILE_READ_ONLY;

      InsertHeadList (&Instance->FileInfoHead, &FvFileInfo->Link);

      FreePool (Name);

    } while (TRUE);

    if (Status == EFI_NOT_FOUND) {
      Status = EFI_SUCCESS;
    }
  }

  Instance->Root->DirReadNext = NULL;
  if (!IsListEmpty (&Instance->FileInfoHead)) {
    Instance->Root->DirReadNext = FVFS_GET_FIRST_FILE_INFO (Instance);
  }

  *RootFile = &Instance->Root->FileProtocol;
  return Status;
}

/**
  Worker function to initialize Unicode Collation support.

  It tries to locate Unicode Collation (2) protocol and matches it with current
  platform language code.

  @param  AgentHandle          The handle used to open Unicode Collation (2) protocol.
  @param  ProtocolGuid         The pointer to Unicode Collation (2) protocol GUID.
  @param  VariableName         The name of the RFC 4646 or ISO 639-2 language variable.
  @param  DefaultLanguage      The default language in case the RFC 4646 or ISO 639-2 language is absent.

  @retval EFI_SUCCESS          The Unicode Collation (2) protocol has been successfully located.
  @retval Others               The Unicode Collation (2) protocol has not been located.

**/
EFI_STATUS
InitializeUnicodeCollationSupportWorker (
  IN       EFI_HANDLE             AgentHandle,
  IN       EFI_GUID               *ProtocolGuid,
  IN CONST CHAR16                 *VariableName,
  IN CONST CHAR8                  *DefaultLanguage
  )
{
  EFI_STATUS                      ReturnStatus;
  EFI_STATUS                      Status;
  UINTN                           NumHandles;
  UINTN                           Index;
  EFI_HANDLE                      *Handles;
  EFI_UNICODE_COLLATION_PROTOCOL  *Uci;
  BOOLEAN                         Iso639Language;
  CHAR8                           *Language;
  CHAR8                           *BestLanguage;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  ProtocolGuid,
                  NULL,
                  &NumHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Iso639Language = (BOOLEAN) (ProtocolGuid == &gEfiUnicodeCollationProtocolGuid);
  GetEfiGlobalVariable2 (VariableName, (VOID**) &Language, NULL);

  ReturnStatus = EFI_UNSUPPORTED;
  for (Index = 0; Index < NumHandles; Index++) {
    //
    // Open Unicode Collation Protocol
    //
    Status = gBS->OpenProtocol (
                    Handles[Index],
                    ProtocolGuid,
                    (VOID **) &Uci,
                    AgentHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Find the best matching matching language from the supported languages
    // of Unicode Collation (2) protocol.
    //
    BestLanguage = GetBestLanguage (
                     Uci->SupportedLanguages,
                     Iso639Language,
                     (Language == NULL) ? "" : Language,
                     DefaultLanguage,
                     NULL
                     );
    if (BestLanguage != NULL) {
      FreePool (BestLanguage);
      mUnicodeCollation = Uci;
      ReturnStatus = EFI_SUCCESS;
      break;
    }
  }

  if (Language != NULL) {
    FreePool (Language);
  }

  FreePool (Handles);

  return ReturnStatus;
}

/**
  Initialize Unicode Collation support.

  It tries to locate Unicode Collation 2 protocol and matches it with current
  platform language code. If for any reason the first attempt fails, it then tries to
  use Unicode Collation Protocol.

  @param  AgentHandle          The handle used to open Unicode Collation (2) protocol.

  @retval EFI_SUCCESS          The Unicode Collation (2) protocol has been successfully located.
  @retval Others               The Unicode Collation (2) protocol has not been located.

**/
EFI_STATUS
InitializeUnicodeCollationSupport (
  IN EFI_HANDLE    AgentHandle
  )
{

  EFI_STATUS       Status;

  Status = EFI_UNSUPPORTED;

  //
  // First try to use RFC 4646 Unicode Collation 2 Protocol.
  //
  Status = InitializeUnicodeCollationSupportWorker (
             AgentHandle,
             &gEfiUnicodeCollation2ProtocolGuid,
             L"PlatformLang",
             (CONST CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLang)
             );
  //
  // If the attempt to use Unicode Collation 2 Protocol fails, then we fall back
  // on the ISO 639-2 Unicode Collation Protocol.
  //
  if (EFI_ERROR (Status)) {
    Status = InitializeUnicodeCollationSupportWorker (
               AgentHandle,
               &gEfiUnicodeCollationProtocolGuid,
               L"Lang",
               (CONST CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultLang)
               );
  }

  return Status;
}

/**
  Test to see if this driver supports ControllerHandle.

  @param  DriverBinding       Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemDriverSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return gBS->OpenProtocol (
                ControllerHandle,
                &gEfiFirmwareVolume2ProtocolGuid,
                NULL,
                gImageHandle,
                ControllerHandle,
                EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                );
}

/**
  Start this driver on ControllerHandle by opening a FV protocol and
  installing a SimpleFileSystem protocol on ControllerHandle.

  @param  DriverBinding        Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemDriverStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                       Status;
  EFI_FIRMWARE_VOLUME2_PROTOCOL    *FvProtocol;
  FV_FILESYSTEM_INSTANCE           *Instance;
  EFI_DEVICE_PATH_PROTOCOL         *FvDevicePath;
  EFI_GUID                         *FvGuid;
  UINTN                            NumChars;

  Status = InitializeUnicodeCollationSupport (DriverBinding->DriverBindingHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open FV protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  (VOID **) &FvProtocol,
                  gImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create an instance
  //
  Instance = AllocateZeroPool (sizeof (FV_FILESYSTEM_INSTANCE));
  ASSERT (Instance != NULL);

  Instance->Root = NULL;
  Instance->FvProtocol = FvProtocol;
  Instance->Signature = FVFS_INSTANCE_SIGNATURE;
  InitializeListHead (&Instance->FileInfoHead);
  InitializeListHead (&Instance->FileHead);
  CopyMem (&Instance->SimpleFs, &mSimpleFsTemplate, sizeof (mSimpleFsTemplate));

  Status = gBS->InstallProtocolInterface(
                  &ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Instance->SimpleFs
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Decide on a filesystem volume label, which will include the FV's guid.
  // Get the device path to find the FV's GUID
  //
  Instance->VolumeLabel = NULL;
  Status =  gBS->OpenProtocol (
                   ControllerHandle,
                   &gEfiDevicePathProtocolGuid,
                   (VOID **) &FvDevicePath,
                   gImageHandle,
                   ControllerHandle,
                   EFI_OPEN_PROTOCOL_GET_PROTOCOL
                   );
  if (!EFI_ERROR (Status)) {
    //
    // Iterate over device path until we find a firmware volume node
    //
    while (!IsDevicePathEndType (FvDevicePath)) {
      if (DevicePathType (FvDevicePath) == MEDIA_DEVICE_PATH &&
          DevicePathSubType (FvDevicePath) == MEDIA_PIWG_FW_VOL_DP) {
        //
        // Allocate the volume label
        //
        Instance->VolumeLabel = AllocateZeroPool (FVFS_VOLUME_LABEL_SIZE);
        //
        // Check the allocation was successful
        //
        if (Instance->VolumeLabel != NULL) {
          //
          // Extract the FV's guid
          //
          FvGuid = &((MEDIA_FW_VOL_DEVICE_PATH *) FvDevicePath)->FvName;
          //
          // Build the volume label string
          //
          NumChars = UnicodeSPrint (
                       Instance->VolumeLabel,
                       FVFS_VOLUME_LABEL_SIZE,
                       FVFS_VOLUME_LABEL_PREFIX L"%g",
                       FvGuid
                       );
          ASSERT ((NumChars + 1) * sizeof (CHAR16) == FVFS_VOLUME_LABEL_SIZE);
        }
        break;
      }
      FvDevicePath = NextDevicePathNode (FvDevicePath);
    }
  }
  //
  // If we didn't decide on a volume label, set a fallback one
  //
  if (Instance->VolumeLabel == NULL) {
    Instance->VolumeLabel = AllocateCopyPool (
                              sizeof (FVFS_FALLBACK_VOLUME_LABEL),
                              FVFS_FALLBACK_VOLUME_LABEL
                              );
  }

  return EFI_SUCCESS;
}

/**
  Stop this driver on ControllerHandle by removing SimpleFileSystem protocol and closing
  the FV protocol on ControllerHandle.

  @param  DriverBinding     Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL       *DriverBinding,
  IN  EFI_HANDLE                        ControllerHandle,
  IN  UINTN                             NumberOfChildren,
  IN  EFI_HANDLE                        *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                       Status;
  FV_FILESYSTEM_INSTANCE           *Instance;
  FV_FILESYSTEM_FILE_INFO          *FvFileInfo;
  LIST_ENTRY                       *Entry;
  LIST_ENTRY                       *DelEntry;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFile;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **) &SimpleFile,
                  DriverBinding->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Instance = FVFS_INSTANCE_FROM_SIMPLE_FS_THIS (SimpleFile);

  if (IsListEmpty (&Instance->FileHead) == FALSE) {
    //
    // Not all opened files are closed
    //
    return EFI_DEVICE_ERROR;
  }

  //
  // Close and uninstall protocols.
  //
  Status = gBS->CloseProtocol (
                   ControllerHandle,
                   &gEfiFirmwareVolume2ProtocolGuid,
                   gImageHandle,
                   ControllerHandle
                   );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  &Instance->SimpleFs
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Free file structures
  //
  if (!IsListEmpty (&Instance->FileInfoHead)) {
    //
    // Free the Subtask list.
    //
    for(Entry = Instance->FileInfoHead.ForwardLink;
        Entry != (&Instance->FileInfoHead);
       ) {
      DelEntry   = Entry;
      Entry      = Entry->ForwardLink;
      FvFileInfo = FVFS_FILE_INFO_FROM_LINK (DelEntry);

      RemoveEntryList (DelEntry);
      FreePool (FvFileInfo);
    }
  }

  if (Instance->Root != NULL) {
    //
    // Root->Name is statically allocated, no need to free.
    //
    if (Instance->Root->FvFileInfo != NULL) {
      FreePool (Instance->Root->FvFileInfo);
    }
    FreePool (Instance->Root);
  }

  //
  // Free Instance
  //
  if (Instance->VolumeLabel != NULL) {
    FreePool (Instance->VolumeLabel);
  }
  FreePool (Instance);

  return EFI_SUCCESS;
}

/**
  The user Entry Point for module FvSimpleFileSystem. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemEntryPoint (
  IN EFI_HANDLE               ImageHandle,
  IN EFI_SYSTEM_TABLE         *SystemTable
  )
{
  EFI_STATUS Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &mDriverBinding,
             ImageHandle,
             &gFvSimpleFileSystemComponentName,
             &gFvSimpleFileSystemComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
