/** @file
  BDS Lib functions which contain all the code to connect console device

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalBdsLib.h"


/**
  Check if we need to save the EFI variable with "ConVarName" as name
  as NV type
  If ConVarName is NULL, then ASSERT().

  @param ConVarName The name of the EFI variable.

  @retval TRUE    Set the EFI variable as NV type.
  @retval FALSE   EFI variable as NV type can be set NonNV.
**/
BOOLEAN
IsNvNeed (
  IN CHAR16 *ConVarName
  )
{
  CHAR16 *Ptr;

  ASSERT (ConVarName != NULL);

  Ptr = ConVarName;

  //
  // If the variable includes "Dev" at last, we consider
  // it does not support NV attribute.
  //
  while (*Ptr != L'\0') {
    Ptr++;
  }

  if (((INTN)((UINTN)Ptr - (UINTN)ConVarName) / sizeof (CHAR16)) <= 3) {
    return TRUE;
  }

  if ((*(Ptr - 3) == 'D') && (*(Ptr - 2) == 'e') && (*(Ptr - 1) == 'v')) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/**
  Fill console handle in System Table if there are no valid console handle in.

  Firstly, check the validation of console handle in System Table. If it is invalid,
  update it by the first console device handle from EFI console variable.

  @param  VarName            The name of the EFI console variable.
  @param  ConsoleGuid        Specified Console protocol GUID.
  @param  ConsoleHandle      On IN,  console handle in System Table to be checked.
                             On OUT, new console handle in system table.
  @param  ProtocolInterface  On IN,  console protocol on console handle in System Table to be checked.
                             On OUT, new console protocol on new console handle in system table.

  @retval TRUE               System Table has been updated.
  @retval FALSE              System Table hasn't been updated.

**/
BOOLEAN
UpdateSystemTableConsole (
  IN     CHAR16                          *VarName,
  IN     EFI_GUID                        *ConsoleGuid,
  IN OUT EFI_HANDLE                      *ConsoleHandle,
  IN OUT VOID                            **ProtocolInterface
  )
{
  EFI_STATUS                Status;
  UINTN                     DevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *FullDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *VarConsole;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  VOID                      *Interface;
  EFI_HANDLE                NewHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *TextOut;

  ASSERT (VarName != NULL);
  ASSERT (ConsoleHandle != NULL);
  ASSERT (ConsoleGuid != NULL);
  ASSERT (ProtocolInterface != NULL);

  if (*ConsoleHandle != NULL) {
    Status = gBS->HandleProtocol (
                   *ConsoleHandle,
                   ConsoleGuid,
                   &Interface
                   );
    if (Status == EFI_SUCCESS && Interface == *ProtocolInterface) {
      //
      // If ConsoleHandle is valid and console protocol on this handle also
      // also matched, just return.
      //
      return FALSE;
    }
  }

  //
  // Get all possible consoles device path from EFI variable
  //
  VarConsole = BdsLibGetVariableAndSize (
                VarName,
                &gEfiGlobalVariableGuid,
                &DevicePathSize
                );
  if (VarConsole == NULL) {
    //
    // If there is no any console device, just return.
    //
    return FALSE;
  }

  FullDevicePath = VarConsole;

  do {
    //
    // Check every instance of the console variable
    //
    Instance  = GetNextDevicePathInstance (&VarConsole, &DevicePathSize);
    if (Instance == NULL) {
      FreePool (FullDevicePath);
      ASSERT (FALSE);
    }

    //
    // Find console device handle by device path instance
    //
    Status = gBS->LocateDevicePath (
                   ConsoleGuid,
                   &Instance,
                   &NewHandle
                   );
    if (!EFI_ERROR (Status)) {
      //
      // Get the console protocol on this console device handle
      //
      Status = gBS->HandleProtocol (
                     NewHandle,
                     ConsoleGuid,
                     &Interface
                     );
      if (!EFI_ERROR (Status)) {
        //
        // Update new console handle in System Table.
        //
        *ConsoleHandle     = NewHandle;
        *ProtocolInterface = Interface;
        if (CompareGuid (ConsoleGuid, &gEfiSimpleTextOutProtocolGuid)) {
          //
          // If it is console out device, set console mode 80x25 if current mode is invalid.
          //
          TextOut = (EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *) Interface;
          if (TextOut->Mode->Mode == -1) {
            TextOut->SetMode (TextOut, 0);
          }
        }
        return TRUE;
      }
    }

  } while (Instance != NULL);

  //
  // No any available console devcie found.
  //
  return FALSE;
}

/**
  This function update console variable based on ConVarName, it can
  add or remove one specific console device path from the variable

  @param  ConVarName               Console related variable name, ConIn, ConOut,
                                   ErrOut.
  @param  CustomizedConDevicePath  The console device path which will be added to
                                   the console variable ConVarName, this parameter
                                   can not be multi-instance.
  @param  ExclusiveDevicePath      The console device path which will be removed
                                   from the console variable ConVarName, this
                                   parameter can not be multi-instance.

  @retval EFI_UNSUPPORTED          The added device path is same to the removed one.
  @retval EFI_SUCCESS              Success add or remove the device path from  the
                                   console variable.

**/
EFI_STATUS
EFIAPI
BdsLibUpdateConsoleVariable (
  IN  CHAR16                    *ConVarName,
  IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *VarConsole;
  UINTN                     DevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;
  UINT32                    Attributes;

  VarConsole      = NULL;
  DevicePathSize  = 0;

  //
  // Notes: check the device path point, here should check
  // with compare memory
  //
  if (CustomizedConDevicePath == ExclusiveDevicePath) {
    return EFI_UNSUPPORTED;
  }
  //
  // Delete the ExclusiveDevicePath from current default console
  //
  VarConsole = BdsLibGetVariableAndSize (
                ConVarName,
                &gEfiGlobalVariableGuid,
                &DevicePathSize
                );

  //
  // Initialize NewDevicePath
  //
  NewDevicePath  = VarConsole;

  //
  // If ExclusiveDevicePath is even the part of the instance in VarConsole, delete it.
  // In the end, NewDevicePath is the final device path.
  //
  if (ExclusiveDevicePath != NULL && VarConsole != NULL) {
      NewDevicePath = BdsLibDelPartMatchInstance (VarConsole, ExclusiveDevicePath);
  }
  //
  // Try to append customized device path to NewDevicePath.
  //
  if (CustomizedConDevicePath != NULL) {
    if (!BdsLibMatchDevicePaths (NewDevicePath, CustomizedConDevicePath)) {
      //
      // Check if there is part of CustomizedConDevicePath in NewDevicePath, delete it.
      //
      NewDevicePath = BdsLibDelPartMatchInstance (NewDevicePath, CustomizedConDevicePath);
      //
      // In the first check, the default console variable will be _ModuleEntryPoint,
      // just append current customized device path
      //
      TempNewDevicePath = NewDevicePath;
      NewDevicePath = AppendDevicePathInstance (NewDevicePath, CustomizedConDevicePath);
      if (TempNewDevicePath != NULL) {
        FreePool(TempNewDevicePath);
      }
    }
  }

  //
  // The attribute for ConInDev, ConOutDev and ErrOutDev does not include NV.
  //
  if (IsNvNeed(ConVarName)) {
    //
    // ConVarName has NV attribute.
    //
    Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE;
  } else {
    //
    // ConVarName does not have NV attribute.
    //
    Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
  }

  //
  // Finally, Update the variable of the default console by NewDevicePath
  //
  DevicePathSize = GetDevicePathSize (NewDevicePath);
  Status = SetVariableAndReportStatusCodeOnError (
             ConVarName,
             &gEfiGlobalVariableGuid,
             Attributes,
             DevicePathSize,
             NewDevicePath
             );
  if ((DevicePathSize == 0) && (Status == EFI_NOT_FOUND)) {
    Status = EFI_SUCCESS;
  }

  if (VarConsole == NewDevicePath) {
    if (VarConsole != NULL) {
      FreePool(VarConsole);
    }
  } else {
    if (VarConsole != NULL) {
      FreePool(VarConsole);
    }
    if (NewDevicePath != NULL) {
      FreePool(NewDevicePath);
    }
  }

  return Status;

}


/**
  Connect the console device base on the variable ConVarName, if
  device path of the ConVarName is multi-instance device path and
  anyone of the instances is connected success, then this function
  will return success.
  If the handle associate with one device path node can not
  be created successfully, then still give chance to do the dispatch,
  which load the missing drivers if possible..

  @param  ConVarName               Console related variable name, ConIn, ConOut,
                                   ErrOut.

  @retval EFI_NOT_FOUND            There is not any console devices connected
                                   success
  @retval EFI_SUCCESS              Success connect any one instance of the console
                                   device path base on the variable ConVarName.

**/
EFI_STATUS
EFIAPI
BdsLibConnectConsoleVariable (
  IN  CHAR16                 *ConVarName
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *StartDevicePath;
  UINTN                     VariableSize;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *Next;
  EFI_DEVICE_PATH_PROTOCOL  *CopyOfDevicePath;
  UINTN                     Size;
  BOOLEAN                   DeviceExist;

  Status      = EFI_SUCCESS;
  DeviceExist = FALSE;

  //
  // Check if the console variable exist
  //
  StartDevicePath = BdsLibGetVariableAndSize (
                      ConVarName,
                      &gEfiGlobalVariableGuid,
                      &VariableSize
                      );
  if (StartDevicePath == NULL) {
    return EFI_UNSUPPORTED;
  }

  CopyOfDevicePath = StartDevicePath;
  do {
    //
    // Check every instance of the console variable
    //
    Instance  = GetNextDevicePathInstance (&CopyOfDevicePath, &Size);
    if (Instance == NULL) {
      FreePool (StartDevicePath);
      return EFI_UNSUPPORTED;
    }

    Next      = Instance;
    while (!IsDevicePathEndType (Next)) {
      Next = NextDevicePathNode (Next);
    }

    SetDevicePathEndNode (Next);
    //
    // Connect the USB console
    // USB console device path is a short-form device path that
    //  starts with the first element being a USB WWID
    //  or a USB Class device path
    //
    if ((DevicePathType (Instance) == MESSAGING_DEVICE_PATH) &&
       ((DevicePathSubType (Instance) == MSG_USB_CLASS_DP)
       || (DevicePathSubType (Instance) == MSG_USB_WWID_DP)
       )) {
      Status = BdsLibConnectUsbDevByShortFormDP (0xFF, Instance);
      if (!EFI_ERROR (Status)) {
        DeviceExist = TRUE;
      }
    } else {
      //
      // Connect the instance device path
      //
      Status = BdsLibConnectDevicePath (Instance);

      if (EFI_ERROR (Status)) {
        //
        // Delete the instance from the console varialbe
        //
        BdsLibUpdateConsoleVariable (ConVarName, NULL, Instance);
      } else {
        DeviceExist = TRUE;
      }
    }
    FreePool(Instance);
  } while (CopyOfDevicePath != NULL);

  FreePool (StartDevicePath);

  if (!DeviceExist) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  This function will search every simpletext device in current system,
  and make every simpletext device as pertantial console device.

**/
VOID
EFIAPI
BdsLibConnectAllConsoles (
  VOID
  )
{
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *ConDevicePath;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;

  Index         = 0;
  HandleCount   = 0;
  HandleBuffer  = NULL;
  ConDevicePath = NULL;

  //
  // Update all the console variables
  //
  gBS->LocateHandleBuffer (
          ByProtocol,
          &gEfiSimpleTextInProtocolGuid,
          NULL,
          &HandleCount,
          &HandleBuffer
          );

  for (Index = 0; Index < HandleCount; Index++) {
    gBS->HandleProtocol (
            HandleBuffer[Index],
            &gEfiDevicePathProtocolGuid,
            (VOID **) &ConDevicePath
            );
    BdsLibUpdateConsoleVariable (L"ConIn", ConDevicePath, NULL);
  }

  if (HandleBuffer != NULL) {
    FreePool(HandleBuffer);
    HandleBuffer = NULL;
  }

  gBS->LocateHandleBuffer (
          ByProtocol,
          &gEfiSimpleTextOutProtocolGuid,
          NULL,
          &HandleCount,
          &HandleBuffer
          );
  for (Index = 0; Index < HandleCount; Index++) {
    gBS->HandleProtocol (
            HandleBuffer[Index],
            &gEfiDevicePathProtocolGuid,
            (VOID **) &ConDevicePath
            );
    BdsLibUpdateConsoleVariable (L"ConOut", ConDevicePath, NULL);
    BdsLibUpdateConsoleVariable (L"ErrOut", ConDevicePath, NULL);
  }

  if (HandleBuffer != NULL) {
    FreePool(HandleBuffer);
  }

  //
  // Connect all console variables
  //
  BdsLibConnectAllDefaultConsoles ();

}

/**
  This function will connect console device base on the console
  device variable ConIn, ConOut and ErrOut.

  @retval EFI_SUCCESS              At least one of the ConIn and ConOut device have
                                   been connected success.
  @retval EFI_STATUS               Return the status of BdsLibConnectConsoleVariable ().

**/
EFI_STATUS
EFIAPI
BdsLibConnectAllDefaultConsoles (
  VOID
  )
{
  EFI_STATUS                Status;
  BOOLEAN                   SystemTableUpdated;

  //
  // Connect all default console variables
  //

  //
  // It seems impossible not to have any ConOut device on platform,
  // so we check the status here.
  //
  Status = BdsLibConnectConsoleVariable (L"ConOut");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Insert the performance probe for Console Out
  //
  PERF_START (NULL, "ConOut", "BDS", 1);
  PERF_END (NULL, "ConOut", "BDS", 0);

  //
  // Because possibly the platform is legacy free, in such case,
  // ConIn devices (Serial Port and PS2 Keyboard ) does not exist,
  // so we need not check the status.
  //
  BdsLibConnectConsoleVariable (L"ConIn");

  //
  // The _ModuleEntryPoint err out var is legal.
  //
  BdsLibConnectConsoleVariable (L"ErrOut");

  SystemTableUpdated = FALSE;
  //
  // Fill console handles in System Table if no console device assignd.
  //
  if (UpdateSystemTableConsole (L"ConIn", &gEfiSimpleTextInProtocolGuid, &gST->ConsoleInHandle, (VOID **) &gST->ConIn)) {
    SystemTableUpdated = TRUE;
  }
  if (UpdateSystemTableConsole (L"ConOut", &gEfiSimpleTextOutProtocolGuid, &gST->ConsoleOutHandle, (VOID **) &gST->ConOut)) {
    SystemTableUpdated = TRUE;
  }
  if (UpdateSystemTableConsole (L"ErrOut", &gEfiSimpleTextOutProtocolGuid, &gST->StandardErrorHandle, (VOID **) &gST->StdErr)) {
    SystemTableUpdated = TRUE;
  }

  if (SystemTableUpdated) {
    //
    // Update the CRC32 in the EFI System Table header
    //
    gST->Hdr.CRC32 = 0;
    gBS->CalculateCrc32 (
          (UINT8 *) &gST->Hdr,
          gST->Hdr.HeaderSize,
          &gST->Hdr.CRC32
          );
  }

  return EFI_SUCCESS;

}

/**
  This function will connect console device except ConIn base on the console
  device variable  ConOut and ErrOut.

  @retval EFI_SUCCESS              At least one of the ConOut device have
                                   been connected success.
  @retval EFI_STATUS               Return the status of BdsLibConnectConsoleVariable ().

**/
EFI_STATUS
EFIAPI
BdsLibConnectAllDefaultConsolesWithOutConIn (
  VOID
  )
{
  EFI_STATUS                Status;
  BOOLEAN                   SystemTableUpdated;

  //
  // Connect all default console variables except ConIn
  //

  //
  // It seems impossible not to have any ConOut device on platform,
  // so we check the status here.
  //
  Status = BdsLibConnectConsoleVariable (L"ConOut");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Insert the performance probe for Console Out
  //
  PERF_START (NULL, "ConOut", "BDS", 1);
  PERF_END (NULL, "ConOut", "BDS", 0);

  //
  // The _ModuleEntryPoint err out var is legal.
  //
  BdsLibConnectConsoleVariable (L"ErrOut");

  SystemTableUpdated = FALSE;
  //
  // Fill console handles in System Table if no console device assignd.
  //
  if (UpdateSystemTableConsole (L"ConOut", &gEfiSimpleTextOutProtocolGuid, &gST->ConsoleOutHandle, (VOID **) &gST->ConOut)) {
    SystemTableUpdated = TRUE;
  }
  if (UpdateSystemTableConsole (L"ErrOut", &gEfiSimpleTextOutProtocolGuid, &gST->StandardErrorHandle, (VOID **) &gST->StdErr)) {
    SystemTableUpdated = TRUE;
  }

  if (SystemTableUpdated) {
    //
    // Update the CRC32 in the EFI System Table header
    //
    gST->Hdr.CRC32 = 0;
    gBS->CalculateCrc32 (
          (UINT8 *) &gST->Hdr,
          gST->Hdr.HeaderSize,
          &gST->Hdr.CRC32
          );
  }

  return EFI_SUCCESS;

}

/**
  Use SystemTable Conout to stop video based Simple Text Out consoles from going
  to the video device. Put up LogoFile on every video device that is a console.

  @param[in]  LogoFile   File name of logo to display on the center of the screen.

  @retval EFI_SUCCESS     ConsoleControl has been flipped to graphics and logo displayed.
  @retval EFI_UNSUPPORTED Logo not found

**/
EFI_STATUS
EFIAPI
EnableQuietBoot (
  IN  EFI_GUID  *LogoFile
  )
{
  EFI_STATUS                    Status;
  EFI_OEM_BADGING_PROTOCOL      *Badging;
  UINT32                        SizeOfX;
  UINT32                        SizeOfY;
  INTN                          DestX;
  INTN                          DestY;
  UINT8                         *ImageData;
  UINTN                         ImageSize;
  UINTN                         BltSize;
  UINT32                        Instance;
  EFI_BADGING_FORMAT            Format;
  EFI_BADGING_DISPLAY_ATTRIBUTE Attribute;
  UINTN                         CoordinateX;
  UINTN                         CoordinateY;
  UINTN                         Height;
  UINTN                         Width;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  EFI_UGA_DRAW_PROTOCOL         *UgaDraw;
  UINT32                        ColorDepth;
  UINT32                        RefreshRate;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
  EFI_BOOT_LOGO_PROTOCOL        *BootLogo;
  UINTN                         NumberOfLogos;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *LogoBlt;
  UINTN                         LogoDestX;
  UINTN                         LogoDestY;
  UINTN                         LogoHeight;
  UINTN                         LogoWidth;
  UINTN                         NewDestX;
  UINTN                         NewDestY;
  UINTN                         NewHeight;
  UINTN                         NewWidth;
  UINT64                        BufferSize;

  UgaDraw = NULL;
  //
  // Try to open GOP first
  //
  Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **) &GraphicsOutput);
  if (EFI_ERROR (Status) && FeaturePcdGet (PcdUgaConsumeSupport)) {
    GraphicsOutput = NULL;
    //
    // Open GOP failed, try to open UGA
    //
    Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiUgaDrawProtocolGuid, (VOID **) &UgaDraw);
  }
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Try to open Boot Logo Protocol.
  //
  BootLogo = NULL;
  gBS->LocateProtocol (&gEfiBootLogoProtocolGuid, NULL, (VOID **) &BootLogo);

  //
  // Erase Cursor from screen
  //
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  Badging = NULL;
  Status  = gBS->LocateProtocol (&gEfiOEMBadgingProtocolGuid, NULL, (VOID **) &Badging);

  if (GraphicsOutput != NULL) {
    SizeOfX = GraphicsOutput->Mode->Info->HorizontalResolution;
    SizeOfY = GraphicsOutput->Mode->Info->VerticalResolution;

  } else if (UgaDraw != NULL && FeaturePcdGet (PcdUgaConsumeSupport)) {
    Status = UgaDraw->GetMode (UgaDraw, &SizeOfX, &SizeOfY, &ColorDepth, &RefreshRate);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  } else {
    return EFI_UNSUPPORTED;
  }

  Blt = NULL;
  NumberOfLogos = 0;
  LogoDestX = 0;
  LogoDestY = 0;
  LogoHeight = 0;
  LogoWidth = 0;
  NewDestX = 0;
  NewDestY = 0;
  NewHeight = 0;
  NewWidth = 0;
  Instance = 0;
  while (1) {
    ImageData = NULL;
    ImageSize = 0;

    if (Badging != NULL) {
      //
      // Get image from OEMBadging protocol.
      //
      Status = Badging->GetImage (
                          Badging,
                          &Instance,
                          &Format,
                          &ImageData,
                          &ImageSize,
                          &Attribute,
                          &CoordinateX,
                          &CoordinateY
                          );
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      //
      // Currently only support BMP format.
      //
      if (Format != EfiBadgingFormatBMP) {
        if (ImageData != NULL) {
          FreePool (ImageData);
        }
        continue;
      }
    } else {
      //
      // Get the specified image from FV.
      //
      Status = GetSectionFromAnyFv (LogoFile, EFI_SECTION_RAW, 0, (VOID **) &ImageData, &ImageSize);
      if (EFI_ERROR (Status)) {
        return EFI_UNSUPPORTED;
      }

      CoordinateX = 0;
      CoordinateY = 0;
      if (!FeaturePcdGet(PcdBootlogoOnlyEnable)) {
        Attribute   = EfiBadgingDisplayAttributeCenter;
      } else {
        Attribute   = EfiBadgingDisplayAttributeCustomized;
      }
    }

    if (Blt != NULL) {
      FreePool (Blt);
    }
    Blt = NULL;
    Status = TranslateBmpToGopBlt (
              ImageData,
              ImageSize,
              &Blt,
              &BltSize,
              &Height,
              &Width
              );
    if (EFI_ERROR (Status)) {
      FreePool (ImageData);

      if (Badging == NULL) {
        return Status;
      } else {
        continue;
      }
    }

    //
    // Calculate the display position according to Attribute.
    //
    switch (Attribute) {
    case EfiBadgingDisplayAttributeLeftTop:
      DestX = CoordinateX;
      DestY = CoordinateY;
      break;

    case EfiBadgingDisplayAttributeCenterTop:
      DestX = (SizeOfX - Width) / 2;
      DestY = CoordinateY;
      break;

    case EfiBadgingDisplayAttributeRightTop:
      DestX = (SizeOfX - Width - CoordinateX);
      DestY = CoordinateY;;
      break;

    case EfiBadgingDisplayAttributeCenterRight:
      DestX = (SizeOfX - Width - CoordinateX);
      DestY = (SizeOfY - Height) / 2;
      break;

    case EfiBadgingDisplayAttributeRightBottom:
      DestX = (SizeOfX - Width - CoordinateX);
      DestY = (SizeOfY - Height - CoordinateY);
      break;

    case EfiBadgingDisplayAttributeCenterBottom:
      DestX = (SizeOfX - Width) / 2;
      DestY = (SizeOfY - Height - CoordinateY);
      break;

    case EfiBadgingDisplayAttributeLeftBottom:
      DestX = CoordinateX;
      DestY = (SizeOfY - Height - CoordinateY);
      break;

    case EfiBadgingDisplayAttributeCenterLeft:
      DestX = CoordinateX;
      DestY = (SizeOfY - Height) / 2;
      break;

    case EfiBadgingDisplayAttributeCenter:
      DestX = (SizeOfX - Width) / 2;
      DestY = (SizeOfY - Height) / 2;
      break;

    case EfiBadgingDisplayAttributeCustomized:
      DestX = (SizeOfX - Width) / 2;
      DestY = ((SizeOfY * 382) / 1000) - Height / 2;
      break;

    default:
      DestX = CoordinateX;
      DestY = CoordinateY;
      break;
    }

    if ((DestX >= 0) && (DestY >= 0)) {
      if (GraphicsOutput != NULL) {
        Status = GraphicsOutput->Blt (
                            GraphicsOutput,
                            Blt,
                            EfiBltBufferToVideo,
                            0,
                            0,
                            (UINTN) DestX,
                            (UINTN) DestY,
                            Width,
                            Height,
                            Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                            );
      } else if (UgaDraw != NULL && FeaturePcdGet (PcdUgaConsumeSupport)) {
        Status = UgaDraw->Blt (
                            UgaDraw,
                            (EFI_UGA_PIXEL *) Blt,
                            EfiUgaBltBufferToVideo,
                            0,
                            0,
                            (UINTN) DestX,
                            (UINTN) DestY,
                            Width,
                            Height,
                            Width * sizeof (EFI_UGA_PIXEL)
                            );
      } else {
        Status = EFI_UNSUPPORTED;
      }

      //
      // Report displayed Logo information.
      //
      if (!EFI_ERROR (Status)) {
        NumberOfLogos++;

        if (LogoWidth == 0) {
          //
          // The first Logo.
          //
          LogoDestX = (UINTN) DestX;
          LogoDestY = (UINTN) DestY;
          LogoWidth = Width;
          LogoHeight = Height;
        } else {
          //
          // Merge new logo with old one.
          //
          NewDestX = MIN ((UINTN) DestX, LogoDestX);
          NewDestY = MIN ((UINTN) DestY, LogoDestY);
          NewWidth = MAX ((UINTN) DestX + Width, LogoDestX + LogoWidth) - NewDestX;
          NewHeight = MAX ((UINTN) DestY + Height, LogoDestY + LogoHeight) - NewDestY;

          LogoDestX = NewDestX;
          LogoDestY = NewDestY;
          LogoWidth = NewWidth;
          LogoHeight = NewHeight;
        }
      }
    }

    FreePool (ImageData);

    if (Badging == NULL) {
      break;
    }
  }

Done:
  if (BootLogo == NULL || NumberOfLogos == 0) {
    //
    // No logo displayed.
    //
    if (Blt != NULL) {
      FreePool (Blt);
    }

    return Status;
  }

  //
  // Advertise displayed Logo information.
  //
  if (NumberOfLogos == 1) {
    //
    // Only one logo displayed, use its Blt buffer directly for BootLogo protocol.
    //
    LogoBlt = Blt;
    Status = EFI_SUCCESS;
  } else {
    //
    // More than one Logo displayed, get merged BltBuffer using VideoToBuffer operation.
    //
    if (Blt != NULL) {
      FreePool (Blt);
    }

    //
    // Ensure the LogoHeight * LogoWidth doesn't overflow
    //
    if (LogoHeight > DivU64x64Remainder ((UINTN) ~0, LogoWidth, NULL)) {
      return EFI_UNSUPPORTED;
    }
    BufferSize = MultU64x64 (LogoWidth, LogoHeight);

    //
    // Ensure the BufferSize * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) doesn't overflow
    //
    if (BufferSize > DivU64x32 ((UINTN) ~0, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL))) {
      return EFI_UNSUPPORTED;
    }

    LogoBlt = AllocateZeroPool ((UINTN)BufferSize * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    if (LogoBlt == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->Blt (
                          GraphicsOutput,
                          LogoBlt,
                          EfiBltVideoToBltBuffer,
                          LogoDestX,
                          LogoDestY,
                          0,
                          0,
                          LogoWidth,
                          LogoHeight,
                          LogoWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                          );
    } else if (UgaDraw != NULL && FeaturePcdGet (PcdUgaConsumeSupport)) {
      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *) LogoBlt,
                          EfiUgaVideoToBltBuffer,
                          LogoDestX,
                          LogoDestY,
                          0,
                          0,
                          LogoWidth,
                          LogoHeight,
                          LogoWidth * sizeof (EFI_UGA_PIXEL)
                          );
    } else {
      Status = EFI_UNSUPPORTED;
    }
  }

  if (!EFI_ERROR (Status)) {
    BootLogo->SetBootLogo (BootLogo, LogoBlt, LogoDestX, LogoDestY, LogoWidth, LogoHeight);
  }
  FreePool (LogoBlt);

  return Status;
}

/**
  Use SystemTable Conout to turn on video based Simple Text Out consoles. The
  Simple Text Out screens will now be synced up with all non video output devices

  @retval EFI_SUCCESS     UGA devices are back in text mode and synced up.

**/
EFI_STATUS
EFIAPI
DisableQuietBoot (
  VOID
  )
{

  //
  // Enable Cursor on Screen
  //
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);
  return EFI_SUCCESS;
}

