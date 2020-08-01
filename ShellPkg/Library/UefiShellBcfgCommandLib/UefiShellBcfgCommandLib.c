/** @file
  Main file for BCFG command.

  (C) Copyright 2014-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Uefi.h>

#include <Guid/GlobalVariable.h>
#include <Guid/ShellLibHiiGuid.h>

#include <Protocol/Shell.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UnicodeCollation.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/SortLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/FileHandleLib.h>
#include <Library/PrintLib.h>
#include <Library/HandleParsingLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootManagerLib.h>

STATIC CONST CHAR16 mFileName[] = L"ShellCommands";
STATIC EFI_HII_HANDLE gShellBcfgHiiHandle  = NULL;

typedef enum {
  BcfgTargetBootOrder    = 0,
  BcfgTargetDriverOrder  = 1,
  BcfgTargetMax          = 2
} BCFG_OPERATION_TARGET;

typedef enum {
  BcfgTypeDump       = 0,
  BcfgTypeAdd        = 1,
  BcfgTypeAddp       = 2,
  BcfgTypeAddh       = 3,
  BcfgTypeRm         = 4,
  BcfgTypeMv         = 5,
  BcfgTypeOpt        = 6,
  BcfgTypeMod        = 7,
  BcfgTypeModf       = 8,
  BcfgTypeModp       = 9,
  BcfgTypeModh       = 10,
  BcfgTypeMax        = 11
} BCFG_OPERATION_TYPE;

typedef struct {
  BCFG_OPERATION_TARGET Target;
  BCFG_OPERATION_TYPE   Type;
  UINT16                Number1;
  UINT16                Number2;
  UINTN                 HandleIndex;
  CHAR16                *FileName;
  CHAR16                *Description;
  UINT16                *Order;
  CONST CHAR16          *OptData;
} BGFG_OPERATION;

/**
  Update the optional data for a boot or driver option.

  If optional data exists it will be changed.

  @param[in]      Index     The boot or driver option index update.
  @param[in]      DataSize  The size in bytes of Data.
  @param[in]      Data      The buffer for the optioanl data.
  @param[in]      Target    The target of the operation.

  @retval EFI_SUCCESS       The data was sucessfully updated.
  @retval other             A error occurred.
**/
EFI_STATUS
UpdateOptionalData(
  UINT16                          Index,
  UINTN                           DataSize,
  UINT8                           *Data,
  IN CONST BCFG_OPERATION_TARGET  Target
  )
{
  EFI_STATUS  Status;
  CHAR16      VariableName[12];
  UINTN       OriginalSize;
  UINT8       *OriginalData;
  UINTN       NewSize;
  UINT8       *NewData;
  UINTN       OriginalOptionDataSize;

  UnicodeSPrint(VariableName, sizeof(VariableName), L"%s%04x", Target == BcfgTargetBootOrder?L"Boot":L"Driver", Index);

  OriginalSize = 0;
  OriginalData = NULL;
  NewData      = NULL;
  NewSize      = 0;

  Status = gRT->GetVariable(
      VariableName,
      (EFI_GUID*)&gEfiGlobalVariableGuid,
      NULL,
      &OriginalSize,
      OriginalData);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    OriginalData = AllocateZeroPool(OriginalSize);
    if (OriginalData == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
    Status = gRT->GetVariable(
        VariableName,
        (EFI_GUID*)&gEfiGlobalVariableGuid,
        NULL,
        &OriginalSize,
        OriginalData);
  }

  if (!EFI_ERROR(Status)) {
    //
    // Allocate new struct and discard old optional data.
    //
    ASSERT (OriginalData != NULL);
    OriginalOptionDataSize  = sizeof(UINT32) + sizeof(UINT16) + StrSize(((CHAR16*)(OriginalData + sizeof(UINT32) + sizeof(UINT16))));
    OriginalOptionDataSize += (*(UINT16*)(OriginalData + sizeof(UINT32)));
    OriginalOptionDataSize -= OriginalSize;
    NewSize = OriginalSize - OriginalOptionDataSize + DataSize;
    NewData = AllocatePool(NewSize);
    if (NewData == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      CopyMem (NewData, OriginalData, OriginalSize - OriginalOptionDataSize);
      CopyMem(NewData + OriginalSize - OriginalOptionDataSize, Data, DataSize);
    }
  }

  if (!EFI_ERROR(Status)) {
    //
    // put the data back under the variable
    //
    Status = gRT->SetVariable(
      VariableName,
      (EFI_GUID*)&gEfiGlobalVariableGuid,
      EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
      NewSize,
      NewData);
  }

  SHELL_FREE_NON_NULL(OriginalData);
  SHELL_FREE_NON_NULL(NewData);
  return (Status);
}

/**
  This function will get a CRC for a boot option.

  @param[in, out] Crc         The CRC value to return.
  @param[in]      BootIndex   The boot option index to CRC.

  @retval EFI_SUCCESS           The CRC was sucessfully returned.
  @retval other                 A error occurred.
**/
EFI_STATUS
GetBootOptionCrc(
  UINT32      *Crc,
  UINT16      BootIndex
  )
{
  CHAR16      VariableName[12];
  EFI_STATUS  Status;
  UINT8       *Buffer;
  UINTN       BufferSize;

  Buffer      = NULL;
  BufferSize  = 0;

  //
  // Get the data Buffer
  //
  UnicodeSPrint(VariableName, sizeof(VariableName), L"%Boot%04x", BootIndex);
  Status = gRT->GetVariable(
      VariableName,
      (EFI_GUID*)&gEfiGlobalVariableGuid,
      NULL,
      &BufferSize,
      NULL);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Buffer = AllocateZeroPool(BufferSize);
    Status = gRT->GetVariable(
        VariableName,
        (EFI_GUID*)&gEfiGlobalVariableGuid,
        NULL,
        &BufferSize,
        Buffer);
  }

  //
  // Get the CRC computed
  //
  if (!EFI_ERROR(Status)) {
    Status = gBS->CalculateCrc32 (Buffer, BufferSize, Crc);
  }

  SHELL_FREE_NON_NULL(Buffer);
  return EFI_SUCCESS;
}

/**
  This function will populate the device path protocol parameter based on TheHandle.

  @param[in]      TheHandle     Driver handle.
  @param[in, out] FilePath      On a sucessful return the device path to the handle.

  @retval EFI_SUCCESS           The device path was sucessfully returned.
  @retval other                 A error from gBS->HandleProtocol.

  @sa HandleProtocol
**/
EFI_STATUS
GetDevicePathForDriverHandle (
  IN EFI_HANDLE                   TheHandle,
  IN OUT EFI_DEVICE_PATH_PROTOCOL **FilePath
  )
{
  EFI_STATUS                Status;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL  *ImageDevicePath;

  Status = gBS->OpenProtocol (
                TheHandle,
                &gEfiLoadedImageProtocolGuid,
                (VOID**)&LoadedImage,
                gImageHandle,
                NULL,
                EFI_OPEN_PROTOCOL_GET_PROTOCOL
               );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                  LoadedImage->DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID**)&ImageDevicePath,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                 );
    if (!EFI_ERROR (Status)) {
//      *DevPath  = DuplicateDevicePath (ImageDevicePath);
//      *FilePath = DuplicateDevicePath (LoadedImage->FilePath);
        *FilePath = AppendDevicePath(ImageDevicePath,LoadedImage->FilePath);
      gBS->CloseProtocol(
                  LoadedImage->DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  gImageHandle,
                  NULL);
    }
    gBS->CloseProtocol(
                TheHandle,
                &gEfiLoadedImageProtocolGuid,
                gImageHandle,
                NULL);
  }
  return (Status);
}

/**
  Functino to get Device Path by a handle.

  @param[in]        TheHandle   Use it to get DevicePath.
  @param[in]        Target      Boot option target.
  @param[in, out]   DevicePath  On a sucessful return the device path to the handle.

  @retval   SHELL_INVALID_PARAMETER The handle was NULL.
  @retval   SHELL_NOT_FOUND         Not found device path by handle.
  @retval   SHELL_SUCCESS           Get device path successfully.
**/
SHELL_STATUS
GetDevicePathByHandle(
  IN     EFI_HANDLE               TheHandle,
  IN     BCFG_OPERATION_TARGET    Target,
  IN OUT EFI_DEVICE_PATH_PROTOCOL **DevicePath
  )
{
  EFI_STATUS   Status;
  SHELL_STATUS ShellStatus;

  UINTN DriverBindingHandleCount;
  UINTN ParentControllerHandleCount;
  UINTN ChildControllerHandleCount;

  ShellStatus = SHELL_SUCCESS;

  if (TheHandle == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", L"Handle Number");
    return SHELL_INVALID_PARAMETER;
  }

  Status = PARSE_HANDLE_DATABASE_UEFI_DRIVERS (TheHandle, &DriverBindingHandleCount, NULL);
  if (EFI_ERROR(Status)) {
    DriverBindingHandleCount = 0;
  }

  Status = PARSE_HANDLE_DATABASE_PARENTS (TheHandle, &ParentControllerHandleCount, NULL);
  if (EFI_ERROR (Status)) {
    ParentControllerHandleCount = 0;
  }

  Status = ParseHandleDatabaseForChildControllers (TheHandle, &ChildControllerHandleCount, NULL);
  if (EFI_ERROR (Status)) {
    ChildControllerHandleCount = 0;
  }

  Status = gBS->HandleProtocol (TheHandle, &gEfiDevicePathProtocolGuid, (VOID**)DevicePath);

  if ( DriverBindingHandleCount    > 0 ||
       ParentControllerHandleCount > 0 ||
       ChildControllerHandleCount  > 0 ||
       !EFI_ERROR(Status)
     ) {
    //
    // The handle points to a real controller which has a device path.
    //
    if (Target == BcfgTargetDriverOrder) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,STRING_TOKEN (STR_GEN_PARAM_INV),
        gShellBcfgHiiHandle,
        L"bcfg",
        L"Handle should point to driver image."
      );
      ShellStatus = SHELL_NOT_FOUND;
    }
  } else {
    //
    // The handle points to a driver image.
    //
    if (Target == BcfgTargetBootOrder) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GEN_PARAM_INV),
        gShellBcfgHiiHandle,
        L"bcfg",
        L"Handle should point to controller."
      );
      ShellStatus = SHELL_NOT_FOUND;
    } else {
      if (EFI_ERROR (GetDevicePathForDriverHandle (TheHandle, DevicePath))) {
        ShellStatus = SHELL_NOT_FOUND;
      }
    }
  }

  return (ShellStatus);
}

/**
  Function to modify an option.

  @param[in] BcfgOperation  Pointer to BCFG operation.
  @param[in] OrderCount     The number if items in CurrentOrder.

  @retval SHELL_SUCCESS             The operation was successful.
  @retval SHELL_INVALID_PARAMETER   A parameter was invalid.
  @retval SHELL_OUT_OF_RESOUCES     A memory allocation failed.
**/
SHELL_STATUS
BcfgMod (
  IN CONST BGFG_OPERATION   *BcfgOperation,
  IN CONST UINTN            OrderCount
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    CurHandle;
  SHELL_STATUS                  ShellStatus;
  CHAR16                        OptionStr[40];
  EFI_SHELL_FILE_INFO           *FileList;
  EFI_SHELL_FILE_INFO           *Arg;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePathBuffer;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePathWalker;
  EFI_BOOT_MANAGER_LOAD_OPTION  LoadOption;

  ShellStatus       = SHELL_SUCCESS;
  FileList          = NULL;
  DevicePath        = NULL;
  DevicePathBuffer  = NULL;

  ZeroMem (&LoadOption, sizeof(EFI_BOOT_MANAGER_LOAD_OPTION));

  if ( (BcfgOperation->Type == BcfgTypeMod  && BcfgOperation->Description == NULL)  ||
       (BcfgOperation->Type == BcfgTypeModf && BcfgOperation->FileName == NULL)     ||
       (BcfgOperation->Type == BcfgTypeModp && BcfgOperation->FileName == NULL)     ||
       (BcfgOperation->Type == BcfgTypeModh && BcfgOperation->HandleIndex == 0)     ||
       (BcfgOperation->Number1 > OrderCount)
     ) {
    return (SHELL_INVALID_PARAMETER);
  }

  if (BcfgOperation->Type == BcfgTypeModh) {
    CurHandle = ConvertHandleIndexToHandle (BcfgOperation->HandleIndex);
    ShellStatus = GetDevicePathByHandle (CurHandle, BcfgOperation->Target, &DevicePathBuffer);
    if (ShellStatus == SHELL_SUCCESS) {
      DevicePath = DuplicateDevicePath (DevicePathBuffer);
    }
  } else if (BcfgOperation->Type == BcfgTypeModf || BcfgOperation->Type == BcfgTypeModp) {
    //
    // Get Device Path by FileName.
    //
    ShellOpenFileMetaArg ((CHAR16 *)BcfgOperation->FileName, EFI_FILE_MODE_READ, &FileList);
    if (FileList == NULL) {
      //
      // The name of file matched nothing.
      //
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellBcfgHiiHandle, L"bcfg", BcfgOperation->FileName);
      ShellStatus = SHELL_INVALID_PARAMETER;
    }
    else if (FileList->Link.ForwardLink != FileList->Link.BackLink) {
      //
      // If the name of file expanded to multiple names, it's fail.
      //
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_BCFG_FILE), gShellBcfgHiiHandle, L"bcfg", BcfgOperation->FileName);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Arg = (EFI_SHELL_FILE_INFO *)GetFirstNode (&FileList->Link);
      if (EFI_ERROR (Arg->Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_BCFG_FILE_OPEN), gShellBcfgHiiHandle, L"bcfg", BcfgOperation->FileName);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        DevicePathBuffer = gEfiShellProtocol->GetDevicePathFromFilePath (Arg->FullName);
        if (DevicePathBuffer == NULL) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_BCFG_FILE_DP), gShellBcfgHiiHandle, L"bcfg", Arg->FullName);
          ShellStatus = SHELL_UNSUPPORTED;
        }
      }
    }

    if (ShellStatus == SHELL_SUCCESS) {
      if (BcfgOperation->Type == BcfgTypeModp) {
        ShellStatus = SHELL_INVALID_PARAMETER;
        DevicePathWalker = DevicePathBuffer;
        while (!IsDevicePathEnd (DevicePathWalker)) {
          if ( DevicePathType (DevicePathWalker) == MEDIA_DEVICE_PATH &&
               DevicePathSubType (DevicePathWalker) == MEDIA_HARDDRIVE_DP
             ) {
            //
            // We found the portion of device path starting with the hard driver partition.
            //
            ShellStatus = SHELL_SUCCESS;
            DevicePath = DuplicateDevicePath (DevicePathWalker);
            break;
          } else {
            DevicePathWalker = NextDevicePathNode (DevicePathWalker);
          }
        }
      } else {
        DevicePath = DuplicateDevicePath (DevicePathBuffer);
      }

      FreePool (DevicePathBuffer);
    }
  }

  if (ShellStatus == SHELL_SUCCESS) {
    if (BcfgOperation->Target == BcfgTargetBootOrder) {
      UnicodeSPrint (OptionStr, sizeof (OptionStr), L"Boot%04x", BcfgOperation->Order[BcfgOperation->Number1]);
    } else {
      UnicodeSPrint (OptionStr, sizeof (OptionStr), L"Driver%04x", BcfgOperation->Order[BcfgOperation->Number1]);
    }
    Status = EfiBootManagerVariableToLoadOption (OptionStr, &LoadOption);
    if (EFI_ERROR(Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_BCFG_NONE), gShellBcfgHiiHandle);
      ShellStatus = SHELL_NOT_FOUND;
    }
  }

  if (ShellStatus == SHELL_SUCCESS) {
    if (BcfgOperation->Type == BcfgTypeMod) {
      SHELL_FREE_NON_NULL (LoadOption.Description);
      LoadOption.Description = AllocateCopyPool (StrSize (BcfgOperation->Description), BcfgOperation->Description);
    } else {
      SHELL_FREE_NON_NULL (LoadOption.FilePath);
      LoadOption.FilePath = DuplicateDevicePath (DevicePath);
    }

    Status = EfiBootManagerLoadOptionToVariable (&LoadOption);
    if (EFI_ERROR(Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_BCFG_SET_VAR_FAIL), gShellBcfgHiiHandle, L"bcfg", OptionStr);
      ShellStatus = SHELL_INVALID_PARAMETER;
    }
  }

  EfiBootManagerFreeLoadOption (&LoadOption);

  if (DevicePath != NULL) {
    FreePool (DevicePath);
  }

  if (FileList != NULL) {
    ShellCloseFileMetaArg (&FileList);
  }

  return (ShellStatus);
}

/**
  Function to add a option.

  @param[in] Position       The position to add Target at.
  @param[in] File           The file to make the target.
  @param[in] Desc           The description text.
  @param[in] CurrentOrder   The pointer to the current order of items.
  @param[in] OrderCount     The number if items in CurrentOrder.
  @param[in] Target         The info on the option to add.
  @param[in] UseHandle      TRUE to use HandleNumber, FALSE to use File and Desc.
  @param[in] UsePath        TRUE to convert to devicepath.
  @param[in] HandleNumber   The handle number to add.

  @retval SHELL_SUCCESS             The operation was successful.
  @retval SHELL_INVALID_PARAMETER   A parameter was invalid.
**/
SHELL_STATUS
BcfgAdd(
  IN       UINTN                  Position,
  IN CONST CHAR16                 *File,
  IN CONST CHAR16                 *Desc,
  IN CONST UINT16                 *CurrentOrder,
  IN CONST UINTN                  OrderCount,
  IN CONST BCFG_OPERATION_TARGET  Target,
  IN CONST BOOLEAN                UseHandle,
  IN CONST BOOLEAN                UsePath,
  IN CONST UINTN                  HandleNumber
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  CHAR16                    *Str;
  UINT8                     *TempByteBuffer;
  UINT8                     *TempByteStart;
  EFI_SHELL_FILE_INFO       *Arg;
  EFI_SHELL_FILE_INFO       *FileList;
  CHAR16                    OptionStr[40];
  UINTN                     DescSize, FilePathSize;
  BOOLEAN                   Found;
  UINTN                     TargetLocation;
  UINTN                     Index;
  EFI_HANDLE                *Handles;
  EFI_HANDLE                CurHandle;
  UINTN                     DriverBindingHandleCount;
  UINTN                     ParentControllerHandleCount;
  UINTN                     ChildControllerHandleCount;
  SHELL_STATUS              ShellStatus;
  UINT16                    *NewOrder;

  if (!UseHandle) {
    if (File == NULL || Desc == NULL) {
      return (SHELL_INVALID_PARAMETER);
    }
  } else {
    if (HandleNumber == 0) {
      return (SHELL_INVALID_PARAMETER);
    }
  }

  if (Position > OrderCount) {
    Position =  OrderCount;
  }

  Str             = NULL;
  FilePath        = NULL;
  FileList        = NULL;
  Handles         = NULL;
  ShellStatus     = SHELL_SUCCESS;
  TargetLocation  = 0xFFFF;

  if (UseHandle) {
    CurHandle = ConvertHandleIndexToHandle(HandleNumber);
    if (CurHandle == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", L"Handle Number");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      if (Target == BcfgTargetBootOrder) {
        //
        //Make sure that the handle should point to a real controller
        //
        Status = PARSE_HANDLE_DATABASE_UEFI_DRIVERS (
                   CurHandle,
                   &DriverBindingHandleCount,
                   NULL);

        Status = PARSE_HANDLE_DATABASE_PARENTS (
                   CurHandle,
                   &ParentControllerHandleCount,
                   NULL);

        Status = ParseHandleDatabaseForChildControllers (
                   CurHandle,
                   &ChildControllerHandleCount,
                   NULL);

        if (DriverBindingHandleCount > 0
              || ParentControllerHandleCount > 0
              || ChildControllerHandleCount > 0) {
          FilePath = NULL;
          Status = gBS->HandleProtocol (
                     CurHandle,
                     &gEfiDevicePathProtocolGuid,
                     (VOID**)&FilePath);
        }
        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_HANDLE), gShellBcfgHiiHandle, L"bcfg", HandleNumber);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      } else {
        //
        //Make sure that the handle should point to driver, not a controller.
        //
        Status = PARSE_HANDLE_DATABASE_UEFI_DRIVERS (
                   CurHandle,
                   &DriverBindingHandleCount,
                   NULL);

        Status = PARSE_HANDLE_DATABASE_PARENTS (
                   CurHandle,
                   &ParentControllerHandleCount,
                   NULL);

        Status = ParseHandleDatabaseForChildControllers (
                   CurHandle,
                   &ChildControllerHandleCount,
                   NULL);

        Status = gBS->HandleProtocol (
                   CurHandle,
                   &gEfiDevicePathProtocolGuid,
                   (VOID**)&FilePath);

        if (DriverBindingHandleCount > 0
              || ParentControllerHandleCount > 0
              || ChildControllerHandleCount > 0
              || !EFI_ERROR(Status) ) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", L"Handle Number");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          //
          // Get the DevicePath from the loaded image information.
          //
          Status = GetDevicePathForDriverHandle(CurHandle, &FilePath);
        }
      }
    }
  } else {
    //
    // Get file info
    //
    ShellOpenFileMetaArg ((CHAR16*)File, EFI_FILE_MODE_READ, &FileList);

    if (FileList == NULL) {
      //
      // If filename matched nothing fail
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellBcfgHiiHandle, L"bcfg", File);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (FileList->Link.ForwardLink != FileList->Link.BackLink) {
      //
      // If filename expanded to multiple names, fail
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_FILE), gShellBcfgHiiHandle, L"bcfg", File);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Arg = (EFI_SHELL_FILE_INFO*)GetFirstNode(&FileList->Link);
      if (EFI_ERROR(Arg->Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_FILE_OPEN), gShellBcfgHiiHandle, L"bcfg", File);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        //
        // Build FilePath to the filename
        //

        //
        // get the device path
        //
        DevicePath = gEfiShellProtocol->GetDevicePathFromFilePath(Arg->FullName);
        if (DevicePath == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_FILE_DP), gShellBcfgHiiHandle, L"bcfg", Arg->FullName);
          ShellStatus = SHELL_UNSUPPORTED;
        } else {
          if (UsePath) {
            DevPath     = DevicePath;
            ShellStatus = SHELL_INVALID_PARAMETER;
            while (!IsDevicePathEnd(DevPath)) {
              if ((DevicePathType(DevPath) == MEDIA_DEVICE_PATH) &&
                (DevicePathSubType(DevPath) == MEDIA_HARDDRIVE_DP)) {

                //
                // If we find it use it instead
                //
                ShellStatus = SHELL_SUCCESS;
                FilePath    = DuplicateDevicePath (DevPath);
                break;
              }
              DevPath = NextDevicePathNode(DevPath);
            }
          } else {
            FilePath = DuplicateDevicePath(DevicePath);
          }
          FreePool(DevicePath);
        }
      }
    }
  }


  if (ShellStatus == SHELL_SUCCESS) {
    //
    // Find a free target ,a brute force implementation
    //
    Found = FALSE;
    for (TargetLocation=0; TargetLocation < 0xFFFF; TargetLocation++) {
      Found = TRUE;
      for (Index=0; Index < OrderCount; Index++) {
        if (CurrentOrder[Index] == TargetLocation) {
          Found = FALSE;
          break;
        }
      }

      if (Found) {
        break;
      }
    }

    if (TargetLocation == 0xFFFF) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_TARGET_NF), gShellBcfgHiiHandle, L"bcfg");
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_TARGET), gShellBcfgHiiHandle, TargetLocation);
    }
  }

  if (ShellStatus == SHELL_SUCCESS) {
    //
    // Add the option
    //
    DescSize = StrSize(Desc);
    FilePathSize = GetDevicePathSize (FilePath);

    TempByteBuffer = AllocateZeroPool(sizeof(UINT32) + sizeof(UINT16) + DescSize + FilePathSize);
    if (TempByteBuffer != NULL) {
      TempByteStart  = TempByteBuffer;
      *((UINT32 *) TempByteBuffer) = LOAD_OPTION_ACTIVE;      // Attributes
      TempByteBuffer += sizeof (UINT32);

      *((UINT16 *) TempByteBuffer) = (UINT16)FilePathSize;    // FilePathListLength
      TempByteBuffer += sizeof (UINT16);

      CopyMem (TempByteBuffer, Desc, DescSize);
      TempByteBuffer += DescSize;
      ASSERT (FilePath != NULL);
      CopyMem (TempByteBuffer, FilePath, FilePathSize);

      UnicodeSPrint (OptionStr, sizeof(OptionStr), L"%s%04x", Target == BcfgTargetBootOrder?L"Boot":L"Driver", TargetLocation);
      Status = gRT->SetVariable (
            OptionStr,
            &gEfiGlobalVariableGuid,
            EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
            sizeof(UINT32) + sizeof(UINT16) + DescSize + FilePathSize,
            TempByteStart
           );

      FreePool(TempByteStart);
    } else {
      Status = EFI_OUT_OF_RESOURCES;
    }

    if (EFI_ERROR(Status)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_SET_VAR_FAIL), gShellBcfgHiiHandle, L"bcfg", OptionStr);
    } else {
      NewOrder = AllocateZeroPool ((OrderCount + 1) * sizeof (NewOrder[0]));
      if (NewOrder != NULL) {
        CopyMem (NewOrder, CurrentOrder, (OrderCount) * sizeof (NewOrder[0]));

        //
        // Insert target into order list
        //
        for (Index = OrderCount; Index > Position; Index--) {
          NewOrder[Index] = NewOrder[Index - 1];
        }

        NewOrder[Position] = (UINT16) TargetLocation;
        Status = gRT->SetVariable (
          Target == BcfgTargetBootOrder ? L"BootOrder" : L"DriverOrder",
          &gEfiGlobalVariableGuid,
          EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
          (OrderCount + 1) * sizeof (UINT16),
          NewOrder
        );

        FreePool (NewOrder);

        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellBcfgHiiHandle, L"bcfg", Target == BcfgTargetBootOrder ? L"BootOrder" : L"DriverOrder");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          Print (L"bcfg: Add %s as %x\n", OptionStr, Position);
        }
      }
    }
  }

//
//If always Free FilePath, will free devicepath in system when use "addh"
//
  if (FilePath!=NULL && !UseHandle) {
    FreePool (FilePath);
  }

  if (Str != NULL) {
    FreePool(Str);
  }

  if (Handles != NULL) {
    FreePool (Handles);
  }

  if (FileList != NULL) {
    ShellCloseFileMetaArg (&FileList);
  }

  return (ShellStatus);
}

/**
  Funciton to remove an item.

  @param[in] Target         The target item to move.
  @param[in] CurrentOrder   The pointer to the current order of items.
  @param[in] OrderCount     The number if items in CurrentOrder.
  @param[in] Location       The current location of the Target.

  @retval SHELL_SUCCESS             The operation was successful.
  @retval SHELL_INVALID_PARAMETER   A parameter was invalid.
**/
SHELL_STATUS
BcfgRemove(
  IN CONST BCFG_OPERATION_TARGET  Target,
  IN CONST UINT16                 *CurrentOrder,
  IN CONST UINTN                  OrderCount,
  IN CONST UINT16                 Location
  )
{
  CHAR16      VariableName[12];
  UINT16      *NewOrder;
  EFI_STATUS  Status;
  UINTN       NewCount;

  UnicodeSPrint(VariableName, sizeof(VariableName), L"%s%04x", Target == BcfgTargetBootOrder?L"Boot":L"Driver", CurrentOrder[Location]);
  Status = gRT->SetVariable(
    VariableName,
    (EFI_GUID*)&gEfiGlobalVariableGuid,
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    0,
    NULL);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellBcfgHiiHandle, L"bcfg", VariableName);
    return (SHELL_INVALID_PARAMETER);
  }
  NewOrder = AllocateZeroPool(OrderCount*sizeof(CurrentOrder[0]));
  if (NewOrder != NULL) {
    NewCount = OrderCount;
    CopyMem(NewOrder, CurrentOrder, OrderCount*sizeof(CurrentOrder[0]));
    CopyMem(NewOrder+Location, NewOrder+Location+1, (OrderCount - Location - 1)*sizeof(CurrentOrder[0]));
    NewCount--;

    Status = gRT->SetVariable(
      Target == BcfgTargetBootOrder?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
      (EFI_GUID*)&gEfiGlobalVariableGuid,
      EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
      NewCount*sizeof(NewOrder[0]),
      NewOrder);
    FreePool(NewOrder);
  } else {
    Status = EFI_OUT_OF_RESOURCES;
  }
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellBcfgHiiHandle, L"bcfg", Target == BcfgTargetBootOrder?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder");
    return (SHELL_INVALID_PARAMETER);
  }
  return (SHELL_SUCCESS);
}

/**
  Funciton to move a item to another location.

  @param[in] Target         The target item to move.
  @param[in] CurrentOrder   The pointer to the current order of items.
  @param[in] OrderCount     The number if items in CurrentOrder.
  @param[in] OldLocation    The current location of the Target.
  @param[in] NewLocation    The desired location of the Target.

  @retval SHELL_SUCCESS             The operation was successful.
  @retval SHELL_INVALID_PARAMETER   A parameter was invalid.
**/
SHELL_STATUS
BcfgMove(
  IN CONST BCFG_OPERATION_TARGET  Target,
  IN CONST UINT16                 *CurrentOrder,
  IN CONST UINTN                  OrderCount,
  IN CONST UINT16                 OldLocation,
  IN       UINT16                 NewLocation
  )
{
  UINT16            *NewOrder;
  EFI_STATUS        Status;
  UINT16            Temp;

  NewOrder = AllocateCopyPool(OrderCount*sizeof(CurrentOrder[0]), CurrentOrder);
  if (NewOrder == NULL) {
    return (SHELL_OUT_OF_RESOURCES);
  }

  //
  // correct the new location
  //
  if (NewLocation >= OrderCount) {
    if (OrderCount > 0) {
      NewLocation = (UINT16)OrderCount - 1;
    } else {
      NewLocation = 0;
    }
  }

  Temp = CurrentOrder[OldLocation];
  CopyMem(NewOrder+OldLocation, NewOrder+OldLocation+1, (OrderCount - OldLocation - 1)*sizeof(CurrentOrder[0]));
  CopyMem(NewOrder+NewLocation+1, NewOrder+NewLocation, (OrderCount - NewLocation - 1)*sizeof(CurrentOrder[0]));
  NewOrder[NewLocation] = Temp;

  Status = gRT->SetVariable(
    Target == BcfgTargetBootOrder?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
    (EFI_GUID*)&gEfiGlobalVariableGuid,
    EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
    OrderCount*sizeof(CurrentOrder[0]),
    NewOrder);

  FreePool(NewOrder);

  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_WRITE_FAIL), gShellBcfgHiiHandle, L"bcfg", Target == BcfgTargetBootOrder?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder");
    return (SHELL_INVALID_PARAMETER);
  }
  return (SHELL_SUCCESS);
}

/**
  Function to add optional data to an option.

  @param[in] OptData      The optional data to add.
  @param[in] CurrentOrder The pointer to the current order of items.
  @param[in] OrderCount   The number if items in CurrentOrder.
  @param[in] Target       The target of the operation.

  @retval SHELL_SUCCESS   The operation was succesful.
**/
SHELL_STATUS
BcfgAddOpt(
  IN CONST CHAR16                 *OptData,
  IN CONST UINT16                 *CurrentOrder,
  IN CONST UINTN                  OrderCount,
  IN CONST BCFG_OPERATION_TARGET  Target
  )
{
  EFI_KEY_OPTION  NewKeyOption;
  EFI_KEY_OPTION *KeyOptionBuffer;
  SHELL_STATUS    ShellStatus;
  EFI_STATUS      Status;
  UINT16          OptionIndex;
  UINT16          LoopCounter;
  UINT64          Intermediate;
  CONST CHAR16    *Temp;
  CONST CHAR16    *Walker;
  CHAR16          *FileName;
  CHAR16          *Temp2;
  CHAR16          *Data;
  UINT32          KeyIndex;
  CHAR16          VariableName[12];
  VOID            *VariableData;

  SHELL_FILE_HANDLE FileHandle;

  Status          = EFI_SUCCESS;
  ShellStatus     = SHELL_SUCCESS;
  Walker          = OptData;
  FileName        = NULL;
  Data            = NULL;
  KeyOptionBuffer = NULL;
  VariableData    = NULL;

  ZeroMem(&NewKeyOption, sizeof(EFI_KEY_OPTION));
  ZeroMem(VariableName, sizeof(VariableName));

  while(Walker[0] == L' ') {
    Walker++;
  }

  //
  // Get the index of the variable we are changing.
  //
  Status = ShellConvertStringToUint64(Walker, &Intermediate, TRUE, TRUE);
  if (EFI_ERROR(Status) || (((UINT16)Intermediate) != Intermediate) || StrStr(Walker, L" ") == NULL || ((UINT16)Intermediate) > ((UINT16)OrderCount)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", L"Option Index");
    ShellStatus = SHELL_INVALID_PARAMETER;
    return (ShellStatus);
  }
  OptionIndex = (UINT16)Intermediate;

  Temp = StrStr(Walker, L" ");
  if (Temp != NULL) {
    Walker = Temp;
  }
  while(Walker[0] == L' ') {
    Walker++;
  }

  //
  // determine whether we have file with data, quote delimited information, or a hot-key
  //
  if (Walker[0] == L'\"') {
    //
    // quoted filename or quoted information.
    //
    Temp = StrStr(Walker+1, L"\"");
    if (Temp == NULL || StrLen(Temp) != 1) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", Walker);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      FileName = StrnCatGrow(&FileName, NULL, Walker+1, 0);
      if (FileName == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellBcfgHiiHandle, L"bcfg");
        ShellStatus = SHELL_OUT_OF_RESOURCES;
        return (ShellStatus);
      }
      Temp2 = StrStr(FileName, L"\"");
      ASSERT(Temp2 != NULL);
      Temp2[0] = CHAR_NULL;
      Temp2++;
      if (StrLen(Temp2)>0) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", Walker);
        ShellStatus = SHELL_INVALID_PARAMETER;
      }
      if (EFI_ERROR(ShellFileExists(Walker))) {
        //
        // Not a file.  must be misc information.
        //
        Data     = FileName;
        FileName = NULL;
      } else {
        FileName = StrnCatGrow(&FileName, NULL, Walker, 0);
      }
    }
  } else {
    //
    // filename or hot key information.
    //
    if (StrStr(Walker, L" ") == NULL) {
      //
      // filename
      //
      if (EFI_ERROR(ShellFileExists(Walker))) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FIND_FAIL), gShellBcfgHiiHandle, L"bcfg", Walker);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        FileName = StrnCatGrow(&FileName, NULL, Walker, 0);
      }
    } else {
      if (Target != BcfgTargetBootOrder) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_BOOT_ONLY), gShellBcfgHiiHandle, L"bcfg");
        ShellStatus = SHELL_INVALID_PARAMETER;
      }

      if (ShellStatus == SHELL_SUCCESS) {
        //
        // Get hot key information
        //
        Status = ShellConvertStringToUint64(Walker, &Intermediate, FALSE, TRUE);
        if (EFI_ERROR(Status) || (((UINT32)Intermediate) != Intermediate) || StrStr(Walker, L" ") == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", Walker);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
        NewKeyOption.KeyData.PackedValue = (UINT32)Intermediate;
        Temp = StrStr(Walker, L" ");
        if (Temp != NULL) {
          Walker = Temp;
        }
        while(Walker[0] == L' ') {
          Walker++;
        }
      }

      if (ShellStatus == SHELL_SUCCESS) {
        //
        // Now we know how many EFI_INPUT_KEY structs we need to attach to the end of the EFI_KEY_OPTION struct.
        // Re-allocate with the added information.
        //
        KeyOptionBuffer = AllocatePool (sizeof(EFI_KEY_OPTION) + (sizeof(EFI_INPUT_KEY) * NewKeyOption.KeyData.Options.InputKeyCount));
        if (KeyOptionBuffer == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_MEM), gShellBcfgHiiHandle, L"bcfg");
          ShellStatus = SHELL_OUT_OF_RESOURCES;
          return ShellStatus;
        }
        CopyMem (KeyOptionBuffer, &NewKeyOption, sizeof(EFI_KEY_OPTION));
      }
      for (LoopCounter = 0 ; ShellStatus == SHELL_SUCCESS && LoopCounter < NewKeyOption.KeyData.Options.InputKeyCount; LoopCounter++) {
        //
        // ScanCode
        //
        Status = ShellConvertStringToUint64(Walker, &Intermediate, FALSE, TRUE);
        if (EFI_ERROR(Status) || (((UINT16)Intermediate) != Intermediate) || StrStr(Walker, L" ") == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", Walker);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
        ((EFI_INPUT_KEY*)(((UINT8*)KeyOptionBuffer) + sizeof(EFI_KEY_OPTION)))[LoopCounter].ScanCode = (UINT16)Intermediate;
        Temp = StrStr(Walker, L" ");
        if (Temp != NULL) {
          Walker = Temp;
        }
        while(Walker[0] == L' ') {
          Walker++;
        }

        //
        // UnicodeChar
        //
        Status = ShellConvertStringToUint64(Walker, &Intermediate, FALSE, TRUE);
        if (EFI_ERROR(Status) || (((UINT16)Intermediate) != Intermediate)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", Walker);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
        ((EFI_INPUT_KEY*)(((UINT8*)KeyOptionBuffer) + sizeof(EFI_KEY_OPTION)))[LoopCounter].UnicodeChar = (UINT16)Intermediate;
        Temp = StrStr(Walker, L" ");
        if (Temp != NULL) {
          Walker = Temp;
        }
        while(Walker[0] == L' ') {
          Walker++;
        }
      }

      if (ShellStatus == SHELL_SUCCESS) {
        //
        // Now do the BootOption / BootOptionCrc
        //
        ASSERT (OptionIndex <= OrderCount);
        KeyOptionBuffer->BootOption    = CurrentOrder[OptionIndex];
        Status = GetBootOptionCrc(&(KeyOptionBuffer->BootOptionCrc), KeyOptionBuffer->BootOption);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", L"Option Index");
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      }

      if (ShellStatus == SHELL_SUCCESS) {
        for (Temp2 = NULL, KeyIndex = 0 ; KeyIndex <= 0xFFFF ; KeyIndex++) {
          UnicodeSPrint(VariableName, sizeof(VariableName), L"Key%04x", KeyIndex);
          Status = GetEfiGlobalVariable2 (VariableName, &VariableData, NULL);
          if (Status == EFI_NOT_FOUND) {
            break;
          }
          if (!EFI_ERROR(Status)) {
            SHELL_FREE_NON_NULL(VariableData);
          }
        }
        if (KeyIndex <= 0xFFFF) {
          Status = gRT->SetVariable(
            VariableName,
            (EFI_GUID*)&gEfiGlobalVariableGuid,
            EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
            sizeof(EFI_KEY_OPTION) + (sizeof(EFI_INPUT_KEY) * NewKeyOption.KeyData.Options.InputKeyCount),
            KeyOptionBuffer);
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_SET_VAR_FAIL), gShellBcfgHiiHandle, L"bcfg", VariableName);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_VAR_NO_NUM), gShellBcfgHiiHandle, L"bcfg");
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
        ASSERT(FileName == NULL && Data == NULL);
      }
    }
  }

  //
  // Shouldn't be possible to have have both. Neither is ok though.
  //
  ASSERT(FileName == NULL || Data == NULL);

  if (ShellStatus == SHELL_SUCCESS && (FileName != NULL || Data != NULL)) {
    if (FileName != NULL) {
      //
      // Open the file and populate the data buffer.
      //
      Status = ShellOpenFileByName(
        FileName,
        &FileHandle,
        EFI_FILE_MODE_READ,
        0);
      if (!EFI_ERROR(Status)) {
        Status = ShellGetFileSize(FileHandle, &Intermediate);
      }
      Data = AllocateZeroPool((UINTN)Intermediate);
      if (Data == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_MEM), gShellBcfgHiiHandle, L"bcfg");
        ShellStatus = SHELL_OUT_OF_RESOURCES;
      }
      if (!EFI_ERROR(Status)) {
        Status = ShellReadFile(FileHandle, (UINTN *)&Intermediate, Data);
      }
    } else {
      Intermediate = StrSize(Data);
    }

    if (!EFI_ERROR(Status) && ShellStatus == SHELL_SUCCESS && Data != NULL) {
      Status = UpdateOptionalData(CurrentOrder[OptionIndex], (UINTN)Intermediate, (UINT8*)Data, Target);
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_SET_VAR_FAIL), gShellBcfgHiiHandle, L"bcfg", VariableName);
        ShellStatus = SHELL_INVALID_PARAMETER;
      }
    }
    if (EFI_ERROR(Status) && ShellStatus == SHELL_SUCCESS) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_SET_VAR_FAIL), gShellBcfgHiiHandle, L"bcfg", VariableName);
      ShellStatus = SHELL_INVALID_PARAMETER;
    }
  }

  SHELL_FREE_NON_NULL(Data);
  SHELL_FREE_NON_NULL(KeyOptionBuffer);
  SHELL_FREE_NON_NULL(FileName);
  return ShellStatus;
}

/**
  Function to dump the Bcfg information.

  @param[in] Op             The operation.
  @param[in] OrderCount     How many to dump.
  @param[in] CurrentOrder   The pointer to the current order of items.
  @param[in] VerboseOutput  TRUE for extra output.  FALSE otherwise.

  @retval SHELL_SUCCESS           The dump was successful.
  @retval SHELL_INVALID_PARAMETER A parameter was invalid.
**/
SHELL_STATUS
BcfgDisplayDump(
  IN CONST CHAR16   *Op,
  IN CONST UINTN    OrderCount,
  IN CONST UINT16   *CurrentOrder,
  IN CONST BOOLEAN  VerboseOutput
  )
{
  EFI_STATUS      Status;
  UINT8           *Buffer;
  UINTN           BufferSize;
  CHAR16          VariableName[12];
  UINTN           LoopVar;
  CHAR16          *DevPathString;
  VOID            *FilePathList;
  UINTN           Errors;
  EFI_LOAD_OPTION *LoadOption;
  CHAR16          *Description;
  UINTN           DescriptionSize;
  UINTN           OptionalDataOffset;

  if (OrderCount == 0) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_BCFG_NONE), gShellBcfgHiiHandle, L"bcfg");
    return (SHELL_SUCCESS);
  }

  Errors = 0;

  for (LoopVar = 0 ; LoopVar < OrderCount ; LoopVar++) {
    Buffer        = NULL;
    BufferSize    = 0;
    DevPathString = NULL;

    UnicodeSPrint(VariableName, sizeof(VariableName), L"%s%04x", Op, CurrentOrder[LoopVar]);

    Status = gRT->GetVariable(
        VariableName,
        (EFI_GUID*)&gEfiGlobalVariableGuid,
        NULL,
        &BufferSize,
        Buffer);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Buffer = AllocateZeroPool(BufferSize);
      Status = gRT->GetVariable(
          VariableName,
          (EFI_GUID*)&gEfiGlobalVariableGuid,
          NULL,
          &BufferSize,
          Buffer);
    }

    if (EFI_ERROR(Status) || Buffer == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_READ_FAIL), gShellBcfgHiiHandle, L"bcfg", VariableName);
      ++Errors;
      goto Cleanup;
    }

    //
    // We expect the Attributes, FilePathListLength, and L'\0'-terminated
    // Description fields to be present.
    //
    if (BufferSize < sizeof *LoadOption + sizeof (CHAR16)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_BCFG_VAR_CORRUPT),
        gShellBcfgHiiHandle,
        L"bcfg",
        VariableName
        );
      ++Errors;
      goto Cleanup;
    }

    LoadOption      = (EFI_LOAD_OPTION *)Buffer;
    Description     = (CHAR16*)(Buffer + sizeof (EFI_LOAD_OPTION));
    DescriptionSize = StrSize (Description);

    if (LoadOption->FilePathListLength != 0) {
      FilePathList = (UINT8 *)Description + DescriptionSize;
      DevPathString = ConvertDevicePathToText(FilePathList, TRUE, FALSE);
    }

    OptionalDataOffset = sizeof *LoadOption + DescriptionSize +
                         LoadOption->FilePathListLength;

    ShellPrintHiiEx(
      -1,
      -1,
      NULL,
      STRING_TOKEN(STR_BCFG_LOAD_OPTIONS),
      gShellBcfgHiiHandle,
      LoopVar,
      VariableName,
      Description,
      DevPathString,
      OptionalDataOffset >= BufferSize ? L'N' : L'Y'
      );
    if (VerboseOutput && (OptionalDataOffset < BufferSize)) {
      DumpHex (
        2,                               // Indent
        0,                               // Offset (displayed)
        BufferSize - OptionalDataOffset, // DataSize
        Buffer + OptionalDataOffset      // UserData
        );
    }

Cleanup:
    if (Buffer != NULL) {
      FreePool(Buffer);
    }
    if (DevPathString != NULL) {
      FreePool(DevPathString);
    }
  }
  return (Errors > 0) ? SHELL_INVALID_PARAMETER : SHELL_SUCCESS;
}

/**
  Function to initialize the BCFG operation structure.

  @param[in] Struct   The stuct to initialize.
**/
VOID
InitBcfgStruct(
  IN BGFG_OPERATION *Struct
  )
{
  ASSERT(Struct != NULL);
  Struct->Target      = BcfgTargetMax;
  Struct->Type        = BcfgTypeMax;
  Struct->Number1     = 0;
  Struct->Number2     = 0;
  Struct->HandleIndex = 0;
  Struct->FileName    = NULL;
  Struct->Description = NULL;
  Struct->Order       = NULL;
  Struct->OptData     = NULL;
}


STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-v", TypeFlag},
  {L"-opt", TypeMaxValue},
  {NULL, TypeMax}
  };

/**
  Function for 'bcfg' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunBcfg (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  LIST_ENTRY            *Package;
  CHAR16                *ProblemParam;
  SHELL_STATUS          ShellStatus;
  UINTN                 ParamNumber;
  CONST CHAR16          *CurrentParam;
  BGFG_OPERATION        CurrentOperation;
  UINTN                 Length;
  UINT64                Intermediate;
  UINT16                Count;

  Length              = 0;
  ProblemParam        = NULL;
  Package             = NULL;
  ShellStatus         = SHELL_SUCCESS;

  InitBcfgStruct(&CurrentOperation);

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellBcfgHiiHandle, L"bcfg", ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // Read in if we are doing -OPT
    //
    if (ShellCommandLineGetFlag(Package, L"-opt")) {
      CurrentOperation.OptData = ShellCommandLineGetValue(Package, L"-opt");
      if (CurrentOperation.OptData == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellBcfgHiiHandle, L"bcfg", L"-opt");
        ShellStatus = SHELL_INVALID_PARAMETER;
      }
      CurrentOperation.Type = BcfgTypeOpt;
    }

    //
    // small block to read the target of the operation
    //
    if ((ShellCommandLineGetCount(Package) < 3 && CurrentOperation.Type != BcfgTypeOpt) ||
        (ShellCommandLineGetCount(Package) < 2 && CurrentOperation.Type == BcfgTypeOpt)
       ){
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellBcfgHiiHandle, L"bcfg");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)ShellCommandLineGetRawValue(Package, 1), L"driver") == 0) {
      CurrentOperation.Target = BcfgTargetDriverOrder;
    } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)ShellCommandLineGetRawValue(Package, 1), L"boot") == 0) {
      CurrentOperation.Target = BcfgTargetBootOrder;
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_DRIVER_BOOT), gShellBcfgHiiHandle, L"bcfg");
      ShellStatus = SHELL_INVALID_PARAMETER;
    }


    //
    // Read in the boot or driver order environment variable (not needed for opt)
    //
    if (ShellStatus == SHELL_SUCCESS && CurrentOperation.Target < BcfgTargetMax) {
      Length = 0;
      Status = gRT->GetVariable(
        CurrentOperation.Target == BcfgTargetBootOrder?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
        (EFI_GUID*)&gEfiGlobalVariableGuid,
        NULL,
        &Length,
        CurrentOperation.Order);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        CurrentOperation.Order = AllocateZeroPool(Length+(4*sizeof(CurrentOperation.Order[0])));
        if (CurrentOperation.Order == NULL) {
          ShellStatus = SHELL_OUT_OF_RESOURCES;
        } else {
          Status = gRT->GetVariable(
            CurrentOperation.Target == BcfgTargetBootOrder?(CHAR16*)L"BootOrder":(CHAR16*)L"DriverOrder",
            (EFI_GUID*)&gEfiGlobalVariableGuid,
            NULL,
            &Length,
            CurrentOperation.Order);
        }
      }
    }

    Count = (UINT16) (Length / sizeof(CurrentOperation.Order[0]));

    //
    // large block to read the type of operation and verify parameter types for the info.
    //
    if (ShellStatus == SHELL_SUCCESS && CurrentOperation.Target < BcfgTargetMax) {
      for (ParamNumber = 2 ; ParamNumber < ShellCommandLineGetCount(Package) && ShellStatus == SHELL_SUCCESS; ParamNumber++) {
        CurrentParam = ShellCommandLineGetRawValue(Package, ParamNumber);
        if        (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"dump") == 0)    {
          CurrentOperation.Type = BcfgTypeDump;
          if (ShellCommandLineGetCount(Package) > 3) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellBcfgHiiHandle, L"bcfg");
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
        } else if (ShellCommandLineGetFlag(Package, L"-v")) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", L"-v (without dump)");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"add") == 0)     {
          if ((ParamNumber + 3) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellBcfgHiiHandle, L"bcfg");
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BcfgTypeAdd;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
            CurrentOperation.Number1     = (UINT16)Intermediate;
            ASSERT(CurrentOperation.FileName == NULL);
            CurrentOperation.FileName    = StrnCatGrow(&CurrentOperation.FileName   , NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
            ASSERT(CurrentOperation.Description == NULL);
            CurrentOperation.Description = StrnCatGrow(&CurrentOperation.Description, NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
          }
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"addp") == 0)    {
          if ((ParamNumber + 3) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellBcfgHiiHandle, L"bcfg");
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BcfgTypeAddp;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
            CurrentOperation.Number1     = (UINT16)Intermediate;
            ASSERT(CurrentOperation.FileName == NULL);
            CurrentOperation.FileName    = StrnCatGrow(&CurrentOperation.FileName   , NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
            ASSERT(CurrentOperation.Description == NULL);
            CurrentOperation.Description = StrnCatGrow(&CurrentOperation.Description, NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
          }
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"addh") == 0)    {
          if ((ParamNumber + 3) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellBcfgHiiHandle, L"bcfg");
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BcfgTypeAddh;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
            CurrentOperation.Number1     = (UINT16)Intermediate;
            CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
            if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", CurrentParam);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
              CurrentOperation.HandleIndex = (UINT16)Intermediate;
              ASSERT(CurrentOperation.Description == NULL);
              CurrentOperation.Description = StrnCatGrow(&CurrentOperation.Description, NULL, ShellCommandLineGetRawValue(Package, ++ParamNumber), 0);
            }
          }
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"rm") == 0)      {
          if ((ParamNumber + 1) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellBcfgHiiHandle, L"bcfg");
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BcfgTypeRm;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
            CurrentOperation.Number1     = (UINT16)Intermediate;
            if (CurrentOperation.Number1 >= Count){
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_NUMB_RANGE), gShellBcfgHiiHandle, L"bcfg", Count);
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
          }
        } else if (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)CurrentParam, L"mv") == 0)      {
          if ((ParamNumber + 2) >= ShellCommandLineGetCount(Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellBcfgHiiHandle, L"bcfg");
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          CurrentOperation.Type = BcfgTypeMv;
          CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
          if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", CurrentParam);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
            CurrentOperation.Number1     = (UINT16)Intermediate;
            if (CurrentOperation.Number1 >= Count){
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_NUMB_RANGE), gShellBcfgHiiHandle, L"bcfg", Count);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              CurrentParam = ShellCommandLineGetRawValue(Package, ++ParamNumber);
              if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber(CurrentParam, TRUE, FALSE)) {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", CurrentParam);
                ShellStatus = SHELL_INVALID_PARAMETER;
              } else {
                Status = ShellConvertStringToUint64(CurrentParam, &Intermediate, TRUE, FALSE);
                CurrentOperation.Number2     = (UINT16)Intermediate;
              }
              if (CurrentOperation.Number2 == CurrentOperation.Number1
                ||CurrentOperation.Number2 >= Count
               ){
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_BCFG_NUMB_RANGE), gShellBcfgHiiHandle, L"bcfg", Count);
                ShellStatus = SHELL_INVALID_PARAMETER;
              }
            }
          }
        }
        else if (gUnicodeCollation->StriColl (gUnicodeCollation, (CHAR16*)CurrentParam, L"mod") == 0) {
          if ((ParamNumber + 2) >= ShellCommandLineGetCount (Package)) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellBcfgHiiHandle, L"bcfg");
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            CurrentOperation.Type = BcfgTypeMod;
            CurrentParam = ShellCommandLineGetRawValue (Package, ++ParamNumber);
            if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber (CurrentParam, TRUE, FALSE)) {
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", CurrentParam);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              Status = ShellConvertStringToUint64 (CurrentParam, &Intermediate, TRUE, FALSE);
              CurrentOperation.Number1 = (UINT16)Intermediate;
              if (CurrentOperation.Number1 >= Count) {
                ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_BCFG_NUMB_RANGE), gShellBcfgHiiHandle, L"bcfg", Count);
                ShellStatus = SHELL_INVALID_PARAMETER;
              } else {
                ASSERT (CurrentOperation.Description == NULL);
                CurrentOperation.Description = StrnCatGrow (&CurrentOperation.Description, NULL, ShellCommandLineGetRawValue (Package, ++ParamNumber), 0);
              }
            }
          }
        } else if (gUnicodeCollation->StriColl (gUnicodeCollation, (CHAR16*)CurrentParam, L"modf") == 0) {
          if ((ParamNumber + 2) >= ShellCommandLineGetCount (Package)) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellBcfgHiiHandle, L"bcfg");
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            CurrentOperation.Type = BcfgTypeModf;
            CurrentParam = ShellCommandLineGetRawValue (Package, ++ParamNumber);
            if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber (CurrentParam, TRUE, FALSE)) {
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", CurrentParam);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              Status = ShellConvertStringToUint64  (CurrentParam, &Intermediate, TRUE, FALSE);
              CurrentOperation.Number1 = (UINT16)Intermediate;
              if (CurrentOperation.Number1 >= Count) {
                ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_BCFG_NUMB_RANGE), gShellBcfgHiiHandle, L"bcfg", Count);
                ShellStatus = SHELL_INVALID_PARAMETER;
              } else {
                ASSERT (CurrentOperation.FileName == NULL);
                CurrentOperation.FileName = StrnCatGrow (&CurrentOperation.FileName, NULL, ShellCommandLineGetRawValue (Package, ++ParamNumber), 0);
              }
            }
          }
        } else if (gUnicodeCollation->StriColl (gUnicodeCollation, (CHAR16*)CurrentParam, L"modp") == 0) {
          if ((ParamNumber + 2) >= ShellCommandLineGetCount (Package)) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellBcfgHiiHandle, L"bcfg");
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            CurrentOperation.Type = BcfgTypeModp;
            CurrentParam = ShellCommandLineGetRawValue (Package, ++ParamNumber);
            if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber (CurrentParam, TRUE, FALSE)) {
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", CurrentParam);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              Status = ShellConvertStringToUint64 (CurrentParam, &Intermediate, TRUE, FALSE);
              CurrentOperation.Number1 = (UINT16)Intermediate;
              if (CurrentOperation.Number1 >= Count) {
                ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_BCFG_NUMB_RANGE), gShellBcfgHiiHandle, L"bcfg", Count);
                ShellStatus = SHELL_INVALID_PARAMETER;
              } else {
                ASSERT (CurrentOperation.FileName == NULL);
                CurrentOperation.FileName = StrnCatGrow (&CurrentOperation.FileName, NULL, ShellCommandLineGetRawValue (Package, ++ParamNumber), 0);
              }
            }
          }
        } else if (gUnicodeCollation->StriColl (gUnicodeCollation, (CHAR16*)CurrentParam, L"modh") == 0) {
          if ((ParamNumber + 2) >= ShellCommandLineGetCount (Package)) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellBcfgHiiHandle, L"bcfg");
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            CurrentOperation.Type = BcfgTypeModh;
            CurrentParam = ShellCommandLineGetRawValue (Package, ++ParamNumber);
            if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber (CurrentParam, TRUE, FALSE)) {
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", CurrentParam);
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
            else {
              Status = ShellConvertStringToUint64 (CurrentParam, &Intermediate, TRUE, FALSE);
              CurrentOperation.Number1 = (UINT16)Intermediate;
              if (CurrentOperation.Number1 >= Count) {
                ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_BCFG_NUMB_RANGE), gShellBcfgHiiHandle, L"bcfg", Count);
                ShellStatus = SHELL_INVALID_PARAMETER;
              } else {
                CurrentParam = ShellCommandLineGetRawValue (Package, ++ParamNumber);
                if (CurrentParam == NULL || !ShellIsHexOrDecimalNumber (CurrentParam, TRUE, FALSE)) {
                  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", CurrentParam);
                  ShellStatus = SHELL_INVALID_PARAMETER;
                } else {
                  Status = ShellConvertStringToUint64 (CurrentParam, &Intermediate, TRUE, FALSE);
                  CurrentOperation.HandleIndex = (UINT16)Intermediate;
                }
              }
            }
          }
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellBcfgHiiHandle, L"bcfg", CurrentParam);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      }
    }
    if (ShellStatus == SHELL_SUCCESS && CurrentOperation.Target < BcfgTargetMax && CurrentOperation.Type < BcfgTypeMax) {
      //
      // we have all the info.  Do the work
      //
      switch (CurrentOperation.Type) {
        case   BcfgTypeDump:
          ShellStatus = BcfgDisplayDump(
            CurrentOperation.Target == BcfgTargetBootOrder?L"Boot":L"Driver",
            Count,
            CurrentOperation.Order,
            ShellCommandLineGetFlag(Package, L"-v"));
          break;
        case   BcfgTypeMv:
          ShellStatus = BcfgMove(
            CurrentOperation.Target,
            CurrentOperation.Order,
            Count,
            CurrentOperation.Number1,
            CurrentOperation.Number2);
          break;
        case   BcfgTypeRm:
          ShellStatus = BcfgRemove(
            CurrentOperation.Target,
            CurrentOperation.Order,
            Count,
            CurrentOperation.Number1);
          break;
        case   BcfgTypeAdd:
        case   BcfgTypeAddp:
        case   BcfgTypeAddh:
          ShellStatus = BcfgAdd(
            CurrentOperation.Number1,
            CurrentOperation.FileName,
            CurrentOperation.Description==NULL?L"":CurrentOperation.Description,
            CurrentOperation.Order,
            Count,
            CurrentOperation.Target,
            (BOOLEAN)(CurrentOperation.Type == BcfgTypeAddh),
            (BOOLEAN)(CurrentOperation.Type == BcfgTypeAddp),
            CurrentOperation.HandleIndex);
          break;
        case   BcfgTypeMod:
        case   BcfgTypeModf:
        case   BcfgTypeModp:
        case   BcfgTypeModh:
          ShellStatus = BcfgMod (&CurrentOperation, Count);
          break;
        case   BcfgTypeOpt:
          ShellStatus = BcfgAddOpt(
            CurrentOperation.OptData,
            CurrentOperation.Order,
            Count,
            CurrentOperation.Target);
          break;
        default:
          ASSERT(FALSE);
      }
    }
  }

  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }
  if (CurrentOperation.FileName != NULL) {
    FreePool(CurrentOperation.FileName);
  }
  if (CurrentOperation.Description != NULL) {
    FreePool(CurrentOperation.Description);
  }
  if (CurrentOperation.Order != NULL) {
    FreePool(CurrentOperation.Order);
  }

  return (ShellStatus);
}


/**
  Function to get the filename with help context if HII will not be used.

  @return   The filename with help text in it.
**/
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameBcfg (
  VOID
  )
{
  return (mFileName);
}

/**
  "Constructor" for the library.

  This will register the handler for the bcfg command.

  @param[in] ImageHandle    the image handle of the process
  @param[in] SystemTable    the EFI System Table pointer
  @param[in] Name           the profile name to use

  @retval EFI_SUCCESS        the shell command handlers were installed sucessfully
  @retval EFI_UNSUPPORTED    the shell level required was not found.
**/
EFI_STATUS
EFIAPI
BcfgLibraryRegisterBcfgCommand (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable,
  IN CONST CHAR16      *Name
  )
{
  if (gShellBcfgHiiHandle != NULL) {
    return (EFI_SUCCESS);
  }

  gShellBcfgHiiHandle = HiiAddPackages (&gShellBcfgHiiGuid, gImageHandle, UefiShellBcfgCommandLibStrings, NULL);
  if (gShellBcfgHiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }

  //
  // install our shell command handler
  //
  ShellCommandRegisterCommandName(L"bcfg", ShellCommandRunBcfg , ShellCommandGetManFileNameBcfg, 0, Name, FALSE, gShellBcfgHiiHandle, STRING_TOKEN(STR_GET_HELP_BCFG));

  return (EFI_SUCCESS);
}

/**
  Destructor for the library.  free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
BcfgLibraryUnregisterBcfgCommand (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gShellBcfgHiiHandle != NULL) {
    HiiRemovePackages(gShellBcfgHiiHandle);
  }
  gShellBcfgHiiHandle = NULL;
  return (EFI_SUCCESS);
}

