/** @file
  CxlFirmwareMgmt Application is used to send and receive FMP commands
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CxlFwMgmtApp.h"

/**
  Opens the file and read its buffer value

  @param[in] FileName                The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] BufferSize              size of buffer to be read

  @retval Buffer                     input file is read and buffer is returned

  **/
EFI_STATUS
ReadFileToBuffer (
  IN  CHAR16  *FileName,
  OUT UINTN   *BufferSize,
  OUT VOID    **Buffer
  )
{
  EFI_STATUS          Status;
  SHELL_FILE_HANDLE   FileHandle;
  UINT64              FileSize;
  VOID                *TempBuffer;
  UINTN               Position;
  EFI_SHELL_PROTOCOL  *ShellProtocol;
  UINTN               TempBufferSize;

  FileSize      = 0;
  Position      = 0;
  ShellProtocol = NULL;

  Status = gBS->LocateProtocol (
                  &gEfiShellProtocolGuid,
                  NULL,
                  (VOID **)&ShellProtocol
                  );

  if (EFI_ERROR (Status)) {
    Print (L"ReadFileToBuffer: Error gEfiShellProtocolGuid %r\n", Status);
    ShellProtocol = NULL;
    *BufferSize   = 0;
    *Buffer       = NULL;
    return Status;
  }

  Status = ShellProtocol->OpenFileByName (
                            FileName,
                            &FileHandle,
                            EFI_FILE_MODE_READ
                            );

  if (EFI_ERROR (Status)) {
    Print (L"ReadFileToBuffer: Error open file by name %r\n", Status);
    return Status;
  }

  Status = ShellProtocol->GetFileSize (FileHandle, &FileSize);

  if (EFI_ERROR (Status)) {
    ShellProtocol->CloseFile (FileHandle);
    return Status;
  }

  if (FileSize > CXL_FW_SIZE) {
    Print (L"ReadFileToBuffer: Error FileSize = %d is greater then 32 MB\n", FileSize);
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  TempBufferSize = (UINTN)FileSize;
  TempBuffer     = AllocateZeroPool (TempBufferSize);
  if (NULL == TempBuffer) {
    ShellProtocol->CloseFile (FileHandle);
    return Status;
  }

  Status = ShellProtocol->SetFilePosition (FileHandle, Position);
  if (EFI_ERROR (Status)) {
    Print (L"Error in setting position...%r (Position = 0x%X)\n", Status, Position);
    return Status;
  }

  Status = ShellProtocol->ReadFile (
                            FileHandle,
                            &TempBufferSize,
                            TempBuffer
                            );

  if (EFI_ERROR (Status)) {
    ShellProtocol->CloseFile (FileHandle);
    return Status;
  }

  Status = ShellProtocol->CloseFile (FileHandle);

  *BufferSize = TempBufferSize;
  *Buffer     = TempBuffer;
  return EFI_SUCCESS;
}

/**
  Get lots of info about a device from its handle.

  @param[in]  NumOfHandles         Number of handle to loop among all

  @retval Handles                  return handle of firmware management protocol

  **/
EFI_STATUS
GetHandleInfo (
  UINTN       *NumOfHandles,
  EFI_HANDLE  **Handles
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareManagementProtocolGuid,
                  NULL,
                  NumOfHandles,
                  Handles
                  );

  if (EFI_ERROR (Status)) {
    Print (L"GetHandleInfo: LocateHandleBuffer failed status = %r, NumOfHandles = %d\n", Status, *NumOfHandles);
    return Status;
  }

  if (0 == *NumOfHandles) {
    Print (L"GetHandleInfo: Handle not found status = %r, NoHandles = %d\n", Status, *NumOfHandles);
    return EFI_NOT_FOUND;
  }

  return Status;
}

/**
  Get CXL controller private data structure from firmware management

  @param[in] Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] Handles                  Number of handle to loop among all
  @param[in] Index                    Index of handle

  @retval Status                      Return EFI_SUCCESS on successfully getting the data

  **/
EFI_STATUS
GetCxlPrivateData (
  CXL_CONTROLLER_PRIVATE_DATA  **Private1,
  EFI_HANDLE                   *Handles,
  UINTN                        Index
  )
{
  EFI_STATUS                        Status;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *FirmwareMgmt;

  Status       = EFI_SUCCESS;
  FirmwareMgmt = NULL;

  Status = gBS->HandleProtocol (
                  Handles[Index],
                  &gEfiFirmwareManagementProtocolGuid,
                  (VOID **)&FirmwareMgmt
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Private1 = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT (FirmwareMgmt);
  if (CXL_CONTROLLER_PRIVATE_DATA_SIGNATURE != (*Private1)->Signature) {
    Print (L"\ngetPrivateStr: Error, Private Data is not for CXL device!\n");
    Status = EFI_NOT_FOUND;
  }

  return Status;
}

/**
  Returns information about the current firmware image(s) of the device.

  @param[in] Bus                  Bus value in BDF of device.
  @param[in] Device               Device value in BDF of device.
  @param[in] Function             Function value in BDF of device.

  @retval Status                  Return EFI_SUCCESS on successfully getting the data

  **/
EFI_STATUS
GetImageInfo (
  UINTN  Bus,
  UINTN  Device,
  UINTN  Function
  )
{
  EFI_STATUS                     Status;
  EFI_HANDLE                     *Handles;
  UINTN                          Index;
  UINTN                          NumOfHandles;
  UINTN                          ImageInfoSize;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR  *ImageInfo;
  UINT32                         DescriptorVersion;
  UINT8                          DescriptorCount;
  UINTN                          DescriptorSize;
  UINT32                         PackageVersion;
  CHAR16                         *PackageVersionName;
  CXL_CONTROLLER_PRIVATE_DATA    *Private;

  Status             = EFI_SUCCESS;
  Handles            = NULL;
  NumOfHandles       = 0;
  ImageInfoSize      = 0;
  ImageInfo          = NULL;
  DescriptorVersion  = 0;
  DescriptorCount    = 0;
  DescriptorSize     = 0;
  PackageVersion     = 0;
  PackageVersionName = NULL;
  Private            = NULL;

  Status = GetHandleInfo (&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print (L"GetImageInfo: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = GetCxlPrivateData (&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
      Print (L"GetImageInfo: Fail to locate handle buffer...\n");
      continue;
    }

    if ((Bus == Private->Bus) && (Device == Private->Device) && (Function == Private->Function)) {
      Status = Private->FirmwareMgmt.GetImageInfo (
                                       &Private->FirmwareMgmt,
                                       &ImageInfoSize,
                                       ImageInfo,
                                       &DescriptorVersion,
                                       &DescriptorCount,
                                       &DescriptorSize,
                                       &PackageVersion,
                                       &PackageVersionName
                                       );

      if (Status == EFI_BUFFER_TOO_SMALL) {
        ImageInfo = AllocateZeroPool (ImageInfoSize);
        if (ImageInfo == NULL) {
          DEBUG ((DEBUG_ERROR, "GetImageInfo: AllocateZeroPool failed!\n"));
          Status = EFI_OUT_OF_RESOURCES;
          return Status;
        }

        Status = Private->FirmwareMgmt.GetImageInfo (
                                         &Private->FirmwareMgmt,
                                         &ImageInfoSize,
                                         ImageInfo,
                                         &DescriptorVersion,
                                         &DescriptorCount,
                                         &DescriptorSize,
                                         &PackageVersion,
                                         &PackageVersionName
                                         );
      }

      if (!EFI_ERROR (Status)) {
        Print (L"===== Current Firmware Image Information =====\n");
        Print (L"Package Version         : %08X\n", PackageVersion);
        Print (L"Package Version Name    : %s\n", PackageVersionName);
        if (ImageInfo != NULL) {
          Print (L"Image Index             : %d\n", ImageInfo->ImageIndex);
          Print (L"Image Type ID           : %g\n", ImageInfo->ImageTypeId);
          Print (L"Image ID                : %016lx\n", ImageInfo->ImageId);
          Print (L"Image ID Name           : %s\n", ImageInfo->ImageIdName);
          Print (L"Version                 : %d\n", ImageInfo->Version);
          Print (L"Version Name            : %a\n", ImageInfo->VersionName);
          Print (L"Size                    : %d\n", ImageInfo->Size);
          Print (L"Attributes Supported    : %d\n", ImageInfo->AttributesSupported);
          Print (L"Attributes Setting      : %d\n", ImageInfo->AttributesSetting);
          Print (L"Compatibilities         : %d\n", ImageInfo->Compatibilities);
        }
      } else {
        Print (L"Calling GetImageInfo Failed with status = %r\n", Status);
      }

      break;
    }
  }

  FreePool (Handles);

  if (NULL != PackageVersionName) {
    FreePool (PackageVersionName);
  }

  if (NULL != ImageInfo) {
    FreePool (ImageInfo);
  }

  return Status;
}

/**
  Returns information about the firmware package of device

  @param[in] Bus                  Bus value in BDF of device.
  @param[in] Device               Device value in BDF of device.
  @param[in] Function             Function value in BDF of device.

  @retval Status                  Return EFI_SUCCESS on successfully getting the data

  **/
EFI_STATUS
GetPackageInfo (
  UINTN  Bus,
  UINTN  Device,
  UINTN  Function
  )
{
  EFI_STATUS                   Status;
  EFI_HANDLE                   *Handles;
  UINTN                        Index;
  UINTN                        NumOfHandles;
  UINT32                       PackageVersion;
  CHAR16                       *PackageVersionName;
  UINT32                       PackageVersionNameMaxLen;
  UINT64                       AttributesSupported;
  UINT64                       AttributesSetting;
  CXL_CONTROLLER_PRIVATE_DATA  *Private;

  Status                   = EFI_SUCCESS;
  Handles                  = NULL;
  NumOfHandles             = 0;
  PackageVersion           = 0;
  PackageVersionName       = NULL;
  PackageVersionNameMaxLen = 0;
  AttributesSupported      = 0;
  AttributesSetting        = 0;
  Private                  = NULL;

  Status = GetHandleInfo (&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print (L"GetPackageInfo: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = GetCxlPrivateData (&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
      Print (L"GetImageInfo: Fail to locate handle buffer...\n");
      continue;
    }

    if ((Bus == Private->Bus) && (Device == Private->Device) && (Function == Private->Function)) {
      PackageVersionName = AllocateZeroPool (CXL_STRING_BUFFER_WIDTH);
      if (PackageVersionName == NULL) {
        DEBUG ((DEBUG_ERROR, "GetImageInfo: AllocateZeroPool failed!\n"));
        Status = EFI_OUT_OF_RESOURCES;
        return Status;
      }

      Status = Private->FirmwareMgmt.GetPackageInfo (
                                       &Private->FirmwareMgmt,
                                       &PackageVersion,
                                       &PackageVersionName,
                                       &PackageVersionNameMaxLen,
                                       &AttributesSupported,
                                       &AttributesSetting
                                       );

      if (!EFI_ERROR (Status)) {
        Print (L"Package Version Name      : %s\n", PackageVersionName);
        Print (L"Package Version           : %d\n", PackageVersion);
        Print (L"Attributes Supported      : %d\n", AttributesSupported);
        Print (L"Attributes Setting        : %d\n", AttributesSetting);
      } else {
        Print (L"Calling FMP.GetPackageInfo...%r\n", Status);
      }

      if (NULL != PackageVersionName) {
        FreePool (PackageVersionName);
      }

      break;
    }
  }

  FreePool (Handles);
  return Status;
}

/**
  Returns information about the image.of device

  @param[in] Bus                  Bus value in BDF of device.
  @param[in] Device               Device value in BDF of device.
  @param[in] Function             Function value in BDF of device.
  @param[in] Slot                 Slot information on which image info is to be taken

  @retval Status                  Return EFI_SUCCESS on successfully getting the data

  **/
EFI_STATUS
GetImage (
  UINTN  Bus,
  UINTN  Device,
  UINTN  Function,
  UINTN  Slot
  )
{
  EFI_STATUS                   Status;
  UINT8                        ImageIndex;
  CHAR16                       *Image;
  UINTN                        ImageSize;
  EFI_HANDLE                   *Handles;
  UINTN                        Index;
  UINTN                        NumOfHandles;
  CXL_CONTROLLER_PRIVATE_DATA  *Private = NULL;

  Status       = EFI_SUCCESS;
  ImageIndex   = (UINT8)Slot;
  Image        = NULL;
  Handles      = NULL;
  NumOfHandles = 0;

  Status = GetHandleInfo (&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print (L"GetImage: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = GetCxlPrivateData (&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
      Print (L"GetImage: Fail to locate handle buffer...\n");
      continue;
    }

    if ((Bus == Private->Bus) && (Device == Private->Device) && (Function == Private->Function)) {
      Status = Private->FirmwareMgmt.GetImage (
                                       &Private->FirmwareMgmt,
                                       ImageIndex,
                                       Image,
                                       &ImageSize
                                       );

      if (Status == EFI_BUFFER_TOO_SMALL) {
        Print (L"\nGetImage: Image Allocated with Size = %d\n", ImageSize);
        Image = AllocateZeroPool (ImageSize);
        if (Image == NULL) {
          DEBUG ((DEBUG_ERROR, "GetImage: AllocateZeroPool failed!\n"));
          Status = EFI_OUT_OF_RESOURCES;
          return Status;
        }

        Status = Private->FirmwareMgmt.GetImage (
                                         &Private->FirmwareMgmt,
                                         ImageIndex,
                                         Image,
                                         &ImageSize
                                         );
      }

      if (!EFI_ERROR (Status)) {
        Print (L"GetImage, Image Size = %d\n", ImageSize);
      } else {
        Print (L"Calling FMP GetImage Failed...%r\n", Status);
      }

      break;
    }
  }

  if (NULL != Handles) {
    FreePool (Handles);
  }

  if (NULL != Image) {
    FreePool (Image);
  }

  return Status;
}

/**
  Updates information about the firmware package

  @param[in] Bus                  Bus value in BDF of device.
  @param[in] Device               Device value in BDF of device.
  @param[in] Function             Function value in BDF of device.

  @retval Status                  Return EFI_SUCCESS on successfully getting the data

  **/
EFI_STATUS
SetPackageInfo (
  UINTN  Bus,
  UINTN  Device,
  UINTN  Function
  )
{
  EFI_STATUS                   Status;
  EFI_HANDLE                   *Handles;
  UINTN                        Index;
  UINTN                        NumOfHandles;
  CONST VOID                   *Image;
  UINTN                        ImageSize;
  CONST VOID                   *VendorCode;
  UINT32                       PackageVersion;
  CHAR16                       PackageVersionName[CXL_STRING_BUFFER_WIDTH];
  CXL_CONTROLLER_PRIVATE_DATA  *Private;

  Status         = EFI_SUCCESS;
  Handles        = NULL;
  NumOfHandles   = 0;
  Image          = NULL;
  ImageSize      = 0;
  VendorCode     = NULL;
  PackageVersion = 1;

  Status = GetHandleInfo (&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print (L"SetPackageInfo: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = GetCxlPrivateData (&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
      Print (L"GetImageInfo: Fail to locate handle buffer...\n");
      continue;
    }

    if ((Bus == Private->Bus) && (Device == Private->Device) && (Function == Private->Function)) {
      StrCpyS (PackageVersionName, CXL_STRING_BUFFER_WIDTH, CXL_PACKAGE_VERSION_NAME_APP);
      Status = Private->FirmwareMgmt.SetPackageInfo (
                                       &Private->FirmwareMgmt,
                                       &Image,
                                       ImageSize,
                                       &VendorCode,
                                       PackageVersion,
                                       PackageVersionName
                                       );

      if (!EFI_ERROR (Status)) {
        Print (L"SetPackageInfo Success\n");
      } else {
        Print (L"Calling FMP SetPackageInfo Failed...%r\n", Status);
      }

      break;
    }
  }

  FreePool (Handles);
  return Status;
}

/**
  Gets BUS, DEVICE, FUNCTION value of all CXL supported device

  @retval Status                  Return EFI_SUCCESS on successfully getting the data

  **/
EFI_STATUS
GetCxlDeviceList (
  )
{
  EFI_HANDLE                   *Handles = NULL;
  UINTN                        Index;
  EFI_STATUS                   Status       = EFI_SUCCESS;
  UINTN                        NumOfHandles = 0;
  bool                         printFlag    = FALSE;
  CXL_CONTROLLER_PRIVATE_DATA  *Private     = NULL;

  Handles      = NULL;
  Status       = EFI_SUCCESS;
  NumOfHandles = 0;
  printFlag    = FALSE;
  Private      = NULL;

  Status = GetHandleInfo (&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print (L"GetCxlDeviceList: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = GetCxlPrivateData (&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
      Print (L"GetImageInfo: Fail to locate handle buffer...\n");
      continue;
    }

    if (printFlag == FALSE) {
      Print (L"Device          BUS    DEVICE    FUNCTION   \n");
      printFlag = TRUE;
    }

    Print (L"CXLDevice[%d]:   %d       %d          %d\n", Index, Private->Bus, Private->Device, Private->Function);
  }

  FreePool (Handles);
  return Status;
}

/**
  Updates the firmware image of the device.

  @param[in] Bus                  Bus value in BDF of device.
  @param[in] Device               Device value in BDF of device.
  @param[in] Function             Function value in BDF of device.
  @param[in] Slot                 Slot information on which image info is to be taken

  @retval Status                  Return EFI_SUCCESS on successfully getting the data

  **/
EFI_STATUS
SetImage (
  UINTN   Bus,
  UINTN   Device,
  UINTN   Function,
  UINTN   Slot,
  CHAR16  *FileName
  )
{
  VOID                         *Buffer;
  UINTN                        BufferSize;
  EFI_STATUS                   Status;
  EFI_HANDLE                   *Handles;
  UINTN                        Index;
  UINTN                        NumOfHandles;
  CXL_CONTROLLER_PRIVATE_DATA  *Private;

  Status       = EFI_SUCCESS;
  Handles      = NULL;
  NumOfHandles = 0;
  Private      = NULL;

  Status = GetHandleInfo (&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print (L"SetImage: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = GetCxlPrivateData (&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
      Print (L"GetImageInfo: Fail to locate handle buffer...\n");
      continue;
    }

    if ((Bus == Private->Bus) && (Device == Private->Device) && (Function == Private->Function)) {
      Status = ReadFileToBuffer (FileName, &BufferSize, &Buffer);
      if (EFI_SUCCESS != Status) {
        Print (L"SetImage: ReadFileToBuffer FMP SetImage Failed...%r\n", Status);
        break;
      }

      Status = Private->FirmwareMgmt.SetImage (
                                       &Private->FirmwareMgmt,
                                       (UINT8)Slot,
                                       Buffer,
                                       BufferSize,
                                       NULL,
                                       NULL,
                                       NULL
                                       );

      if (!EFI_ERROR (Status)) {
        Print (L"SetImage Success\n");
      } else {
        Print (L"SetImage: Calling FMP SetImage Failed...%r\n", Status);
      }

      break;
    }
  }

  FreePool (Handles);
  return Status;
}

/**
  Check the firmware image of the device.

  @param[in] Bus                  Bus value in BDF of device.
  @param[in] Device               Device value in BDF of device.
  @param[in] Function             Function value in BDF of device.
  @param[in] Slot                 Slot information on which image info is to be taken

  @retval Status                  Return EFI_SUCCESS on successfully getting the data

  **/
EFI_STATUS
CheckImage (
  UINTN  Bus,
  UINTN  Device,
  UINTN  Function,
  UINTN  Slot
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  Print (L"CheckImage Command not supported...\n");
  return Status;
}

/**
  Prints help page of the CXL Firmware management application

  **/
void
PrintHelpPage (
  )
{
  Print (L" -fGetCXLDeviceList                                          : Get CXL Device List (GetCxlDeviceList).\n");
  Print (L" -fimginfo <b: Bus> <d: Device> <f: Function>                : Get information about current firmware image (GetImageInfo).\n");
  Print (L" -fsetimg  <b: Bus> <d: Device> <f: Function> <Slot> <file>  : Firmware download and ActivateFw (SetImage).\n");
  Print (L" -fgetimg  <b: Bus> <d: Device> <f: Function> <Slot>         : Get information about the firmware package (GetImage).\n");
  Print (L" -fchkimg  <b: Bus> <d: Device> <f: Function> <Slot> <file>  : Check the validity of a firmware image (CheckImage).\n");
  Print (L" -fsetpack <b: Bus> <d: Device> <f: Function>                : Set information about the firmware package (SetPackageInfo).\n");
  Print (L" -fgetpack <b: Bus> <d: Device> <f: Function>                : Get information about the firmware package (GetPackageInfo).\n");
}

/**
  Check the input character is digit or not.

  @param[in] Character                  Input character

  @retval Status                        Return value as 0 if input is character else 1

  **/
BOOLEAN
IsDigit (
  CHAR16  Character
  )
{
  return (Character >= '0') && (Character <= '9');
}

/**
  Check the length of input string.

  @param[in] String               Input string value.

  @retval Status                  Return length of input string

  **/
int
GetStrLength (
  CHAR16  *String
  )
{
  int  Length = 0;

  while (*String != '\0') {
    Length++;
    String++;
  }

  return Length;
}

/**
  Check the input string is number or not.

  @param[in] String               Input string value.

  @retval                         Return True if string is number else FALSE

  **/
BOOLEAN
IsNumber (
  CHAR16  *String
  )
{
  int  Length = GetStrLength (String);

  for (int Index = 0; Index < Length; Index++) {
    if (IsDigit (String[Index]) == FALSE) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Gets Bus device Function (BDF) value of the device

  @retval Bus                     Bus value in BDF of device.
  @retval Device                  Device value in BDF of device.
  @retval Function                Function value in BDF of device.
  @retval Slot                    Slot information on which image info is to be taken

  **/
BOOLEAN
GetBdfValues (
  UINTN   Argc,
  CHAR16  **Argv,
  UINTN   *Bus,
  UINTN   *Device,
  UINTN   *Function,
  UINTN   *Slot,
  CHAR16  **FileName
  )
{
  CHAR16  *Bus1;
  CHAR16  *Dev1;
  CHAR16  *Func1;
  CHAR16  *Slot1;
  CHAR16  *FileName1;

  Bus1      = NULL;
  Dev1      = NULL;
  Func1     = NULL;
  Slot1     = NULL;
  FileName1 = NULL;

  Bus1  = Argv[2];
  Dev1  = Argv[3];
  Func1 = Argv[4];

  *Bus      = StrDecimalToUintn (Argv[2]);
  *Device   = StrDecimalToUintn (Argv[3]);
  *Function = StrDecimalToUintn (Argv[4]);

  if (Argc >= 6) {
    Slot1 = Argv[5];
    *Slot = StrDecimalToUintn (Argv[5]);
    if (IsNumber (Slot1) == FALSE) {
      return FALSE;
    }
  }

  if (Argc == 7) {
    *FileName = AllocateZeroPool (CXL_MAX_FILE_NAME_LENGTH);
    if (NULL == *FileName) {
      DEBUG ((DEBUG_ERROR, "GetBdfValues: EFI Out of resources...\n"));
      return FALSE;
    }

    FileName1 = Argv[6];
    StrCpyS (*FileName, CXL_MAX_FILE_NAME_LENGTH, FileName1);
    if (IsNumber (FileName1) == TRUE) {
      return FALSE;
    }
  }

  if ((IsNumber (Bus1) == FALSE) || (IsNumber (Dev1) == FALSE) || (IsNumber (Func1) == FALSE)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Validates input argument value of Bus device Function (BDF) value of the device

  @param[in] Bus                  Bus value in BDF of device.
  @param[in] Device               Device value in BDF of device.
  @param[in] Function             Function value in BDF of device.
  @param[in] Slot                 Slot information on which image info is to be takenBOOLEAN

  @retval                         True if param are correct otherwise False
  **/
BOOLEAN
ValidateArguments (
  UINTN                   Argc,
  CHAR16                  **Argv,
  UINTN                   *Bus,
  UINTN                   *Device,
  UINTN                   *Function,
  UINTN                   *Slot,
  CHAR16                  **FileName,
  CXL_FMP_OPERATION_TYPE  OpType
  )
{
  bool  IsBdfRequire = TRUE;

  switch (OpType) {
    case OpTypeDisplayHelp:
      IsBdfRequire = FALSE;
      break;

    case OpTypeListDevice:
      if (Argc != 2) {
        Print (L"Invalid argument...\n");
        return FALSE;
      }

      IsBdfRequire = FALSE;
      break;

    case OpTypeFmpGetImgInfo:
      if (Argc != 5) {
        Print (L"Invalid argument...\n");
        return FALSE;
      }

      break;

    case OpTypeFmpSetImg:
      if (Argc != 7) {
        Print (L"Invalid argument...\n");
        return FALSE;
      }

      break;

    case OpTypeGetImage:
      if (Argc != 6) {
        Print (L"Invalid argument...\n");
        return FALSE;
      }

      break;

    case OpTypeFmpCheckImg:
      if (Argc != 7) {
        Print (L"Invalid argument...\n");
        return FALSE;
      }

      break;

    case OpTypeFmpGetPkgInfo:
      if (Argc != 5) {
        Print (L"Invalid argument...\n");
        return FALSE;
      }

      break;

    case OpTypeSetPkgInfo:
      if (Argc != 5) {
        Print (L"Invalid argument...\n");
        return FALSE;
      }

      break;

    default:
      return FALSE;
  }

  if (IsBdfRequire == TRUE) {
    if (FALSE == GetBdfValues (Argc, Argv, Bus, Device, Function, Slot, FileName)) {
      Print (L"Invalid argument...\n");
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Get operation type from input arguments in CXL Application

  @retval                          DisplayHelp, ListDevice, FmpGetImgInfo, GetImage, SetPkgInfo, FmpSetImg, FmpCheckImg, FmpGetPkgInfo

  **/
CXL_FMP_OPERATION_TYPE
GetOperationType (
  UINTN   Argc,
  CHAR16  **Argv
  )
{
  CXL_FMP_OPERATION_TYPE  OpType;
  CHAR16                  *String;

  String = NULL;

  if (1 == Argc) {
    OpType = OpTypeDisplayHelp;
    goto END;
  }

  String = Argv[1];

  if (!StrCmp (String, L"-fGetCXLDeviceList")) {
    OpType = OpTypeListDevice;
  } else if (!StrCmp (String, L"-fimginfo")) {
    OpType = OpTypeFmpGetImgInfo;
  } else if (!StrCmp (String, L"-fsetimg")) {
    OpType = OpTypeFmpSetImg;
  } else if (!StrCmp (String, L"-fgetimg")) {
    OpType = OpTypeGetImage;
  } else if (!StrCmp (String, L"-fchkimg")) {
    OpType = OpTypeFmpCheckImg;
  } else if (!StrCmp (String, L"-fsetpack")) {
    OpType = OpTypeSetPkgInfo;
  } else if (!StrCmp (String, L"-fgetpack")) {
    OpType = OpTypeFmpGetPkgInfo;
  } else {
    Print (L"Invalid argument...\n");
    OpType = OpTypeDisplayHelp;
  }

END:
  return OpType;
}

/**
  Parse the argument from input arguments in CXL Application

  @retval Bus                     Bus value in BDF of device.
  @retval Device                  Device value in BDF of device.
  @retval Function                Function value in BDF of device.
  @retval Slot                    Slot information on which image info is to be taken

  **/
CXL_FMP_OPERATION_TYPE
ParseArguments (
  UINTN   Argc,
  CHAR16  **Argv,
  UINTN   *Bus,
  UINTN   *Device,
  UINTN   *Function,
  UINTN   *Slot,
  CHAR16  **FileName
  )
{
  CXL_FMP_OPERATION_TYPE  OpType;

  OpType = GetOperationType (Argc, Argv);
  if ((OpType == OpTypeDisplayHelp) || (OpType == OpTypeListDevice)) {
    return OpType;
  }

  if (ValidateArguments (Argc, Argv, Bus, Device, Function, Slot, FileName, OpType) == FALSE) {
    Print (L"Arguments Validation Fail\n");
    OpType = OpTypeDisplayHelp;
  }

  return OpType;
}

/**
  CXL Application main function which will call the operation

  @retval Status                      Return EFI_SUCCESS on successfully calling the requested operation
  **/
EFI_STATUS
EFIAPI
CxlFwMain (
  IN  UINTN   Argc,
  IN  CHAR16  **Argv
  )
{
  CXL_FMP_OPERATION_TYPE  OpType;
  UINTN                   Bus;
  UINTN                   Device;
  UINTN                   Function;
  UINTN                   Slot;
  CHAR16                  *FileName;
  EFI_STATUS              Status;

  FileName = NULL;
  Status   = EFI_SUCCESS;

  OpType = ParseArguments (Argc, Argv, &Bus, &Device, &Function, &Slot, &FileName);

  switch (OpType) {
    case OpTypeDisplayHelp:
      PrintHelpPage ();
      Status = EFI_SUCCESS;
      break;

    case OpTypeListDevice:
      Status = GetCxlDeviceList ();
      break;

    case OpTypeFmpGetImgInfo:
      Status = GetImageInfo (Bus, Device, Function);
      break;

    case OpTypeFmpSetImg:
      Status = SetImage (Bus, Device, Function, Slot, FileName);
      break;

    case OpTypeGetImage:
      Status = GetImage (Bus, Device, Function, Slot);
      break;

    case OpTypeFmpCheckImg:
      Status = CheckImage (Bus, Device, Function, Slot);
      break;

    case OpTypeFmpGetPkgInfo:
      Status = GetPackageInfo (Bus, Device, Function);
      break;

    case OpTypeSetPkgInfo:
      Status = SetPackageInfo (Bus, Device, Function);
      break;

    default:
      Print (L"Invalid Operation Type\n");
      break;
  }

  if (NULL != FileName) {
    FreePool (FileName);
  }

  return Status;
}

/**
  Shell Entry library of the function

  @param[in] ImageHandle              Image Handle for entry lib.

  @retval Status                      Return EFI_SUCCESS on successfully calling the requested operation

  **/
EFI_STATUS
EFIAPI
MyShellCEntryLib (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_SHELL_PARAMETERS_PROTOCOL  *EfiShellParametersProtocol;
  EFI_STATUS                     Status;

  EfiShellParametersProtocol =  NULL;

  Status = SystemTable->BootServices->OpenProtocol (
                                        ImageHandle,
                                        &gEfiShellParametersProtocolGuid,
                                        (VOID **)&EfiShellParametersProtocol,
                                        ImageHandle,
                                        NULL,
                                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                                        );

  if (!EFI_ERROR (Status)) {
    Status = CxlFwMain (EfiShellParametersProtocol->Argc, EfiShellParametersProtocol->Argv);
  } else {
    ASSERT (FALSE);
  }

  return Status;
}


