/** @file
  Provides 'initrd' dynamic UEFI shell command to load a Linux initrd
  via its GUIDed vendor media path

  Copyright (c) 2020, Arm, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>

#include <Guid/LinuxEfiInitrdMedia.h>

#include <Protocol/DevicePath.h>
#include <Protocol/HiiPackageList.h>
#include <Protocol/LoadFile2.h>
#include <Protocol/ShellDynamicCommand.h>

#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH          VenMediaNode;
  EFI_DEVICE_PATH_PROTOCOL    EndNode;
} SINGLE_NODE_VENDOR_MEDIA_DEVPATH;
#pragma pack ()

STATIC EFI_HII_HANDLE        mLinuxInitrdShellCommandHiiHandle;
STATIC EFI_PHYSICAL_ADDRESS  mInitrdFileAddress;
STATIC UINTN                 mInitrdFileSize;
STATIC EFI_HANDLE            mInitrdLoadFile2Handle;

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-u", TypeFlag },
  { NULL,  TypeMax  }
};

STATIC CONST SINGLE_NODE_VENDOR_MEDIA_DEVPATH  mInitrdDevicePath = {
  {
    {
      MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP,{ sizeof (VENDOR_DEVICE_PATH)                                       }
    },
    LINUX_EFI_INITRD_MEDIA_GUID
  },{
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL)                                 }
  }
};

STATIC
BOOLEAN
IsOtherInitrdDevicePathAlreadyInstalled (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_HANDLE                Handle;

  DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)&mInitrdDevicePath;
  Status     = gBS->LocateDevicePath (
                      &gEfiLoadFile2ProtocolGuid,
                      &DevicePath,
                      &Handle
                      );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Check whether the existing instance is one that we installed during
  // a previous invocation.
  //
  if (Handle == mInitrdLoadFile2Handle) {
    return FALSE;
  }

  return TRUE;
}

STATIC
EFI_STATUS
EFIAPI
InitrdLoadFile2 (
  IN      EFI_LOAD_FILE2_PROTOCOL   *This,
  IN      EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN      BOOLEAN                   BootPolicy,
  IN  OUT UINTN                     *BufferSize,
  OUT     VOID                      *Buffer     OPTIONAL
  )
{
  if (BootPolicy) {
    return EFI_UNSUPPORTED;
  }

  if ((BufferSize == NULL) || !IsDevicePathValid (FilePath, 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((FilePath->Type != END_DEVICE_PATH_TYPE) ||
      (FilePath->SubType != END_ENTIRE_DEVICE_PATH_SUBTYPE) ||
      (mInitrdFileSize == 0))
  {
    return EFI_NOT_FOUND;
  }

  if ((Buffer == NULL) || (*BufferSize < mInitrdFileSize)) {
    *BufferSize = mInitrdFileSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  ASSERT (mInitrdFileAddress != 0);

  gBS->CopyMem (Buffer, (VOID *)(UINTN)mInitrdFileAddress, mInitrdFileSize);
  *BufferSize = mInitrdFileSize;
  return EFI_SUCCESS;
}

STATIC CONST EFI_LOAD_FILE2_PROTOCOL  mInitrdLoadFile2 = {
  InitrdLoadFile2,
};

STATIC
EFI_STATUS
UninstallLoadFile2Protocol (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mInitrdLoadFile2Handle != NULL) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    mInitrdLoadFile2Handle,
                    &gEfiDevicePathProtocolGuid,
                    &mInitrdDevicePath,
                    &gEfiLoadFile2ProtocolGuid,
                    &mInitrdLoadFile2,
                    NULL
                    );
    if (!EFI_ERROR (Status)) {
      mInitrdLoadFile2Handle = NULL;
    }

    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
VOID
FreeInitrdFile (
  VOID
  )
{
  if (mInitrdFileSize != 0) {
    gBS->FreePages (mInitrdFileAddress, EFI_SIZE_TO_PAGES (mInitrdFileSize));
    mInitrdFileSize = 0;
  }
}

STATIC
EFI_STATUS
CacheInitrdFile (
  IN  SHELL_FILE_HANDLE  FileHandle
  )
{
  EFI_STATUS  Status;
  UINT64      FileSize;
  UINTN       ReadSize;

  Status = gEfiShellProtocol->GetFileSize (FileHandle, &FileSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((FileSize == 0) || (FileSize > MAX_UINTN)) {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiLoaderData,
                  EFI_SIZE_TO_PAGES ((UINTN)FileSize),
                  &mInitrdFileAddress
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ReadSize = (UINTN)FileSize;
  Status   = gEfiShellProtocol->ReadFile (
                                  FileHandle,
                                  &ReadSize,
                                  (VOID *)(UINTN)mInitrdFileAddress
                                  );
  if (EFI_ERROR (Status) || (ReadSize < FileSize)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: failed to read initrd file - %r 0x%lx 0x%lx\n",
      __func__,
      Status,
      (UINT64)ReadSize,
      FileSize
      ));
    goto FreeMemory;
  }

  if (mInitrdLoadFile2Handle == NULL) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mInitrdLoadFile2Handle,
                    &gEfiDevicePathProtocolGuid,
                    &mInitrdDevicePath,
                    &gEfiLoadFile2ProtocolGuid,
                    &mInitrdLoadFile2,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  mInitrdFileSize = (UINTN)FileSize;
  return EFI_SUCCESS;

FreeMemory:
  gBS->FreePages (mInitrdFileAddress, EFI_SIZE_TO_PAGES ((UINTN)FileSize));
  return Status;
}

/**
  Function for 'initrd' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
STATIC
SHELL_STATUS
EFIAPI
RunInitrd (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS         Status;
  LIST_ENTRY         *Package;
  CHAR16             *ProblemParam;
  CONST CHAR16       *Param;
  CHAR16             *Filename;
  SHELL_STATUS       ShellStatus;
  SHELL_FILE_HANDLE  FileHandle;

  ProblemParam = NULL;
  ShellStatus  = SHELL_SUCCESS;

  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GEN_PROBLEM),
        mLinuxInitrdShellCommandHiiHandle,
        L"initrd",
        ProblemParam
        );
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else if (IsOtherInitrdDevicePathAlreadyInstalled ()) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_GEN_ALREADY_INSTALLED),
      mLinuxInitrdShellCommandHiiHandle,
      L"initrd"
      );
    ShellStatus = SHELL_UNSUPPORTED;
  } else {
    if (ShellCommandLineGetCount (Package) > 2) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GEN_TOO_MANY),
        mLinuxInitrdShellCommandHiiHandle,
        L"initrd"
        );
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount (Package) < 2) {
      if (ShellCommandLineGetFlag (Package, L"-u")) {
        FreeInitrdFile ();
        UninstallLoadFile2Protocol ();
      } else {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_GEN_TOO_FEW),
          mLinuxInitrdShellCommandHiiHandle,
          L"initrd"
          );
        ShellStatus = SHELL_INVALID_PARAMETER;
      }
    } else {
      Param = ShellCommandLineGetRawValue (Package, 1);
      ASSERT (Param != NULL);

      Filename = ShellFindFilePath (Param);
      if (Filename == NULL) {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_GEN_FIND_FAIL),
          mLinuxInitrdShellCommandHiiHandle,
          L"initrd",
          Param
          );
        ShellStatus = SHELL_NOT_FOUND;
      } else {
        Status = ShellOpenFileByName (
                   Filename,
                   &FileHandle,
                   EFI_FILE_MODE_READ,
                   0
                   );
        if (!EFI_ERROR (Status)) {
          FreeInitrdFile ();
          Status = CacheInitrdFile (FileHandle);
          ShellCloseFile (&FileHandle);
        }

        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL),
            mLinuxInitrdShellCommandHiiHandle,
            L"initrd",
            Param
            );
          ShellStatus = SHELL_NOT_FOUND;
        }

        FreePool (Filename);
      }
    }
  }

  return ShellStatus;
}

/**
  This is the shell command handler function pointer callback type.  This
  function handles the command when it is invoked in the shell.

  @param[in] This                   The instance of the
                                    EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] SystemTable            The pointer to the system table.
  @param[in] ShellParameters        The parameters associated with the command.
  @param[in] Shell                  The instance of the shell protocol used in
                                    the context of processing this command.

  @return EFI_SUCCESS               the operation was successful
  @return other                     the operation failed.
**/
SHELL_STATUS
EFIAPI
LinuxInitrdCommandHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN EFI_SYSTEM_TABLE                    *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL       *ShellParameters,
  IN EFI_SHELL_PROTOCOL                  *Shell
  )
{
  gEfiShellParametersProtocol = ShellParameters;
  gEfiShellProtocol           = Shell;

  return RunInitrd (gImageHandle, SystemTable);
}

/**
  This is the command help handler function pointer callback type.  This
  function is responsible for displaying help information for the associated
  command.

  @param[in] This                   The instance of the
                                    EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] Language               The pointer to the language string to use.

  @return string                    Pool allocated help string, must be freed
                                    by caller
**/
STATIC
CHAR16 *
EFIAPI
LinuxInitrdGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN CONST CHAR8                         *Language
  )
{
  return HiiGetString (
           mLinuxInitrdShellCommandHiiHandle,
           STRING_TOKEN (STR_GET_HELP_INITRD),
           Language
           );
}

STATIC EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  mLinuxInitrdDynamicCommand = {
  L"initrd",
  LinuxInitrdCommandHandler,
  LinuxInitrdGetHelp
};

/**
  Retrieve HII package list from ImageHandle and publish to HII database.

  @param ImageHandle            The image handle of the process.

  @return HII handle.
**/
STATIC
EFI_HII_HANDLE
InitializeHiiPackage (
  EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *PackageList;
  EFI_HII_HANDLE               HiiHandle;

  //
  // Retrieve HII package list from ImageHandle
  //
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiHiiPackageListProtocolGuid,
                  (VOID **)&PackageList,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Publish HII package list to HII Database.
  //
  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           NULL,
                           &HiiHandle
                           );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return HiiHandle;
}

/**
  Entry point of Linux Initrd dynamic UEFI Shell command.

  Produce the DynamicCommand protocol to handle "initrd" command.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           Initrd command is executed successfully.
  @retval EFI_ABORTED           HII package was failed to initialize.
  @retval others                Other errors when executing Initrd command.
**/
EFI_STATUS
EFIAPI
LinuxInitrdDynamicShellCommandEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mLinuxInitrdShellCommandHiiHandle = InitializeHiiPackage (ImageHandle);
  if (mLinuxInitrdShellCommandHiiHandle == NULL) {
    return EFI_ABORTED;
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mLinuxInitrdDynamicCommand
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Unload the dynamic UEFI Shell command.

  @param ImageHandle            The image handle of the process.

  @retval EFI_SUCCESS           The image is unloaded.
  @retval Others                Failed to unload the image.
**/
EFI_STATUS
EFIAPI
LinuxInitrdDynamicShellCommandUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  FreeInitrdFile ();

  Status = UninstallLoadFile2Protocol ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->UninstallProtocolInterface (
                  ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  &mLinuxInitrdDynamicCommand
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HiiRemovePackages (mLinuxInitrdShellCommandHiiHandle);
  return EFI_SUCCESS;
}
