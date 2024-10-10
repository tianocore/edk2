/** @file
  CxlFirmwareMgmt Application is used to send and receive FMP commands
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CxlFirmwareMgmt.h"

void strCpyApp_c16(CHAR16  *st1, const CHAR16  *st2)
{
  int i = 0;
  for (i = 0; st2[i] != '\0'; i++) {
    st1[i] = st2[i];
  }
  st1[i] = '\0';
}

void strCpy(CHAR16  *st1, CHAR16  *st2)
{
  int i = 0;
  for (i = 0; st2[i] != '\0'; i++) {
      st1[i] = st2[i];
  }
  st1[i] = '\0';
}

EFI_STATUS
ReadFileToBuffer (
  IN  CHAR16    *FileName,
  OUT UINTN     *BufferSize,
  OUT VOID      **Buffer
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  SHELL_FILE_HANDLE     FileHandle;
  UINTN                 FileSize = 0;
  VOID                  *TempBuffer;
  UINTN                 Position = 0;
  EFI_SHELL_PROTOCOL    *ShellProtocol = NULL;
  UINTN                 TempBufferSize; 
  Status = gBS->LocateProtocol (
             &gEfiShellProtocolGuid,
             NULL,
             (VOID **)&ShellProtocol
             );

  if (EFI_ERROR (Status)) {
    Print (L"ReadFileToBuffer: Error gEfiShellProtocolGuid %r\n", Status);
    ShellProtocol = NULL;
    *BufferSize = 0; 
    *Buffer = NULL;
    return Status;
   }

  Status = ShellProtocol->OpenFileByName (
                            FileName,
                            &FileHandle,
                            EFI_FILE_MODE_READ
                            );

  if (EFI_ERROR (Status)) {
    Print(L"ReadFileToBuffer: Error open file by name %r\n", Status);
    return Status;
  }

  Status = ShellProtocol->GetFileSize (FileHandle, &FileSize);

  if (EFI_ERROR (Status)) {
    ShellProtocol->CloseFile (FileHandle);
    return Status;
  }

  if (FileSize > CXL_FW_SIZE) {
    Print(L"ReadFileToBuffer: Error FileSize = %d is greater then 32 MB\n", FileSize);
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  TempBufferSize = FileSize;
  TempBuffer = AllocateZeroPool (TempBufferSize);
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
  *Buffer = TempBuffer;
  return EFI_SUCCESS;
}

EFI_STATUS getHandleNum(UINTN  *NumOfHandles, EFI_HANDLE  **Handles) {
  EFI_STATUS    Status = EFI_SUCCESS;
  Status = gBS->LocateHandleBuffer(
                  ByProtocol,
                  &gEfiFirmwareManagementProtocolGuid,
                  NULL,
                  NumOfHandles,
                  Handles
  );

  if (EFI_ERROR(Status)) {
      Print(L"getHandleNum: LocateHandleBuffer failed status = %r, NumOfHandles = %d\n", Status, *NumOfHandles);
      return Status;
  }

  if (0 == *NumOfHandles) {
      Print(L"getHandleNum: Handle not found status = %r, NoHandles = %d\n", Status, *NumOfHandles);
      return EFI_NOT_FOUND;
  }
  return Status;
}

EFI_STATUS getPrivateStr(CXL_CONTROLLER_PRIVATE_DATA  **Private1, EFI_HANDLE  *Handles, UINTN Index) {

  EFI_STATUS                        Status = EFI_SUCCESS;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *FirmwareMgmt = NULL;

  Status = gBS->HandleProtocol(
                  Handles[Index],
                  &gEfiFirmwareManagementProtocolGuid,
                  (VOID**)&FirmwareMgmt
                  );

  if (EFI_ERROR(Status)) {
    return Status;
  }

  *Private1 = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT(FirmwareMgmt);
  if (CXL_CONTROLLER_PRIVATE_DATA_SIGNATURE != (*Private1)->Signature) {
    Print(L"\ngetPrivateStr: Error, Private Data is not for CXL device!\n");
    Status = EFI_NOT_FOUND;
  }
  return Status;
}

EFI_STATUS GetImageInfo(UINTN Bus, UINTN Device, UINTN Func)
{
  EFI_STATUS                       Status = EFI_SUCCESS;
  EFI_HANDLE                       *Handles = NULL;
  UINTN                            Index;
  UINTN                            NumOfHandles = 0;
  UINTN                            ImageInfoSize = 0;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR    *ImageInfo = NULL;
  UINT32                           DescriptorVersion = 0;
  UINT8                            DescriptorCount = 0;
  UINTN                            DescriptorSize = 0;
  UINT32                           PackageVersion = 0;
  CHAR16                           *PackageVersionName = NULL;
  CXL_CONTROLLER_PRIVATE_DATA      *Private = NULL;

  Status = getHandleNum(&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print(L"GetImageInfo: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = getPrivateStr(&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
      Print(L"GetImageInfo: Fail to locate handle buffer...\n");
      continue;
    }

    if (Bus == Private->Bus && Device == Private->Dev && Func == Private->Func) {
      Status = Private->FirmwareMgmt.GetImageInfo(
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
        ImageInfo = AllocateZeroPool(ImageInfoSize);
        if (ImageInfo == NULL) {
          DEBUG((EFI_D_ERROR, "GetImageInfo: AllocateZeroPool failed!\n"));
          Status = EFI_OUT_OF_RESOURCES;
          return Status;
        }

        Status = Private->FirmwareMgmt.GetImageInfo(
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

      if (!EFI_ERROR(Status)) {
        Print(L"===== Current Firmware Image Information =====\n");
        Print(L"Package Version         : %08X\n", PackageVersion);
        Print(L"Package Version Name    : %s\n", PackageVersionName);
        Print(L"Image Index             : %d\n", ImageInfo->ImageIndex);
        Print(L"Image Type ID           : %g\n", ImageInfo->ImageTypeId);
        Print(L"Image ID                : %016lx\n", ImageInfo->ImageId);
        Print(L"Image ID Name           : %s\n", ImageInfo->ImageIdName);
        Print(L"Version                 : %d\n", ImageInfo->Version);
        Print(L"Version Name            : %s\n", ImageInfo->VersionName);
        Print(L"Size                    : %d\n", ImageInfo->Size);
        Print(L"Attributes Supported    : %d\n", ImageInfo->AttributesSupported);
        Print(L"Attributes Setting      : %d\n", ImageInfo->AttributesSetting);
        Print(L"Compatibilities         : %d\n", ImageInfo->Compatibilities);
      } else {
          Print(L"Calling GetImageInfo Failed with status = %r\n", Status);
        }

    break;
    }
  }

  FreePool(Handles);

  if (NULL != PackageVersionName) {
    FreePool(PackageVersionName);
  }

  if (NULL != ImageInfo) {
    FreePool(ImageInfo);
  }
  return Status;
}

EFI_STATUS GetPackageInfo(UINTN Bus, UINTN Device, UINTN Func)
{
  EFI_STATUS                     Status = EFI_SUCCESS;
  EFI_HANDLE                     *Handles = NULL;
  UINTN                          Index;
  UINTN                          NumOfHandles = 0;
  UINT32                         PackageVersion = 0;
  CHAR16                         *PackageVersionName = NULL;
  UINT32                         PackageVersionNameMaxLen = 0;
  UINT64                         AttributesSupported = 0;
  UINT64                         AttributesSetting = 0;
  CXL_CONTROLLER_PRIVATE_DATA    *Private = NULL;

  Status = getHandleNum(&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print(L"GetPackageInfo: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = getPrivateStr(&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
        Print(L"GetImageInfo: Fail to locate handle buffer...\n");
        continue;
    }

    if (Bus == Private->Bus && Device == Private->Dev && Func == Private->Func) {
      PackageVersionName = AllocateZeroPool(CXL_STRING_BUFFER_WIDTH);
      if (PackageVersionName == NULL) {
        DEBUG((EFI_D_ERROR, "GetImageInfo: AllocateZeroPool failed!\n"));
        Status = EFI_OUT_OF_RESOURCES;
        return Status;
      }

      Status = Private->FirmwareMgmt.GetPackageInfo(
                                       &Private->FirmwareMgmt,
                                       &PackageVersion,
                                       &PackageVersionName,
                                       &PackageVersionNameMaxLen,
                                       &AttributesSupported,
                                       &AttributesSetting
                                       );

      if (!EFI_ERROR(Status)) {
          Print(L"Package Version Name      : %s\n", PackageVersionName);
          Print(L"Package Version           : %d\n", PackageVersion);
          Print(L"Attributes Supported      : %d\n", AttributesSupported);
          Print(L"Attributes Setting        : %d\n", AttributesSetting);
      } else {
          Print(L"Calling FMP.GetPackageInfo...%r\n", Status);
        }

      if (NULL != PackageVersionName) {
        FreePool(PackageVersionName);
      }
    break;
    }
  }

  FreePool(Handles);
  return Status;
}

EFI_STATUS GetImage(UINTN Bus, UINTN Device, UINTN Func, UINTN Slot)
{
  EFI_STATUS    Status = EFI_SUCCESS;
  UINT8         ImageIndex = Slot;
  CHAR16        *Image = NULL;
  UINTN         ImageSize;
  EFI_HANDLE    *Handles = NULL;
  UINTN         Index;
  UINTN         NumOfHandles = 0;
  CXL_CONTROLLER_PRIVATE_DATA  *Private = NULL;

  Status = getHandleNum(&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print(L"GetImage: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = getPrivateStr(&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
      Print(L"GetImage: Fail to locate handle buffer...\n");
      continue;
    }

    if (Bus == Private->Bus && Device == Private->Dev && Func == Private->Func) {
        Status = Private->FirmwareMgmt.GetImage(
                                         &Private->FirmwareMgmt,
                                         ImageIndex,
                                         Image,
                                         &ImageSize
                                         );

        if (Status == EFI_BUFFER_TOO_SMALL) {
          Print(L"\nGetImage: Image Allocated with size = %d\n", ImageSize);
          Image = AllocateZeroPool(ImageSize);
          if (Image == NULL) {
            DEBUG((EFI_D_ERROR, "GetImage: AllocateZeroPool failed!\n"));
            Status = EFI_OUT_OF_RESOURCES;
            return Status;
          }

          Status = Private->FirmwareMgmt.GetImage(
                                           &Private->FirmwareMgmt,
                                           ImageIndex,
                                           Image,
                                           &ImageSize
                                           );
        }

        if (!EFI_ERROR(Status)) {
          Print(L"GetImage, Image Size = %d\n", ImageSize);
        } else {
          Print(L"Calling FMP GetImage Failed...%r\n", Status);
          }
    break;
    }
  }

  if (NULL != Handles) {
    FreePool(Handles);
  }

  if (NULL != Image) {
    FreePool(Image);
  }
  return Status;
}

EFI_STATUS SetPackageInfo(UINTN Bus, UINTN Device, UINTN Func)
{
  EFI_STATUS    Status = EFI_SUCCESS;
  EFI_HANDLE    *Handles = NULL;
  UINTN         Index;
  UINTN         NumOfHandles = 0;

  CONST VOID    *Image = NULL;
  UINTN         ImageSize = 0;
  CONST VOID    *VendorCode = NULL;
  UINT32        PackageVersion = 1;
  CHAR16        PackageVersionName[CXL_STRING_BUFFER_WIDTH];

  CXL_CONTROLLER_PRIVATE_DATA    *Private = NULL;

  Status = getHandleNum(&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print(L"SetPackageInfo: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = getPrivateStr(&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
      Print(L"GetImageInfo: Fail to locate handle buffer...\n");
      continue;
    }

    if (Bus == Private->Bus && Device == Private->Dev && Func == Private->Func) {
      strCpyApp_c16(PackageVersionName, CXL_PACKAGE_VERSION_NAME_APP);
      Status = Private->FirmwareMgmt.SetPackageInfo(
                                       &Private->FirmwareMgmt,
                                       &Image,
                                       ImageSize,
                                       &VendorCode,
                                       PackageVersion,
                                       PackageVersionName
                                       );

      if (!EFI_ERROR(Status)) {
          Print(L"SetPackageInfo Success\n");
      } else {
          Print(L"Calling FMP SetPackageInfo Failed...%r\n", Status);
        }
      break;
    }
  }

  FreePool(Handles);
  return Status;
}

EFI_STATUS GetCXLDeviceList()
{
  EFI_HANDLE                      *Handles = NULL;
  UINTN                           Index;
  EFI_STATUS                      Status = EFI_SUCCESS;
  UINTN                           NumOfHandles = 0;
  bool                            printFlag = FALSE;
  CXL_CONTROLLER_PRIVATE_DATA     *Private = NULL;

  Status = getHandleNum(&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print(L"GetCXLDeviceList: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = getPrivateStr(&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
      Print(L"GetImageInfo: Fail to locate handle buffer...\n");
      continue;
    }
    
    if (printFlag == FALSE) {
      Print(L"Device          BUS    DEVICE    FUNCTION   \n");
      printFlag = TRUE;
    }
    
    Print(L"CXLDevice[%d]:   %d       %d          %d\n", Index, Private->Bus, Private->Dev, Private->Func);
  }

  FreePool(Handles);
  return Status;
}

EFI_STATUS
SetImage(UINTN Bus, UINTN Device, UINTN Func, UINTN Slot, CHAR16 *FileName)
{
  VOID          *Buffer;
  UINTN         BufferSize;
  EFI_STATUS    Status = EFI_SUCCESS;
  EFI_HANDLE    *Handles = NULL;
  UINTN         Index;
  UINTN         NumOfHandles = 0;
  CXL_CONTROLLER_PRIVATE_DATA    *Private = NULL;

  Status = getHandleNum(&NumOfHandles, &Handles);
  if (Status != EFI_SUCCESS) {
    Print(L"SetImage: Fail to locate handle buffer...\n");
    return Status;
  }

  for (Index = 0; Index < NumOfHandles; Index++) {
    Status = getPrivateStr(&Private, Handles, Index);
    if (Status != EFI_SUCCESS) {
      Print(L"GetImageInfo: Fail to locate handle buffer...\n");
      continue;
    }

    if (Bus == Private->Bus && Device == Private->Dev && Func == Private->Func) {
      Status = ReadFileToBuffer(FileName, &BufferSize, &Buffer);
      if (EFI_SUCCESS != Status) {
        Print(L"SetImage: ReadFileToBuffer FMP SetImage Failed...%r\n", Status);
        break;
      }

      Status = Private->FirmwareMgmt.SetImage(
                                       &Private->FirmwareMgmt,
                                       Slot,
                                       Buffer,
                                       BufferSize,
                                       NULL,
                                       NULL,
                                       NULL
                                       );

      if (!EFI_ERROR(Status)) {
          Print(L"SetImage Success\n");
      }
      else {
          Print(L"SetImage: Calling FMP SetImage Failed...%r\n", Status);
      }
    break;
    }
  }

  FreePool(Handles);
  return Status;
}

EFI_STATUS
CheckImage(UINTN Bus, UINTN Device, UINTN Func, UINTN Slot)
{
  EFI_STATUS Status = EFI_SUCCESS;
  Print(L"CheckImage Command not supported...\n");
  return Status;
}

void
PrintHelpPage ()
{
  Print (L" -fGetCXLDeviceList                                          : Get CXL Device List (GetCXLDeviceList).\n");
  Print (L" -fimginfo <b: Bus> <d: Device> <f: Function>                : Get information about current firmware image (GetImageInfo).\n");
  Print (L" -fsetimg  <b: Bus> <d: Device> <f: Function> <Slot> <file>  : Firmware download and activate (SetImage).\n");
  Print (L" -fgetimg  <b: Bus> <d: Device> <f: Function> <Slot>         : Get information about the firmware package (GetImage).\n");
  Print (L" -fchkimg  <b: Bus> <d: Device> <f: Function> <Slot> <file>  : Check the validity of a firmware image (CheckImage).\n");
  Print (L" -fsetpack <b: Bus> <d: Device> <f: Function>                : Set information about the firmware package (SetPackageInfo).\n");
  Print (L" -fgetpack <b: Bus> <d: Device> <f: Function>                : Get information about the firmware package (GetPackageInfo).\n");
}

BOOLEAN isDigit(char ch)
{
  return (ch >= '0') && (ch <= '9');
}

int strlen16(CHAR16 *str)
{
  int len = 0;
  while (*str != '\0') {
    len++;
    str++;
  }
  return len;
}

BOOLEAN isNumber(CHAR16 *str)
{
  int len = strlen16(str);
  for (int i = 0; i < len; i++) {
    if (isDigit(str[i]) == FALSE) {
      return FALSE;
    }
  }
  return TRUE;
}

BOOLEAN getBDF(UINTN Argc, CHAR16 **Argv, UINTN *Bus, UINTN *Device, UINTN *Func, UINTN *Slot, CHAR16 **FileName) {

  CHAR16    *Bus1 = NULL;
  CHAR16    *Dev1 = NULL;
  CHAR16    *Func1 = NULL;
  CHAR16    *Slot1 = NULL;
  CHAR16    *FileName1 = NULL;

  Bus1 = Argv[2];
  Dev1 = Argv[3];
  Func1 = Argv[4];

  *Bus = StrDecimalToUintn(Argv[2]);
  *Device = StrDecimalToUintn(Argv[3]);
  *Func = StrDecimalToUintn(Argv[4]);

  if (Argc >= 6) {
    Slot1 = Argv[5];
    *Slot = StrDecimalToUintn(Argv[5]);
    if (isNumber(Slot1) == FALSE) {
      return FALSE;
    }
  }

  if (Argc == 7) {
    *FileName = AllocateZeroPool(CXL_MAX_FILE_NAME_LENGTH);
    if (NULL == *FileName) {
      DEBUG((EFI_D_ERROR, "getBDF: EFI Out of resources...\n"));
      return FALSE;
    }

    FileName1 = Argv[6];
    strCpy(*FileName, FileName1);
    if (isNumber(FileName1) == TRUE) {
      return FALSE;
    }
  }

  if (isNumber(Bus1) == FALSE || isNumber(Dev1) == FALSE || isNumber(Func1) == FALSE) {
    return FALSE;
  }
  return TRUE;
}

BOOLEAN
validArguments(UINTN Argc, CHAR16 **Argv, UINTN *Bus, UINTN *Device, UINTN *Func, UINTN *Slot, CHAR16 **FileName, CXL_FMP_OPERATION_TYPE OpType)
{
  bool    isBDFRequire = TRUE;
  switch (OpType) {
    case OpTypeDisplayHelp:
      isBDFRequire = FALSE;
      break;

    case OpTypeListDevice:
      if (Argc != 2) {
        Print(L"Invalid argument...\n");
        return FALSE;
      }
      isBDFRequire = FALSE;
      break;

    case OpTypeFmpGetImgInfo:
      if (Argc != 5) {
        Print(L"Invalid argument...\n");
        return FALSE;
      }
      break;

    case OpTypeFmpSetImg:
      if (Argc != 7) {
        Print(L"Invalid argument...\n");
        return FALSE;
      }
      break;

    case OpTypeGetImage:
      if (Argc != 6) {
        Print(L"Invalid argument...\n");
        return FALSE;
      }
      break;

    case OpTypeFmpCheckImg:
      if (Argc != 7) {
        Print(L"Invalid argument...\n");
        return FALSE;
      }
      break;

    case OpTypeFmpGetPkgInfo:
      if (Argc != 5) {
        Print(L"Invalid argument...\n");
        return FALSE;
      }
      break;

    case OpTypeSetPkgInfo:
      if (Argc != 5) {
        Print(L"Invalid argument...\n");
        return FALSE;
      }
      break;

    default:
      return FALSE;
  }

  if (isBDFRequire == TRUE) {
    if (FALSE == getBDF(Argc, Argv, Bus, Device, Func, Slot, FileName)) {
      Print(L"Invalid argument...\n");
      return FALSE;
    }
  }
  return TRUE;
}

CXL_FMP_OPERATION_TYPE GetOptype(UINTN Argc, CHAR16 **Argv) {
  CXL_FMP_OPERATION_TYPE  OpType;
  CHAR16                  *str = NULL;

  if (1 == Argc) {
    OpType = OpTypeDisplayHelp;
    goto END;
  }

  str = Argv[1];

  if (!StrCmp(str, L"-fGetCXLDeviceList")) {
    OpType = OpTypeListDevice;
  } else if (!StrCmp(str, L"-fimginfo")) {
      OpType = OpTypeFmpGetImgInfo;
  } else if (!StrCmp(str, L"-fsetimg")) {
      OpType = OpTypeFmpSetImg;
  } else if (!StrCmp(str, L"-fgetimg")) {
      OpType = OpTypeGetImage;
  } else if (!StrCmp(str, L"-fchkimg")) {
      OpType = OpTypeFmpCheckImg;
  } else if (!StrCmp(str, L"-fsetpack")) {
      OpType = OpTypeSetPkgInfo;
  } else if (!StrCmp(str, L"-fgetpack")) {
      OpType = OpTypeFmpGetPkgInfo;
  } else {
      Print(L"Invalid argument...\n");
      OpType = OpTypeDisplayHelp;
    }

END:
  return OpType;
}

CXL_FMP_OPERATION_TYPE ParseArguments(UINTN Argc, CHAR16  **Argv, UINTN *Bus, UINTN *Device, UINTN *Func, UINTN *Slot, CHAR16 **FileName) {

  CXL_FMP_OPERATION_TYPE OpType;
  OpType = GetOptype(Argc, Argv);
  if (OpType == OpTypeDisplayHelp || OpType == OpTypeListDevice) {
    return OpType;
  }

  if (validArguments(Argc, Argv, Bus, Device, Func, Slot, FileName, OpType) == FALSE) {
    Print(L"Arguments Validation Fail\n");
    OpType = OpTypeDisplayHelp;
  }
  return OpType;
}

EFI_STATUS
EFIAPI
cxlFWMain(
  IN  UINTN   Argc,
  IN  CHAR16  **Argv
  )
{
  CXL_FMP_OPERATION_TYPE    OpType;
  UINTN                     Bus, Device, Func, Slot;
  CHAR16                    *FileName = NULL;
  EFI_STATUS                Status = EFI_SUCCESS;
  OpType = ParseArguments(Argc, Argv, &Bus, &Device, &Func, &Slot, &FileName);

  switch (OpType){
    case OpTypeDisplayHelp:
      PrintHelpPage();
      Status = EFI_SUCCESS;
      break;

    case OpTypeListDevice:
      Status = GetCXLDeviceList();
      break;

    case OpTypeFmpGetImgInfo:
      Status = GetImageInfo(Bus, Device, Func);
      break;

    case OpTypeFmpSetImg:
      Status = SetImage(Bus, Device, Func, Slot, FileName);
      break;

    case OpTypeGetImage:
      Status = GetImage(Bus, Device, Func, Slot);
      break;

    case OpTypeFmpCheckImg:
      Status = CheckImage(Bus, Device, Func, Slot);
      break;

    case OpTypeFmpGetPkgInfo:
      Status = GetPackageInfo(Bus, Device, Func);
      break;

    case OpTypeSetPkgInfo:
      Status = SetPackageInfo(Bus, Device, Func);
      break;

  default:
    Print(L"Invalid Operation Type\n");
    break;
  }

  if (NULL != FileName) {
    FreePool(FileName);
  }
  return Status;
}

EFI_STATUS
EFIAPI
MyShellCEntryLib (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_SHELL_PARAMETERS_PROTOCOL  *EfiShellParametersProtocol;
  EFI_STATUS                     Status;
  EfiShellParametersProtocol  =  NULL;

  Status = SystemTable->BootServices->OpenProtocol(ImageHandle,
                          &gEfiShellParametersProtocolGuid,
                          (VOID **)&EfiShellParametersProtocol,
                          ImageHandle,
                          NULL,
                          EFI_OPEN_PROTOCOL_GET_PROTOCOL
                          );

  if (!EFI_ERROR(Status)) {
    Status = cxlFWMain ( EfiShellParametersProtocol->Argc, EfiShellParametersProtocol->Argv);
    } else {
      ASSERT(FALSE);
    }
  return Status;
}
