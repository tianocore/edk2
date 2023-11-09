/** @file
  Provides interface to advanced shell functionality for parsing both handle and protocol database.

  Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2015-2021 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiHandleParsingLib.h"
#include "IndustryStandard/Acpi10.h"
#include "IndustryStandard/Pci.h"
#include <PiDxe.h>
#include <Protocol/FirmwareVolume2.h>

EFI_HII_HANDLE     mHandleParsingHiiHandle = NULL;
HANDLE_INDEX_LIST  mHandleList             = {
  {
    { NULL, NULL }, 0, 0
  }, 0
};
GUID_INFO_BLOCK    *mGuidList;
UINTN              mGuidListCount;

/**
  Function to find the file name associated with a LoadedImageProtocol.

  @param[in] LoadedImage     An instance of LoadedImageProtocol.

  @retval                    A string representation of the file name associated
                             with LoadedImage, or NULL if no name can be found.
**/
CHAR16 *
FindLoadedImageFileName (
  IN EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage
  )
{
  EFI_GUID                       *NameGuid;
  EFI_STATUS                     Status;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *Fv;
  VOID                           *Buffer;
  UINTN                          BufferSize;
  UINT32                         AuthenticationStatus;

  if ((LoadedImage == NULL) || (LoadedImage->FilePath == NULL)) {
    return NULL;
  }

  NameGuid = EfiGetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)LoadedImage->FilePath);

  if (NameGuid == NULL) {
    return NULL;
  }

  //
  // Get the FirmwareVolume2Protocol of the device handle that this image was loaded from.
  //
  Status = gBS->HandleProtocol (LoadedImage->DeviceHandle, &gEfiFirmwareVolume2ProtocolGuid, (VOID **)&Fv);

  //
  // FirmwareVolume2Protocol is PI, and is not required to be available.
  //
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Read the user interface section of the image.
  //
  Buffer = NULL;
  Status = Fv->ReadSection (Fv, NameGuid, EFI_SECTION_USER_INTERFACE, 0, &Buffer, &BufferSize, &AuthenticationStatus);

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // ReadSection returns just the section data, without any section header. For
  // a user interface section, the only data is the file name.
  //
  return Buffer;
}

/**
  Function to translate the EFI_MEMORY_TYPE into a string.

  @param[in] Memory     The memory type.

  @retval               A string representation of the type allocated from BS Pool.
**/
CHAR16 *
ConvertMemoryType (
  IN CONST EFI_MEMORY_TYPE  Memory
  )
{
  CHAR16  *RetVal;

  RetVal = NULL;

  switch (Memory) {
    case EfiReservedMemoryType:       StrnCatGrow (&RetVal, NULL, L"EfiReservedMemoryType", 0);
      break;
    case EfiLoaderCode:               StrnCatGrow (&RetVal, NULL, L"EfiLoaderCode", 0);
      break;
    case EfiLoaderData:               StrnCatGrow (&RetVal, NULL, L"EfiLoaderData", 0);
      break;
    case EfiBootServicesCode:         StrnCatGrow (&RetVal, NULL, L"EfiBootServicesCode", 0);
      break;
    case EfiBootServicesData:         StrnCatGrow (&RetVal, NULL, L"EfiBootServicesData", 0);
      break;
    case EfiRuntimeServicesCode:      StrnCatGrow (&RetVal, NULL, L"EfiRuntimeServicesCode", 0);
      break;
    case EfiRuntimeServicesData:      StrnCatGrow (&RetVal, NULL, L"EfiRuntimeServicesData", 0);
      break;
    case EfiConventionalMemory:       StrnCatGrow (&RetVal, NULL, L"EfiConventionalMemory", 0);
      break;
    case EfiUnusableMemory:           StrnCatGrow (&RetVal, NULL, L"EfiUnusableMemory", 0);
      break;
    case EfiACPIReclaimMemory:        StrnCatGrow (&RetVal, NULL, L"EfiACPIReclaimMemory", 0);
      break;
    case EfiACPIMemoryNVS:            StrnCatGrow (&RetVal, NULL, L"EfiACPIMemoryNVS", 0);
      break;
    case EfiMemoryMappedIO:           StrnCatGrow (&RetVal, NULL, L"EfiMemoryMappedIO", 0);
      break;
    case EfiMemoryMappedIOPortSpace:  StrnCatGrow (&RetVal, NULL, L"EfiMemoryMappedIOPortSpace", 0);
      break;
    case EfiPalCode:                  StrnCatGrow (&RetVal, NULL, L"EfiPalCode", 0);
      break;
    case EfiMaxMemoryType:            StrnCatGrow (&RetVal, NULL, L"EfiMaxMemoryType", 0);
      break;
    default: ASSERT (FALSE);
  }

  return (RetVal);
}

/**
  Function to translate the EFI_GRAPHICS_PIXEL_FORMAT into a string.

  @param[in] Fmt     The format type.

  @retval               A string representation of the type allocated from BS Pool.
**/
CHAR16 *
ConvertPixelFormat (
  IN CONST EFI_GRAPHICS_PIXEL_FORMAT  Fmt
  )
{
  CHAR16  *RetVal;

  RetVal = NULL;

  switch (Fmt) {
    case PixelRedGreenBlueReserved8BitPerColor: StrnCatGrow (&RetVal, NULL, L"PixelRedGreenBlueReserved8BitPerColor", 0);
      break;
    case PixelBlueGreenRedReserved8BitPerColor: StrnCatGrow (&RetVal, NULL, L"PixelBlueGreenRedReserved8BitPerColor", 0);
      break;
    case PixelBitMask:                          StrnCatGrow (&RetVal, NULL, L"PixelBitMask", 0);
      break;
    case PixelBltOnly:                          StrnCatGrow (&RetVal, NULL, L"PixelBltOnly", 0);
      break;
    case PixelFormatMax:                        StrnCatGrow (&RetVal, NULL, L"PixelFormatMax", 0);
      break;
    default: ASSERT (FALSE);
  }

  return (RetVal);
}

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
  mGuidListCount = 0;
  mGuidList      = NULL;

  //
  // Do nothing with mHandleParsingHiiHandle.  Initialize HII as needed.
  //
  return (EFI_SUCCESS);
}

/**
  Initialization function for HII packages.

**/
VOID
HandleParsingHiiInit (
  VOID
  )
{
  if (mHandleParsingHiiHandle == NULL) {
    mHandleParsingHiiHandle = HiiAddPackages (&gHandleParsingHiiGuid, gImageHandle, UefiHandleParsingLibStrings, NULL);
    ASSERT (mHandleParsingHiiHandle != NULL);
  }
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
  UINTN  LoopCount;

  for (LoopCount = 0; mGuidList != NULL && LoopCount < mGuidListCount; LoopCount++) {
    SHELL_FREE_NON_NULL (mGuidList[LoopCount].GuidId);
  }

  SHELL_FREE_NON_NULL (mGuidList);
  if (mHandleParsingHiiHandle != NULL) {
    HiiRemovePackages (mHandleParsingHiiHandle);
  }

  return (EFI_SUCCESS);
}

/**
  Function to dump information about LoadedImage.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has LoadedImage installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A poitner to a string containing the information.
**/
CHAR16 *
EFIAPI
LoadedImageProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  EFI_STATUS                 Status;
  CHAR16                     *RetVal;
  CHAR16                     *Temp;
  CHAR16                     *FileName;
  CHAR8                      *PdbFileName;
  CHAR16                     *FilePath;
  CHAR16                     *CodeType;
  CHAR16                     *DataType;

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  FileName = FindLoadedImageFileName (LoadedImage);
  FilePath = ConvertDevicePathToText (LoadedImage->FilePath, TRUE, TRUE);
  if (!Verbose) {
    if (FileName == NULL) {
      FileName = FilePath;
    } else {
      SHELL_FREE_NON_NULL (FilePath);
    }

    return FileName;
  }

  HandleParsingHiiInit ();
  RetVal = NULL;
  if (FileName != NULL) {
    Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_LI_DUMP_NAME), NULL);

    if (Temp != NULL) {
      RetVal = CatSPrint (NULL, Temp, FileName);
    }

    SHELL_FREE_NON_NULL (Temp);
    SHELL_FREE_NON_NULL (FileName);
  }

  Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_LI_DUMP_MAIN), NULL);
  if (Temp == NULL) {
    return NULL;
  }

  PdbFileName = PeCoffLoaderGetPdbPointer (LoadedImage->ImageBase);
  DataType    = ConvertMemoryType (LoadedImage->ImageDataType);
  CodeType    = ConvertMemoryType (LoadedImage->ImageCodeType);

  RetVal = CatSPrint (
             RetVal,
             Temp,
             LoadedImage->Revision,
             LoadedImage->ParentHandle,
             LoadedImage->SystemTable,
             LoadedImage->DeviceHandle,
             FilePath,
             PdbFileName,
             LoadedImage->LoadOptionsSize,
             LoadedImage->LoadOptions,
             LoadedImage->ImageBase,
             LoadedImage->ImageSize,
             CodeType,
             DataType,
             LoadedImage->Unload
             );

  SHELL_FREE_NON_NULL (Temp);
  SHELL_FREE_NON_NULL (FilePath);
  SHELL_FREE_NON_NULL (CodeType);
  SHELL_FREE_NON_NULL (DataType);

  return RetVal;
}

/**
  Function to dump information about GOP.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has LoadedImage installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A poitner to a string containing the information.
**/
CHAR16 *
EFIAPI
GraphicsOutputProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  EFI_STATUS                            Status;
  CHAR16                                *RetVal;
  CHAR16                                *Temp;
  CHAR16                                *Fmt;
  CHAR16                                *TempRetVal;
  UINTN                                 GopInfoSize;
  UINT32                                Mode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *GopInfo;

  if (!Verbose) {
    return (CatSPrint (NULL, L"GraphicsOutput"));
  }

  HandleParsingHiiInit ();

  Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_GOP_DUMP_MAIN), NULL);
  if (Temp == NULL) {
    return NULL;
  }

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&GraphicsOutput,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    SHELL_FREE_NON_NULL (Temp);
    return NULL;
  }

  Fmt = ConvertPixelFormat (GraphicsOutput->Mode->Info->PixelFormat);

  RetVal = CatSPrint (
             NULL,
             Temp,
             GraphicsOutput->Mode->MaxMode,
             GraphicsOutput->Mode->Mode,
             GraphicsOutput->Mode->FrameBufferBase,
             (UINT64)GraphicsOutput->Mode->FrameBufferSize,
             (UINT64)GraphicsOutput->Mode->SizeOfInfo,
             GraphicsOutput->Mode->Info->Version,
             GraphicsOutput->Mode->Info->HorizontalResolution,
             GraphicsOutput->Mode->Info->VerticalResolution,
             Fmt,
             GraphicsOutput->Mode->Info->PixelsPerScanLine,
             GraphicsOutput->Mode->Info->PixelFormat != PixelBitMask ? 0 : GraphicsOutput->Mode->Info->PixelInformation.RedMask,
             GraphicsOutput->Mode->Info->PixelFormat != PixelBitMask ? 0 : GraphicsOutput->Mode->Info->PixelInformation.GreenMask,
             GraphicsOutput->Mode->Info->PixelFormat != PixelBitMask ? 0 : GraphicsOutput->Mode->Info->PixelInformation.BlueMask
             );

  SHELL_FREE_NON_NULL (Temp);

  Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_GOP_RES_LIST_MAIN), NULL);
  if (Temp == NULL) {
    SHELL_FREE_NON_NULL (RetVal);
    goto EXIT;
  }

  TempRetVal = CatSPrint (RetVal, Temp);
  SHELL_FREE_NON_NULL (RetVal);
  if (TempRetVal == NULL) {
    goto EXIT;
  }

  RetVal = TempRetVal;
  SHELL_FREE_NON_NULL (Temp);

  Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_GOP_RES_LIST_ENTRY), NULL);
  if (Temp == NULL) {
    SHELL_FREE_NON_NULL (RetVal);
    goto EXIT;
  }

  for (Mode = 0; Mode < GraphicsOutput->Mode->MaxMode; Mode++) {
    Status = GraphicsOutput->QueryMode (
                               GraphicsOutput,
                               Mode,
                               &GopInfoSize,
                               &GopInfo
                               );
    if (EFI_ERROR (Status)) {
      continue;
    }

    TempRetVal = CatSPrint (
                   RetVal,
                   Temp,
                   Mode,
                   GopInfo->HorizontalResolution,
                   GopInfo->VerticalResolution
                   );

    SHELL_FREE_NON_NULL (GopInfo);
    SHELL_FREE_NON_NULL (RetVal);
    RetVal = TempRetVal;
  }

EXIT:
  SHELL_FREE_NON_NULL (Temp);
  SHELL_FREE_NON_NULL (Fmt);

  return RetVal;
}

/**
  Function to dump information about EDID Discovered Protocol.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has LoadedImage installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A pointer to a string containing the information.
**/
CHAR16 *
EFIAPI
EdidDiscoveredProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_EDID_DISCOVERED_PROTOCOL  *EdidDiscovered;
  EFI_STATUS                    Status;
  CHAR16                        *RetVal;
  CHAR16                        *Temp;
  CHAR16                        *TempRetVal;

  if (!Verbose) {
    return (CatSPrint (NULL, L"EDIDDiscovered"));
  }

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiEdidDiscoveredProtocolGuid,
                  (VOID **)&EdidDiscovered,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_EDID_DISCOVERED_MAIN), NULL);
  if (Temp == NULL) {
    return NULL;
  }

  RetVal = CatSPrint (NULL, Temp, EdidDiscovered->SizeOfEdid);
  SHELL_FREE_NON_NULL (Temp);

  if (EdidDiscovered->SizeOfEdid != 0) {
    Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_EDID_DISCOVERED_DATA), NULL);
    if (Temp == NULL) {
      SHELL_FREE_NON_NULL (RetVal);
      return NULL;
    }

    TempRetVal = CatSPrint (RetVal, Temp);
    SHELL_FREE_NON_NULL (RetVal);
    RetVal = TempRetVal;

    TempRetVal = CatSDumpHex (RetVal, 4, 0, EdidDiscovered->SizeOfEdid, EdidDiscovered->Edid);
    RetVal     = TempRetVal;
  }

  return RetVal;
}

/**
  Function to dump information about EDID Active Protocol.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has LoadedImage installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A pointer to a string containing the information.
**/
CHAR16 *
EFIAPI
EdidActiveProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_EDID_ACTIVE_PROTOCOL  *EdidActive;
  EFI_STATUS                Status;
  CHAR16                    *RetVal;
  CHAR16                    *Temp;
  CHAR16                    *TempRetVal;

  if (!Verbose) {
    return (CatSPrint (NULL, L"EDIDActive"));
  }

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiEdidActiveProtocolGuid,
                  (VOID **)&EdidActive,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_EDID_ACTIVE_MAIN), NULL);
  if (Temp == NULL) {
    return NULL;
  }

  RetVal = CatSPrint (NULL, Temp, EdidActive->SizeOfEdid);
  SHELL_FREE_NON_NULL (Temp);

  if (EdidActive->SizeOfEdid != 0) {
    Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_EDID_ACTIVE_DATA), NULL);
    if (Temp == NULL) {
      SHELL_FREE_NON_NULL (RetVal);
      return NULL;
    }

    TempRetVal = CatSPrint (RetVal, Temp);
    SHELL_FREE_NON_NULL (RetVal);
    RetVal = TempRetVal;

    TempRetVal = CatSDumpHex (RetVal, 4, 0, EdidActive->SizeOfEdid, EdidActive->Edid);
    RetVal     = TempRetVal;
  }

  return RetVal;
}

/**
  Function to dump information about PciRootBridgeIo.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has PciRootBridgeIo installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A poitner to a string containing the information.
**/
CHAR16 *
EFIAPI
PciRootBridgeIoDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL    *PciRootBridgeIo;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Configuration;
  UINT64                             Supports;
  UINT64                             Attributes;
  CHAR16                             *Temp;
  CHAR16                             *Temp2;
  CHAR16                             *RetVal;
  EFI_STATUS                         Status;

  RetVal = NULL;

  if (!Verbose) {
    return (CatSPrint (NULL, L"PciRootBridgeIo"));
  }

  HandleParsingHiiInit ();

  Status = gBS->HandleProtocol (
                  TheHandle,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  (VOID **)&PciRootBridgeIo
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PCIRB_DUMP_PH), NULL);
  if (Temp == NULL) {
    return NULL;
  }

  Temp2 = CatSPrint (NULL, Temp, PciRootBridgeIo->ParentHandle);
  FreePool (Temp);
  RetVal = Temp2;
  Temp2  = NULL;

  Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PCIRB_DUMP_SEG), NULL);
  if (Temp == NULL) {
    SHELL_FREE_NON_NULL (RetVal);
    return NULL;
  }

  Temp2 = CatSPrint (RetVal, Temp, PciRootBridgeIo->SegmentNumber);
  FreePool (Temp);
  FreePool (RetVal);
  RetVal = Temp2;
  Temp2  = NULL;

  Supports   = 0;
  Attributes = 0;
  Status     = PciRootBridgeIo->GetAttributes (PciRootBridgeIo, &Supports, &Attributes);
  if (!EFI_ERROR (Status)) {
    Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PCIRB_DUMP_ATT), NULL);
    if (Temp == NULL) {
      SHELL_FREE_NON_NULL (RetVal);
      return NULL;
    }

    Temp2 = CatSPrint (RetVal, Temp, Attributes);
    FreePool (Temp);
    FreePool (RetVal);
    RetVal = Temp2;
    Temp2  = NULL;

    Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PCIRB_DUMP_SUPPORTS), NULL);
    if (Temp == NULL) {
      SHELL_FREE_NON_NULL (RetVal);
      return NULL;
    }

    Temp2 = CatSPrint (RetVal, Temp, Supports);
    FreePool (Temp);
    FreePool (RetVal);
    RetVal = Temp2;
    Temp2  = NULL;
  }

  Configuration = NULL;
  Status        = PciRootBridgeIo->Configuration (PciRootBridgeIo, (VOID **)&Configuration);
  if (!EFI_ERROR (Status) && (Configuration != NULL)) {
    Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PCIRB_DUMP_TITLE), NULL);
    if (Temp == NULL) {
      SHELL_FREE_NON_NULL (RetVal);
      return NULL;
    }

    Temp2 = CatSPrint (RetVal, Temp, Supports);
    FreePool (Temp);
    FreePool (RetVal);
    RetVal = Temp2;
    Temp2  = NULL;
    while (Configuration->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
      Temp = NULL;
      switch (Configuration->ResType) {
        case ACPI_ADDRESS_SPACE_TYPE_MEM:
          Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PCIRB_DUMP_MEM), NULL);
          break;
        case ACPI_ADDRESS_SPACE_TYPE_IO:
          Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PCIRB_DUMP_IO), NULL);
          break;
        case ACPI_ADDRESS_SPACE_TYPE_BUS:
          Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PCIRB_DUMP_BUS), NULL);
          break;
      }

      if (Temp != NULL) {
        Temp2 = CatSPrint (RetVal, L"\r\n%s", Temp);
        FreePool (Temp);
        FreePool (RetVal);
        RetVal = Temp2;
        Temp2  = NULL;
      }

      Temp2 = CatSPrint (
                RetVal,
                L"%%H%02x    %016lx  %016lx  %02x%%N",
                Configuration->SpecificFlag,
                Configuration->AddrRangeMin,
                Configuration->AddrRangeMax,
                Configuration->AddrSpaceGranularity
                );
      FreePool (RetVal);
      RetVal = Temp2;
      Temp2  = NULL;
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
CHAR16 *
EFIAPI
TxtOutProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Dev;
  INTN                             Index;
  UINTN                            Col;
  UINTN                            Row;
  EFI_STATUS                       Status;
  CHAR16                           *RetVal;
  UINTN                            Size;
  CHAR16                           *Temp;
  UINTN                            NewSize;

  if (!Verbose) {
    return (NULL);
  }

  HandleParsingHiiInit ();

  RetVal = NULL;
  Size   = 0;

  Status = gBS->HandleProtocol (
                  TheHandle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID **)&Dev
                  );

  ASSERT_EFI_ERROR (Status);
  ASSERT (Dev != NULL && Dev->Mode != NULL);

  Size   = (Dev->Mode->MaxMode + 1) * 80;
  RetVal = AllocateZeroPool (Size);

  Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_TXT_OUT_DUMP_HEADER), NULL);
  if (Temp != NULL) {
    UnicodeSPrint (RetVal, Size, Temp, Dev, Dev->Mode->Attribute);
    FreePool (Temp);
  }

  //
  // Dump TextOut Info
  //
  Temp = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_TXT_OUT_DUMP_LINE), NULL);
  for (Index = 0; Index < Dev->Mode->MaxMode; Index++) {
    Status  = Dev->QueryMode (Dev, Index, &Col, &Row);
    NewSize = Size - StrSize (RetVal);
    UnicodeSPrint (
      RetVal + StrLen (RetVal),
      NewSize,
      Temp == NULL ? L"" : Temp,
      Index == Dev->Mode->Mode ? L'*' : L' ',
      Index,
      !EFI_ERROR (Status) ? (INTN)Col : -1,
      !EFI_ERROR (Status) ? (INTN)Row : -1
      );
  }

  FreePool (Temp);
  return (RetVal);
}

STATIC CONST UINTN  VersionStringSize = 60;

/**
  Function to dump information about EfiDriverSupportedEfiVersion protocol.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has the protocol installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A poitner to a string containing the information.
**/
CHAR16 *
EFIAPI
DriverEfiVersionProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL  *DriverEfiVersion;
  EFI_STATUS                                 Status;
  CHAR16                                     *RetVal;

  Status = gBS->HandleProtocol (
                  TheHandle,
                  &gEfiDriverSupportedEfiVersionProtocolGuid,
                  (VOID **)&DriverEfiVersion
                  );

  ASSERT_EFI_ERROR (Status);

  RetVal = AllocateZeroPool (VersionStringSize);
  if (RetVal != NULL) {
    UnicodeSPrint (RetVal, VersionStringSize, L"0x%08x", DriverEfiVersion->FirmwareVersion);
  }

  return (RetVal);
}

/**
  Function to convert device path to string.

  This will allocate the return buffer from boot services pool.

  @param[in] DevPath        Pointer to device path instance.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.
  @param[in] Length         Maximum allowed text length of the device path.

  @retval A pointer to a string containing the information.
**/
CHAR16 *
ConvertDevicePathToShortText (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevPath,
  IN CONST BOOLEAN                   Verbose,
  IN CONST UINTN                     Length
  )
{
  CHAR16  *Temp;
  CHAR16  *Temp2;
  UINTN   Size;

  //
  // I cannot decide whether to allow shortcuts here (the second BOOLEAN on the next line)
  //
  Temp = ConvertDevicePathToText (DevPath, TRUE, TRUE);
  if (!Verbose && (Temp != NULL) && (StrLen (Temp) > Length)) {
    Temp2 = NULL;
    Size  = 0;
    Temp2 = StrnCatGrow (&Temp2, &Size, L"..", 0);
    Temp2 = StrnCatGrow (&Temp2, &Size, Temp+(StrLen (Temp) - (Length - 2)), 0);
    FreePool (Temp);
    Temp = Temp2;
  }

  return (Temp);
}

/**
  Function to dump protocol information.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has the protocol installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.
  @param[in] Protocol       The protocol is needed to dump.

  @retval A pointer to a string containing the information.
**/
STATIC CHAR16 *
EFIAPI
DevicePathProtocolDumpInformationEx (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose,
  IN       EFI_GUID    *Protocol
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  CHAR16                    *DevPathStr;
  CHAR16                    *DevPathStrTemp;
  UINTN                     Size;
  EFI_STATUS                Status;

  DevPathStr     = NULL;
  DevPathStrTemp = NULL;
  Status         = gBS->OpenProtocol (TheHandle, Protocol, (VOID **)&DevPath, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (!EFI_ERROR (Status)) {
    DevPathStr = ConvertDevicePathToShortText (DevPath, Verbose, 30);
    if (Verbose) {
      Size           = StrSize (DevPathStr) + sizeof (CHAR16) * 2;
      DevPathStrTemp = AllocateZeroPool (Size);
      if (DevPathStrTemp != NULL) {
        StrnCatS (DevPathStrTemp, Size/sizeof (CHAR16), L"  ", 2);
        StrnCatS (DevPathStrTemp, Size/sizeof (CHAR16), DevPathStr, StrLen (DevPathStr));
      }

      FreePool (DevPathStr);
      DevPathStr = DevPathStrTemp;
    }

    gBS->CloseProtocol (TheHandle, Protocol, gImageHandle, NULL);
  }

  return DevPathStr;
}

/**
  Function to dump information about DevicePath protocol.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has the protocol installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A pointer to a string containing the information.
**/
CHAR16 *
EFIAPI
DevicePathProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  return DevicePathProtocolDumpInformationEx (TheHandle, Verbose, &gEfiDevicePathProtocolGuid);
}

/**
  Function to dump information about LoadedImageDevicePath protocol.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has the protocol installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A pointer to a string containing the information.
**/
CHAR16 *
EFIAPI
LoadedImageDevicePathProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  return DevicePathProtocolDumpInformationEx (TheHandle, Verbose, &gEfiLoadedImageDevicePathProtocolGuid);
}

/**
  Function to dump information about BusSpecificDriverOverride protocol.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has the protocol installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A pointer to a string containing the information.
**/
CHAR16 *
EFIAPI
BusSpecificDriverOverrideProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_STATUS                                 Status;
  CHAR16                                     *GetString;
  CHAR16                                     *RetVal;
  CHAR16                                     *TempRetVal;
  EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL  *BusSpecificDriverOverride;
  EFI_LOADED_IMAGE_PROTOCOL                  *LoadedImage;
  EFI_HANDLE                                 ImageHandle;
  UINTN                                      Size;

  if (!Verbose) {
    return NULL;
  }

  Size        = 0;
  GetString   = NULL;
  RetVal      = NULL;
  TempRetVal  = NULL;
  ImageHandle = 0;

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiBusSpecificDriverOverrideProtocolGuid,
                  (VOID **)&BusSpecificDriverOverride,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  HandleParsingHiiInit ();
  GetString = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_BSDO_DUMP_MAIN), NULL);
  if (GetString == NULL) {
    return NULL;
  }

  do {
    Status = BusSpecificDriverOverride->GetDriver (
                                          BusSpecificDriverOverride,
                                          &ImageHandle
                                          );
    if (!EFI_ERROR (Status)) {
      Status = gBS->HandleProtocol (
                      ImageHandle,
                      &gEfiLoadedImageProtocolGuid,
                      (VOID **)&LoadedImage
                      );
      if (!EFI_ERROR (Status)) {
        TempRetVal = CatSPrint (
                       TempRetVal,
                       GetString,
                       ConvertHandleToHandleIndex (ImageHandle),
                       ConvertDevicePathToText (LoadedImage->FilePath, TRUE, TRUE)
                       );
        StrnCatGrow (&RetVal, &Size, TempRetVal, 0);
        SHELL_FREE_NON_NULL (TempRetVal);
      }
    }
  } while (!EFI_ERROR (Status));

  SHELL_FREE_NON_NULL (GetString);
  return RetVal;
}

/**
  Function to dump information about BlockIo protocol.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has the protocol installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A pointer to a string containing the information.
**/
CHAR16 *
EFIAPI
BlockIoProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_STATUS             Status;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;
  EFI_BLOCK_IO_MEDIA     *BlockMedia;
  CHAR16                 *GetString;
  CHAR16                 *RetVal;

  if (!Verbose) {
    return NULL;
  }

  GetString = NULL;
  RetVal    = NULL;

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlockIo,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  BlockMedia = BlockIo->Media;
  //
  // Per spec:
  //   The function (ReadBlocks) must return EFI_NO_MEDIA or
  //   EFI_MEDIA_CHANGED even if LBA, BufferSize, or Buffer are invalid so the caller can probe
  //   for changes in media state.
  //
  BlockIo->ReadBlocks (
             BlockIo,
             BlockIo->Media->MediaId,
             0,
             0,
             NULL
             );

  HandleParsingHiiInit ();
  GetString = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_BLOCKIO_INFO), NULL);
  if (GetString == NULL) {
    return NULL;
  }

  RetVal = CatSPrint (
             RetVal,
             GetString,
             BlockMedia->RemovableMedia ? L"Removable " : L"Fixed ",
             BlockMedia->MediaPresent ? L"" : L"not-present ",
             BlockMedia->MediaId,
             BlockMedia->BlockSize,
             BlockMedia->LastBlock,
             MultU64x32 (BlockMedia->LastBlock + 1, BlockMedia->BlockSize),
             BlockMedia->LogicalPartition ? L"partition" : L"raw",
             BlockMedia->ReadOnly ? L"ro" : L"rw",
             BlockMedia->WriteCaching ? L"cached" : L"!cached"
             );

  SHELL_FREE_NON_NULL (GetString);
  return RetVal;
}

/**
  Function to dump information about DebugSupport Protocol.

  @param[in] TheHandle      The handle that has the protocol installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A pointer to a string containing the information.
**/
CHAR16 *
EFIAPI
DebugSupportProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_STATUS                  Status;
  EFI_DEBUG_SUPPORT_PROTOCOL  *DebugSupport;
  CHAR16                      *GetString;
  CHAR16                      *RetVal;

  if (!Verbose) {
    return NULL;
  }

  GetString = NULL;
  RetVal    = NULL;
  Status    = gBS->OpenProtocol (
                     TheHandle,
                     &gEfiDebugSupportProtocolGuid,
                     (VOID **)&DebugSupport,
                     gImageHandle,
                     NULL,
                     EFI_OPEN_PROTOCOL_GET_PROTOCOL
                     );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  HandleParsingHiiInit ();
  GetString = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_DEBUGSUPPORT_INFO), NULL);
  if (GetString == NULL) {
    return NULL;
  }

  //
  // Dump Debug support info
  //
  switch (DebugSupport->Isa) {
    case (IsaIa32):
      RetVal = CatSPrint (RetVal, GetString, L"IA-32");
      break;
    case (IsaIpf):
      RetVal = CatSPrint (RetVal, GetString, L"IPF");
      break;
    case (IsaEbc):
      RetVal = CatSPrint (RetVal, GetString, L"EBC");
      break;
    default:
      SHELL_FREE_NON_NULL (GetString);
      GetString = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_DEBUGSUPPORT_UNKNOWN), NULL);
      RetVal    = GetString != NULL ? CatSPrint (RetVal, GetString, DebugSupport->Isa) : NULL;
      break;
  }

  SHELL_FREE_NON_NULL (GetString);
  return RetVal;
}

/**
  Function to dump information about PciIoProtocol.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has PciRootBridgeIo installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A poitner to a string containing the information.
**/
CHAR16 *
EFIAPI
PciIoProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           Pci;
  UINTN                Segment;
  UINTN                Bus;
  UINTN                Device;
  UINTN                Function;
  UINTN                Index;
  CHAR16               *GetString;
  CHAR16               *TempRetVal;
  CHAR16               *RetVal;

  if (!Verbose) {
    return (NULL);
  }

  RetVal     = NULL;
  GetString  = NULL;
  TempRetVal = NULL;
  Status     = gBS->OpenProtocol (
                      TheHandle,
                      &gEfiPciIoProtocolGuid,
                      (VOID **)&PciIo,
                      gImageHandle,
                      NULL,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0, sizeof (Pci), &Pci);
  PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);
  HandleParsingHiiInit ();
  GetString = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PCIIO_DUMP_MAIN), NULL);
  if (GetString == NULL) {
    return NULL;
  }

  RetVal = CatSPrint (
             NULL,
             GetString,
             Segment,
             Bus,
             Device,
             Function,
             PciIo->RomSize,
             PciIo->RomImage,
             Pci.Hdr.VendorId,
             Pci.Hdr.DeviceId,
             Pci.Hdr.ClassCode[0],
             Pci.Hdr.ClassCode[1],
             Pci.Hdr.ClassCode[2]
             );
  for (Index = 0; Index < sizeof (Pci); Index++) {
    if ((Index % 0x10) == 0) {
      TempRetVal = CatSPrint (RetVal, L"\r\n       %02x", *((UINT8 *)(&Pci) + Index));
    } else {
      TempRetVal = CatSPrint (RetVal, L"%02x", *((UINT8 *)(&Pci) + Index));
    }

    FreePool (RetVal);
    RetVal     = TempRetVal;
    TempRetVal = NULL;
  }

  FreePool (GetString);
  return RetVal;
}

/**
  Function to dump information about UsbIoProtocol.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has PciRootBridgeIo installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A poitner to a string containing the information.
**/
CHAR16 *
EFIAPI
UsbIoProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_STATUS                    Status;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDesc;
  CHAR16                        *GetString;
  CHAR16                        *RetVal;

  if (!Verbose) {
    return (NULL);
  }

  RetVal    = NULL;
  GetString = NULL;
  Status    = gBS->OpenProtocol (
                     TheHandle,
                     &gEfiUsbIoProtocolGuid,
                     (VOID **)&UsbIo,
                     gImageHandle,
                     NULL,
                     EFI_OPEN_PROTOCOL_GET_PROTOCOL
                     );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  UsbIo->UsbGetInterfaceDescriptor (UsbIo, &InterfaceDesc);
  HandleParsingHiiInit ();
  GetString = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_USBIO_DUMP_MAIN), NULL);
  if (GetString == NULL) {
    return NULL;
  }

  RetVal = CatSPrint (
             NULL,
             GetString,
             InterfaceDesc.InterfaceNumber,
             InterfaceDesc.InterfaceClass,
             InterfaceDesc.InterfaceSubClass,
             InterfaceDesc.InterfaceProtocol
             );

  FreePool (GetString);
  return RetVal;
}

/**
  Function to dump information about EfiAdapterInformation Protocol.

  @param[in] TheHandle      The handle that has the protocol installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A pointer to a string containing the information.
**/
CHAR16 *
EFIAPI
AdapterInformationDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_STATUS                        Status;
  EFI_ADAPTER_INFORMATION_PROTOCOL  *EfiAdptrInfoProtocol;
  UINTN                             InfoTypesBufferCount;
  UINTN                             GuidIndex;
  EFI_GUID                          *InfoTypesBuffer;
  CHAR16                            *GuidStr;
  CHAR16                            *TempStr;
  CHAR16                            *RetVal;
  CHAR16                            *TempRetVal;
  VOID                              *InformationBlock;
  UINTN                             InformationBlockSize;

  if (!Verbose) {
    return (CatSPrint (NULL, L"AdapterInfo"));
  }

  InfoTypesBuffer  = NULL;
  InformationBlock = NULL;

  Status = gBS->OpenProtocol (
                  (EFI_HANDLE)(TheHandle),
                  &gEfiAdapterInformationProtocolGuid,
                  (VOID **)&EfiAdptrInfoProtocol,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Get a list of supported information types for this instance of the protocol.
  //
  Status = EfiAdptrInfoProtocol->GetSupportedTypes (
                                   EfiAdptrInfoProtocol,
                                   &InfoTypesBuffer,
                                   &InfoTypesBufferCount
                                   );
  RetVal = NULL;
  if (EFI_ERROR (Status)) {
    TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_GET_SUPP_TYPES_FAILED), NULL);
    if (TempStr != NULL) {
      RetVal = CatSPrint (NULL, TempStr, Status);
    } else {
      goto ERROR_EXIT;
    }
  } else {
    TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_SUPP_TYPE_HEADER), NULL);
    if (TempStr == NULL) {
      goto ERROR_EXIT;
    }

    RetVal = CatSPrint (NULL, TempStr);
    SHELL_FREE_NON_NULL (TempStr);

    for (GuidIndex = 0; GuidIndex < InfoTypesBufferCount; GuidIndex++) {
      TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_GUID_NUMBER), NULL);
      if (TempStr == NULL) {
        goto ERROR_EXIT;
      }

      TempRetVal = CatSPrint (RetVal, TempStr, (GuidIndex + 1), &InfoTypesBuffer[GuidIndex]);
      SHELL_FREE_NON_NULL (RetVal);
      RetVal = TempRetVal;
      SHELL_FREE_NON_NULL (TempStr);

      TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_GUID_STRING), NULL);
      if (TempStr == NULL) {
        goto ERROR_EXIT;
      }

      if (CompareGuid (&InfoTypesBuffer[GuidIndex], &gEfiAdapterInfoMediaStateGuid)) {
        TempRetVal = CatSPrint (RetVal, TempStr, L"gEfiAdapterInfoMediaStateGuid");
        SHELL_FREE_NON_NULL (RetVal);
        RetVal = TempRetVal;
      } else if (CompareGuid (&InfoTypesBuffer[GuidIndex], &gEfiAdapterInfoNetworkBootGuid)) {
        TempRetVal = CatSPrint (RetVal, TempStr, L"gEfiAdapterInfoNetworkBootGuid");
        SHELL_FREE_NON_NULL (RetVal);
        RetVal = TempRetVal;
      } else if (CompareGuid (&InfoTypesBuffer[GuidIndex], &gEfiAdapterInfoSanMacAddressGuid)) {
        TempRetVal = CatSPrint (RetVal, TempStr, L"gEfiAdapterInfoSanMacAddressGuid");
        SHELL_FREE_NON_NULL (RetVal);
        RetVal = TempRetVal;
      } else if (CompareGuid (&InfoTypesBuffer[GuidIndex], &gEfiAdapterInfoUndiIpv6SupportGuid)) {
        TempRetVal = CatSPrint (RetVal, TempStr, L"gEfiAdapterInfoUndiIpv6SupportGuid");
        SHELL_FREE_NON_NULL (RetVal);
        RetVal = TempRetVal;
      } else {
        GuidStr = GetStringNameFromGuid (&InfoTypesBuffer[GuidIndex], NULL);
        if (GuidStr == NULL) {
          TempRetVal = CatSPrint (RetVal, TempStr, L"UnknownInfoType");
          SHELL_FREE_NON_NULL (RetVal);
          RetVal = TempRetVal;

          SHELL_FREE_NON_NULL (TempStr);
          SHELL_FREE_NON_NULL (GuidStr);
          //
          // So that we never have to pass this UnknownInfoType to the parsing function "GetInformation" service of AIP
          //
          continue;
        } else {
          TempRetVal = CatSPrint (RetVal, TempStr, GuidStr);
          SHELL_FREE_NON_NULL (RetVal);
          RetVal = TempRetVal;
          SHELL_FREE_NON_NULL (GuidStr);
        }
      }

      SHELL_FREE_NON_NULL (TempStr);

      Status = EfiAdptrInfoProtocol->GetInformation (
                                       EfiAdptrInfoProtocol,
                                       &InfoTypesBuffer[GuidIndex],
                                       &InformationBlock,
                                       &InformationBlockSize
                                       );

      if (EFI_ERROR (Status)) {
        TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_GETINFO_FAILED), NULL);
        if (TempStr == NULL) {
          goto ERROR_EXIT;
        }

        TempRetVal = CatSPrint (RetVal, TempStr, Status);
        SHELL_FREE_NON_NULL (RetVal);
        RetVal = TempRetVal;
      } else {
        if (CompareGuid (&InfoTypesBuffer[GuidIndex], &gEfiAdapterInfoMediaStateGuid)) {
          TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_MEDIA_STATE), NULL);
          if (TempStr == NULL) {
            goto ERROR_EXIT;
          }

          TempRetVal = CatSPrint (
                         RetVal,
                         TempStr,
                         ((EFI_ADAPTER_INFO_MEDIA_STATE *)InformationBlock)->MediaState,
                         ((EFI_ADAPTER_INFO_MEDIA_STATE *)InformationBlock)->MediaState
                         );
          SHELL_FREE_NON_NULL (RetVal);
          RetVal = TempRetVal;
        } else if (CompareGuid (&InfoTypesBuffer[GuidIndex], &gEfiAdapterInfoNetworkBootGuid)) {
          TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_NETWORK_BOOT_INFO), NULL);
          if (TempStr == NULL) {
            goto ERROR_EXIT;
          }

          TempRetVal = CatSPrint (
                         RetVal,
                         TempStr,
                         ((EFI_ADAPTER_INFO_NETWORK_BOOT *)InformationBlock)->iScsiIpv4BootCapablity,
                         ((EFI_ADAPTER_INFO_NETWORK_BOOT *)InformationBlock)->iScsiIpv6BootCapablity,
                         ((EFI_ADAPTER_INFO_NETWORK_BOOT *)InformationBlock)->FCoeBootCapablity,
                         ((EFI_ADAPTER_INFO_NETWORK_BOOT *)InformationBlock)->OffloadCapability,
                         ((EFI_ADAPTER_INFO_NETWORK_BOOT *)InformationBlock)->iScsiMpioCapability,
                         ((EFI_ADAPTER_INFO_NETWORK_BOOT *)InformationBlock)->iScsiIpv4Boot,
                         ((EFI_ADAPTER_INFO_NETWORK_BOOT *)InformationBlock)->iScsiIpv6Boot,
                         ((EFI_ADAPTER_INFO_NETWORK_BOOT *)InformationBlock)->FCoeBoot
                         );
          SHELL_FREE_NON_NULL (RetVal);
          RetVal = TempRetVal;
        } else if (CompareGuid (&InfoTypesBuffer[GuidIndex], &gEfiAdapterInfoSanMacAddressGuid) == TRUE) {
          TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_SAN_MAC_ADDRESS_INFO), NULL);
          if (TempStr == NULL) {
            goto ERROR_EXIT;
          }

          TempRetVal = CatSPrint (
                         RetVal,
                         TempStr,
                         ((EFI_ADAPTER_INFO_SAN_MAC_ADDRESS *)InformationBlock)->SanMacAddress.Addr[0],
                         ((EFI_ADAPTER_INFO_SAN_MAC_ADDRESS *)InformationBlock)->SanMacAddress.Addr[1],
                         ((EFI_ADAPTER_INFO_SAN_MAC_ADDRESS *)InformationBlock)->SanMacAddress.Addr[2],
                         ((EFI_ADAPTER_INFO_SAN_MAC_ADDRESS *)InformationBlock)->SanMacAddress.Addr[3],
                         ((EFI_ADAPTER_INFO_SAN_MAC_ADDRESS *)InformationBlock)->SanMacAddress.Addr[4],
                         ((EFI_ADAPTER_INFO_SAN_MAC_ADDRESS *)InformationBlock)->SanMacAddress.Addr[5]
                         );
          SHELL_FREE_NON_NULL (RetVal);
          RetVal = TempRetVal;
        } else if (CompareGuid (&InfoTypesBuffer[GuidIndex], &gEfiAdapterInfoUndiIpv6SupportGuid) == TRUE) {
          TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_UNDI_IPV6_INFO), NULL);
          if (TempStr == NULL) {
            goto ERROR_EXIT;
          }

          TempRetVal = CatSPrint (
                         RetVal,
                         TempStr,
                         ((EFI_ADAPTER_INFO_UNDI_IPV6_SUPPORT *)InformationBlock)->Ipv6Support
                         );
          SHELL_FREE_NON_NULL (RetVal);
          RetVal = TempRetVal;
        } else {
          TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_UNKNOWN_INFO_TYPE), NULL);
          if (TempStr == NULL) {
            goto ERROR_EXIT;
          }

          TempRetVal = CatSPrint (RetVal, TempStr, &InfoTypesBuffer[GuidIndex]);
          SHELL_FREE_NON_NULL (RetVal);
          RetVal = TempRetVal;
        }
      }

      SHELL_FREE_NON_NULL (TempStr);
      SHELL_FREE_NON_NULL (InformationBlock);
    }
  }

  SHELL_FREE_NON_NULL (InfoTypesBuffer);
  return RetVal;

ERROR_EXIT:
  SHELL_FREE_NON_NULL (RetVal);
  SHELL_FREE_NON_NULL (InfoTypesBuffer);
  SHELL_FREE_NON_NULL (InformationBlock);
  return NULL;
}

/**
  Function to dump information about EFI_FIRMWARE_MANAGEMENT_PROTOCOL Protocol.

  @param[in] TheHandle      The handle that has the protocol installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A pointer to a string containing the information.
**/
CHAR16 *
EFIAPI
FirmwareManagementDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_STATUS                        Status;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *EfiFwMgmtProtocol;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR     *ImageInfo;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR_V1  *ImageInfoV1;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR_V2  *ImageInfoV2;
  UINT64                            AttributeSetting;
  UINTN                             ImageInfoSize;
  UINTN                             DescriptorSize;
  UINT32                            DescriptorVersion;
  UINT32                            PackageVersion;
  UINT8                             DescriptorCount;
  UINT8                             Index;
  UINT8                             Index1;
  UINT8                             ImageCount;
  CHAR16                            *PackageVersionName;
  CHAR16                            *TempStr;
  CHAR16                            *RetVal;
  CHAR16                            *TempRetVal;
  CHAR16                            *AttributeSettingStr;
  BOOLEAN                           Found;
  BOOLEAN                           AttributeSupported;

  //
  // Initialize local variables
  //
  ImageCount          = 0;
  ImageInfoSize       = 1;
  AttributeSetting    = 0;
  Found               = FALSE;
  AttributeSupported  = FALSE;
  ImageInfo           = NULL;
  ImageInfoV1         = NULL;
  ImageInfoV2         = NULL;
  PackageVersionName  = NULL;
  RetVal              = NULL;
  TempRetVal          = NULL;
  TempStr             = NULL;
  AttributeSettingStr = NULL;

  if (!Verbose) {
    return (CatSPrint (NULL, L"FirmwareManagement"));
  }

  Status = gBS->OpenProtocol (
                  (EFI_HANDLE)(TheHandle),
                  &gEfiFirmwareManagementProtocolGuid,
                  (VOID **)&EfiFwMgmtProtocol,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = EfiFwMgmtProtocol->GetImageInfo (
                                EfiFwMgmtProtocol,
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
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      Status = EfiFwMgmtProtocol->GetImageInfo (
                                    EfiFwMgmtProtocol,
                                    &ImageInfoSize,
                                    ImageInfo,
                                    &DescriptorVersion,
                                    &DescriptorCount,
                                    &DescriptorSize,
                                    &PackageVersion,
                                    &PackageVersionName
                                    );
    }
  }

  if (EFI_ERROR (Status)) {
    goto ERROR_EXIT;
  }

  //
  // Decode Image Descriptor data only if its version is supported
  //
  if (DescriptorVersion <= EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION) {
    if (ImageInfo == NULL) {
      goto ERROR_EXIT;
    }

    ImageInfoV1 = (EFI_FIRMWARE_IMAGE_DESCRIPTOR_V1 *)ImageInfo;
    ImageInfoV2 = (EFI_FIRMWARE_IMAGE_DESCRIPTOR_V2 *)ImageInfo;

    //
    // Set ImageInfoSize in return buffer
    //
    TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_FMP_IMAGE_INFO_SIZE), NULL);
    if (TempStr == NULL) {
      goto ERROR_EXIT;
    }

    RetVal = CatSPrint (NULL, TempStr, ImageInfoSize);
    SHELL_FREE_NON_NULL (TempStr);

    //
    // Set DescriptorVersion in return buffer
    //
    TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_FMP_DESCRIPTOR_VERSION), NULL);
    if (TempStr == NULL) {
      goto ERROR_EXIT;
    }

    TempRetVal = CatSPrint (RetVal, TempStr, DescriptorVersion);
    SHELL_FREE_NON_NULL (RetVal);
    RetVal = TempRetVal;
    SHELL_FREE_NON_NULL (TempStr);

    //
    // Set DescriptorCount in return buffer
    //
    TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_FMP_DESCRIPTOR_COUNT), NULL);
    if (TempStr == NULL) {
      goto ERROR_EXIT;
    }

    TempRetVal = CatSPrint (RetVal, TempStr, DescriptorCount);
    SHELL_FREE_NON_NULL (RetVal);
    RetVal = TempRetVal;
    SHELL_FREE_NON_NULL (TempStr);

    //
    // Set DescriptorSize in return buffer
    //
    TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_FMP_DESCRIPTOR_SIZE), NULL);
    if (TempStr == NULL) {
      goto ERROR_EXIT;
    }

    TempRetVal = CatSPrint (RetVal, TempStr, DescriptorSize);
    SHELL_FREE_NON_NULL (RetVal);
    RetVal = TempRetVal;
    SHELL_FREE_NON_NULL (TempStr);

    //
    // Set PackageVersion in return buffer
    //
    TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_FMP_PACKAGE_VERSION), NULL);
    if (TempStr == NULL) {
      goto ERROR_EXIT;
    }

    TempRetVal = CatSPrint (RetVal, TempStr, PackageVersion);
    SHELL_FREE_NON_NULL (RetVal);
    RetVal = TempRetVal;
    SHELL_FREE_NON_NULL (TempStr);

    //
    // Set PackageVersionName in return buffer
    //
    TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_FMP_PACKAGE_VERSION_NAME), NULL);
    if (TempStr == NULL) {
      goto ERROR_EXIT;
    }

    TempRetVal = CatSPrint (RetVal, TempStr, PackageVersionName);
    SHELL_FREE_NON_NULL (RetVal);
    RetVal = TempRetVal;
    SHELL_FREE_NON_NULL (TempStr);

    for (Index = 0; Index < DescriptorCount; Index++) {
      //
      // First check if Attribute is supported
      // and generate a string for AttributeSetting field
      //
      SHELL_FREE_NON_NULL (AttributeSettingStr);
      AttributeSupported = FALSE;
      AttributeSetting   = 0;
      if (DescriptorVersion == EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION_V1) {
        if (ImageInfoV1[Index].AttributesSupported != 0x0) {
          AttributeSupported = TRUE;
          AttributeSetting   = ImageInfoV1[Index].AttributesSetting;
        }
      } else if (DescriptorVersion == EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION_V2) {
        if (ImageInfoV2[Index].AttributesSupported != 0x0) {
          AttributeSupported = TRUE;
          AttributeSetting   = ImageInfoV2[Index].AttributesSetting;
        }
      } else {
        if (ImageInfo[Index].AttributesSupported != 0x0) {
          AttributeSupported = TRUE;
          AttributeSetting   = ImageInfo[Index].AttributesSetting;
        }
      }

      if (!AttributeSupported) {
        AttributeSettingStr = CatSPrint (NULL, L"None");
      } else {
        AttributeSettingStr = CatSPrint (NULL, L"(");

        if ((AttributeSetting & IMAGE_ATTRIBUTE_IMAGE_UPDATABLE) != 0x0) {
          TempRetVal = CatSPrint (AttributeSettingStr, L" IMAGE_ATTRIBUTE_IMAGE_UPDATABLE");
          SHELL_FREE_NON_NULL (AttributeSettingStr);
          AttributeSettingStr = TempRetVal;
        }

        if ((AttributeSetting & IMAGE_ATTRIBUTE_RESET_REQUIRED) != 0x0) {
          TempRetVal = CatSPrint (AttributeSettingStr, L" IMAGE_ATTRIBUTE_RESET_REQUIRED");
          SHELL_FREE_NON_NULL (AttributeSettingStr);
          AttributeSettingStr = TempRetVal;
        }

        if ((AttributeSetting & IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED) != 0x0) {
          TempRetVal = CatSPrint (AttributeSettingStr, L" IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED");
          SHELL_FREE_NON_NULL (AttributeSettingStr);
          AttributeSettingStr = TempRetVal;
        }

        if ((AttributeSetting & IMAGE_ATTRIBUTE_IN_USE) != 0x0) {
          TempRetVal = CatSPrint (AttributeSettingStr, L" IMAGE_ATTRIBUTE_IN_USE");
          SHELL_FREE_NON_NULL (AttributeSettingStr);
          AttributeSettingStr = TempRetVal;
        }

        if ((AttributeSetting & IMAGE_ATTRIBUTE_UEFI_IMAGE) != 0x0) {
          TempRetVal = CatSPrint (AttributeSettingStr, L" IMAGE_ATTRIBUTE_UEFI_IMAGE");
          SHELL_FREE_NON_NULL (AttributeSettingStr);
          AttributeSettingStr = TempRetVal;
        }

        TempRetVal = CatSPrint (AttributeSettingStr, L" )");
        SHELL_FREE_NON_NULL (AttributeSettingStr);
        AttributeSettingStr = TempRetVal;
      }

      if (DescriptorVersion == EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION_V1) {
        if (ImageInfoV1[Index].ImageIndex != 0x0) {
          ImageCount++;
        }

        TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_FMP_IMAGE_DESCRIPTOR_INFO_V1), NULL);
        if (TempStr == NULL) {
          goto ERROR_EXIT;
        }

        TempRetVal = CatSPrint (
                       RetVal,
                       TempStr,
                       Index,
                       ImageInfoV1[Index].ImageIndex,
                       &ImageInfoV1[Index].ImageTypeId,
                       ImageInfoV1[Index].ImageId,
                       ImageInfoV1[Index].ImageIdName,
                       ImageInfoV1[Index].Version,
                       ImageInfoV1[Index].VersionName,
                       ImageInfoV1[Index].Size,
                       ImageInfoV1[Index].AttributesSupported,
                       AttributeSettingStr,
                       ImageInfoV1[Index].Compatibilities
                       );
        SHELL_FREE_NON_NULL (RetVal);
        RetVal = TempRetVal;
        SHELL_FREE_NON_NULL (TempStr);
      } else if (DescriptorVersion == EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION_V2) {
        if (ImageInfoV2[Index].ImageIndex != 0x0) {
          ImageCount++;
        }

        TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_FMP_IMAGE_DESCRIPTOR_INFO_V2), NULL);
        if (TempStr == NULL) {
          goto ERROR_EXIT;
        }

        TempRetVal = CatSPrint (
                       RetVal,
                       TempStr,
                       Index,
                       ImageInfoV2[Index].ImageIndex,
                       &ImageInfoV2[Index].ImageTypeId,
                       ImageInfoV2[Index].ImageId,
                       ImageInfoV2[Index].ImageIdName,
                       ImageInfoV2[Index].Version,
                       ImageInfoV2[Index].VersionName,
                       ImageInfoV2[Index].Size,
                       ImageInfoV2[Index].AttributesSupported,
                       AttributeSettingStr,
                       ImageInfoV2[Index].Compatibilities,
                       ImageInfoV2[Index].LowestSupportedImageVersion
                       );
        SHELL_FREE_NON_NULL (RetVal);
        RetVal = TempRetVal;
        SHELL_FREE_NON_NULL (TempStr);
      } else {
        if (ImageInfo[Index].ImageIndex != 0x0) {
          ImageCount++;
        }

        TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_FMP_IMAGE_DESCRIPTOR_INFO), NULL);
        if (TempStr == NULL) {
          goto ERROR_EXIT;
        }

        TempRetVal = CatSPrint (
                       RetVal,
                       TempStr,
                       Index,
                       ImageInfo[Index].ImageIndex,
                       &ImageInfo[Index].ImageTypeId,
                       ImageInfo[Index].ImageId,
                       ImageInfo[Index].ImageIdName,
                       ImageInfo[Index].Version,
                       ImageInfo[Index].VersionName,
                       ImageInfo[Index].Size,
                       ImageInfo[Index].AttributesSupported,
                       AttributeSettingStr,
                       ImageInfo[Index].Compatibilities,
                       ImageInfo[Index].LowestSupportedImageVersion,
                       ImageInfo[Index].LastAttemptVersion,
                       ImageInfo[Index].LastAttemptStatus,
                       ImageInfo[Index].HardwareInstance
                       );
        SHELL_FREE_NON_NULL (RetVal);
        RetVal = TempRetVal;
        SHELL_FREE_NON_NULL (TempStr);
      }
    }
  }

  if (ImageCount > 0) {
    for (Index = 0; Index < DescriptorCount; Index++) {
      for (Index1 = Index+1; Index1 < DescriptorCount; Index1++) {
        if (DescriptorVersion == EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION_V1) {
          if (ImageInfoV1[Index].ImageId == ImageInfoV1[Index1].ImageId) {
            Found = TRUE;
            //
            // At least one match found indicating presense of non unique ImageId values so no more comparisons needed
            //
            goto ENDLOOP;
          }
        } else if (DescriptorVersion == EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION_V2) {
          if (ImageInfoV2[Index].ImageId == ImageInfoV2[Index1].ImageId) {
            Found = TRUE;
            //
            // At least one match found indicating presense of non unique ImageId values so no more comparisons needed
            //
            goto ENDLOOP;
          }
        } else {
          if (ImageInfo[Index].ImageId == ImageInfo[Index1].ImageId) {
            Found = TRUE;
            //
            // At least one match found indicating presense of non unique ImageId values so no more comparisons needed
            //
            goto ENDLOOP;
          }
        }
      }
    }
  }

ENDLOOP:
  //
  // Check if ImageId with duplicate value was found
  //
  if (Found) {
    TempStr = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_FMP_IMAGEID_NON_UNIQUE), NULL);
    if (TempStr == NULL) {
      goto ERROR_EXIT;
    }

    TempRetVal = CatSPrint (RetVal, TempStr);
    SHELL_FREE_NON_NULL (RetVal);
    RetVal = TempRetVal;
    SHELL_FREE_NON_NULL (TempStr);
  }

  SHELL_FREE_NON_NULL (ImageInfo);
  SHELL_FREE_NON_NULL (PackageVersionName);
  SHELL_FREE_NON_NULL (AttributeSettingStr);

  return RetVal;

ERROR_EXIT:
  SHELL_FREE_NON_NULL (RetVal);
  SHELL_FREE_NON_NULL (ImageInfo);
  SHELL_FREE_NON_NULL (PackageVersionName);
  SHELL_FREE_NON_NULL (AttributeSettingStr);

  return NULL;
}

/**
  Function to dump information about Partition Information protocol.

  This will allocate the return buffer from boot services pool.

  @param[in] TheHandle      The handle that has the protocol installed.
  @param[in] Verbose        TRUE for additional information, FALSE otherwise.

  @retval A pointer to a string containing the information.
**/
CHAR16 *
EFIAPI
PartitionInfoProtocolDumpInformation (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST BOOLEAN     Verbose
  )
{
  EFI_STATUS                   Status;
  EFI_PARTITION_INFO_PROTOCOL  *PartitionInfo;
  CHAR16                       *PartitionType;
  CHAR16                       *EfiSystemPartition;
  CHAR16                       *RetVal;

  if (!Verbose) {
    return NULL;
  }

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiPartitionInfoProtocolGuid,
                  (VOID **)&PartitionInfo,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  HandleParsingHiiInit ();

  switch (PartitionInfo->Type) {
    case PARTITION_TYPE_OTHER:
      PartitionType = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PARTINFO_DUMP_TYPE_OTHER), NULL);
      break;
    case PARTITION_TYPE_MBR:
      PartitionType = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PARTINFO_DUMP_TYPE_MBR), NULL);
      break;
    case PARTITION_TYPE_GPT:
      PartitionType = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PARTINFO_DUMP_TYPE_GPT), NULL);
      break;
    default:
      PartitionType = NULL;
      break;
  }

  if (PartitionType == NULL) {
    return NULL;
  }

  if (PartitionInfo->System == 1) {
    EfiSystemPartition = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PARTINFO_DUMP_EFI_SYS_PART), NULL);
  } else {
    EfiSystemPartition = HiiGetString (mHandleParsingHiiHandle, STRING_TOKEN (STR_PARTINFO_DUMP_NOT_EFI_SYS_PART), NULL);
  }

  if (EfiSystemPartition == NULL) {
    SHELL_FREE_NON_NULL (PartitionType);
    return NULL;
  }

  RetVal = CatSPrint (
             NULL,
             L"%s\r\n%s",
             PartitionType,
             EfiSystemPartition
             );

  SHELL_FREE_NON_NULL (EfiSystemPartition);
  SHELL_FREE_NON_NULL (PartitionType);
  return RetVal;
}

//
// Put the information on the NT32 protocol GUIDs here so we are not dependant on the Nt32Pkg
//
#define LOCAL_EFI_WIN_NT_THUNK_PROTOCOL_GUID \
  { \
    0x58c518b1, 0x76f3, 0x11d4, { 0xbc, 0xea, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define LOCAL_EFI_WIN_NT_BUS_DRIVER_IO_PROTOCOL_GUID \
  { \
    0x96eb4ad6, 0xa32a, 0x11d4, { 0xbc, 0xfd, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define LOCAL_EFI_WIN_NT_SERIAL_PORT_GUID \
  { \
    0xc95a93d, 0xa006, 0x11d4, { 0xbc, 0xfa, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }
STATIC CONST EFI_GUID  WinNtThunkProtocolGuid = LOCAL_EFI_WIN_NT_THUNK_PROTOCOL_GUID;
STATIC CONST EFI_GUID  WinNtIoProtocolGuid    = LOCAL_EFI_WIN_NT_BUS_DRIVER_IO_PROTOCOL_GUID;
STATIC CONST EFI_GUID  WinNtSerialPortGuid    = LOCAL_EFI_WIN_NT_SERIAL_PORT_GUID;

//
// Deprecated protocols we dont want to link from IntelFrameworkModulePkg
//
#define LOCAL_EFI_ISA_IO_PROTOCOL_GUID \
  { \
  0x7ee2bd44, 0x3da0, 0x11d4, { 0x9a, 0x38, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }
#define LOCAL_EFI_ISA_ACPI_PROTOCOL_GUID \
  { \
  0x64a892dc, 0x5561, 0x4536, { 0x92, 0xc7, 0x79, 0x9b, 0xfc, 0x18, 0x33, 0x55 } \
  }
STATIC CONST EFI_GUID  EfiIsaIoProtocolGuid   = LOCAL_EFI_ISA_IO_PROTOCOL_GUID;
STATIC CONST EFI_GUID  EfiIsaAcpiProtocolGuid = LOCAL_EFI_ISA_ACPI_PROTOCOL_GUID;

STATIC CONST GUID_INFO_BLOCK  mGuidStringListNT[] = {
  { STRING_TOKEN (STR_WINNT_THUNK),       (EFI_GUID *)&WinNtThunkProtocolGuid, NULL },
  { STRING_TOKEN (STR_WINNT_DRIVER_IO),   (EFI_GUID *)&WinNtIoProtocolGuid,    NULL },
  { STRING_TOKEN (STR_WINNT_SERIAL_PORT), (EFI_GUID *)&WinNtSerialPortGuid,    NULL },
  { 0,                                    NULL,                                NULL },
};

STATIC CONST GUID_INFO_BLOCK  mGuidStringList[] = {
  { STRING_TOKEN (STR_LOADED_IMAGE),        &gEfiLoadedImageProtocolGuid,                      LoadedImageProtocolDumpInformation               },
  { STRING_TOKEN (STR_DEVICE_PATH),         &gEfiDevicePathProtocolGuid,                       DevicePathProtocolDumpInformation                },
  { STRING_TOKEN (STR_IMAGE_PATH),          &gEfiLoadedImageDevicePathProtocolGuid,            LoadedImageDevicePathProtocolDumpInformation     },
  { STRING_TOKEN (STR_DEVICE_PATH_UTIL),    &gEfiDevicePathUtilitiesProtocolGuid,              NULL                                             },
  { STRING_TOKEN (STR_DEVICE_PATH_TXT),     &gEfiDevicePathToTextProtocolGuid,                 NULL                                             },
  { STRING_TOKEN (STR_DEVICE_PATH_FTXT),    &gEfiDevicePathFromTextProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_DEVICE_PATH_PC),      &gEfiPcAnsiGuid,                                   NULL                                             },
  { STRING_TOKEN (STR_DEVICE_PATH_VT100),   &gEfiVT100Guid,                                    NULL                                             },
  { STRING_TOKEN (STR_DEVICE_PATH_VT100P),  &gEfiVT100PlusGuid,                                NULL                                             },
  { STRING_TOKEN (STR_DEVICE_PATH_VTUTF8),  &gEfiVTUTF8Guid,                                   NULL                                             },
  { STRING_TOKEN (STR_DRIVER_BINDING),      &gEfiDriverBindingProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_PLATFORM_OVERRIDE),   &gEfiPlatformDriverOverrideProtocolGuid,           NULL                                             },
  { STRING_TOKEN (STR_BUS_OVERRIDE),        &gEfiBusSpecificDriverOverrideProtocolGuid,        BusSpecificDriverOverrideProtocolDumpInformation },
  { STRING_TOKEN (STR_DRIVER_DIAG),         &gEfiDriverDiagnosticsProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_DRIVER_DIAG2),        &gEfiDriverDiagnostics2ProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_DRIVER_CN),           &gEfiComponentNameProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_DRIVER_CN2),          &gEfiComponentName2ProtocolGuid,                   NULL                                             },
  { STRING_TOKEN (STR_PLAT_DRV_CFG),        &gEfiPlatformToDriverConfigurationProtocolGuid,    NULL                                             },
  { STRING_TOKEN (STR_DRIVER_VERSION),      &gEfiDriverSupportedEfiVersionProtocolGuid,        DriverEfiVersionProtocolDumpInformation          },
  { STRING_TOKEN (STR_TXT_IN),              &gEfiSimpleTextInProtocolGuid,                     NULL                                             },
  { STRING_TOKEN (STR_TXT_IN_EX),           &gEfiSimpleTextInputExProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_TXT_OUT),             &gEfiSimpleTextOutProtocolGuid,                    TxtOutProtocolDumpInformation                    },
  { STRING_TOKEN (STR_SIM_POINTER),         &gEfiSimplePointerProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_ABS_POINTER),         &gEfiAbsolutePointerProtocolGuid,                  NULL                                             },
  { STRING_TOKEN (STR_SERIAL_IO),           &gEfiSerialIoProtocolGuid,                         NULL                                             },
  { STRING_TOKEN (STR_GRAPHICS_OUTPUT),     &gEfiGraphicsOutputProtocolGuid,                   GraphicsOutputProtocolDumpInformation            },
  { STRING_TOKEN (STR_EDID_DISCOVERED),     &gEfiEdidDiscoveredProtocolGuid,                   EdidDiscoveredProtocolDumpInformation            },
  { STRING_TOKEN (STR_EDID_ACTIVE),         &gEfiEdidActiveProtocolGuid,                       EdidActiveProtocolDumpInformation                },
  { STRING_TOKEN (STR_EDID_OVERRIDE),       &gEfiEdidOverrideProtocolGuid,                     NULL                                             },
  { STRING_TOKEN (STR_CON_IN),              &gEfiConsoleInDeviceGuid,                          NULL                                             },
  { STRING_TOKEN (STR_CON_OUT),             &gEfiConsoleOutDeviceGuid,                         NULL                                             },
  { STRING_TOKEN (STR_STD_ERR),             &gEfiStandardErrorDeviceGuid,                      NULL                                             },
  { STRING_TOKEN (STR_LOAD_FILE),           &gEfiLoadFileProtocolGuid,                         NULL                                             },
  { STRING_TOKEN (STR_LOAD_FILE2),          &gEfiLoadFile2ProtocolGuid,                        NULL                                             },
  { STRING_TOKEN (STR_SIMPLE_FILE_SYS),     &gEfiSimpleFileSystemProtocolGuid,                 NULL                                             },
  { STRING_TOKEN (STR_TAPE_IO),             &gEfiTapeIoProtocolGuid,                           NULL                                             },
  { STRING_TOKEN (STR_DISK_IO),             &gEfiDiskIoProtocolGuid,                           NULL                                             },
  { STRING_TOKEN (STR_BLK_IO),              &gEfiBlockIoProtocolGuid,                          BlockIoProtocolDumpInformation                   },
  { STRING_TOKEN (STR_UC),                  &gEfiUnicodeCollationProtocolGuid,                 NULL                                             },
  { STRING_TOKEN (STR_UC2),                 &gEfiUnicodeCollation2ProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_PCIRB_IO),            &gEfiPciRootBridgeIoProtocolGuid,                  PciRootBridgeIoDumpInformation                   },
  { STRING_TOKEN (STR_PCI_IO),              &gEfiPciIoProtocolGuid,                            PciIoProtocolDumpInformation                     },
  { STRING_TOKEN (STR_SCSI_PT),             &gEfiScsiPassThruProtocolGuid,                     NULL                                             },
  { STRING_TOKEN (STR_SCSI_IO),             &gEfiScsiIoProtocolGuid,                           NULL                                             },
  { STRING_TOKEN (STR_SCSI_PT_EXT),         &gEfiExtScsiPassThruProtocolGuid,                  NULL                                             },
  { STRING_TOKEN (STR_ISCSI),               &gEfiIScsiInitiatorNameProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_USB_IO),              &gEfiUsbIoProtocolGuid,                            UsbIoProtocolDumpInformation                     },
  { STRING_TOKEN (STR_USB_HC),              &gEfiUsbHcProtocolGuid,                            NULL                                             },
  { STRING_TOKEN (STR_USB_HC2),             &gEfiUsb2HcProtocolGuid,                           NULL                                             },
  { STRING_TOKEN (STR_DEBUG_SUPPORT),       &gEfiDebugSupportProtocolGuid,                     DebugSupportProtocolDumpInformation              },
  { STRING_TOKEN (STR_DEBUG_PORT),          &gEfiDebugPortProtocolGuid,                        NULL                                             },
  { STRING_TOKEN (STR_DECOMPRESS),          &gEfiDecompressProtocolGuid,                       NULL                                             },
  { STRING_TOKEN (STR_ACPI_TABLE),          &gEfiAcpiTableProtocolGuid,                        NULL                                             },
  { STRING_TOKEN (STR_EBC_INTERPRETER),     &gEfiEbcProtocolGuid,                              NULL                                             },
  { STRING_TOKEN (STR_SNP),                 &gEfiSimpleNetworkProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_NII),                 &gEfiNetworkInterfaceIdentifierProtocolGuid,       NULL                                             },
  { STRING_TOKEN (STR_NII_31),              &gEfiNetworkInterfaceIdentifierProtocolGuid_31,    NULL                                             },
  { STRING_TOKEN (STR_PXE_BC),              &gEfiPxeBaseCodeProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_PXE_CB),              &gEfiPxeBaseCodeCallbackProtocolGuid,              NULL                                             },
  { STRING_TOKEN (STR_BIS),                 &gEfiBisProtocolGuid,                              NULL                                             },
  { STRING_TOKEN (STR_MNP_SB),              &gEfiManagedNetworkServiceBindingProtocolGuid,     NULL                                             },
  { STRING_TOKEN (STR_MNP),                 &gEfiManagedNetworkProtocolGuid,                   NULL                                             },
  { STRING_TOKEN (STR_ARP_SB),              &gEfiArpServiceBindingProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_ARP),                 &gEfiArpProtocolGuid,                              NULL                                             },
  { STRING_TOKEN (STR_DHCPV4_SB),           &gEfiDhcp4ServiceBindingProtocolGuid,              NULL                                             },
  { STRING_TOKEN (STR_DHCPV4),              &gEfiDhcp4ProtocolGuid,                            NULL                                             },
  { STRING_TOKEN (STR_TCPV4_SB),            &gEfiTcp4ServiceBindingProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_TCPV4),               &gEfiTcp4ProtocolGuid,                             NULL                                             },
  { STRING_TOKEN (STR_IPV4_SB),             &gEfiIp4ServiceBindingProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_IPV4),                &gEfiIp4ProtocolGuid,                              NULL                                             },
  { STRING_TOKEN (STR_IPV4_CFG),            &gEfiIp4ConfigProtocolGuid,                        NULL                                             },
  { STRING_TOKEN (STR_IPV4_CFG2),           &gEfiIp4Config2ProtocolGuid,                       NULL                                             },
  { STRING_TOKEN (STR_UDPV4_SB),            &gEfiUdp4ServiceBindingProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_UDPV4),               &gEfiUdp4ProtocolGuid,                             NULL                                             },
  { STRING_TOKEN (STR_MTFTPV4_SB),          &gEfiMtftp4ServiceBindingProtocolGuid,             NULL                                             },
  { STRING_TOKEN (STR_MTFTPV4),             &gEfiMtftp4ProtocolGuid,                           NULL                                             },
  { STRING_TOKEN (STR_AUTH_INFO),           &gEfiAuthenticationInfoProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_HASH_SB),             &gEfiHashServiceBindingProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_HASH),                &gEfiHashProtocolGuid,                             NULL                                             },
  { STRING_TOKEN (STR_HII_FONT),            &gEfiHiiFontProtocolGuid,                          NULL                                             },
  { STRING_TOKEN (STR_HII_STRING),          &gEfiHiiStringProtocolGuid,                        NULL                                             },
  { STRING_TOKEN (STR_HII_IMAGE),           &gEfiHiiImageProtocolGuid,                         NULL                                             },
  { STRING_TOKEN (STR_HII_DATABASE),        &gEfiHiiDatabaseProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_HII_CONFIG_ROUT),     &gEfiHiiConfigRoutingProtocolGuid,                 NULL                                             },
  { STRING_TOKEN (STR_HII_CONFIG_ACC),      &gEfiHiiConfigAccessProtocolGuid,                  NULL                                             },
  { STRING_TOKEN (STR_HII_FORM_BROWSER2),   &gEfiFormBrowser2ProtocolGuid,                     NULL                                             },
  { STRING_TOKEN (STR_DRIVER_FAM_OVERRIDE), &gEfiDriverFamilyOverrideProtocolGuid,             NULL                                             },
  { STRING_TOKEN (STR_PCD),                 &gPcdProtocolGuid,                                 NULL                                             },
  { STRING_TOKEN (STR_TCG),                 &gEfiTcgProtocolGuid,                              NULL                                             },
  { STRING_TOKEN (STR_HII_PACKAGE_LIST),    &gEfiHiiPackageListProtocolGuid,                   NULL                                             },

  //
  // the ones under this are deprecated by the current UEFI Spec, but may be found anyways...
  //
  { STRING_TOKEN (STR_SHELL_INTERFACE),     &gEfiShellInterfaceGuid,                           NULL                                             },
  { STRING_TOKEN (STR_SHELL_ENV2),          &gEfiShellEnvironment2Guid,                        NULL                                             },
  { STRING_TOKEN (STR_SHELL_ENV),           &gEfiShellEnvironment2Guid,                        NULL                                             },
  { STRING_TOKEN (STR_DEVICE_IO),           &gEfiDeviceIoProtocolGuid,                         NULL                                             },
  { STRING_TOKEN (STR_UGA_DRAW),            &gEfiUgaDrawProtocolGuid,                          NULL                                             },
  { STRING_TOKEN (STR_UGA_IO),              &gEfiUgaIoProtocolGuid,                            NULL                                             },
  { STRING_TOKEN (STR_ESP),                 &gEfiPartTypeSystemPartGuid,                       NULL                                             },
  { STRING_TOKEN (STR_GPT_NBR),             &gEfiPartTypeLegacyMbrGuid,                        NULL                                             },
  { STRING_TOKEN (STR_DRIVER_CONFIG),       &gEfiDriverConfigurationProtocolGuid,              NULL                                             },
  { STRING_TOKEN (STR_DRIVER_CONFIG2),      &gEfiDriverConfiguration2ProtocolGuid,             NULL                                             },

  //
  // these are using local (non-global) definitions to reduce package dependancy.
  //
  { STRING_TOKEN (STR_ISA_IO),              (EFI_GUID *)&EfiIsaIoProtocolGuid,                 NULL                                             },
  { STRING_TOKEN (STR_ISA_ACPI),            (EFI_GUID *)&EfiIsaAcpiProtocolGuid,               NULL                                             },

  //
  // the ones under this are GUID identified structs, not protocols
  //
  { STRING_TOKEN (STR_FILE_INFO),           &gEfiFileInfoGuid,                                 NULL                                             },
  { STRING_TOKEN (STR_FILE_SYS_INFO),       &gEfiFileSystemInfoGuid,                           NULL                                             },

  //
  // the ones under this are misc GUIDS.
  //
  { STRING_TOKEN (STR_EFI_GLOBAL_VARIABLE), &gEfiGlobalVariableGuid,                           NULL                                             },

  //
  // UEFI 2.2
  //
  { STRING_TOKEN (STR_IP6_SB),              &gEfiIp6ServiceBindingProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_IP6),                 &gEfiIp6ProtocolGuid,                              NULL                                             },
  { STRING_TOKEN (STR_IP6_CONFIG),          &gEfiIp6ConfigProtocolGuid,                        NULL                                             },
  { STRING_TOKEN (STR_MTFTP6_SB),           &gEfiMtftp6ServiceBindingProtocolGuid,             NULL                                             },
  { STRING_TOKEN (STR_MTFTP6),              &gEfiMtftp6ProtocolGuid,                           NULL                                             },
  { STRING_TOKEN (STR_DHCP6_SB),            &gEfiDhcp6ServiceBindingProtocolGuid,              NULL                                             },
  { STRING_TOKEN (STR_DHCP6),               &gEfiDhcp6ProtocolGuid,                            NULL                                             },
  { STRING_TOKEN (STR_UDP6_SB),             &gEfiUdp6ServiceBindingProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_UDP6),                &gEfiUdp6ProtocolGuid,                             NULL                                             },
  { STRING_TOKEN (STR_TCP6_SB),             &gEfiTcp6ServiceBindingProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_TCP6),                &gEfiTcp6ProtocolGuid,                             NULL                                             },
  { STRING_TOKEN (STR_VLAN_CONFIG),         &gEfiVlanConfigProtocolGuid,                       NULL                                             },
  { STRING_TOKEN (STR_EAP),                 &gEfiEapProtocolGuid,                              NULL                                             },
  { STRING_TOKEN (STR_EAP_MGMT),            &gEfiEapManagementProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_FTP4_SB),             &gEfiFtp4ServiceBindingProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_FTP4),                &gEfiFtp4ProtocolGuid,                             NULL                                             },
  { STRING_TOKEN (STR_IP_SEC_CONFIG),       &gEfiIpSecConfigProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_DH),                  &gEfiDriverHealthProtocolGuid,                     NULL                                             },
  { STRING_TOKEN (STR_DEF_IMG_LOAD),        &gEfiDeferredImageLoadProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_USER_CRED),           &gEfiUserCredentialProtocolGuid,                   NULL                                             },
  { STRING_TOKEN (STR_USER_MNGR),           &gEfiUserManagerProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_ATA_PASS_THRU),       &gEfiAtaPassThruProtocolGuid,                      NULL                                             },

  //
  // UEFI 2.3
  //
  { STRING_TOKEN (STR_FW_MGMT),             &gEfiFirmwareManagementProtocolGuid,               FirmwareManagementDumpInformation                },
  { STRING_TOKEN (STR_IP_SEC),              &gEfiIpSecProtocolGuid,                            NULL                                             },
  { STRING_TOKEN (STR_IP_SEC2),             &gEfiIpSec2ProtocolGuid,                           NULL                                             },

  //
  // UEFI 2.3.1
  //
  { STRING_TOKEN (STR_KMS),                 &gEfiKmsProtocolGuid,                              NULL                                             },
  { STRING_TOKEN (STR_BLK_IO2),             &gEfiBlockIo2ProtocolGuid,                         NULL                                             },
  { STRING_TOKEN (STR_SSC),                 &gEfiStorageSecurityCommandProtocolGuid,           NULL                                             },
  { STRING_TOKEN (STR_UCRED2),              &gEfiUserCredential2ProtocolGuid,                  NULL                                             },

  //
  // UEFI 2.4
  //
  { STRING_TOKEN (STR_DISK_IO2),            &gEfiDiskIo2ProtocolGuid,                          NULL                                             },
  { STRING_TOKEN (STR_ADAPTER_INFO),        &gEfiAdapterInformationProtocolGuid,               AdapterInformationDumpInformation                },

  //
  // UEFI2.5
  //
  { STRING_TOKEN (STR_TLS_SB),              &gEfiTlsServiceBindingProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_TLS),                 &gEfiTlsProtocolGuid,                              NULL                                             },
  { STRING_TOKEN (STR_TLS_CONFIG),          &gEfiTlsConfigurationProtocolGuid,                 NULL                                             },
  { STRING_TOKEN (STR_SUPPLICANT_SB),       &gEfiSupplicantServiceBindingProtocolGuid,         NULL                                             },
  { STRING_TOKEN (STR_SUPPLICANT),          &gEfiSupplicantProtocolGuid,                       NULL                                             },

  //
  // UEFI2.6
  //
  { STRING_TOKEN (STR_WIFI2),               &gEfiWiFi2ProtocolGuid,                            NULL                                             },
  { STRING_TOKEN (STR_RAMDISK),             &gEfiRamDiskProtocolGuid,                          NULL                                             },
  { STRING_TOKEN (STR_HII_ID),              &gEfiHiiImageDecoderProtocolGuid,                  NULL                                             },
  { STRING_TOKEN (STR_HII_IE),              &gEfiHiiImageExProtocolGuid,                       NULL                                             },
  { STRING_TOKEN (STR_SD_MPT),              &gEfiSdMmcPassThruProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_ERASE_BLOCK),         &gEfiEraseBlockProtocolGuid,                       NULL                                             },

  //
  // UEFI2.7
  //
  { STRING_TOKEN (STR_BLUETOOTH_ATTR),      &gEfiBluetoothAttributeProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_BLUETOOTH_ATTR_SB),   &gEfiBluetoothAttributeServiceBindingProtocolGuid, NULL                                             },
  { STRING_TOKEN (STR_BLUETOOTH_LE_CONFIG), &gEfiBluetoothLeConfigProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_UFS_DEV_CONFIG),      &gEfiUfsDeviceConfigProtocolGuid,                  NULL                                             },
  { STRING_TOKEN (STR_HTTP_BOOT_CALL),      &gEfiHttpBootCallbackProtocolGuid,                 NULL                                             },
  { STRING_TOKEN (STR_RESET_NOTI),          &gEfiResetNotificationProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_PARTITION_INFO),      &gEfiPartitionInfoProtocolGuid,                    PartitionInfoProtocolDumpInformation             },
  { STRING_TOKEN (STR_HII_POPUP),           &gEfiHiiPopupProtocolGuid,                         NULL                                             },

  //
  // UEFI 2.8
  //
  { STRING_TOKEN (STR_REST_EX),             &gEfiRestExProtocolGuid,                           NULL                                             },
  { STRING_TOKEN (STR_REDFISH_DISCOVER),    &gEfiRedfishDiscoverProtocolGuid,                  NULL                                             },

  //
  // PI Spec ones
  //
  { STRING_TOKEN (STR_IDE_CONT_INIT),       &gEfiIdeControllerInitProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_DISK_INFO),           &gEfiDiskInfoProtocolGuid,                         NULL                                             },

  //
  // PI Spec 1.0
  //
  { STRING_TOKEN (STR_BDS_ARCH),            &gEfiBdsArchProtocolGuid,                          NULL                                             },
  { STRING_TOKEN (STR_CPU_ARCH),            &gEfiCpuArchProtocolGuid,                          NULL                                             },
  { STRING_TOKEN (STR_MET_ARCH),            &gEfiMetronomeArchProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_MON_ARCH),            &gEfiMonotonicCounterArchProtocolGuid,             NULL                                             },
  { STRING_TOKEN (STR_RTC_ARCH),            &gEfiRealTimeClockArchProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_RESET_ARCH),          &gEfiResetArchProtocolGuid,                        NULL                                             },
  { STRING_TOKEN (STR_RT_ARCH),             &gEfiRuntimeArchProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_SEC_ARCH),            &gEfiSecurityArchProtocolGuid,                     NULL                                             },
  { STRING_TOKEN (STR_TIMER_ARCH),          &gEfiTimerArchProtocolGuid,                        NULL                                             },
  { STRING_TOKEN (STR_VAR_ARCH),            &gEfiVariableWriteArchProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_V_ARCH),              &gEfiVariableArchProtocolGuid,                     NULL                                             },
  { STRING_TOKEN (STR_SECP),                &gEfiSecurityPolicyProtocolGuid,                   NULL                                             },
  { STRING_TOKEN (STR_WDT_ARCH),            &gEfiWatchdogTimerArchProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_SCR),                 &gEfiStatusCodeRuntimeProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_SMB_HC),              &gEfiSmbusHcProtocolGuid,                          NULL                                             },
  { STRING_TOKEN (STR_FV_2),                &gEfiFirmwareVolume2ProtocolGuid,                  NULL                                             },
  { STRING_TOKEN (STR_FV_BLOCK),            &gEfiFirmwareVolumeBlockProtocolGuid,              NULL                                             },
  { STRING_TOKEN (STR_CAP_ARCH),            &gEfiCapsuleArchProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_MP_SERVICE),          &gEfiMpServiceProtocolGuid,                        NULL                                             },
  { STRING_TOKEN (STR_HBRAP),               &gEfiPciHostBridgeResourceAllocationProtocolGuid,  NULL                                             },
  { STRING_TOKEN (STR_PCIP),                &gEfiPciPlatformProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_PCIO),                &gEfiPciOverrideProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_PCIE),                &gEfiPciEnumerationCompleteProtocolGuid,           NULL                                             },
  { STRING_TOKEN (STR_IPCID),               &gEfiIncompatiblePciDeviceSupportProtocolGuid,     NULL                                             },
  { STRING_TOKEN (STR_PCIHPI),              &gEfiPciHotPlugInitProtocolGuid,                   NULL                                             },
  { STRING_TOKEN (STR_PCIHPR),              &gEfiPciHotPlugRequestProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_SMBIOS),              &gEfiSmbiosProtocolGuid,                           NULL                                             },
  { STRING_TOKEN (STR_S3_SAVE),             &gEfiS3SaveStateProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_S3_S_SMM),            &gEfiS3SmmSaveStateProtocolGuid,                   NULL                                             },
  { STRING_TOKEN (STR_RSC),                 &gEfiRscHandlerProtocolGuid,                       NULL                                             },
  { STRING_TOKEN (STR_S_RSC),               &gEfiSmmRscHandlerProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_ACPI_SDT),            &gEfiAcpiSdtProtocolGuid,                          NULL                                             },
  { STRING_TOKEN (STR_SIO),                 &gEfiSioProtocolGuid,                              NULL                                             },
  { STRING_TOKEN (STR_S_CPU2),              &gEfiSmmCpuIo2ProtocolGuid,                        NULL                                             },
  { STRING_TOKEN (STR_S_BASE2),             &gEfiSmmBase2ProtocolGuid,                         NULL                                             },
  { STRING_TOKEN (STR_S_ACC_2),             &gEfiSmmAccess2ProtocolGuid,                       NULL                                             },
  { STRING_TOKEN (STR_S_CON_2),             &gEfiSmmControl2ProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_S_CONFIG),            &gEfiSmmConfigurationProtocolGuid,                 NULL                                             },
  { STRING_TOKEN (STR_S_RTL),               &gEfiSmmReadyToLockProtocolGuid,                   NULL                                             },
  { STRING_TOKEN (STR_DS_RTL),              &gEfiDxeSmmReadyToLockProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_S_COMM),              &gEfiSmmCommunicationProtocolGuid,                 NULL                                             },
  { STRING_TOKEN (STR_S_STAT),              &gEfiSmmStatusCodeProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_S_CPU),               &gEfiSmmCpuProtocolGuid,                           NULL                                             },
  { STRING_TOKEN (STR_S_PCIRBIO),           &gEfiSmmPciRootBridgeIoProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_S_SWD),               &gEfiSmmSwDispatch2ProtocolGuid,                   NULL                                             },
  { STRING_TOKEN (STR_S_SXD),               &gEfiSmmSxDispatch2ProtocolGuid,                   NULL                                             },
  { STRING_TOKEN (STR_S_PTD2),              &gEfiSmmPeriodicTimerDispatch2ProtocolGuid,        NULL                                             },
  { STRING_TOKEN (STR_S_UD2),               &gEfiSmmUsbDispatch2ProtocolGuid,                  NULL                                             },
  { STRING_TOKEN (STR_S_GD2),               &gEfiSmmGpiDispatch2ProtocolGuid,                  NULL                                             },
  { STRING_TOKEN (STR_S_SBD2),              &gEfiSmmStandbyButtonDispatch2ProtocolGuid,        NULL                                             },
  { STRING_TOKEN (STR_S_PBD2),              &gEfiSmmPowerButtonDispatch2ProtocolGuid,          NULL                                             },
  { STRING_TOKEN (STR_S_ITD2),              &gEfiSmmIoTrapDispatch2ProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_PCD),                 &gEfiPcdProtocolGuid,                              NULL                                             },
  { STRING_TOKEN (STR_FVB2),                &gEfiFirmwareVolumeBlock2ProtocolGuid,             NULL                                             },
  { STRING_TOKEN (STR_CPUIO2),              &gEfiCpuIo2ProtocolGuid,                           NULL                                             },
  { STRING_TOKEN (STR_LEGACY_R2),           &gEfiLegacyRegion2ProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_S2ARCH),              &gEfiSecurity2ArchProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_EODXE),               &gEfiSmmEndOfDxeProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_ISAHC),               &gEfiIsaHcProtocolGuid,                            NULL                                             },
  { STRING_TOKEN (STR_ISAHC_B),             &gEfiIsaHcServiceBindingProtocolGuid,              NULL                                             },
  { STRING_TOKEN (STR_SIO_C),               &gEfiSioControlProtocolGuid,                       NULL                                             },
  { STRING_TOKEN (STR_GET_PCD),             &gEfiGetPcdInfoProtocolGuid,                       NULL                                             },
  { STRING_TOKEN (STR_I2C_M),               &gEfiI2cMasterProtocolGuid,                        NULL                                             },
  { STRING_TOKEN (STR_I2CIO),               &gEfiI2cIoProtocolGuid,                            NULL                                             },
  { STRING_TOKEN (STR_I2CEN),               &gEfiI2cEnumerateProtocolGuid,                     NULL                                             },
  { STRING_TOKEN (STR_I2C_H),               &gEfiI2cHostProtocolGuid,                          NULL                                             },
  { STRING_TOKEN (STR_I2C_BCM),             &gEfiI2cBusConfigurationManagementProtocolGuid,    NULL                                             },
  { STRING_TOKEN (STR_TCG2),                &gEfiTcg2ProtocolGuid,                             NULL                                             },
  { STRING_TOKEN (STR_TIMESTAMP),           &gEfiTimestampProtocolGuid,                        NULL                                             },
  { STRING_TOKEN (STR_RNG),                 &gEfiRngProtocolGuid,                              NULL                                             },
  { STRING_TOKEN (STR_NVMEPT),              &gEfiNvmExpressPassThruProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_H2_SB),               &gEfiHash2ServiceBindingProtocolGuid,              NULL                                             },
  { STRING_TOKEN (STR_HASH2),               &gEfiHash2ProtocolGuid,                            NULL                                             },
  { STRING_TOKEN (STR_BIO_C),               &gEfiBlockIoCryptoProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_SCR),                 &gEfiSmartCardReaderProtocolGuid,                  NULL                                             },
  { STRING_TOKEN (STR_SCE),                 &gEfiSmartCardEdgeProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_USB_FIO),             &gEfiUsbFunctionIoProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_BC_HC),               &gEfiBluetoothHcProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_BC_IO_SB),            &gEfiBluetoothIoServiceBindingProtocolGuid,        NULL                                             },
  { STRING_TOKEN (STR_BC_IO),               &gEfiBluetoothIoProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_BC_C),                &gEfiBluetoothConfigProtocolGuid,                  NULL                                             },
  { STRING_TOKEN (STR_REG_EXP),             &gEfiRegularExpressionProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_B_MGR_P),             &gEfiBootManagerPolicyProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_CKH),                 &gEfiConfigKeywordHandlerProtocolGuid,             NULL                                             },
  { STRING_TOKEN (STR_WIFI),                &gEfiWiFiProtocolGuid,                             NULL                                             },
  { STRING_TOKEN (STR_EAP_M),               &gEfiEapManagement2ProtocolGuid,                   NULL                                             },
  { STRING_TOKEN (STR_EAP_C),               &gEfiEapConfigurationProtocolGuid,                 NULL                                             },
  { STRING_TOKEN (STR_PKCS7),               &gEfiPkcs7VerifyProtocolGuid,                      NULL                                             },
  { STRING_TOKEN (STR_NET_DNS4_SB),         &gEfiDns4ServiceBindingProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_NET_DNS4),            &gEfiDns4ProtocolGuid,                             NULL                                             },
  { STRING_TOKEN (STR_NET_DNS6_SB),         &gEfiDns6ServiceBindingProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_NET_DNS6),            &gEfiDns6ProtocolGuid,                             NULL                                             },
  { STRING_TOKEN (STR_NET_HTTP_SB),         &gEfiHttpServiceBindingProtocolGuid,               NULL                                             },
  { STRING_TOKEN (STR_NET_HTTP),            &gEfiHttpProtocolGuid,                             NULL                                             },
  { STRING_TOKEN (STR_NET_HTTP_U),          &gEfiHttpUtilitiesProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_REST),                &gEfiRestProtocolGuid,                             NULL                                             },

  //
  // PI 1.5
  //
  { STRING_TOKEN (STR_MM_EOD),              &gEfiMmEndOfDxeProtocolGuid,                       NULL                                             },
  { STRING_TOKEN (STR_MM_ITD),              &gEfiMmIoTrapDispatchProtocolGuid,                 NULL                                             },
  { STRING_TOKEN (STR_MM_PBD),              &gEfiMmPowerButtonDispatchProtocolGuid,            NULL                                             },
  { STRING_TOKEN (STR_MM_SBD),              &gEfiMmStandbyButtonDispatchProtocolGuid,          NULL                                             },
  { STRING_TOKEN (STR_MM_GD),               &gEfiMmGpiDispatchProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_MM_UD),               &gEfiMmUsbDispatchProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_MM_PTD),              &gEfiMmPeriodicTimerDispatchProtocolGuid,          NULL                                             },
  { STRING_TOKEN (STR_MM_SXD),              &gEfiMmSxDispatchProtocolGuid,                     NULL                                             },
  { STRING_TOKEN (STR_MM_SWD),              &gEfiMmSwDispatchProtocolGuid,                     NULL                                             },
  { STRING_TOKEN (STR_MM_PRBI),             &gEfiMmPciRootBridgeIoProtocolGuid,                NULL                                             },
  { STRING_TOKEN (STR_MM_CPU),              &gEfiMmCpuProtocolGuid,                            NULL                                             },
  { STRING_TOKEN (STR_MM_STACODE),          &gEfiMmStatusCodeProtocolGuid,                     NULL                                             },
  { STRING_TOKEN (STR_DXEMM_RTL),           &gEfiDxeMmReadyToLockProtocolGuid,                 NULL                                             },
  { STRING_TOKEN (STR_MM_CONFIG),           &gEfiMmConfigurationProtocolGuid,                  NULL                                             },
  { STRING_TOKEN (STR_MM_RTL),              &gEfiMmReadyToLockProtocolGuid,                    NULL                                             },
  { STRING_TOKEN (STR_MM_CONTROL),          &gEfiMmControlProtocolGuid,                        NULL                                             },
  { STRING_TOKEN (STR_MM_ACCESS),           &gEfiMmAccessProtocolGuid,                         NULL                                             },
  { STRING_TOKEN (STR_MM_BASE),             &gEfiMmBaseProtocolGuid,                           NULL                                             },
  { STRING_TOKEN (STR_MM_CPUIO),            &gEfiMmCpuIoProtocolGuid,                          NULL                                             },
  { STRING_TOKEN (STR_MM_RH),               &gEfiMmRscHandlerProtocolGuid,                     NULL                                             },
  { STRING_TOKEN (STR_MM_COM),              &gEfiMmCommunicationProtocolGuid,                  NULL                                             },

  //
  // UEFI Shell Spec 2.0
  //
  { STRING_TOKEN (STR_SHELL_PARAMETERS),    &gEfiShellParametersProtocolGuid,                  NULL                                             },
  { STRING_TOKEN (STR_SHELL),               &gEfiShellProtocolGuid,                            NULL                                             },

  //
  // UEFI Shell Spec 2.1
  //
  { STRING_TOKEN (STR_SHELL_DYNAMIC),       &gEfiShellDynamicCommandProtocolGuid,              NULL                                             },

  //
  // Misc
  //
  { STRING_TOKEN (STR_PCDINFOPROT),         &gGetPcdInfoProtocolGuid,                          NULL                                             },

  //
  // terminator
  //
  { 0,                                      NULL,                                              NULL                                             },
};

/**
  Function to get the node for a protocol or struct from it's GUID.

  if Guid is NULL, then ASSERT.

  @param[in] Guid               The GUID to look for the name of.

  @return                       The node.
**/
CONST GUID_INFO_BLOCK *
InternalShellGetNodeFromGuid (
  IN CONST EFI_GUID  *Guid
  )
{
  CONST GUID_INFO_BLOCK  *ListWalker;
  UINTN                  LoopCount;

  ASSERT (Guid != NULL);

  for (LoopCount = 0, ListWalker = mGuidList; mGuidList != NULL && LoopCount < mGuidListCount; LoopCount++, ListWalker++) {
    if (CompareGuid (ListWalker->GuidId, Guid)) {
      return (ListWalker);
    }
  }

  if (PcdGetBool (PcdShellIncludeNtGuids)) {
    for (ListWalker = mGuidStringListNT; ListWalker != NULL && ListWalker->GuidId != NULL; ListWalker++) {
      if (CompareGuid (ListWalker->GuidId, Guid)) {
        return (ListWalker);
      }
    }
  }

  for (ListWalker = mGuidStringList; ListWalker != NULL && ListWalker->GuidId != NULL; ListWalker++) {
    if (CompareGuid (ListWalker->GuidId, Guid)) {
      return (ListWalker);
    }
  }

  return (NULL);
}

/**
Function to add a new GUID/Name mapping.

@param[in] Guid       The Guid
@param[in] NameID     The STRING id of the HII string to use
@param[in] DumpFunc   The pointer to the dump function


@retval EFI_SUCCESS           The operation was successful
@retval EFI_OUT_OF_RESOURCES  A memory allocation failed
@retval EFI_INVALID_PARAMETER Guid NameId was invalid
**/
EFI_STATUS
InsertNewGuidNameMapping (
  IN CONST EFI_GUID            *Guid,
  IN CONST EFI_STRING_ID       NameID,
  IN CONST DUMP_PROTOCOL_INFO  DumpFunc OPTIONAL
  )
{
  ASSERT (Guid   != NULL);
  ASSERT (NameID != 0);

  mGuidList = ReallocatePool (
                mGuidListCount * sizeof (GUID_INFO_BLOCK),
                (mGuidListCount + 1) * sizeof (GUID_INFO_BLOCK),
                mGuidList
                );
  if (mGuidList == NULL) {
    mGuidListCount = 0;
    return (EFI_OUT_OF_RESOURCES);
  }

  mGuidListCount++;

  mGuidList[mGuidListCount - 1].GuidId   = AllocateCopyPool (sizeof (EFI_GUID), Guid);
  mGuidList[mGuidListCount - 1].StringId = NameID;
  mGuidList[mGuidListCount - 1].DumpInfo = DumpFunc;

  if (mGuidList[mGuidListCount - 1].GuidId == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  return (EFI_SUCCESS);
}

/**
  Function to add a new GUID/Name mapping.

  This cannot overwrite an existing mapping.

  @param[in] Guid       The Guid
  @param[in] TheName    The Guid's name
  @param[in] Lang       RFC4646 language code list or NULL

  @retval EFI_SUCCESS           The operation was successful
  @retval EFI_ACCESS_DENIED     There was a duplicate
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed
  @retval EFI_INVALID_PARAMETER Guid or TheName was NULL
**/
EFI_STATUS
EFIAPI
AddNewGuidNameMapping (
  IN CONST EFI_GUID  *Guid,
  IN CONST CHAR16    *TheName,
  IN CONST CHAR8     *Lang OPTIONAL
  )
{
  EFI_STRING_ID  NameID;

  HandleParsingHiiInit ();

  if ((Guid == NULL) || (TheName == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  if ((InternalShellGetNodeFromGuid (Guid)) != NULL) {
    return (EFI_ACCESS_DENIED);
  }

  NameID = HiiSetString (mHandleParsingHiiHandle, 0, (CHAR16 *)TheName, Lang);
  if (NameID == 0) {
    return (EFI_OUT_OF_RESOURCES);
  }

  return (InsertNewGuidNameMapping (Guid, NameID, NULL));
}

/**
  Function to get the name of a protocol or struct from it's GUID.

  if Guid is NULL, then ASSERT.

  @param[in] Guid               The GUID to look for the name of.
  @param[in] Lang               The language to use.

  @return                       pointer to string of the name.  The caller
                                is responsible to free this memory.
**/
CHAR16 *
EFIAPI
GetStringNameFromGuid (
  IN CONST EFI_GUID  *Guid,
  IN CONST CHAR8     *Lang OPTIONAL
  )
{
  CONST GUID_INFO_BLOCK  *Id;

  HandleParsingHiiInit ();

  Id = InternalShellGetNodeFromGuid (Guid);
  if (Id == NULL) {
    return NULL;
  }

  return HiiGetString (mHandleParsingHiiHandle, Id->StringId, Lang);
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
CHAR16 *
EFIAPI
GetProtocolInformationDump (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST EFI_GUID    *Guid,
  IN CONST BOOLEAN     Verbose
  )
{
  CONST GUID_INFO_BLOCK  *Id;

  ASSERT (TheHandle  != NULL);
  ASSERT (Guid       != NULL);

  if ((TheHandle == NULL) || (Guid == NULL)) {
    return (NULL);
  }

  Id = InternalShellGetNodeFromGuid (Guid);
  if ((Id != NULL) && (Id->DumpInfo != NULL)) {
    return (Id->DumpInfo (TheHandle, Verbose));
  }

  return (NULL);
}

/**
  Function to get the Guid for a protocol or struct based on it's string name.

  do not modify the returned Guid.

  @param[in] Name           The pointer to the string name.
  @param[in] Lang           The pointer to the language code.
  @param[out] Guid          The pointer to the Guid.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
EFIAPI
GetGuidFromStringName (
  IN CONST CHAR16  *Name,
  IN CONST CHAR8   *Lang OPTIONAL,
  OUT EFI_GUID     **Guid
  )
{
  CONST GUID_INFO_BLOCK  *ListWalker;
  CHAR16                 *String;
  UINTN                  LoopCount;

  HandleParsingHiiInit ();

  ASSERT (Guid != NULL);
  if (Guid == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  *Guid = NULL;

  if (PcdGetBool (PcdShellIncludeNtGuids)) {
    for (ListWalker = mGuidStringListNT; ListWalker != NULL && ListWalker->GuidId != NULL; ListWalker++) {
      String = HiiGetString (mHandleParsingHiiHandle, ListWalker->StringId, Lang);
      if ((Name != NULL) && (String != NULL) && (StringNoCaseCompare (&Name, &String) == 0)) {
        *Guid = ListWalker->GuidId;
      }

      SHELL_FREE_NON_NULL (String);
      if (*Guid != NULL) {
        return (EFI_SUCCESS);
      }
    }
  }

  for (ListWalker = mGuidStringList; ListWalker != NULL && ListWalker->GuidId != NULL; ListWalker++) {
    String = HiiGetString (mHandleParsingHiiHandle, ListWalker->StringId, Lang);
    if ((Name != NULL) && (String != NULL) && (StringNoCaseCompare (&Name, &String) == 0)) {
      *Guid = ListWalker->GuidId;
    }

    SHELL_FREE_NON_NULL (String);
    if (*Guid != NULL) {
      return (EFI_SUCCESS);
    }
  }

  for (LoopCount = 0, ListWalker = mGuidList; mGuidList != NULL && LoopCount < mGuidListCount; LoopCount++, ListWalker++) {
    String = HiiGetString (mHandleParsingHiiHandle, ListWalker->StringId, Lang);
    if ((Name != NULL) && (String != NULL) && (StringNoCaseCompare (&Name, &String) == 0)) {
      *Guid = ListWalker->GuidId;
    }

    SHELL_FREE_NON_NULL (String);
    if (*Guid != NULL) {
      return (EFI_SUCCESS);
    }
  }

  return (EFI_NOT_FOUND);
}

/**
  Get best support language for this driver.

  First base on the user input language  to search, second base on the current
  platform used language to search, third get the first language from the
  support language list. The caller need to free the buffer of the best language.

  @param[in] SupportedLanguages      The support languages for this driver.
  @param[in] InputLanguage           The user input language.
  @param[in] Iso639Language          Whether get language for ISO639.

  @return                            The best support language for this driver.
**/
CHAR8 *
EFIAPI
GetBestLanguageForDriver (
  IN CONST CHAR8  *SupportedLanguages,
  IN CONST CHAR8  *InputLanguage,
  IN BOOLEAN      Iso639Language
  )
{
  CHAR8  *LanguageVariable;
  CHAR8  *BestLanguage;

  GetVariable2 (Iso639Language ? L"Lang" : L"PlatformLang", &gEfiGlobalVariableGuid, (VOID **)&LanguageVariable, NULL);

  BestLanguage = GetBestLanguage (
                   SupportedLanguages,
                   Iso639Language,
                   (InputLanguage != NULL) ? InputLanguage : "",
                   (LanguageVariable != NULL) ? LanguageVariable : "",
                   SupportedLanguages,
                   NULL
                   );

  if (LanguageVariable != NULL) {
    FreePool (LanguageVariable);
  }

  return BestLanguage;
}

/**
  Function to retrieve the driver name (if possible) from the ComponentName or
  ComponentName2 protocol

  @param[in] TheHandle      The driver handle to get the name of.
  @param[in] Language       The language to use.

  @retval NULL              The name could not be found.
  @return                   A pointer to the string name.  Do not de-allocate the memory.
**/
CONST CHAR16 *
EFIAPI
GetStringNameFromHandle (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST CHAR8       *Language
  )
{
  EFI_COMPONENT_NAME2_PROTOCOL  *CompNameStruct;
  EFI_STATUS                    Status;
  CHAR16                        *RetVal;
  CHAR8                         *BestLang;

  BestLang = NULL;

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **)&CompNameStruct,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    BestLang = GetBestLanguageForDriver (CompNameStruct->SupportedLanguages, Language, FALSE);
    Status   = CompNameStruct->GetDriverName (CompNameStruct, BestLang, &RetVal);
    if (BestLang != NULL) {
      FreePool (BestLang);
      BestLang = NULL;
    }

    if (!EFI_ERROR (Status)) {
      return (RetVal);
    }
  }

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiComponentNameProtocolGuid,
                  (VOID **)&CompNameStruct,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    BestLang = GetBestLanguageForDriver (CompNameStruct->SupportedLanguages, Language, FALSE);
    Status   = CompNameStruct->GetDriverName (CompNameStruct, BestLang, &RetVal);
    if (BestLang != NULL) {
      FreePool (BestLang);
    }

    if (!EFI_ERROR (Status)) {
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
InternalShellInitHandleList (
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

  InitializeListHead (&mHandleList.List.Link);
  mHandleList.NextIndex = 1;
  Status                = gBS->LocateHandleBuffer (
                                 AllHandles,
                                 NULL,
                                 NULL,
                                 &HandleCount,
                                 &HandleBuffer
                                 );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return (Status);
  }

  for (mHandleList.NextIndex = 1; mHandleList.NextIndex <= HandleCount; mHandleList.NextIndex++) {
    ListWalker = AllocateZeroPool (sizeof (HANDLE_LIST));
    if (ListWalker != NULL) {
      ListWalker->TheHandle = HandleBuffer[mHandleList.NextIndex - 1];
      ListWalker->TheIndex  = mHandleList.NextIndex;
      InsertTailList (&mHandleList.List.Link, &ListWalker->Link);
    }
  }

  FreePool (HandleBuffer);
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
ConvertHandleToHandleIndex (
  IN CONST EFI_HANDLE  TheHandle
  )
{
  EFI_STATUS   Status;
  EFI_GUID     **ProtocolBuffer;
  UINTN        ProtocolCount;
  HANDLE_LIST  *ListWalker;

  if (TheHandle == NULL) {
    return 0;
  }

  InternalShellInitHandleList ();

  for (ListWalker = (HANDLE_LIST *)GetFirstNode (&mHandleList.List.Link)
       ; !IsNull (&mHandleList.List.Link, &ListWalker->Link)
       ; ListWalker = (HANDLE_LIST *)GetNextNode (&mHandleList.List.Link, &ListWalker->Link)
       )
  {
    if (ListWalker->TheHandle == TheHandle) {
      //
      // Verify that TheHandle is still present in the Handle Database
      //
      Status = gBS->ProtocolsPerHandle (TheHandle, &ProtocolBuffer, &ProtocolCount);
      if (EFI_ERROR (Status)) {
        //
        // TheHandle is not present in the Handle Database, so delete from the handle list
        //
        RemoveEntryList (&ListWalker->Link);
        return 0;
      }

      FreePool (ProtocolBuffer);
      return (ListWalker->TheIndex);
    }
  }

  //
  // Verify that TheHandle is valid handle
  //
  Status = gBS->ProtocolsPerHandle (TheHandle, &ProtocolBuffer, &ProtocolCount);
  if (EFI_ERROR (Status)) {
    //
    // TheHandle is not valid, so do not add to handle list
    //
    return 0;
  }

  FreePool (ProtocolBuffer);

  ListWalker = AllocateZeroPool (sizeof (HANDLE_LIST));
  if (ListWalker == NULL) {
    return 0;
  }

  ListWalker->TheHandle = TheHandle;
  ListWalker->TheIndex  = mHandleList.NextIndex++;
  InsertTailList (&mHandleList.List.Link, &ListWalker->Link);
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
ConvertHandleIndexToHandle (
  IN CONST UINTN  TheIndex
  )
{
  EFI_STATUS   Status;
  EFI_GUID     **ProtocolBuffer;
  UINTN        ProtocolCount;
  HANDLE_LIST  *ListWalker;

  InternalShellInitHandleList ();

  if (TheIndex >= mHandleList.NextIndex) {
    return NULL;
  }

  for (ListWalker = (HANDLE_LIST *)GetFirstNode (&mHandleList.List.Link)
       ; !IsNull (&mHandleList.List.Link, &ListWalker->Link)
       ; ListWalker = (HANDLE_LIST *)GetNextNode (&mHandleList.List.Link, &ListWalker->Link)
       )
  {
    if ((ListWalker->TheIndex == TheIndex) && (ListWalker->TheHandle != NULL)) {
      //
      // Verify that LinkWalker->TheHandle is valid handle
      //
      Status = gBS->ProtocolsPerHandle (ListWalker->TheHandle, &ProtocolBuffer, &ProtocolCount);
      if (!EFI_ERROR (Status)) {
        FreePool (ProtocolBuffer);
      } else {
        //
        // TheHandle is not valid, so do not add to handle list
        //
        ListWalker->TheHandle = NULL;
      }

      return (ListWalker->TheHandle);
    }
  }

  return NULL;
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
  IN CONST EFI_HANDLE  DriverBindingHandle OPTIONAL,
  IN CONST EFI_HANDLE  ControllerHandle OPTIONAL,
  IN UINTN             *HandleCount,
  OUT EFI_HANDLE       **HandleBuffer,
  OUT UINTN            **HandleType
  )
{
  EFI_STATUS                           Status;
  UINTN                                HandleIndex;
  EFI_GUID                             **ProtocolGuidArray;
  UINTN                                ArrayCount;
  UINTN                                ProtocolIndex;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfo;
  UINTN                                OpenInfoCount;
  UINTN                                OpenInfoIndex;
  UINTN                                ChildIndex;
  INTN                                 DriverBindingHandleIndex;

  ASSERT (HandleCount  != NULL);
  ASSERT (HandleBuffer != NULL);
  ASSERT (HandleType   != NULL);
  ASSERT (DriverBindingHandle != NULL || ControllerHandle != NULL);

  *HandleCount  = 0;
  *HandleBuffer = NULL;
  *HandleType   = NULL;

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
  if (*HandleType == NULL) {
    SHELL_FREE_NON_NULL (*HandleBuffer);
    *HandleCount = 0;
    return EFI_OUT_OF_RESOURCES;
  }

  DriverBindingHandleIndex = -1;
  for (HandleIndex = 0; HandleIndex < *HandleCount; HandleIndex++) {
    if ((DriverBindingHandle != NULL) && ((*HandleBuffer)[HandleIndex] == DriverBindingHandle)) {
      DriverBindingHandleIndex = (INTN)HandleIndex;
    }
  }

  for (HandleIndex = 0; HandleIndex < *HandleCount; HandleIndex++) {
    //
    // Retrieve the list of all the protocols on each handle
    //
    Status = gBS->ProtocolsPerHandle (
                    (*HandleBuffer)[HandleIndex],
                    &ProtocolGuidArray,
                    &ArrayCount
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++) {
      //
      // Set the bit describing what this handle has
      //
      if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiLoadedImageProtocolGuid)) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_IMAGE_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverBindingProtocolGuid)) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_DRIVER_BINDING_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverConfiguration2ProtocolGuid)) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_DRIVER_CONFIGURATION_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverConfigurationProtocolGuid)) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_DRIVER_CONFIGURATION_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverDiagnostics2ProtocolGuid)) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_DRIVER_DIAGNOSTICS_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverDiagnosticsProtocolGuid)) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_DRIVER_DIAGNOSTICS_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiComponentName2ProtocolGuid)) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_COMPONENT_NAME_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiComponentNameProtocolGuid)) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_COMPONENT_NAME_HANDLE;
      } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDevicePathProtocolGuid)) {
        (*HandleType)[HandleIndex] |= (UINTN)HR_DEVICE_HANDLE;
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
      if (EFI_ERROR (Status)) {
        continue;
      }

      if (ControllerHandle == NULL) {
        //
        // ControllerHandle == NULL and DriverBindingHandle != NULL.
        // Return information on all the controller handles that the driver specified by DriverBindingHandle is managing
        //
        for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
          if ((OpenInfo[OpenInfoIndex].AgentHandle == DriverBindingHandle) && ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0)) {
            (*HandleType)[HandleIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CONTROLLER_HANDLE);
            if (DriverBindingHandleIndex != -1) {
              (*HandleType)[DriverBindingHandleIndex] |= (UINTN)HR_DEVICE_DRIVER;
            }
          }

          if ((OpenInfo[OpenInfoIndex].AgentHandle == DriverBindingHandle) && ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0)) {
            (*HandleType)[HandleIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CONTROLLER_HANDLE);
            if (DriverBindingHandleIndex != -1) {
              (*HandleType)[DriverBindingHandleIndex] |= (UINTN)(HR_BUS_DRIVER | HR_DEVICE_DRIVER);
            }

            for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
              if (OpenInfo[OpenInfoIndex].ControllerHandle == (*HandleBuffer)[ChildIndex]) {
                (*HandleType)[ChildIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CHILD_HANDLE);
              }
            }
          }
        }
      }

      if ((DriverBindingHandle == NULL) && (ControllerHandle != NULL)) {
        if (ControllerHandle == (*HandleBuffer)[HandleIndex]) {
          (*HandleType)[HandleIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CONTROLLER_HANDLE);
          for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
            if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
              for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                if (OpenInfo[OpenInfoIndex].AgentHandle == (*HandleBuffer)[ChildIndex]) {
                  (*HandleType)[ChildIndex] |= (UINTN)HR_DEVICE_DRIVER;
                }
              }
            }

            if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
              for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                if (OpenInfo[OpenInfoIndex].AgentHandle == (*HandleBuffer)[ChildIndex]) {
                  (*HandleType)[ChildIndex] |= (UINTN)(HR_BUS_DRIVER | HR_DEVICE_DRIVER);
                }

                if (OpenInfo[OpenInfoIndex].ControllerHandle == (*HandleBuffer)[ChildIndex]) {
                  (*HandleType)[ChildIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CHILD_HANDLE);
                }
              }
            }
          }
        } else {
          for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
            if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
              if (OpenInfo[OpenInfoIndex].ControllerHandle == ControllerHandle) {
                (*HandleType)[HandleIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_PARENT_HANDLE);
              }
            }
          }
        }
      }

      if ((DriverBindingHandle != NULL) && (ControllerHandle != NULL)) {
        if (ControllerHandle == (*HandleBuffer)[HandleIndex]) {
          (*HandleType)[HandleIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CONTROLLER_HANDLE);
          for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
            if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
              if (OpenInfo[OpenInfoIndex].AgentHandle == DriverBindingHandle) {
                if (DriverBindingHandleIndex != -1) {
                  (*HandleType)[DriverBindingHandleIndex] |= (UINTN)HR_DEVICE_DRIVER;
                }
              }
            }

            if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
              if (OpenInfo[OpenInfoIndex].AgentHandle == DriverBindingHandle) {
                for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                  if (OpenInfo[OpenInfoIndex].ControllerHandle == (*HandleBuffer)[ChildIndex]) {
                    (*HandleType)[ChildIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_CHILD_HANDLE);
                  }
                }
              }

              for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                if (OpenInfo[OpenInfoIndex].AgentHandle == (*HandleBuffer)[ChildIndex]) {
                  (*HandleType)[ChildIndex] |= (UINTN)(HR_BUS_DRIVER | HR_DEVICE_DRIVER);
                }
              }
            }
          }
        } else {
          for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
            if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
              if (OpenInfo[OpenInfoIndex].ControllerHandle == ControllerHandle) {
                (*HandleType)[HandleIndex] |= (UINTN)(HR_DEVICE_HANDLE | HR_PARENT_HANDLE);
              }
            }
          }
        }
      }

      FreePool (OpenInfo);
    }

    FreePool (ProtocolGuidArray);
  }

  return EFI_SUCCESS;
}

/**
  Gets all the related EFI_HANDLEs based on the single EFI_HANDLE and the mask
  supplied.

  This function will scan all EFI_HANDLES in the UEFI environment's handle database
  and return all the ones with the specified relationship (Mask) to the specified
  controller handle.

  If both DriverBindingHandle and ControllerHandle are NULL, then ASSERT.
  If MatchingHandleCount is NULL, then ASSERT.

  If MatchingHandleBuffer is not NULL upon a successful return the memory must be
  caller freed.

  @param[in] DriverBindingHandle    Handle to a object with Driver Binding protocol
                                    on it.
  @param[in] ControllerHandle       Handle to a device with Device Path protocol on it.
  @param[in] Mask                   Mask of what relationship(s) is desired.
  @param[in] MatchingHandleCount    Poitner to UINTN specifying number of HANDLES in
                                    MatchingHandleBuffer.
  @param[out] MatchingHandleBuffer  On a successful return a buffer of MatchingHandleCount
                                    EFI_HANDLEs and a terminating NULL EFI_HANDLE.

  @retval EFI_SUCCESS               The operation was successful and any related handles
                                    are in MatchingHandleBuffer;
  @retval EFI_NOT_FOUND             No matching handles were found.
  @retval EFI_INVALID_PARAMETER     A parameter was invalid or out of range.
**/
EFI_STATUS
EFIAPI
ParseHandleDatabaseByRelationship (
  IN CONST EFI_HANDLE  DriverBindingHandle OPTIONAL,
  IN CONST EFI_HANDLE  ControllerHandle OPTIONAL,
  IN CONST UINTN       Mask,
  IN UINTN             *MatchingHandleCount,
  OUT EFI_HANDLE       **MatchingHandleBuffer OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       *HandleType;
  UINTN       HandleIndex;

  ASSERT (MatchingHandleCount != NULL);
  ASSERT (DriverBindingHandle != NULL || ControllerHandle != NULL);

  if ((Mask & HR_VALID_MASK) != Mask) {
    return (EFI_INVALID_PARAMETER);
  }

  if (((Mask & HR_CHILD_HANDLE) != 0) && (DriverBindingHandle == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  *MatchingHandleCount = 0;
  if (MatchingHandleBuffer != NULL) {
    *MatchingHandleBuffer = NULL;
  }

  HandleBuffer = NULL;
  HandleType   = NULL;

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
        if (*MatchingHandleBuffer == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
        } else {
          for (HandleIndex = 0, *MatchingHandleCount = 0
               ; HandleIndex < HandleCount
               ; HandleIndex++
               )
          {
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
        } // *MatchingHandleBuffer == NULL (ELSE)
      } // MacthingHandleBuffer == NULL (ELSE)
    } // *MatchingHandleCount  == 0 (ELSE)
  } // no error on ParseHandleDatabaseByRelationshipWithType

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  if (HandleType != NULL) {
    FreePool (HandleType);
  }

  ASSERT (
    (MatchingHandleBuffer == NULL) ||
    (*MatchingHandleCount == 0 && *MatchingHandleBuffer == NULL) ||
    (*MatchingHandleCount != 0 && *MatchingHandleBuffer != NULL)
    );
  return Status;
}

/**
  Gets handles for any child controllers of the passed in controller.

  @param[in] ControllerHandle       The handle of the "parent controller"
  @param[out] MatchingHandleCount   Pointer to the number of handles in
                                    MatchingHandleBuffer on return.
  @param[out] MatchingHandleBuffer  Buffer containing handles on a successful
                                    return.


  @retval EFI_SUCCESS               The operation was successful.
**/
EFI_STATUS
EFIAPI
ParseHandleDatabaseForChildControllers (
  IN CONST EFI_HANDLE  ControllerHandle,
  OUT UINTN            *MatchingHandleCount,
  OUT EFI_HANDLE       **MatchingHandleBuffer OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       HandleIndex;
  UINTN       DriverBindingHandleCount;
  EFI_HANDLE  *DriverBindingHandleBuffer;
  UINTN       DriverBindingHandleIndex;
  UINTN       ChildControllerHandleCount;
  EFI_HANDLE  *ChildControllerHandleBuffer;
  UINTN       ChildControllerHandleIndex;
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
  HandleBufferForReturn = GetHandleListByProtocol (NULL);
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
         )
    {
      for (HandleIndex = 0; HandleIndex < *MatchingHandleCount; HandleIndex++) {
        if (HandleBufferForReturn[HandleIndex] == ChildControllerHandleBuffer[ChildControllerHandleIndex]) {
          break;
        }
      }

      if (HandleIndex >= *MatchingHandleCount) {
        HandleBufferForReturn[(*MatchingHandleCount)++] = ChildControllerHandleBuffer[ChildControllerHandleIndex];
      }
    }

    FreePool (ChildControllerHandleBuffer);
  }

  FreePool (DriverBindingHandleBuffer);

  if ((MatchingHandleBuffer == NULL) || (*MatchingHandleCount == 0)) {
    //
    // The caller is not interested in the actual handles, or we've found none.
    //
    FreePool (HandleBufferForReturn);
    HandleBufferForReturn = NULL;
  }

  if (MatchingHandleBuffer != NULL) {
    *MatchingHandleBuffer = HandleBufferForReturn;
  }

  ASSERT (
    (MatchingHandleBuffer == NULL) ||
    (*MatchingHandleCount == 0 && *MatchingHandleBuffer == NULL) ||
    (*MatchingHandleCount != 0 && *MatchingHandleBuffer != NULL)
    );

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
VOID *
BuffernCatGrow (
  IN OUT VOID   **DestinationBuffer,
  IN OUT UINTN  *DestinationSize,
  IN     VOID   *SourceBuffer,
  IN     UINTN  SourceSize
  )
{
  UINTN  LocalDestinationSize;
  UINTN  LocalDestinationFinalSize;

  ASSERT (DestinationBuffer != NULL);

  if ((SourceSize == 0) || (SourceBuffer == NULL)) {
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
    *DestinationBuffer = AllocateZeroPool (LocalDestinationFinalSize);
  } else {
    // reallocate
    *DestinationBuffer = ReallocatePool (LocalDestinationSize, LocalDestinationFinalSize, *DestinationBuffer);
  }

  ASSERT (*DestinationBuffer != NULL);

  // copy
  return (CopyMem (((UINT8 *)(*DestinationBuffer)) + LocalDestinationSize, SourceBuffer, SourceSize));
}

/**
  Gets handles for any child devices produced by the passed in driver.

  @param[in] DriverHandle           The handle of the driver.
  @param[in] MatchingHandleCount    Pointer to the number of handles in
                                    MatchingHandleBuffer on return.
  @param[out] MatchingHandleBuffer  Buffer containing handles on a successful
                                    return.
  @retval EFI_SUCCESS               The operation was successful.
  @sa ParseHandleDatabaseByRelationship
**/
EFI_STATUS
EFIAPI
ParseHandleDatabaseForChildDevices (
  IN CONST EFI_HANDLE  DriverHandle,
  IN UINTN             *MatchingHandleCount,
  OUT EFI_HANDLE       **MatchingHandleBuffer OPTIONAL
  )
{
  EFI_HANDLE  *Buffer;
  EFI_HANDLE  *Buffer2;
  UINTN       Count1;
  UINTN       Count2;
  UINTN       HandleIndex;
  EFI_STATUS  Status;
  UINTN       HandleBufferSize;

  ASSERT (MatchingHandleCount != NULL);

  HandleBufferSize     = 0;
  Buffer               = NULL;
  Buffer2              = NULL;
  *MatchingHandleCount = 0;

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
      if (EFI_ERROR (Status)) {
        break;
      }

      //
      // save out required and optional data elements
      //
      *MatchingHandleCount += Count2;
      if (MatchingHandleBuffer != NULL) {
        *MatchingHandleBuffer = BuffernCatGrow ((VOID **)MatchingHandleBuffer, &HandleBufferSize, Buffer2, Count2 * sizeof (Buffer2[0]));
      }

      //
      // free the memory
      //
      if (Buffer2 != NULL) {
        FreePool (Buffer2);
      }
    }
  }

  if (Buffer != NULL) {
    FreePool (Buffer);
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
EFI_HANDLE *
EFIAPI
GetHandleListByProtocol (
  IN CONST EFI_GUID  *ProtocolGuid OPTIONAL
  )
{
  EFI_HANDLE  *HandleList;
  UINTN       Size;
  EFI_STATUS  Status;

  Size       = 0;
  HandleList = NULL;

  //
  // We cannot use LocateHandleBuffer since we need that NULL item on the ends of the list!
  //
  if (ProtocolGuid == NULL) {
    Status = gBS->LocateHandle (AllHandles, NULL, NULL, &Size, HandleList);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      HandleList = AllocateZeroPool (Size + sizeof (EFI_HANDLE));
      if (HandleList == NULL) {
        return (NULL);
      }

      Status                               = gBS->LocateHandle (AllHandles, NULL, NULL, &Size, HandleList);
      HandleList[Size/sizeof (EFI_HANDLE)] = NULL;
    }
  } else {
    Status = gBS->LocateHandle (ByProtocol, (EFI_GUID *)ProtocolGuid, NULL, &Size, HandleList);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      HandleList = AllocateZeroPool (Size + sizeof (EFI_HANDLE));
      if (HandleList == NULL) {
        return (NULL);
      }

      Status                               = gBS->LocateHandle (ByProtocol, (EFI_GUID *)ProtocolGuid, NULL, &Size, HandleList);
      HandleList[Size/sizeof (EFI_HANDLE)] = NULL;
    }
  }

  if (EFI_ERROR (Status)) {
    if (HandleList != NULL) {
      FreePool (HandleList);
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
EFI_HANDLE *
EFIAPI
GetHandleListByProtocolList (
  IN CONST EFI_GUID  **ProtocolGuids
  )
{
  EFI_HANDLE      *HandleList;
  UINTN           Size;
  UINTN           TotalSize;
  UINTN           TempSize;
  EFI_STATUS      Status;
  CONST EFI_GUID  **GuidWalker;
  EFI_HANDLE      *HandleWalker1;
  EFI_HANDLE      *HandleWalker2;

  Size       = 0;
  HandleList = NULL;
  TotalSize  = sizeof (EFI_HANDLE);

  for (GuidWalker = ProtocolGuids; GuidWalker != NULL && *GuidWalker != NULL; GuidWalker++, Size = 0) {
    Status = gBS->LocateHandle (ByProtocol, (EFI_GUID *)(*GuidWalker), NULL, &Size, NULL);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      TotalSize += Size;
    }
  }

  //
  // No handles were found...
  //
  if (TotalSize == sizeof (EFI_HANDLE)) {
    return (NULL);
  }

  HandleList = AllocateZeroPool (TotalSize);
  if (HandleList == NULL) {
    return (NULL);
  }

  Size = 0;
  for (GuidWalker = ProtocolGuids; GuidWalker != NULL && *GuidWalker != NULL; GuidWalker++) {
    TempSize = TotalSize - Size;
    Status   = gBS->LocateHandle (ByProtocol, (EFI_GUID *)(*GuidWalker), NULL, &TempSize, HandleList+(Size/sizeof (EFI_HANDLE)));

    //
    // Allow for missing protocols... Only update the 'used' size upon success.
    //
    if (!EFI_ERROR (Status)) {
      Size += TempSize;
    }
  }

  ASSERT (HandleList[(TotalSize/sizeof (EFI_HANDLE))-1] == NULL);

  for (HandleWalker1 = HandleList; HandleWalker1 != NULL && *HandleWalker1 != NULL; HandleWalker1++) {
    for (HandleWalker2 = HandleWalker1 + 1; HandleWalker2 != NULL && *HandleWalker2 != NULL; HandleWalker2++) {
      if (*HandleWalker1 == *HandleWalker2) {
        //
        // copy memory back 1 handle width.
        //
        CopyMem (HandleWalker2, HandleWalker2 + 1, TotalSize - ((HandleWalker2-HandleList+1)*sizeof (EFI_HANDLE)));
      }
    }
  }

  return (HandleList);
}

/**
  Return all supported GUIDs.

  @param[out]     Guids  The buffer to return all supported GUIDs.
  @param[in, out] Count  On input, the count of GUIDs the buffer can hold,
                         On output, the count of GUIDs to return.

  @retval EFI_INVALID_PARAMETER Count is NULL.
  @retval EFI_BUFFER_TOO_SMALL  Buffer is not enough to hold all GUIDs.
  @retval EFI_SUCCESS           GUIDs are returned successfully.
**/
EFI_STATUS
EFIAPI
GetAllMappingGuids (
  OUT EFI_GUID  *Guids,
  IN OUT UINTN  *Count
  )
{
  UINTN  GuidCount;
  UINTN  NtGuidCount;
  UINTN  Index;

  if (Count == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  NtGuidCount = 0;
  if (PcdGetBool (PcdShellIncludeNtGuids)) {
    NtGuidCount = ARRAY_SIZE (mGuidStringListNT) - 1;
  }

  GuidCount = ARRAY_SIZE (mGuidStringList) - 1;

  if (*Count < NtGuidCount + GuidCount + mGuidListCount) {
    *Count = NtGuidCount + GuidCount + mGuidListCount;
    return EFI_BUFFER_TOO_SMALL;
  }

  for (Index = 0; Index < NtGuidCount; Index++) {
    CopyGuid (&Guids[Index], mGuidStringListNT[Index].GuidId);
  }

  for (Index = 0; Index < GuidCount; Index++) {
    CopyGuid (&Guids[NtGuidCount + Index], mGuidStringList[Index].GuidId);
  }

  for (Index = 0; Index < mGuidListCount; Index++) {
    CopyGuid (&Guids[NtGuidCount + GuidCount + Index], mGuidList[Index].GuidId);
  }

  return EFI_SUCCESS;
}
