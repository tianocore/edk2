/** @file
  Provides interface to advanced shell functionality for parsing both handle and protocol database.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiHandleParsingLib.h"
#include "IndustryStandard/Acpi10.h"

EFI_HANDLE mHandleParsingHiiHandle;
HANDLE_INDEX_LIST mHandleList = {{{NULL,NULL},0,0},0};

/**
  Constructor for the library.

  @param[in] ImageHandle    Ignored.
  @param[in] SystemTable    Ignored.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
HandleParsingLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mHandleParsingHiiHandle = HiiAddPackages (&gHandleParsingHiiGuid, gImageHandle, UefiHandleParsingLibStrings, NULL);
  if (mHandleParsingHiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }

  return (EFI_SUCCESS);
}

/**
  Destructor for the library.  free any resources.

  @param[in] ImageHandle    Ignored.
  @param[in] SystemTable    Ignored.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
HandleParsingLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (mHandleParsingHiiHandle != NULL) {
    HiiRemovePackages(mHandleParsingHiiHandle);
  }
  return (EFI_SUCCESS);
}

/*
CHAR16*
EFIAPI
LoadedImageProtocolDumpInformation(
  IN CONST EFI_HANDLE TheHandle,
  IN CONST BOOLEAN    Verbose
  )
{
  EFI_LOADED_IMAGE_PROTOCOL         *Image;
  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_PROTOCOL          *DevPath;
  EFI_DEVICE_PATH_PROTOCOL          *DevPathNode;
  VOID                              *Buffer;
  UINTN                             BufferSize;
  UINT32                            AuthenticationStatus;
  EFI_GUID                          *NameGuid;
  EFI_FIRMWARE_VOLUME_PROTOCOL      *FV;
  EFI_FIRMWARE_VOLUME2_PROTOCOL     *FV2;

  FV          = NULL;
  FV2         = NULL;
  Buffer      = NULL;
  BufferSize  = 0;

  Status      = HandleProtocol (
    TheHandle,
    &gEfiLoadedImageProtocolGuid,
    &Image);
  ASSERT_EFI_ERROR(Status);

  DevPath     = UnpackDevicePath (Image->FilePath);

  if (DevPath == NULL) {
    return NULL;
  }

  DevPathNode = DevPath;

  while (!IsDevicePathEnd (DevPathNode)) {
    //
    // Find the Fv File path
    //
    NameGuid = GetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)DevPathNode);
    if (NameGuid != NULL) {
      Status = BS->HandleProtocol (
                    Image->DeviceHandle,
                    &gEfiFirmwareVolumeProtocolGuid,
                    &FV
                   );
      if (!EFI_ERROR (Status)) {
        Status = FV->ReadSection (
                      FV,
                      NameGuid,
                      EFI_SECTION_USER_INTERFACE,
                      0,
                      &Buffer,
                      &BufferSize,
                      &AuthenticationStatus
                     );
        if (!EFI_ERROR (Status)) {
          break;
        }

        Buffer = NULL;
      } else {
        Status = BS->HandleProtocol (
                      Image->DeviceHandle,
                      &gEfiFirmwareVolume2ProtocolGuid,
                      &FV2
                     );
        if (!EFI_ERROR (Status)) {
          Status = FV2->ReadSection (
                          FV2,
                          NameGuid,
                          EFI_SECTION_USER_INTERFACE,
                          0,
                          &Buffer,
                          &BufferSize,
                          &AuthenticationStatus
                         );
          if (!EFI_ERROR (Status)) {
            break;
          }

          Buffer = NULL;
        }
      }
    }
    //
    // Next device path node
    //
    DevPathNode = NextDevicePathNode (DevPathNode);
  }

  FreePool (DevPath);
  return Buffer;
}
*/

/**
  Function to dump information about PciRootBridgeIo.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has PciRootBridgeIo installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A poitner to a string containing the information.
**/
CHAR16*
EFIAPI
PciRootBridgeIoDumpInformation(
  IN CONST EFI_HANDLE TheHandle,
  IN CONST BOOLEAN    Verbose
  )
{
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL   *PciRootBridgeIo;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Configuration;
  UINT64                            Supports;
  UINT64                            Attributes;
  CHAR16                            *Temp;
  CHAR16                            *Temp2;
  CHAR16                            *RetVal;
  EFI_STATUS                        Status;

  RetVal  = NULL;

  if (!Verbose) {
    return (CatSPrint(NULL, L"PciRootBridgeIo"));
  }

  Status = gBS->HandleProtocol(
    TheHandle,
    &gEfiPciRootBridgeIoProtocolGuid,
    (VOID**)&PciRootBridgeIo);

  if (EFI_ERROR(Status)) {
    return NULL;
  }

  Temp = HiiGetString(mHandleParsingHiiHandle, STRING_TOKEN(STR_PCIRB_DUMP_PH), NULL);
  ASSERT (Temp != NULL);
  Temp2 = CatSPrint(L"\r\n", Temp, PciRootBridgeIo->ParentHandle);
  FreePool(Temp);
  RetVal = Temp2;
  Temp2 = NULL;
 
  Temp = HiiGetString(mHandleParsingHiiHandle, STRING_TOKEN(STR_PCIRB_DUMP_SEG), NULL);
  ASSERT (Temp != NULL);
  Temp2 = CatSPrint(RetVal, Temp, PciRootBridgeIo->SegmentNumber);
  FreePool(Temp);
  FreePool(RetVal);
  RetVal = Temp2;
  Temp2 = NULL;

  Supports   = 0;
  Attributes = 0;
  Status = PciRootBridgeIo->GetAttributes (PciRootBridgeIo, &Supports, &Attributes);
  if (!EFI_ERROR(Status)) {
    Temp = HiiGetString(mHandleParsingHiiHandle, STRING_TOKEN(STR_PCIRB_DUMP_ATT), NULL);
    ASSERT (Temp != NULL);    
    Temp2 = CatSPrint(RetVal, Temp, Attributes);
    FreePool(Temp);
    FreePool(RetVal);
    RetVal = Temp2;
    Temp2 = NULL;
    
    Temp = HiiGetString(mHandleParsingHiiHandle, STRING_TOKEN(STR_PCIRB_DUMP_SUPPORTS), NULL);
    ASSERT (Temp != NULL);
    Temp2 = CatSPrint(RetVal, Temp, Supports);
    FreePool(Temp);
    FreePool(RetVal);
    RetVal = Temp2;
    Temp2 = NULL;
  }

  Configuration   = NULL;
  Status = PciRootBridgeIo->Configuration (PciRootBridgeIo, (VOID **) &Configuration);
  if (!EFI_ERROR(Status) && Configuration != NULL) {
    Temp = HiiGetString(mHandleParsingHiiHandle, STRING_TOKEN(STR_PCIRB_DUMP_TITLE), NULL);
    ASSERT (Temp != NULL);
    Temp2 = CatSPrint(RetVal, Temp, Supports);
    FreePool(Temp);
    FreePool(RetVal);
    RetVal = Temp2;
    Temp2 = NULL;
    while (Configuration->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
      Temp = NULL;
      switch (Configuration->ResType) {
      case ACPI_ADDRESS_SPACE_TYPE_MEM:
        Temp = HiiGetString(mHandleParsingHiiHandle, STRING_TOKEN(STR_PCIRB_DUMP_MEM), NULL);
        break;
      case ACPI_ADDRESS_SPACE_TYPE_IO:
        Temp = HiiGetString(mHandleParsingHiiHandle, STRING_TOKEN(STR_PCIRB_DUMP_IO), NULL);
        break;
      case ACPI_ADDRESS_SPACE_TYPE_BUS:
        Temp = HiiGetString(mHandleParsingHiiHandle, STRING_TOKEN(STR_PCIRB_DUMP_BUS), NULL);
        break;
      }
      if (Temp != NULL) {
        Temp2 = CatSPrint(RetVal, L"%s", Temp);
        FreePool(Temp);
        FreePool(RetVal);
        RetVal = Temp2;
        Temp2 = NULL;
      }

      Temp2 = CatSPrint(RetVal, 
        L"%H%02x    %016lx  %016lx  %02x%N\r\n",
        Configuration->SpecificFlag,
        Configuration->AddrRangeMin,
        Configuration->AddrRangeMax,
        Configuration->AddrSpaceGranularity
        );
      FreePool(RetVal);
      RetVal = Temp2;
      Temp2 = NULL;
      Configuration++;
    }
  }
  return (RetVal);
}

/**
  Function to dump information about SimpleTextOut.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has SimpleTextOut installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A poitner to a string containing the information.
**/
CHAR16*
EFIAPI
TxtOutProtocolDumpInformation(
  IN CONST EFI_HANDLE TheHandle,
  IN CONST BOOLEAN    Verbose
  )
{
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *Dev;
  INTN                            Index;
  UINTN                           Col;
  UINTN                           Row;
  EFI_STATUS                      Status;
  CHAR16                          *RetVal;
  UINTN                           Size;
  CHAR16                          *Temp;
  UINTN                           NewSize;

  if (!Verbose) {
    return (NULL);
  }

  RetVal  = NULL;
  Size    = 0;

  Status = gBS->HandleProtocol(
    TheHandle,
    &gEfiSimpleTextOutProtocolGuid,
    (VOID**)&Dev);

  ASSERT_EFI_ERROR(Status);
  ASSERT (Dev != NULL && Dev->Mode != NULL);

  Size = (Dev->Mode->MaxMode + 1) * 80;
  RetVal = AllocateZeroPool(Size);

  Temp = HiiGetString(mHandleParsingHiiHandle, STRING_TOKEN(STR_TXT_OUT_DUMP_HEADER), NULL);
  if (Temp != NULL) {
    UnicodeSPrint(RetVal, Size, Temp, Dev, Dev->Mode->Attribute);
    FreePool(Temp);
  }

  //
  // Dump TextOut Info
  //
  Temp = HiiGetString(mHandleParsingHiiHandle, STRING_TOKEN(STR_TXT_OUT_DUMP_LINE), NULL);
  for (Index = 0; Index < Dev->Mode->MaxMode; Index++) {
    Status = Dev->QueryMode (Dev, Index, &Col, &Row);
    NewSize = Size - StrSize(RetVal);
    UnicodeSPrint(
      RetVal + StrLen(RetVal),
      NewSize,
      Temp == NULL?L"":Temp,
      Index == Dev->Mode->Mode ? L'*' : L' ',
      Index,
      !EFI_ERROR(Status)?Col:-1,
      !EFI_ERROR(Status)?Row:-1
     );
  }
  FreePool(Temp);
  return (RetVal);
}

STATIC CONST UINTN VersionStringSize = 60;

/**
  Function to dump information about EfiDriverSupportedEfiVersion protocol.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has the protocol installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A poitner to a string containing the information.
**/
CHAR16*
EFIAPI
DriverEfiVersionProtocolDumpInformation(
  IN CONST EFI_HANDLE TheHandle,
  IN CONST BOOLEAN    Verbose
  )
{
  EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL *DriverEfiVersion;
  EFI_STATUS                                Status;
  CHAR16                                    *RetVal;

  Status = gBS->HandleProtocol(
    TheHandle,
    &gEfiDriverSupportedEfiVersionProtocolGuid,
    (VOID**)&DriverEfiVersion);

  ASSERT_EFI_ERROR(Status);

  RetVal = AllocateZeroPool(VersionStringSize);
  ASSERT(RetVal != NULL);
  UnicodeSPrint(RetVal, VersionStringSize, L"0x%08x", DriverEfiVersion->FirmwareVersion);
  return (RetVal);
}

/**
  Function to dump information about DevicePath protocol.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has the protocol installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A poitner to a string containing the information.
**/
CHAR16*
EFIAPI
DevicePathProtocolDumpInformation(
  IN CONST EFI_HANDLE TheHandle,
  IN CONST BOOLEAN    Verbose
  )
{
  EFI_DEVICE_PATH_PROTOCOL          *DevPath;
  CHAR16                            *Temp;
  CHAR16                            *Temp2;
  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL  *DevPathToText;
  Temp = NULL;

  Status = gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid, NULL, (VOID**)&DevPathToText);
  if (!EFI_ERROR(Status)) {
    Status = gBS->OpenProtocol(TheHandle, &gEfiDevicePathProtocolGuid, (VOID**)&DevPath, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (!EFI_ERROR(Status)) {
      //
      // I cannot decide whether to allow shortcuts here (the second BOOLEAN on the next line)
      //
      Temp = DevPathToText->ConvertDevicePathToText(DevPath, TRUE, TRUE);
      gBS->CloseProtocol(TheHandle, &gEfiDevicePathProtocolGuid, gImageHandle, NULL);
    }
  }
  if (!Verbose && Temp != NULL && StrLen(Temp) > 30) {
    Temp2 = NULL;
    Temp2 = StrnCatGrow(&Temp2, NULL, Temp+(StrLen(Temp) - 30), 30);
    FreePool(Temp);
    Temp = Temp2;
  }
  return (Temp);
}

//
// Put the information on the NT32 protocol GUIDs here so we are not dependant on the Nt32Pkg
//
#define LOCAL_EFI_WIN_NT_THUNK_PROTOCOL_GUID \
  { \
    0x58c518b1, 0x76f3, 0x11d4, 0xbc, 0xea, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 \
  }

#define LOCAL_EFI_WIN_NT_BUS_DRIVER_IO_PROTOCOL_GUID \
  { \
    0x96eb4ad6, 0xa32a, 0x11d4, 0xbc, 0xfd, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 \
  }

#define LOCAL_EFI_WIN_NT_SERIAL_PORT_GUID \
  { \
    0xc95a93d, 0xa006, 0x11d4, 0xbc, 0xfa, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 \
  }
STATIC CONST EFI_GUID WinNtThunkProtocolGuid = LOCAL_EFI_WIN_NT_THUNK_PROTOCOL_GUID;
STATIC CONST EFI_GUID WinNtIoProtocolGuid    = LOCAL_EFI_WIN_NT_BUS_DRIVER_IO_PROTOCOL_GUID;
STATIC CONST EFI_GUID WinNtSerialPortGuid    = LOCAL_EFI_WIN_NT_SERIAL_PORT_GUID;

STATIC CONST GUID_INFO_BLOCK mGuidStringListNT[] = {
  {STRING_TOKEN(STR_WINNT_THUNK),           (EFI_GUID*)&WinNtThunkProtocolGuid,               NULL},
  {STRING_TOKEN(STR_WINNT_DRIVER_IO),       (EFI_GUID*)&WinNtIoProtocolGuid,                  NULL},
  {STRING_TOKEN(STR_WINNT_SERIAL_PORT),     (EFI_GUID*)&WinNtSerialPortGuid,                  NULL},
  {STRING_TOKEN(STR_UNKNOWN_DEVICE),        NULL,                                             NULL},
};

STATIC CONST GUID_INFO_BLOCK mGuidStringList[] = {
  {STRING_TOKEN(STR_LOADED_IMAGE),          &gEfiLoadedImageProtocolGuid,                     NULL},
  {STRING_TOKEN(STR_DEVICE_PATH),           &gEfiDevicePathProtocolGuid,                      DevicePathProtocolDumpInformation},
  {STRING_TOKEN(STR_IMAGE_PATH),            &gEfiLoadedImageDevicePathProtocolGuid,           DevicePathProtocolDumpInformation},
  {STRING_TOKEN(STR_DEVICE_PATH_UTIL),      &gEfiDevicePathUtilitiesProtocolGuid,             NULL},
  {STRING_TOKEN(STR_DEVICE_PATH_TXT),       &gEfiDevicePathToTextProtocolGuid,                NULL},
  {STRING_TOKEN(STR_DEVICE_PATH_FTXT),      &gEfiDevicePathFromTextProtocolGuid,              NULL},
  {STRING_TOKEN(STR_DEVICE_PATH_PC),        &gEfiPcAnsiGuid,                                  NULL},
  {STRING_TOKEN(STR_DEVICE_PATH_VT100),     &gEfiVT100Guid,                                   NULL},
  {STRING_TOKEN(STR_DEVICE_PATH_VT100P),    &gEfiVT100PlusGuid,                               NULL},
  {STRING_TOKEN(STR_DEVICE_PATH_VTUTF8),    &gEfiVTUTF8Guid,                                  NULL},
  {STRING_TOKEN(STR_DRIVER_BINDING),        &gEfiDriverBindingProtocolGuid,                   NULL},
  {STRING_TOKEN(STR_PLATFORM_OVERRIDE),     &gEfiPlatformDriverOverrideProtocolGuid,          NULL},
  {STRING_TOKEN(STR_BUS_OVERRIDE),          &gEfiBusSpecificDriverOverrideProtocolGuid,       NULL},
  {STRING_TOKEN(STR_DRIVER_DIAG),           &gEfiDriverDiagnosticsProtocolGuid,               NULL},
  {STRING_TOKEN(STR_DRIVER_DIAG2),          &gEfiDriverDiagnostics2ProtocolGuid,              NULL},
  {STRING_TOKEN(STR_DRIVER_CN),             &gEfiComponentNameProtocolGuid,                   NULL},
  {STRING_TOKEN(STR_DRIVER_CN2),            &gEfiComponentName2ProtocolGuid,                  NULL},
  {STRING_TOKEN(STR_PLAT_DRV_CFG),          &gEfiPlatformToDriverConfigurationProtocolGuid,   NULL},
  {STRING_TOKEN(STR_DRIVER_VERSION),        &gEfiDriverSupportedEfiVersionProtocolGuid,       DriverEfiVersionProtocolDumpInformation},
  {STRING_TOKEN(STR_TXT_IN),                &gEfiSimpleTextInProtocolGuid,                    NULL},
  {STRING_TOKEN(STR_TXT_IN_EX),             &gEfiSimpleTextInputExProtocolGuid,               NULL},
  {STRING_TOKEN(STR_TXT_OUT),               &gEfiSimpleTextOutProtocolGuid,                   TxtOutProtocolDumpInformation},
  {STRING_TOKEN(STR_SIM_POINTER),           &gEfiSimplePointerProtocolGuid,                   NULL},
  {STRING_TOKEN(STR_ABS_POINTER),           &gEfiAbsolutePointerProtocolGuid,                 NULL},
  {STRING_TOKEN(STR_SERIAL_IO),             &gEfiSerialIoProtocolGuid,                        NULL},
  {STRING_TOKEN(STR_GRAPHICS_OUTPUT),       &gEfiGraphicsOutputProtocolGuid,                  NULL},
  {STRING_TOKEN(STR_EDID_DISCOVERED),       &gEfiEdidDiscoveredProtocolGuid,                  NULL},
  {STRING_TOKEN(STR_EDID_ACTIVE),           &gEfiEdidActiveProtocolGuid,                      NULL},
  {STRING_TOKEN(STR_EDID_OVERRIDE),         &gEfiEdidOverrideProtocolGuid,                    NULL},
  {STRING_TOKEN(STR_CON_IN),                &gEfiConsoleInDeviceGuid,                         NULL},
  {STRING_TOKEN(STR_CON_OUT),               &gEfiConsoleOutDeviceGuid,                        NULL},
  {STRING_TOKEN(STR_STD_ERR),               &gEfiStandardErrorDeviceGuid,                     NULL},
  {STRING_TOKEN(STR_LOAD_FILE),             &gEfiLoadFileProtocolGuid,                        NULL},
  {STRING_TOKEN(STR_LOAD_FILE2),            &gEfiLoadFile2ProtocolGuid,                       NULL},
  {STRING_TOKEN(STR_SIMPLE_FILE_SYS),       &gEfiSimpleFileSystemProtocolGuid,                NULL},
  {STRING_TOKEN(STR_TAPE_IO),               &gEfiTapeIoProtocolGuid,                          NULL},
  {STRING_TOKEN(STR_DISK_IO),               &gEfiDiskIoProtocolGuid,                          NULL},
  {STRING_TOKEN(STR_BLK_IO),                &gEfiBlockIoProtocolGuid,                         NULL},
  {STRING_TOKEN(STR_UC),                    &gEfiUnicodeCollationProtocolGuid,                NULL},
  {STRING_TOKEN(STR_UC2),                   &gEfiUnicodeCollation2ProtocolGuid,               NULL},
  {STRING_TOKEN(STR_PCIRB_IO),              &gEfiPciRootBridgeIoProtocolGuid,                 PciRootBridgeIoDumpInformation},
  {STRING_TOKEN(STR_PCI_IO),                &gEfiPciIoProtocolGuid,                           NULL},
  {STRING_TOKEN(STR_SCSI_PT),               &gEfiScsiPassThruProtocolGuid,                    NULL},
  {STRING_TOKEN(STR_SCSI_IO),               &gEfiScsiIoProtocolGuid,                          NULL},
  {STRING_TOKEN(STR_SCSI_PT_EXT),           &gEfiExtScsiPassThruProtocolGuid,                 NULL},
  {STRING_TOKEN(STR_ISCSI),                 &gEfiIScsiInitiatorNameProtocolGuid,              NULL},
  {STRING_TOKEN(STR_USB_IO),                &gEfiUsbIoProtocolGuid,                           NULL},
  {STRING_TOKEN(STR_USB_HC),                &gEfiUsbHcProtocolGuid,                           NULL},
  {STRING_TOKEN(STR_USB_HC2),               &gEfiUsb2HcProtocolGuid,                          NULL},
  {STRING_TOKEN(STR_DEBUG_SUPPORT),         &gEfiDebugSupportProtocolGuid,                    NULL},
  {STRING_TOKEN(STR_DEBUG_PORT),            &gEfiDebugPortProtocolGuid,                       NULL},
  {STRING_TOKEN(STR_DECOMPRESS),            &gEfiDecompressProtocolGuid,                      NULL},
  {STRING_TOKEN(STR_ACPI_TABLE),            &gEfiAcpiTableProtocolGuid,                       NULL},
  {STRING_TOKEN(STR_EBC_INTERPRETER),       &gEfiEbcProtocolGuid,                             NULL},
  {STRING_TOKEN(STR_SNP),                   &gEfiSimpleNetworkProtocolGuid,                   NULL},
  {STRING_TOKEN(STR_NII),                   &gEfiNetworkInterfaceIdentifierProtocolGuid,      NULL},
  {STRING_TOKEN(STR_NII_31),                &gEfiNetworkInterfaceIdentifierProtocolGuid_31,   NULL},
  {STRING_TOKEN(STR_PXE_BC),                &gEfiPxeBaseCodeProtocolGuid,                     NULL},
  {STRING_TOKEN(STR_PXE_CB),                &gEfiPxeBaseCodeCallbackProtocolGuid,             NULL},
  {STRING_TOKEN(STR_BIS),                   &gEfiBisProtocolGuid,                             NULL},
  {STRING_TOKEN(STR_MNP_SB),                &gEfiManagedNetworkServiceBindingProtocolGuid,    NULL},
  {STRING_TOKEN(STR_MNP),                   &gEfiManagedNetworkProtocolGuid,                  NULL},
  {STRING_TOKEN(STR_ARP_SB),                &gEfiArpServiceBindingProtocolGuid,               NULL},
  {STRING_TOKEN(STR_ARP),                   &gEfiArpProtocolGuid,                             NULL},
  {STRING_TOKEN(STR_DHCPV4_SB),             &gEfiDhcp4ServiceBindingProtocolGuid,             NULL},
  {STRING_TOKEN(STR_DHCPV4),                &gEfiDhcp4ProtocolGuid,                           NULL},
  {STRING_TOKEN(STR_TCPV4_SB),              &gEfiTcp4ServiceBindingProtocolGuid,              NULL},
  {STRING_TOKEN(STR_TCPV4),                 &gEfiTcp4ProtocolGuid,                            NULL},
  {STRING_TOKEN(STR_IPV4_SB),               &gEfiIp4ServiceBindingProtocolGuid,               NULL},
  {STRING_TOKEN(STR_IPV4),                  &gEfiIp4ProtocolGuid,                             NULL},
  {STRING_TOKEN(STR_IPV4_CFG),              &gEfiIp4ConfigProtocolGuid,                       NULL},
  {STRING_TOKEN(STR_SHELL_PARAMETERS),      &gEfiShellParametersProtocolGuid,                 NULL},
  {STRING_TOKEN(STR_SHELL),                 &gEfiShellProtocolGuid,                           NULL},
  {STRING_TOKEN(STR_UDPV4_SB),              &gEfiUdp4ServiceBindingProtocolGuid,              NULL},
  {STRING_TOKEN(STR_UDPV4),                 &gEfiUdp4ProtocolGuid,                            NULL},
  {STRING_TOKEN(STR_MTFTPV4_SB),            &gEfiMtftp4ServiceBindingProtocolGuid,            NULL},
  {STRING_TOKEN(STR_MTFTPV4),               &gEfiMtftp4ProtocolGuid,                          NULL},
  {STRING_TOKEN(STR_AUTH_INFO),             &gEfiAuthenticationInfoProtocolGuid,              NULL},
  {STRING_TOKEN(STR_HASH_SB),               &gEfiHashServiceBindingProtocolGuid,              NULL},
  {STRING_TOKEN(STR_HASH),                  &gEfiHashProtocolGuid,                            NULL},
  {STRING_TOKEN(STR_HII_FONT),              &gEfiHiiFontProtocolGuid,                         NULL},
  {STRING_TOKEN(STR_HII_STRING),            &gEfiHiiStringProtocolGuid,                       NULL},
  {STRING_TOKEN(STR_HII_IMAGE),             &gEfiHiiImageProtocolGuid,                        NULL},
  {STRING_TOKEN(STR_HII_DATABASE),          &gEfiHiiDatabaseProtocolGuid,                     NULL},
  {STRING_TOKEN(STR_HII_CONFIG_ROUT),       &gEfiHiiConfigRoutingProtocolGuid,                NULL},
  {STRING_TOKEN(STR_HII_CONFIG_ACC),        &gEfiHiiConfigAccessProtocolGuid,                 NULL},
  {STRING_TOKEN(STR_HII_FORM_BROWSER2),     &gEfiFormBrowser2ProtocolGuid,                    NULL},
  {STRING_TOKEN(STR_DRIVER_FAM_OVERRIDE),   &gEfiDriverFamilyOverrideProtocolGuid,            NULL},
  {STRING_TOKEN(STR_PCD),                   &gPcdProtocolGuid,                                NULL},
  {STRING_TOKEN(STR_TCG),                   &gEfiTcgProtocolGuid,                             NULL},
  {STRING_TOKEN(STR_HII_PACKAGE_LIST),      &gEfiHiiPackageListProtocolGuid,                  NULL},

//
// the ones under this are deprecated by the current UEFI Spec, but may be found anyways...
//
  {STRING_TOKEN(STR_SHELL_INTERFACE),       &gEfiShellInterfaceGuid,                          NULL},
  {STRING_TOKEN(STR_SHELL_ENV2),            &gEfiShellEnvironment2Guid,                       NULL},
  {STRING_TOKEN(STR_SHELL_ENV),             &gEfiShellEnvironment2Guid,                       NULL},
  {STRING_TOKEN(STR_DEVICE_IO),             &gEfiDeviceIoProtocolGuid,                        NULL},
  {STRING_TOKEN(STR_UGA_DRAW),              &gEfiUgaDrawProtocolGuid,                         NULL},
  {STRING_TOKEN(STR_UGA_IO),                &gEfiUgaIoProtocolGuid,                           NULL},
  {STRING_TOKEN(STR_ESP),                   &gEfiPartTypeSystemPartGuid,                      NULL},
  {STRING_TOKEN(STR_GPT_NBR),               &gEfiPartTypeLegacyMbrGuid,                       NULL},
  {STRING_TOKEN(STR_DRIVER_CONFIG),         &gEfiDriverConfigurationProtocolGuid,             NULL},
  {STRING_TOKEN(STR_DRIVER_CONFIG2),        &gEfiDriverConfiguration2ProtocolGuid,            NULL},

//
// the ones under this are GUID identified structs, not protocols
//
  {STRING_TOKEN(STR_FILE_INFO),             &gEfiFileInfoGuid,                                NULL},
  {STRING_TOKEN(STR_FILE_SYS_INFO),         &gEfiFileSystemInfoGuid,                          NULL},

//
// the ones under this are misc GUIDS.
//
  {STRING_TOKEN(STR_EFI_GLOBAL_VARIABLE),   &gEfiGlobalVariableGuid,                          NULL},

//
// UEFI 2.2
//
  {STRING_TOKEN(STR_IP6_SB),                &gEfiIp6ServiceBindingProtocolGuid,               NULL},
  {STRING_TOKEN(STR_IP6),                   &gEfiIp6ProtocolGuid,                             NULL},
  {STRING_TOKEN(STR_IP6_CONFIG),            &gEfiIp6ConfigProtocolGuid,                       NULL},
  {STRING_TOKEN(STR_MTFTP6_SB),             &gEfiMtftp6ServiceBindingProtocolGuid,            NULL},
  {STRING_TOKEN(STR_MTFTP6),                &gEfiMtftp6ProtocolGuid,                          NULL},
  {STRING_TOKEN(STR_DHCP6_SB),              &gEfiDhcp6ServiceBindingProtocolGuid,             NULL},
  {STRING_TOKEN(STR_DHCP6),                 &gEfiDhcp6ProtocolGuid,                           NULL},
  {STRING_TOKEN(STR_UDP6_SB),               &gEfiUdp6ServiceBindingProtocolGuid,              NULL},
  {STRING_TOKEN(STR_UDP6),                  &gEfiUdp6ProtocolGuid,                            NULL},
  {STRING_TOKEN(STR_TCP6_SB),               &gEfiTcp6ServiceBindingProtocolGuid,              NULL},
  {STRING_TOKEN(STR_TCP6),                  &gEfiTcp6ProtocolGuid,                            NULL},
  {STRING_TOKEN(STR_VLAN_CONFIG),           &gEfiVlanConfigProtocolGuid,                      NULL},
  {STRING_TOKEN(STR_EAP),                   &gEfiEapProtocolGuid,                             NULL},
  {STRING_TOKEN(STR_EAP_MGMT),              &gEfiEapManagementProtocolGuid,                   NULL},
  {STRING_TOKEN(STR_FTP4_SB),               &gEfiFtp4ServiceBindingProtocolGuid,              NULL},
  {STRING_TOKEN(STR_FTP4),                  &gEfiFtp4ProtocolGuid,                            NULL},
  {STRING_TOKEN(STR_IP_SEC_CONFIG),         &gEfiIpSecConfigProtocolGuid,                     NULL},
  {STRING_TOKEN(STR_DH),                    &gEfiDriverHealthProtocolGuid,                    NULL},
  {STRING_TOKEN(STR_DEF_IMG_LOAD),          &gEfiDeferredImageLoadProtocolGuid,               NULL},
  {STRING_TOKEN(STR_USER_CRED),             &gEfiUserCredentialProtocolGuid,                  NULL},
  {STRING_TOKEN(STR_USER_MNGR),             &gEfiUserManagerProtocolGuid,                     NULL},
  {STRING_TOKEN(STR_ATA_PASS_THRU),         &gEfiAtaPassThruProtocolGuid,                     NULL},

//
// UEFI 2.3
//
  {STRING_TOKEN(STR_FW_MGMT),               &gEfiFirmwareManagementProtocolGuid,              NULL},
  {STRING_TOKEN(STR_IP_SEC),                &gEfiIpSecProtocolGuid,                           NULL},
  {STRING_TOKEN(STR_IP_SEC2),               &gEfiIpSec2ProtocolGuid,                          NULL},

//
// UEFI 2.3.1
//
  {STRING_TOKEN(STR_KMS),                   &gEfiKmsProtocolGuid,                             NULL},
  {STRING_TOKEN(STR_BLK_IO2),               &gEfiBlockIo2ProtocolGuid,                        NULL},
  {STRING_TOKEN(STR_SSC),                   &gEfiStorageSecurityCommandProtocolGuid,          NULL},
  {STRING_TOKEN(STR_UC2),                   &gEfiUserCredential2ProtocolGuid,                 NULL},

//
// terminator
//
  {STRING_TOKEN(STR_UNKNOWN_DEVICE),        NULL,                                             NULL},
};

/**
  Function to get the node for a protocol or struct from it's GUID.

  if Guid is NULL, then ASSERT.

  @param[in] Guid               The GUID to look for the name of.

  @return                       The node.
**/
CONST GUID_INFO_BLOCK *
EFIAPI
InternalShellGetNodeFromGuid(
  IN CONST EFI_GUID* Guid
  )
{
  CONST GUID_INFO_BLOCK *ListWalker;

  ASSERT(Guid != NULL);

  if (PcdGetBool(PcdShellIncludeNtGuids)) {
    for (ListWalker = mGuidStringListNT ; ListWalker != NULL && ListWalker->GuidId != NULL ; ListWalker++) {
      if (CompareGuid(ListWalker->GuidId, Guid)) {
        return (ListWalker);
      }
    }
  }
  for (ListWalker = mGuidStringList ; ListWalker != NULL && ListWalker->GuidId != NULL ; ListWalker++) {
    if (CompareGuid(ListWalker->GuidId, Guid)) {
      return (ListWalker);
    }
  }
  return (ListWalker);
}

/**
  Function to get the name of a protocol or struct from it's GUID.

  if Guid is NULL, then ASSERT.

  @param[in] Guid               The GUID to look for the name of.
  @param[in] Lang               The language to use.

  @return                       pointer to string of the name.  The caller
                                is responsible to free this memory.
**/
CHAR16*
EFIAPI
GetStringNameFromGuid(
  IN CONST EFI_GUID *Guid,
  IN CONST CHAR8    *Lang OPTIONAL
  )
{
  CONST GUID_INFO_BLOCK *Id;

  Id = InternalShellGetNodeFromGuid(Guid);
  return (HiiGetString(mHandleParsingHiiHandle, Id->StringId, Lang));
}

/**
  Function to dump protocol information from a handle.

  This function will return a allocated string buffer containing the
  information.  The caller is responsible for freeing the memory.

  If Guid is NULL, ASSERT().
  If TheHandle is NULL, ASSERT().

  @param[in] TheHandle      The handle to dump information from.
  @param[in] Guid           The GUID of the protocol to dump.
  @param[in] Verbose        TRUE for extra info.  FALSE otherwise.

  @return                   The pointer to string.
  @retval NULL              An error was encountered.
**/
CHAR16*
EFIAPI
GetProtocolInformationDump(
  IN CONST EFI_HANDLE TheHandle,
  IN CONST EFI_GUID   *Guid,
  IN CONST BOOLEAN    Verbose
  )
{
  CONST GUID_INFO_BLOCK *Id;

  ASSERT(TheHandle  != NULL);
  ASSERT(Guid       != NULL);

  if (TheHandle == NULL || Guid == NULL) {
    return (NULL);
  }

  Id = InternalShellGetNodeFromGuid(Guid);
  if (Id != NULL && Id->DumpInfo != NULL) {
    return (Id->DumpInfo(TheHandle, Verbose));
  }
  return (NULL);
}

/**
  Function to get the Guid for a protocol or struct based on it's string name.

  @param[in] Name           The pointer to the string name.
  @param[in] Lang           The pointer to the language code.
  @param[in] Guid           The pointer to the Guid.

  @retval EFI_SUCCESS       The operation was sucessful.
**/
EFI_STATUS
EFIAPI
GetGuidFromStringName(
  IN CONST CHAR16 *Name,
  IN CONST CHAR8  *Lang OPTIONAL,
  IN EFI_GUID     **Guid
  )
{
  CONST GUID_INFO_BLOCK  *ListWalker;
  CHAR16                     *String;

  ASSERT(Guid != NULL);
  if (Guid == NULL) {
    return (EFI_INVALID_PARAMETER);
  }
  *Guid = NULL;

  if (PcdGetBool(PcdShellIncludeNtGuids)) {
    for (ListWalker = mGuidStringListNT ; ListWalker != NULL && ListWalker->GuidId != NULL ; ListWalker++) {
      String = HiiGetString(mHandleParsingHiiHandle, ListWalker->StringId, Lang);
      if (Name != NULL && String != NULL && StrCmp(Name, String)==0) {
        *Guid = ListWalker->GuidId;
      }
      SHELL_FREE_NON_NULL(String);
      if (*Guid != NULL) {
        return (EFI_SUCCESS);
      }
    }
  }
  for (ListWalker = mGuidStringList ; ListWalker != NULL && ListWalker->GuidId != NULL ; ListWalker++) {
    String = HiiGetString(mHandleParsingHiiHandle, ListWalker->StringId, Lang);
    if (Name != NULL && String != NULL && StrCmp(Name, String)==0) {
      *Guid = ListWalker->GuidId;
    }
    SHELL_FREE_NON_NULL(String);
    if (*Guid != NULL) {
      return (EFI_SUCCESS);
    }
  }
  return (EFI_NOT_FOUND);
}

/**
  Function to retrieve the driver name (if possible) from the ComponentName or
  ComponentName2 protocol

  @param[in] TheHandle      The driver handle to get the name of.
  @param[in] Language       The language to use.

  @retval NULL              The name could not be found.
  @return                   A pointer to the string name.  Do not de-allocate the memory.
**/
CONST CHAR16*
EFIAPI
GetStringNameFromHandle(
  IN CONST EFI_HANDLE TheHandle,
  IN CONST CHAR8      *Language
  )
{
  EFI_COMPONENT_NAME2_PROTOCOL  *CompNameStruct;
  EFI_STATUS                    Status;
  CHAR16                        *RetVal;

  Status = gBS->OpenProtocol(
    TheHandle,
    &gEfiComponentName2ProtocolGuid,
    (VOID**)&CompNameStruct,
    gImageHandle,
    NULL,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (!EFI_ERROR(Status)) {
    Status = CompNameStruct->GetDriverName(CompNameStruct, (CHAR8*)Language, &RetVal);
    if (!EFI_ERROR(Status)) {
      return (RetVal);
    }
  }
  Status = gBS->OpenProtocol(
    TheHandle,
    &gEfiComponentNameProtocolGuid,
    (VOID**)&CompNameStruct,
    gImageHandle,
    NULL,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (!EFI_ERROR(Status)) {
    Status = CompNameStruct->GetDriverName(CompNameStruct, (CHAR8*)Language, &RetVal);
    if (!EFI_ERROR(Status)) {
      return (RetVal);
    }
  }
  return (NULL);
}

/**
  Function to initialize the file global mHandleList object for use in
  vonverting handles to index and index to handle.

  @retval EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
EFIAPI
InternalShellInitHandleList(
  VOID
  )
{
  EFI_STATUS   Status;
  EFI_HANDLE   *HandleBuffer;
  UINTN        HandleCount;
  HANDLE_LIST  *ListWalker;

  if (mHandleList.NextIndex != 0) {
    return EFI_SUCCESS;
  }
  InitializeListHead(&mHandleList.List.Link);
  mHandleList.NextIndex = 1;
  Status = gBS->LocateHandleBuffer (
                AllHandles,
                NULL,
                NULL,
                &HandleCount,
                &HandleBuffer
               );
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status)) {
    return (Status);
  }
  for (mHandleList.NextIndex = 1 ; mHandleList.NextIndex <= HandleCount ; mHandleList.NextIndex++){
    ListWalker = AllocateZeroPool(sizeof(HANDLE_LIST));
    ASSERT(ListWalker != NULL);
    ListWalker->TheHandle = HandleBuffer[mHandleList.NextIndex-1];
    ListWalker->TheIndex  = mHandleList.NextIndex;
    InsertTailList(&mHandleList.List.Link,&ListWalker->Link);
  }
  FreePool(HandleBuffer);
  return (EFI_SUCCESS);
}

/**
  Function to retrieve the human-friendly index of a given handle.  If the handle
  does not have a index one will be automatically assigned.  The index value is valid
  until the termination of the shell application.

  @param[in] TheHandle    The handle to retrieve an index for.

  @retval 0               A memory allocation failed.
  @return                 The index of the handle.

**/
UINTN
EFIAPI
ConvertHandleToHandleIndex(
  IN CONST EFI_HANDLE TheHandle
  )
{
  HANDLE_LIST *ListWalker;
  if (TheHandle == NULL) {
    return 0;
  }

  InternalShellInitHandleList();

  for (ListWalker = (HANDLE_LIST*)GetFirstNode(&mHandleList.List.Link)
    ;  !IsNull(&mHandleList.List.Link,&ListWalker->Link)
    ;  ListWalker = (HANDLE_LIST*)GetNextNode(&mHandleList.List.Link,&ListWalker->Link)
   ){
    if (ListWalker->TheHandle == TheHandle) {
      return (ListWalker->TheIndex);
    }
  }
  ListWalker = AllocateZeroPool(sizeof(HANDLE_LIST));
  ASSERT(ListWalker != NULL);
  ListWalker->TheHandle = TheHandle;
  ListWalker->TheIndex  = mHandleList.NextIndex++;
  InsertTailList(&mHandleList.List.Link,&ListWalker->Link);
  return (ListWalker->TheIndex);
}



/**
  Function to retrieve the EFI_HANDLE from the human-friendly index.

  @param[in] TheIndex     The index to retrieve the EFI_HANDLE for.

  @retval NULL            The index was invalid.
  @return                 The EFI_HANDLE that index represents.

**/
EFI_HANDLE
EFIAPI
ConvertHandleIndexToHandle(
  IN CONST UINTN TheIndex
  )
{
  HANDLE_LIST *ListWalker;

  InternalShellInitHandleList();

  if (TheIndex >= mHandleList.NextIndex) {
    return (NULL);
  }

  for (ListWalker = (HANDLE_LIST*)GetFirstNode(&mHandleList.List.Link)
    ;  !IsNull(&mHandleList.List.Link,&ListWalker->Link)
    ;  ListWalker = (HANDLE_LIST*)GetNextNode(&mHandleList.List.Link,&ListWalker->Link)
   ){
    if (ListWalker->TheIndex == TheIndex) {
      return (ListWalker->TheHandle);
    }
  }
  return (NULL);
}

/**
  Gets all the related EFI_HANDLEs based on the mask supplied.

  This function scans all EFI_HANDLES in the UEFI environment's handle database
  and returns the ones with the specified relationship (Mask) to the specified
  controller handle.

  If both DriverBindingHandle and ControllerHandle are NULL, then ASSERT.
  If MatchingHandleCount is NULL, then ASSERT.

  If MatchingHandleBuffer is not NULL upon a successful return the memory must be
  caller freed.

  @param[in] DriverBindingHandle    The handle with Driver Binding protocol on it.
  @param[in] ControllerHandle       The handle with Device Path protocol on it.
  @param[in] MatchingHandleCount    The pointer to UINTN that specifies the number of HANDLES in
                                    MatchingHandleBuffer.
  @param[out] MatchingHandleBuffer  On a successful return, a buffer of MatchingHandleCount
                                    EFI_HANDLEs with a terminating NULL EFI_HANDLE.
  @param[out] HandleType            An array of type information.

  @retval EFI_SUCCESS               The operation was successful, and any related handles
                                    are in MatchingHandleBuffer.
  @retval EFI_NOT_FOUND             No matching handles were found.
  @retval EFI_INVALID_PARAMETER     A parameter was invalid or out of range.
**/
EFI_STATUS
EFIAPI
ParseHandleDatabaseByRelationshipWithType (
  IN CONST EFI_HANDLE DriverBindingHandle OPTIONAL,
  IN CONST EFI_HANDLE ControllerHandle OPTIONAL,
  IN UINTN            *HandleCount,
  OUT EFI_HANDLE      **HandleBuffer,
  OUT UINTN           **HandleType
  )
{
  EFI_STATUS                          Status;
  UINTN                               HandleIndex;
  EFI_GUID                            **ProtocolGuidArray;
  UINTN                               ArrayCount;
  UINTN                               ProtocolIndex;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenInfo;
  UINTN                               OpenInfoCount;
  UINTN                               OpenInfoIndex;
  UINTN                               ChildIndex;

  ASSERT(HandleCount  != NULL);
  ASSERT(HandleBuffer != NULL);
  ASSERT(HandleType   != NULL);
  ASSERT(DriverBindingHandle != NULL || ControllerHandle != NULL);

  *HandleCount                  = 0;
  *HandleBuffer                 = NULL;
  *HandleType                   = NULL;

  //
  // Retrieve the list of all handles from the handle database
  //
  Status = gBS->LocateHandleBuffer (
                AllHandles,
                NULL,
                NULL,
                HandleCount,
                HandleBuffer
               );
  if (EFI_ERROR (Status)) {
    return (Status);
  }

  *HandleType = AllocateZeroPool (*HandleCount * sizeof (UINTN));
  ASSERT(*HandleType != NULL);

  for (HandleIndex = 0; HandleIndex < *HandleCount; HandleIndex++) {
    //
    // Retrieve the list of all the protocols on each handle
    //
    Status = gBS->ProtocolsPerHandle (
                  (*HandleBuffer)[HandleIndex],
                  &ProtocolGuidArray,
                  &ArrayCount
                 );
    if (!EFI_ERROR (Status)) {

      for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++) {

        //
        // Set the bit describing what this handle has
        //
        if        (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiLoadedImageProtocolGuid)         ) {
          (*HandleType)[HandleIndex] |= HR_IMAGE_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverBindingProtocolGuid)       ) {
          (*HandleType)[HandleIndex] |= HR_DRIVER_BINDING_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverConfiguration2ProtocolGuid)) {
          (*HandleType)[HandleIndex] |= HR_DRIVER_CONFIGURATION_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverConfigurationProtocolGuid) ) {
          (*HandleType)[HandleIndex] |= HR_DRIVER_CONFIGURATION_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverDiagnostics2ProtocolGuid)  ) {
          (*HandleType)[HandleIndex] |= HR_DRIVER_DIAGNOSTICS_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverDiagnosticsProtocolGuid)   ) {
          (*HandleType)[HandleIndex] |= HR_DRIVER_DIAGNOSTICS_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiComponentName2ProtocolGuid)      ) {
          (*HandleType)[HandleIndex] |= HR_COMPONENT_NAME_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiComponentNameProtocolGuid)       ) {
          (*HandleType)[HandleIndex] |= HR_COMPONENT_NAME_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDevicePathProtocolGuid)          ) {
          (*HandleType)[HandleIndex] |= HR_DEVICE_HANDLE;
        } else {
          DEBUG_CODE_BEGIN();
          ASSERT((*HandleType)[HandleIndex] == (*HandleType)[HandleIndex]);
          DEBUG_CODE_END();
        }
        //
        // Retrieve the list of agents that have opened each protocol
        //
        Status = gBS->OpenProtocolInformation (
                      (*HandleBuffer)[HandleIndex],
                      ProtocolGuidArray[ProtocolIndex],
                      &OpenInfo,
                      &OpenInfoCount
                     );
        if (!EFI_ERROR (Status)) {
          for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
            if (DriverBindingHandle != NULL && OpenInfo[OpenInfoIndex].AgentHandle == DriverBindingHandle) {
              if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) == EFI_OPEN_PROTOCOL_BY_DRIVER) {
                (*HandleType)[HandleIndex] |= (HR_DEVICE_HANDLE | HR_CONTROLLER_HANDLE);
              }
              if (ControllerHandle != NULL && (*HandleBuffer)[HandleIndex] == ControllerHandle) {
                if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) == EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) {
                  for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                    if ((*HandleBuffer)[ChildIndex] == OpenInfo[OpenInfoIndex].ControllerHandle) {
                      (*HandleType)[ChildIndex] |= (HR_DEVICE_HANDLE | HR_CHILD_HANDLE);
                    }
                  }
                }
              }
            }
            if (DriverBindingHandle == NULL && OpenInfo[OpenInfoIndex].ControllerHandle == ControllerHandle) {
              if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) == EFI_OPEN_PROTOCOL_BY_DRIVER) {
                for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                  if ((*HandleBuffer)[ChildIndex] == OpenInfo[OpenInfoIndex].AgentHandle) {
                    (*HandleType)[ChildIndex] |= HR_DEVICE_DRIVER;
                  }
                }
              }
              if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) == EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) {
                (*HandleType)[HandleIndex] |= HR_PARENT_HANDLE;
                for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                  if ((*HandleBuffer)[ChildIndex] == OpenInfo[OpenInfoIndex].AgentHandle) {
                    (*HandleType)[ChildIndex] |= HR_BUS_DRIVER;
                  }
                }
              }
            }
          }

          FreePool (OpenInfo);
        }
      }

      FreePool (ProtocolGuidArray);
    }
  }

  if (EFI_ERROR(Status)) {
    if (*HandleType != NULL) {
      FreePool (*HandleType);
    }
    if (*HandleBuffer != NULL) {
      FreePool (*HandleBuffer);
    }

    *HandleCount  = 0;
    *HandleBuffer = NULL;
    *HandleType   = NULL;
  }

  return Status;
}

/**
  Gets all the related EFI_HANDLEs based on the single EFI_HANDLE and the mask
  supplied.

  This function will scan all EFI_HANDLES in the UEFI environment's handle database
  and return all the ones with the specified relationship (Mask) to the specified
  controller handle.

  If both DriverBindingHandle and ControllerHandle are NULL, then ASSERT.
  If MatchingHandleCount is NULL, then ASSERT.

  If MatchingHandleBuffer is not NULL upon a sucessful return the memory must be
  caller freed.

  @param[in] DriverBindingHandle    Handle to a object with Driver Binding protocol
                                    on it.
  @param[in] ControllerHandle       Handle to a device with Device Path protocol on it.
  @param[in] Mask                   Mask of what relationship(s) is desired.
  @param[in] MatchingHandleCount    Poitner to UINTN specifying number of HANDLES in
                                    MatchingHandleBuffer.
  @param[out] MatchingHandleBuffer  On a sucessful return a buffer of MatchingHandleCount
                                    EFI_HANDLEs and a terminating NULL EFI_HANDLE.

  @retval EFI_SUCCESS               The operation was sucessful and any related handles
                                    are in MatchingHandleBuffer;
  @retval EFI_NOT_FOUND             No matching handles were found.
  @retval EFI_INVALID_PARAMETER     A parameter was invalid or out of range.
**/
EFI_STATUS
EFIAPI
ParseHandleDatabaseByRelationship (
  IN CONST EFI_HANDLE       DriverBindingHandle OPTIONAL,
  IN CONST EFI_HANDLE       ControllerHandle OPTIONAL,
  IN CONST UINTN            Mask,
  IN UINTN                  *MatchingHandleCount,
  OUT EFI_HANDLE            **MatchingHandleBuffer OPTIONAL
  )
{
  EFI_STATUS            Status;
  UINTN                 HandleCount;
  EFI_HANDLE            *HandleBuffer;
  UINTN                 *HandleType;
  UINTN                 HandleIndex;

  ASSERT(MatchingHandleCount != NULL);
  ASSERT(DriverBindingHandle != NULL || ControllerHandle != NULL);

  if ((Mask & HR_VALID_MASK) != Mask) {
    return (EFI_INVALID_PARAMETER);
  }

  if ((Mask & HR_CHILD_HANDLE) != 0 && DriverBindingHandle == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  *MatchingHandleCount = 0;
  if (MatchingHandleBuffer != NULL) {
    *MatchingHandleBuffer = NULL;
  }

  HandleBuffer  = NULL;
  HandleType    = NULL;

  Status = ParseHandleDatabaseByRelationshipWithType (
            DriverBindingHandle,
            ControllerHandle,
            &HandleCount,
            &HandleBuffer,
            &HandleType
           );
  if (!EFI_ERROR (Status)) {
    //
    // Count the number of handles that match the attributes in Mask
    //
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
      if ((HandleType[HandleIndex] & Mask) == Mask) {
        (*MatchingHandleCount)++;
      }
    }
    //
    // If no handles match the attributes in Mask then return EFI_NOT_FOUND
    //
    if (*MatchingHandleCount == 0) {
      Status = EFI_NOT_FOUND;
    } else {

      if (MatchingHandleBuffer == NULL) {
        //
        // Someone just wanted the count...
        //
        Status = EFI_SUCCESS;
      } else {
        //
        // Allocate a handle buffer for the number of handles that matched the attributes in Mask
        //
        *MatchingHandleBuffer = AllocateZeroPool ((*MatchingHandleCount +1)* sizeof (EFI_HANDLE));
        ASSERT(*MatchingHandleBuffer != NULL);

        for (HandleIndex = 0,*MatchingHandleCount = 0
          ;  HandleIndex < HandleCount
          ;  HandleIndex++
         ){
          //
          // Fill the allocated buffer with the handles that matched the attributes in Mask
          //
          if ((HandleType[HandleIndex] & Mask) == Mask) {
            (*MatchingHandleBuffer)[(*MatchingHandleCount)++] = HandleBuffer[HandleIndex];
          }
        }

        //
        // Make the last one NULL
        //
        (*MatchingHandleBuffer)[*MatchingHandleCount] = NULL;

        Status = EFI_SUCCESS;
      } // MacthingHandleBuffer == NULL (ELSE)
    } // *MatchingHandleCount  == 0 (ELSE)
  } // no error on ParseHandleDatabaseByRelationshipWithType

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  if (HandleType != NULL) {
    FreePool (HandleType);
  }

  return Status;
}

/**
  Gets handles for any child controllers of the passed in controller.

  @param[in] ControllerHandle       The handle of the "parent controller"
  @param[in] MatchingHandleCount    Pointer to the number of handles in
                                    MatchingHandleBuffer on return.
  @param[out] MatchingHandleBuffer  Buffer containing handles on a successful
                                    return.


  @retval EFI_SUCCESS               The operation was sucessful.
**/
EFI_STATUS
EFIAPI
ParseHandleDatabaseForChildControllers(
  IN CONST EFI_HANDLE       ControllerHandle,
  IN UINTN                  *MatchingHandleCount,
  OUT EFI_HANDLE            **MatchingHandleBuffer OPTIONAL
  )
{
  EFI_STATUS  Status;
//  UINTN       HandleIndex;
  UINTN       DriverBindingHandleCount;
  EFI_HANDLE  *DriverBindingHandleBuffer;
  UINTN       DriverBindingHandleIndex;
  UINTN       ChildControllerHandleCount;
  EFI_HANDLE  *ChildControllerHandleBuffer;
  UINTN       ChildControllerHandleIndex;
//  BOOLEAN     Found;
  EFI_HANDLE  *HandleBufferForReturn;

  if (MatchingHandleCount == NULL) {
    return (EFI_INVALID_PARAMETER);
  }
  *MatchingHandleCount = 0;

  Status = PARSE_HANDLE_DATABASE_UEFI_DRIVERS (
            ControllerHandle,
            &DriverBindingHandleCount,
            &DriverBindingHandleBuffer
           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get a buffer big enough for all the controllers.
  //
  HandleBufferForReturn = GetHandleListByProtocol(&gEfiDevicePathProtocolGuid);
  if (HandleBufferForReturn == NULL) {
    FreePool (DriverBindingHandleBuffer);
    return (EFI_NOT_FOUND);
  }

  for (DriverBindingHandleIndex = 0; DriverBindingHandleIndex < DriverBindingHandleCount; DriverBindingHandleIndex++) {
    Status = PARSE_HANDLE_DATABASE_MANAGED_CHILDREN (
              DriverBindingHandleBuffer[DriverBindingHandleIndex],
              ControllerHandle,
              &ChildControllerHandleCount,
              &ChildControllerHandleBuffer
             );
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (ChildControllerHandleIndex = 0;
         ChildControllerHandleIndex < ChildControllerHandleCount;
         ChildControllerHandleIndex++
       ) {
//      Found = FALSE;
      HandleBufferForReturn[(*MatchingHandleCount)++] = ChildControllerHandleBuffer[ChildControllerHandleIndex];
//      for (HandleIndex = 0; HandleBufferForReturn[HandleIndex] != NULL; HandleIndex++) {
//        if (HandleBufferForReturn[HandleIndex] == ChildControllerHandleBuffer[ChildControllerHandleIndex]) {
//          Found = TRUE;
//          break;
//        }
//      }

//      if (Found) {
//        HandleBufferForReturn[(*MatchingHandleCount)++] = ChildControllerHandleBuffer[ChildControllerHandleIndex];
//      }
    }

    FreePool (ChildControllerHandleBuffer);
  }

  FreePool (DriverBindingHandleBuffer);

  if (MatchingHandleBuffer != NULL) {
    *MatchingHandleBuffer = HandleBufferForReturn;
  } else {
    FreePool(HandleBufferForReturn);
  }

  return (EFI_SUCCESS);
}

/**
  Appends 1 buffer to another buffer.  This will re-allocate the destination buffer
  if necessary to fit all of the data.

  If DestinationBuffer is NULL, then ASSERT().

  @param[in, out]  DestinationBuffer The pointer to the pointer to the buffer to append onto.
  @param[in, out]  DestinationSize   The pointer to the size of DestinationBuffer.
  @param[in]       SourceBuffer      The pointer to the buffer to append onto DestinationBuffer.
  @param[in]       SourceSize        The number of bytes of SourceBuffer to append.

  @retval NULL                      A memory allocation failed.
  @retval NULL                      A parameter was invalid.
  @return                           A pointer to (*DestinationBuffer).
**/
VOID*
EFIAPI
BuffernCatGrow (
  IN OUT VOID   **DestinationBuffer,
  IN OUT UINTN  *DestinationSize,
  IN     VOID   *SourceBuffer,
  IN     UINTN  SourceSize
  )
{
  UINTN LocalDestinationSize;
  UINTN LocalDestinationFinalSize;

  ASSERT(DestinationBuffer != NULL);

  if (SourceSize == 0 || SourceBuffer == NULL) {
    return (*DestinationBuffer);
  }

  if (DestinationSize == NULL) {
    LocalDestinationSize = 0;
  } else {
    LocalDestinationSize = *DestinationSize;
  }

  LocalDestinationFinalSize = LocalDestinationSize + SourceSize;

  if (DestinationSize != NULL) {
    *DestinationSize = LocalDestinationSize;
  }

  if (LocalDestinationSize == 0) {
    // allcoate
    *DestinationBuffer = AllocateZeroPool(LocalDestinationFinalSize);
  } else {
    // reallocate
    *DestinationBuffer = ReallocatePool(LocalDestinationSize, LocalDestinationFinalSize, *DestinationBuffer);
  }

  ASSERT(*DestinationBuffer != NULL);

  // copy
  return (CopyMem(((UINT8*)(*DestinationBuffer)) + LocalDestinationSize, SourceBuffer, SourceSize));
}

/**
  Gets handles for any child devices produced by the passed in driver.

  @param[in] DriverHandle           The handle of the driver.
  @param[in] MatchingHandleCount    Pointer to the number of handles in
                                    MatchingHandleBuffer on return.
  @param[out] MatchingHandleBuffer  Buffer containing handles on a successful
                                    return.
  @retval EFI_SUCCESS               The operation was sucessful.
  @sa ParseHandleDatabaseByRelationship
**/
EFI_STATUS
EFIAPI
ParseHandleDatabaseForChildDevices(
  IN CONST EFI_HANDLE       DriverHandle,
  IN UINTN                  *MatchingHandleCount,
  OUT EFI_HANDLE            **MatchingHandleBuffer OPTIONAL
  )
{
  EFI_HANDLE      *Buffer;
  EFI_HANDLE      *Buffer2;
  UINTN           Count1;
  UINTN           Count2;
  UINTN           HandleIndex;
  EFI_STATUS      Status;
  UINTN           HandleBufferSize;

  ASSERT(MatchingHandleCount != NULL);

  HandleBufferSize      = 0;
  Buffer                = NULL;
  Buffer2               = NULL;
  *MatchingHandleCount  = 0;

  Status = PARSE_HANDLE_DATABASE_DEVICES (
            DriverHandle,
            &Count1,
            &Buffer
           );
  if (!EFI_ERROR (Status)) {
    for (HandleIndex = 0; HandleIndex < Count1; HandleIndex++) {
      //
      // now find the children
      //
      Status = PARSE_HANDLE_DATABASE_MANAGED_CHILDREN (
                DriverHandle,
                Buffer[HandleIndex],
                &Count2,
                &Buffer2
               );
      if (EFI_ERROR(Status)) {
        break;
      }
      //
      // save out required and optional data elements
      //
      *MatchingHandleCount += Count2;
      if (MatchingHandleBuffer != NULL) {
        *MatchingHandleBuffer = BuffernCatGrow((VOID**)MatchingHandleBuffer, &HandleBufferSize, Buffer2, Count2 * sizeof(Buffer2[0]));
      }

      //
      // free the memory
      //
      if (Buffer2 != NULL) {
        FreePool(Buffer2);
      }
    }
  }

  if (Buffer != NULL) {
    FreePool(Buffer);
  }
  return (Status);
}

/**
  Function to get all handles that support a given protocol or all handles.

  @param[in] ProtocolGuid The guid of the protocol to get handles for.  If NULL
                          then the function will return all handles.

  @retval NULL            A memory allocation failed.
  @return                 A NULL terminated list of handles.
**/
EFI_HANDLE*
EFIAPI
GetHandleListByProtocol (
  IN CONST EFI_GUID *ProtocolGuid OPTIONAL
  )
{
  EFI_HANDLE          *HandleList;
  UINTN               Size;
  EFI_STATUS          Status;

  Size = 0;
  HandleList = NULL;

  //
  // We cannot use LocateHandleBuffer since we need that NULL item on the ends of the list!
  //
  if (ProtocolGuid == NULL) {
    Status = gBS->LocateHandle(AllHandles, NULL, NULL, &Size, HandleList);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      HandleList = AllocateZeroPool(Size + sizeof(EFI_HANDLE));
      if (HandleList == NULL) {
        return (NULL);
      }
      Status = gBS->LocateHandle(AllHandles, NULL, NULL, &Size, HandleList);
      HandleList[Size/sizeof(EFI_HANDLE)] = NULL;
    }
  } else {
    Status = gBS->LocateHandle(ByProtocol, (EFI_GUID*)ProtocolGuid, NULL, &Size, HandleList);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      HandleList = AllocateZeroPool(Size + sizeof(EFI_HANDLE));
      if (HandleList == NULL) {
        return (NULL);
      }
      Status = gBS->LocateHandle(ByProtocol, (EFI_GUID*)ProtocolGuid, NULL, &Size, HandleList);
      HandleList[Size/sizeof(EFI_HANDLE)] = NULL;
    }
  }
  if (EFI_ERROR(Status)) {
    if (HandleList != NULL) {
      FreePool(HandleList);
    }
    return (NULL);
  }
  return (HandleList);
}

/**
  Function to get all handles that support some protocols.

  @param[in] ProtocolGuids  A NULL terminated list of protocol GUIDs.

  @retval NULL              A memory allocation failed.
  @retval NULL              ProtocolGuids was NULL.
  @return                   A NULL terminated list of EFI_HANDLEs.
**/
EFI_HANDLE*
EFIAPI
GetHandleListByProtocolList (
  IN CONST EFI_GUID **ProtocolGuids
  )
{
  EFI_HANDLE          *HandleList;
  UINTN               Size;
  UINTN               TotalSize;
  UINTN               TempSize;
  EFI_STATUS          Status;
  CONST EFI_GUID      **GuidWalker;
  EFI_HANDLE          *HandleWalker1;
  EFI_HANDLE          *HandleWalker2;

  Size        = 0;
  HandleList  = NULL;
  TotalSize   = sizeof(EFI_HANDLE);

  for (GuidWalker = ProtocolGuids ; GuidWalker != NULL && *GuidWalker != NULL ; GuidWalker++,Size = 0){
    Status = gBS->LocateHandle(ByProtocol, (EFI_GUID*)(*GuidWalker), NULL, &Size, NULL);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      TotalSize += Size;
    }
  }

  //
  // No handles were found... 
  //
  if (TotalSize == sizeof(EFI_HANDLE)) {
    return (NULL);
  }

  HandleList = AllocateZeroPool(TotalSize);
  if (HandleList == NULL) {
    return (NULL);
  }

  Size = 0;
  for (GuidWalker = ProtocolGuids ; GuidWalker != NULL && *GuidWalker != NULL ; GuidWalker++){
    TempSize = TotalSize - Size;
    Status = gBS->LocateHandle(ByProtocol, (EFI_GUID*)(*GuidWalker), NULL, &TempSize, HandleList+(Size/sizeof(EFI_HANDLE)));

    //
    // Allow for missing protocols... Only update the 'used' size upon success.
    //
    if (!EFI_ERROR(Status)) {
      Size += TempSize;
    }
  }
  ASSERT(HandleList[(TotalSize/sizeof(EFI_HANDLE))-1] == NULL);

  for (HandleWalker1 = HandleList ; HandleWalker1 != NULL && *HandleWalker1 != NULL ; HandleWalker1++) {
    for (HandleWalker2 = HandleWalker1 + 1; HandleWalker2 != NULL && *HandleWalker2 != NULL ; HandleWalker2++) {
      if (*HandleWalker1 == *HandleWalker2) {
        //
        // copy memory back 1 handle width.
        //
        CopyMem(HandleWalker2, HandleWalker2 + 1, TotalSize - ((HandleWalker2-HandleList+1)*sizeof(EFI_HANDLE)));
      }
    }
  }

  return (HandleList);
}










