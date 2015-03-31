/** @file
  ConsoleOut Routines that speak VGA.

Copyright (c) 2007 - 2013, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "FbGop.h"

EFI_PIXEL_BITMASK  mPixelBitMask = {0x0000FF, 0x00FF00, 0xFF0000, 0x000000};

//
// Save controller attributes during first start
//
UINT64                         mOriginalPciAttributes;
BOOLEAN                        mPciAttributesSaved = FALSE;


//
// EFI Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL gFbGopDriverBinding = {
  FbGopDriverBindingSupported,
  FbGopDriverBindingStart,
  FbGopDriverBindingStop,
  0x3,
  NULL,
  NULL
};

//
// Native resolution in EDID DetailedTiming[0]
//
UINT32    mNativeModeHorizontal;
UINT32    mNativeModeVertical;

/**
  Supported.

  @param  This                   Pointer to driver binding protocol
  @param  Controller             Controller handle to connect
  @param  RemainingDevicePath    A pointer to the remaining portion of a device
                                 path

  @retval EFI_STATUS             EFI_SUCCESS:This controller can be managed by this
                                 driver, Otherwise, this controller cannot be
                                 managed by this driver

**/
EFI_STATUS
EFIAPI
FbGopDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;
  EFI_DEV_PATH              *Node;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // See if this is a PCI Graphics Controller by looking at the Command register and
  // Class Code Register
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (Pci) / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status = EFI_UNSUPPORTED;
  if (Pci.Hdr.ClassCode[2] == 0x03 || (Pci.Hdr.ClassCode[2] == 0x00 && Pci.Hdr.ClassCode[1] == 0x01)) {

    Status = EFI_SUCCESS;
    //
    // If this is a graphics controller,
    // go further check RemainingDevicePath validation
    //
    if (RemainingDevicePath != NULL) {
      Node = (EFI_DEV_PATH *) RemainingDevicePath;
      //
      // Check if RemainingDevicePath is the End of Device Path Node, 
      // if yes, return EFI_SUCCESS
      //
      if (!IsDevicePathEnd (Node)) {
        //
        // If RemainingDevicePath isn't the End of Device Path Node,
        // check its validation
        //
        if (Node->DevPath.Type != ACPI_DEVICE_PATH ||
            Node->DevPath.SubType != ACPI_ADR_DP ||
            DevicePathNodeLength(&Node->DevPath) < sizeof(ACPI_ADR_DEVICE_PATH)) {
          Status = EFI_UNSUPPORTED;
        }
      }
    }
  }

Done:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}


/**
  Install Graphics Output Protocol onto VGA device handles.

  @param  This                   Pointer to driver binding protocol
  @param  Controller             Controller handle to connect
  @param  RemainingDevicePath    A pointer to the remaining portion of a device
                                 path

  @return EFI_STATUS

**/
EFI_STATUS
EFIAPI
FbGopDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_PCI_IO_PROTOCOL       *PciIo;  
  UINT64                    Supports;

  DEBUG ((EFI_D_INFO, "GOP START\n"));
  
  //
  // Initialize local variables
  //
  PciIo            = NULL;
  ParentDevicePath = NULL;

  //
  // Prepare for status code
  //
  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the IO Abstraction(s) needed
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  //
  // Save original PCI attributes
  //
  if (!mPciAttributesSaved) {
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationGet,
                      0,
                      &mOriginalPciAttributes
                      );
    
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    mPciAttributesSaved = TRUE;
  }

  //
  // Get supported PCI attributes
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Supports &= (EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_IO_16);
  if (Supports == 0 || Supports == (EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_IO_16)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }  
  
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_PC_ENABLE,
    ParentDevicePath
    );
  //
  // Enable the device and make sure VGA cycles are being forwarded to this VGA device
  //
  Status = PciIo->Attributes (
             PciIo,
             EfiPciIoAttributeOperationEnable,
             EFI_PCI_DEVICE_ENABLE,
             NULL
             );
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_RESOURCE_CONFLICT,
      ParentDevicePath
      );
    goto Done;
  }

  if (RemainingDevicePath != NULL) {
    if (IsDevicePathEnd (RemainingDevicePath)) {
      //
      // If RemainingDevicePath is the End of Device Path Node,
      // don't create any child device and return EFI_SUCESS
      Status = EFI_SUCCESS;
      goto Done;
    }
  }
  
  //
  // Create child handle and install GraphicsOutputProtocol on it
  //
  Status = FbGopChildHandleInstall (
             This,
             Controller,
             PciIo,
             NULL,
             ParentDevicePath,
             RemainingDevicePath
             );
  
Done:
  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
          
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_PROGRESS_CODE,
      EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_PC_DISABLE,
      ParentDevicePath
      );

    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_PROGRESS_CODE,
      EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_NOT_DETECTED,
      ParentDevicePath
      );
    if (!HasChildHandle (Controller)) {
      if (mPciAttributesSaved) {
        //
        // Restore original PCI attributes
        //
        PciIo->Attributes (
                        PciIo,
                        EfiPciIoAttributeOperationSet,
                        mOriginalPciAttributes,
                        NULL
                        );
      }
    }
    //
    // Release PCI I/O Protocols on the controller handle.
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }

  return Status;
}


/**
  Stop.

  @param  This                   Pointer to driver binding protocol
  @param  Controller             Controller handle to connect
  @param  NumberOfChildren       Number of children handle created by this driver
  @param  ChildHandleBuffer      Buffer containing child handle created

  @retval EFI_SUCCESS            Driver disconnected successfully from controller
  @retval EFI_UNSUPPORTED        Cannot find FB_VIDEO_DEV structure

**/
EFI_STATUS
EFIAPI
FbGopDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                   Status;
  BOOLEAN                      AllChildrenStopped;
  UINTN                        Index;
  EFI_PCI_IO_PROTOCOL          *PciIo;

  AllChildrenStopped = TRUE;

  if (NumberOfChildren == 0) {
    //
    // Close PCI I/O protocol on the controller handle
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    return EFI_SUCCESS;
  }

  for (Index = 0; Index < NumberOfChildren; Index++) {
        
    Status = EFI_SUCCESS;

    FbGopChildHandleUninstall (This, Controller, ChildHandleBuffer[Index]);

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  if (!HasChildHandle (Controller)) {
    if (mPciAttributesSaved) {
      Status = gBS->HandleProtocol (
                      Controller,
                      &gEfiPciIoProtocolGuid,
                      (VOID **) &PciIo
                      );
      ASSERT_EFI_ERROR (Status);
      
      //
      // Restore original PCI attributes
      //
      Status = PciIo->Attributes (
                        PciIo,
                        EfiPciIoAttributeOperationSet,
                        mOriginalPciAttributes,
                        NULL
                        );
      ASSERT_EFI_ERROR (Status);
    }
  }


  return EFI_SUCCESS;
}


/**
  Install child handles if the Handle supports MBR format.

  @param  This                   Calling context.
  @param  ParentHandle           Parent Handle
  @param  ParentPciIo            Parent PciIo interface
  @param  ParentLegacyBios       Parent LegacyBios interface
  @param  ParentDevicePath       Parent Device Path
  @param  RemainingDevicePath    Remaining Device Path

  @retval EFI_SUCCESS            If a child handle was added
  @retval other                  A child handle was not added

**/
EFI_STATUS
FbGopChildHandleInstall (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ParentHandle,
  IN  EFI_PCI_IO_PROTOCOL          *ParentPciIo,
  IN  VOID                         *ParentLegacyBios,
  IN  EFI_DEVICE_PATH_PROTOCOL     *ParentDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS               Status;
  FB_VIDEO_DEV            *FbGopPrivate;
  PCI_TYPE00               Pci;
  ACPI_ADR_DEVICE_PATH     AcpiDeviceNode;
  BOOLEAN                  ProtocolInstalled;

  //
  // Allocate the private device structure for video device
  //
  FbGopPrivate = (FB_VIDEO_DEV *) AllocateZeroPool (
																					sizeof (FB_VIDEO_DEV)
																					);
  if (NULL == FbGopPrivate) {
		Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // See if this is a VGA compatible controller or not
  //
  Status = ParentPciIo->Pci.Read (
                          ParentPciIo,
                          EfiPciIoWidthUint32,
                          0,
                          sizeof (Pci) / sizeof (UINT32),
                          &Pci
                          );
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_CONTROLLER_ERROR,
      ParentDevicePath
      );
    goto Done;
  }
  
  //
  // Initialize the child private structure
  //
  FbGopPrivate->Signature = FB_VIDEO_DEV_SIGNATURE;

  //
  // Fill in Graphics Output specific mode structures
  //  
  FbGopPrivate->ModeData              = NULL;
    
  FbGopPrivate->VbeFrameBuffer        = NULL;

  FbGopPrivate->EdidDiscovered.SizeOfEdid  = 0;
  FbGopPrivate->EdidDiscovered.Edid        = NULL;
  FbGopPrivate->EdidActive.SizeOfEdid      = 0;
  FbGopPrivate->EdidActive.Edid            = NULL;
  
  //
  // Fill in the Graphics Output Protocol
  //
  FbGopPrivate->GraphicsOutput.QueryMode = FbGopGraphicsOutputQueryMode;
  FbGopPrivate->GraphicsOutput.SetMode   = FbGopGraphicsOutputSetMode;


  //
  // Allocate buffer for Graphics Output Protocol mode information
  //
  FbGopPrivate->GraphicsOutput.Mode = (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *) AllocatePool (
                                             sizeof (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE)
                                             );
  if (NULL == FbGopPrivate->GraphicsOutput.Mode) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  FbGopPrivate->GraphicsOutput.Mode->Info = (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *) AllocatePool (
                                             sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION)
                                             );
  if (NULL ==  FbGopPrivate->GraphicsOutput.Mode->Info) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Set Gop Device Path, here RemainingDevicePath will not be one End of Device Path Node.
  //
  if ((RemainingDevicePath == NULL) || (!IsDevicePathEnd (RemainingDevicePath))) {
    if (RemainingDevicePath == NULL) {
      ZeroMem (&AcpiDeviceNode, sizeof (ACPI_ADR_DEVICE_PATH));
      AcpiDeviceNode.Header.Type = ACPI_DEVICE_PATH;
      AcpiDeviceNode.Header.SubType = ACPI_ADR_DP;
      AcpiDeviceNode.ADR = ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, 0, 0);
      SetDevicePathNodeLength (&AcpiDeviceNode.Header, sizeof (ACPI_ADR_DEVICE_PATH));
    
      FbGopPrivate->GopDevicePath = AppendDevicePathNode (
                                          ParentDevicePath,
                                          (EFI_DEVICE_PATH_PROTOCOL *) &AcpiDeviceNode
                                          );
    } else {
      FbGopPrivate->GopDevicePath = AppendDevicePathNode (ParentDevicePath, RemainingDevicePath);
    }
    
    //
    // Creat child handle and device path protocol firstly
    //
    FbGopPrivate->Handle = NULL;
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &FbGopPrivate->Handle,
                    &gEfiDevicePathProtocolGuid,
                    FbGopPrivate->GopDevicePath,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }
  
  //
  // When check for VBE, PCI I/O protocol is needed, so use parent's protocol interface temporally
  //
  FbGopPrivate->PciIo                 = ParentPciIo;

  //
  // Check for VESA BIOS Extensions for modes that are compatible with Graphics Output
  //
  Status = FbGopCheckForVbe (FbGopPrivate);
  DEBUG ((EFI_D_INFO, "FbGopCheckForVbe - %r\n", Status));
  
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    //goto Done;    
  }

  ProtocolInstalled = FALSE;
  
  //
  // Creat child handle and install Graphics Output Protocol,EDID Discovered/Active Protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &FbGopPrivate->Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  &FbGopPrivate->GraphicsOutput,  
                  &gEfiEdidDiscoveredProtocolGuid,
                  &FbGopPrivate->EdidDiscovered,                                                    
                  &gEfiEdidActiveProtocolGuid,
                  &FbGopPrivate->EdidActive,                  
                  NULL
                  );

  if (!EFI_ERROR (Status)) {
    //
    // Open the Parent Handle for the child
    //
    Status = gBS->OpenProtocol (
                    ParentHandle,
                    &gEfiPciIoProtocolGuid,
                    (VOID **) &FbGopPrivate->PciIo,
                    This->DriverBindingHandle,
                    FbGopPrivate->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    ProtocolInstalled = TRUE;
  }
  
Done:
  if (EFI_ERROR (Status)) {
    //
    // Free private data structure
    //
    FbGopDeviceReleaseResource (FbGopPrivate);
  }

  return Status;
}


/**
  Deregister an video child handle and free resources.

  @param  This                   Protocol instance pointer.
  @param  Controller             Video controller handle
  @param  Handle                 Video child handle

  @return EFI_STATUS

**/
EFI_STATUS
FbGopChildHandleUninstall (
  EFI_DRIVER_BINDING_PROTOCOL    *This,
  EFI_HANDLE                     Controller,
  EFI_HANDLE                     Handle
  )
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput;  
  FB_VIDEO_DEV                 *FbGopPrivate;
  EFI_PCI_IO_PROTOCOL          *PciIo;

  FbGopPrivate     = NULL;
  GraphicsOutput   = NULL;
  PciIo            = NULL;
  Status           = EFI_UNSUPPORTED;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
      FbGopPrivate = FB_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (GraphicsOutput);
  }
  
  if (FbGopPrivate == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Close PCI I/O protocol that opened by child handle
  //
  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  This->DriverBindingHandle,
                  Handle
                  );

  //
  // Uninstall protocols on child handle
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                    FbGopPrivate->Handle,
                    &gEfiDevicePathProtocolGuid,
                    FbGopPrivate->GopDevicePath,
                    &gEfiGraphicsOutputProtocolGuid,
                    &FbGopPrivate->GraphicsOutput,
                    NULL
                    );
 
  if (EFI_ERROR (Status)) {
    gBS->OpenProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           (VOID **) &PciIo,
           This->DriverBindingHandle,
           Handle,
           EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
           );
    return Status;
  }
  
  //
  // Release all allocated resources
  //
  FbGopDeviceReleaseResource (FbGopPrivate);

  return EFI_SUCCESS;
}


/**
  Release resource for biso video instance.

  @param  FbGopPrivate       Video child device private data structure

**/
VOID
FbGopDeviceReleaseResource (
  FB_VIDEO_DEV  *FbGopPrivate
  )
{
  if (FbGopPrivate == NULL) {
    return ;
  }

  //
  // Release all the resourses occupied by the FB_VIDEO_DEV
  //
  
  //
  // Free VBE Frame Buffer
  //
  if (FbGopPrivate->VbeFrameBuffer != NULL) {
    FreePool (FbGopPrivate->VbeFrameBuffer);
  }
  
  //
  // Free mode data
  //
  if (FbGopPrivate->ModeData != NULL) {
    FreePool (FbGopPrivate->ModeData);
  }  

  //
  // Free graphics output protocol occupied resource
  //
  if (FbGopPrivate->GraphicsOutput.Mode != NULL) {
    if (FbGopPrivate->GraphicsOutput.Mode->Info != NULL) {
        FreePool (FbGopPrivate->GraphicsOutput.Mode->Info);
        FbGopPrivate->GraphicsOutput.Mode->Info = NULL;
    }
    FreePool (FbGopPrivate->GraphicsOutput.Mode);
    FbGopPrivate->GraphicsOutput.Mode = NULL;
  }  

  if (FbGopPrivate->GopDevicePath!= NULL) {
    FreePool (FbGopPrivate->GopDevicePath);
  }

  FreePool (FbGopPrivate);

  return ;
}



/**
  Check if all video child handles have been uninstalled.

  @param  Controller             Video controller handle

  @return TRUE                   Child handles exist.
  @return FALSE                  All video child handles have been uninstalled.

**/
BOOLEAN
HasChildHandle (
  IN EFI_HANDLE  Controller
  )
{
  UINTN                                Index;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfoBuffer;
  UINTN                                EntryCount;
  BOOLEAN                              HasChild;
  EFI_STATUS                           Status;

  EntryCount = 0;
  HasChild   = FALSE;
  Status = gBS->OpenProtocolInformation (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  &OpenInfoBuffer,
                  &EntryCount
                  );
  for (Index = 0; Index < EntryCount; Index++) {
    if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
      HasChild = TRUE;
    }
  }
  
  return HasChild;
}

/**
  Check for VBE device.

  @param  FbGopPrivate       Pointer to FB_VIDEO_DEV structure

  @retval EFI_SUCCESS            VBE device found

**/
EFI_STATUS
FbGopCheckForVbe (
  IN OUT FB_VIDEO_DEV  *FbGopPrivate
  )
{
  EFI_STATUS                             Status;  
  FB_VIDEO_MODE_DATA                     *ModeBuffer;
  FB_VIDEO_MODE_DATA                     *CurrentModeData;  
  UINTN                                  ModeNumber;  
  UINTN                                  BitsPerPixel; 
  UINTN                                  BytesPerScanLine;
  UINT32                                 HorizontalResolution;
  UINT32                                 VerticalResolution;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL          *VbeFrameBuffer;  
  EFI_HOB_GUID_TYPE                      *GuidHob;
  FRAME_BUFFER_INFO                      *pFbInfo;
  
  Status = EFI_SUCCESS;
  //
  // Find the frame buffer information guid hob
  //
  GuidHob = GetFirstGuidHob (&gUefiFrameBufferInfoGuid);
  ASSERT (GuidHob != NULL);
  pFbInfo = (FRAME_BUFFER_INFO *)GET_GUID_HOB_DATA (GuidHob);
  
  //
  // Add mode to the list of available modes
  //
  VbeFrameBuffer = NULL;
  ModeBuffer     = NULL;
  
  ModeNumber           = 1;  
  BitsPerPixel         = pFbInfo->BitsPerPixel;
  HorizontalResolution = pFbInfo->HorizontalResolution;
  VerticalResolution   = pFbInfo->VerticalResolution;
  BytesPerScanLine     = HorizontalResolution * (BitsPerPixel / 8);
  
  ModeBuffer = (FB_VIDEO_MODE_DATA *) AllocatePool (
																						ModeNumber * sizeof (FB_VIDEO_MODE_DATA)
																			);
  if (NULL == ModeBuffer) {
	  Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  VbeFrameBuffer =
			(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) AllocatePool (
																					BytesPerScanLine * VerticalResolution
																				  );
  if (NULL == VbeFrameBuffer) {
	  Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }
       
  if (FbGopPrivate->ModeData != NULL) {
    FreePool (FbGopPrivate->ModeData);
  }
    
  if (FbGopPrivate->VbeFrameBuffer != NULL) {
    FreePool (FbGopPrivate->VbeFrameBuffer);
  }  
  
  CurrentModeData = &ModeBuffer[ModeNumber - 1];
  CurrentModeData->BytesPerScanLine = (UINT16)BytesPerScanLine;
  
  CurrentModeData->Red      = *(FB_VIDEO_COLOR_PLACEMENT *)&(pFbInfo->Red);  
  CurrentModeData->Blue     = *(FB_VIDEO_COLOR_PLACEMENT *)&(pFbInfo->Blue);    
  CurrentModeData->Green    = *(FB_VIDEO_COLOR_PLACEMENT *)&(pFbInfo->Green);    
  CurrentModeData->Reserved = *(FB_VIDEO_COLOR_PLACEMENT *)&(pFbInfo->Reserved);  
  
  CurrentModeData->BitsPerPixel    = (UINT32)BitsPerPixel;
  CurrentModeData->HorizontalResolution = HorizontalResolution;
  CurrentModeData->VerticalResolution   = VerticalResolution;  
  CurrentModeData->FrameBufferSize = CurrentModeData->BytesPerScanLine * CurrentModeData->VerticalResolution;    
  CurrentModeData->LinearFrameBuffer = (VOID *) (UINTN) pFbInfo->LinearFrameBuffer;
  CurrentModeData->VbeModeNumber        = 0;
  CurrentModeData->ColorDepth           = 32;
  CurrentModeData->RefreshRate          = 60;
  
  CurrentModeData->PixelFormat = PixelBitMask;
  if ((CurrentModeData->BitsPerPixel == 32) &&
      (CurrentModeData->Red.Mask == 0xff) && (CurrentModeData->Green.Mask == 0xff) && (CurrentModeData->Blue.Mask == 0xff)) {
    if ((CurrentModeData->Red.Position == 0) && (CurrentModeData->Green.Position == 8) && (CurrentModeData->Blue.Position == 16)) {
      CurrentModeData->PixelFormat = PixelRedGreenBlueReserved8BitPerColor;
    } else if ((CurrentModeData->Blue.Position == 0) && (CurrentModeData->Green.Position == 8) && (CurrentModeData->Red.Position == 16)) {
      CurrentModeData->PixelFormat = PixelBlueGreenRedReserved8BitPerColor;
    }
  }  
  
  CopyMem (&(CurrentModeData->PixelBitMask), &mPixelBitMask, sizeof (EFI_PIXEL_BITMASK));    
          
  FbGopPrivate->ModeData       = ModeBuffer;
  FbGopPrivate->VbeFrameBuffer = VbeFrameBuffer;
  
  //
  // Assign Gop's Blt function
  //
  FbGopPrivate->GraphicsOutput.Blt     = FbGopGraphicsOutputVbeBlt;
  
  FbGopPrivate->GraphicsOutput.Mode->MaxMode = 1;
  FbGopPrivate->GraphicsOutput.Mode->Mode    = 0;      
  FbGopPrivate->GraphicsOutput.Mode->Info->Version = 0;
  FbGopPrivate->GraphicsOutput.Mode->Info->HorizontalResolution = HorizontalResolution;
  FbGopPrivate->GraphicsOutput.Mode->Info->VerticalResolution   = VerticalResolution;   
  FbGopPrivate->GraphicsOutput.Mode->Info->PixelFormat = CurrentModeData->PixelFormat;
  CopyMem (&(FbGopPrivate->GraphicsOutput.Mode->Info->PixelInformation), &mPixelBitMask, sizeof (EFI_PIXEL_BITMASK));  
  FbGopPrivate->GraphicsOutput.Mode->Info->PixelsPerScanLine = HorizontalResolution;    
  FbGopPrivate->GraphicsOutput.Mode->SizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  FbGopPrivate->GraphicsOutput.Mode->FrameBufferBase = (EFI_PHYSICAL_ADDRESS) (UINTN) CurrentModeData->LinearFrameBuffer;
  FbGopPrivate->GraphicsOutput.Mode->FrameBufferSize =  CurrentModeData->FrameBufferSize;
  
  //
  // Find the best mode to initialize
  //  

Done:
  //
  // If there was an error, then free the mode structure
  //
  if (EFI_ERROR (Status)) {
      
    if (VbeFrameBuffer != NULL) {
      FreePool (VbeFrameBuffer);
    }    
    
    if (ModeBuffer != NULL) {
      FreePool (ModeBuffer);
    }    
  }

  return Status;
}


//
// Graphics Output Protocol Member Functions for VESA BIOS Extensions
//

/**
  Graphics Output protocol interface to get video mode.

  @param  This                   Protocol instance pointer.
  @param  ModeNumber             The mode number to return information on.
  @param  SizeOfInfo             A pointer to the size, in bytes, of the Info
                                 buffer.
  @param  Info                   Caller allocated buffer that returns information
                                 about ModeNumber.

  @retval EFI_SUCCESS            Mode information returned.
  @retval EFI_DEVICE_ERROR       A hardware error occurred trying to retrieve the
                                 video mode.
  @retval EFI_NOT_STARTED        Video display is not initialized. Call SetMode ()
  @retval EFI_INVALID_PARAMETER  One of the input args was NULL.

**/
EFI_STATUS
EFIAPI
FbGopGraphicsOutputQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
{
  FB_VIDEO_DEV        *FbGopPrivate;
  FB_VIDEO_MODE_DATA  *ModeData;

  FbGopPrivate = FB_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (This);

  if (This == NULL || Info == NULL || SizeOfInfo == NULL || ModeNumber >= This->Mode->MaxMode) {
    return EFI_INVALID_PARAMETER;
  }

  *Info = (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *) AllocatePool (
																										sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION)
																										);
  if (NULL == *Info) {
    return EFI_OUT_OF_RESOURCES;
  }

  *SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

  ModeData = &FbGopPrivate->ModeData[ModeNumber];
  (*Info)->Version = 0;
  (*Info)->HorizontalResolution = ModeData->HorizontalResolution;
  (*Info)->VerticalResolution   = ModeData->VerticalResolution;
  (*Info)->PixelFormat = ModeData->PixelFormat;
  CopyMem (&((*Info)->PixelInformation), &(ModeData->PixelBitMask), sizeof(ModeData->PixelBitMask));

  (*Info)->PixelsPerScanLine =  (ModeData->BytesPerScanLine * 8) / ModeData->BitsPerPixel;

  return EFI_SUCCESS;
}

/**
  Graphics Output protocol interface to set video mode.

  @param  This                   Protocol instance pointer.
  @param  ModeNumber             The mode number to be set.

  @retval EFI_SUCCESS            Graphics mode was changed.
  @retval EFI_DEVICE_ERROR       The device had an error and could not complete the
                                 request.
  @retval EFI_UNSUPPORTED        ModeNumber is not supported by this device.

**/
EFI_STATUS
EFIAPI
FbGopGraphicsOutputSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL * This,
  IN  UINT32                       ModeNumber
  )
{  
  FB_VIDEO_DEV          *FbGopPrivate;
  FB_VIDEO_MODE_DATA    *ModeData;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FbGopPrivate = FB_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (This);

  ModeData = &FbGopPrivate->ModeData[ModeNumber];

  if (ModeNumber >= This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }
  
  if (ModeNumber == This->Mode->Mode) {
    //
    // Clear screen to black
    //    
    ZeroMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    FbGopGraphicsOutputVbeBlt (
                        This,
                        &Background,
                        EfiBltVideoFill,
                        0,
                        0,
                        0,
                        0,
                        ModeData->HorizontalResolution,
                        ModeData->VerticalResolution,
                        0
    );
    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
  
}

/**
  Update physical frame buffer, copy 4 bytes block, then copy remaining bytes.

  @param   PciIo              The pointer of EFI_PCI_IO_PROTOCOL
  @param   VbeBuffer          The data to transfer to screen
  @param   MemAddress         Physical frame buffer base address
  @param   DestinationX       The X coordinate of the destination for BltOperation
  @param   DestinationY       The Y coordinate of the destination for BltOperation
  @param   TotalBytes         The total bytes of copy
  @param   VbePixelWidth      Bytes per pixel
  @param   BytesPerScanLine   Bytes per scan line

**/
VOID
CopyVideoBuffer (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT8                 *VbeBuffer,
  IN  VOID                  *MemAddress,
  IN  UINTN                 DestinationX,
  IN  UINTN                 DestinationY,
  IN  UINTN                 TotalBytes,
  IN  UINT32                VbePixelWidth,
  IN  UINTN                 BytesPerScanLine
  )
{
  UINTN                 FrameBufferAddr;
  UINTN                 CopyBlockNum;
  UINTN                 RemainingBytes;
  UINTN                 UnalignedBytes;
  EFI_STATUS            Status;

  FrameBufferAddr = (UINTN) MemAddress + (DestinationY * BytesPerScanLine) + DestinationX * VbePixelWidth;

  //
  // If TotalBytes is less than 4 bytes, only start byte copy.
  //
  if (TotalBytes < 4) {
    Status = PciIo->Mem.Write (
                     PciIo,
                     EfiPciIoWidthUint8,
                     EFI_PCI_IO_PASS_THROUGH_BAR,
                     (UINT64) FrameBufferAddr,
                     TotalBytes,
                     VbeBuffer
                     );
    ASSERT_EFI_ERROR (Status);
    return;
  }

  //
  // If VbeBuffer is not 4-byte aligned, start byte copy.
  //
  UnalignedBytes  = (4 - ((UINTN) VbeBuffer & 0x3)) & 0x3;

  if (UnalignedBytes != 0) {
    Status = PciIo->Mem.Write (
                     PciIo,
                     EfiPciIoWidthUint8,
                     EFI_PCI_IO_PASS_THROUGH_BAR,
                     (UINT64) FrameBufferAddr,
                     UnalignedBytes,
                     VbeBuffer
                     );
    ASSERT_EFI_ERROR (Status);
    FrameBufferAddr += UnalignedBytes;
    VbeBuffer       += UnalignedBytes;
  }

  //
  // Calculate 4-byte block count and remaining bytes.
  //
  CopyBlockNum   = (TotalBytes - UnalignedBytes) >> 2;
  RemainingBytes = (TotalBytes - UnalignedBytes) &  3;

  //
  // Copy 4-byte block and remaining bytes to physical frame buffer.
  //
  if (CopyBlockNum != 0) {
    Status = PciIo->Mem.Write (
                    PciIo,
                    EfiPciIoWidthUint32,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) FrameBufferAddr,
                    CopyBlockNum,
                    VbeBuffer
                    );
    ASSERT_EFI_ERROR (Status);
  }

  if (RemainingBytes != 0) {
    FrameBufferAddr += (CopyBlockNum << 2);
    VbeBuffer       += (CopyBlockNum << 2);
    Status = PciIo->Mem.Write (
                    PciIo,
                    EfiPciIoWidthUint8,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) FrameBufferAddr,
                    RemainingBytes,
                    VbeBuffer
                    );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Worker function to block transfer for VBE device.

  @param  FbGopPrivate       Instance of FB_VIDEO_DEV
  @param  BltBuffer              The data to transfer to screen
  @param  BltOperation           The operation to perform
  @param  SourceX                The X coordinate of the source for BltOperation
  @param  SourceY                The Y coordinate of the source for BltOperation
  @param  DestinationX           The X coordinate of the destination for
                                 BltOperation
  @param  DestinationY           The Y coordinate of the destination for
                                 BltOperation
  @param  Width                  The width of a rectangle in the blt rectangle in
                                 pixels
  @param  Height                 The height of a rectangle in the blt rectangle in
                                 pixels
  @param  Delta                  Not used for EfiBltVideoFill and
                                 EfiBltVideoToVideo operation. If a Delta of 0 is
                                 used, the entire BltBuffer will be operated on. If
                                 a subrectangle of the BltBuffer is used, then
                                 Delta represents the number of bytes in a row of
                                 the BltBuffer.
  @param  Mode                   Mode data.

  @retval EFI_INVALID_PARAMETER  Invalid parameter passed in
  @retval EFI_SUCCESS            Blt operation success

**/
EFI_STATUS
FbGopVbeBltWorker (
  IN  FB_VIDEO_DEV                       *FbGopPrivate,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer, OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta,
  IN  FB_VIDEO_MODE_DATA               *Mode
  )
{
  EFI_PCI_IO_PROTOCOL            *PciIo;
  EFI_TPL                        OriginalTPL;
  UINTN                          DstY;
  UINTN                          SrcY;
  UINTN                          DstX;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Blt;
  VOID                           *MemAddress;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *VbeFrameBuffer;
  UINTN                          BytesPerScanLine;
  UINTN                          Index;
  UINT8                          *VbeBuffer;
  UINT8                          *VbeBuffer1;
  UINT8                          *BltUint8;
  UINT32                         VbePixelWidth;
  UINT32                         Pixel;
  UINTN                          TotalBytes;

  PciIo             = FbGopPrivate->PciIo;

  VbeFrameBuffer    = FbGopPrivate->VbeFrameBuffer;
  MemAddress        = Mode->LinearFrameBuffer;
  BytesPerScanLine  = Mode->BytesPerScanLine;
  VbePixelWidth     = Mode->BitsPerPixel / 8;
  BltUint8          = (UINT8 *) BltBuffer;
  TotalBytes        = Width * VbePixelWidth;

  if (((UINTN) BltOperation) >= EfiGraphicsOutputBltOperationMax) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // We need to fill the Virtual Screen buffer with the blt data.
  // The virtual screen is upside down, as the first row is the bootom row of
  // the image.
  //
  if (BltOperation == EfiBltVideoToBltBuffer) {
    //
    // Video to BltBuffer: Source is Video, destination is BltBuffer
    //
    if (SourceY + Height > Mode->VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (SourceX + Width > Mode->HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // BltBuffer to Video: Source is BltBuffer, destination is Video
    //
    if (DestinationY + Height > Mode->VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestinationX + Width > Mode->HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // If Delta is zero, then the entire BltBuffer is being used, so Delta
  // is the number of bytes in each row of BltBuffer.  Since BltBuffer is Width pixels size,
  // the number of bytes in each row can be computed.
  //
  if (Delta == 0) {
    Delta = Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  }
  //
  // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
  // We would not want a timer based event (Cursor, ...) to come in while we are
  // doing this operation.
  //
  OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

  switch (BltOperation) {
  case EfiBltVideoToBltBuffer:
    for (SrcY = SourceY, DstY = DestinationY; DstY < (Height + DestinationY); SrcY++, DstY++) {
      Blt = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) (BltUint8 + DstY * Delta + DestinationX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      //
      // Shuffle the packed bytes in the hardware buffer to match EFI_GRAPHICS_OUTPUT_BLT_PIXEL
      //
      VbeBuffer = ((UINT8 *) VbeFrameBuffer + (SrcY * BytesPerScanLine + SourceX * VbePixelWidth));
      for (DstX = DestinationX; DstX < (Width + DestinationX); DstX++) {
        Pixel         = VbeBuffer[0] | VbeBuffer[1] << 8 | VbeBuffer[2] << 16 | VbeBuffer[3] << 24;
        Blt->Red      = (UINT8) ((Pixel >> Mode->Red.Position) & Mode->Red.Mask);
        Blt->Blue     = (UINT8) ((Pixel >> Mode->Blue.Position) & Mode->Blue.Mask);
        Blt->Green    = (UINT8) ((Pixel >> Mode->Green.Position) & Mode->Green.Mask);
        Blt->Reserved = 0;
        Blt++;
        VbeBuffer += VbePixelWidth;
      }

    }
    break;

  case EfiBltVideoToVideo:
    for (Index = 0; Index < Height; Index++) {
      if (DestinationY <= SourceY) {
        SrcY  = SourceY + Index;
        DstY  = DestinationY + Index;
      } else {
        SrcY  = SourceY + Height - Index - 1;
        DstY  = DestinationY + Height - Index - 1;
      }

      VbeBuffer   = ((UINT8 *) VbeFrameBuffer + DstY * BytesPerScanLine + DestinationX * VbePixelWidth);
      VbeBuffer1  = ((UINT8 *) VbeFrameBuffer + SrcY * BytesPerScanLine + SourceX * VbePixelWidth);

      gBS->CopyMem (
            VbeBuffer,
            VbeBuffer1,
            TotalBytes
            );

      //
      // Update physical frame buffer.
      //
      CopyVideoBuffer (
        PciIo,
        VbeBuffer,
        MemAddress,
        DestinationX,
        DstY,
        TotalBytes,
        VbePixelWidth,
        BytesPerScanLine
        );
    }
    break;

  case EfiBltVideoFill:
    VbeBuffer = (UINT8 *) ((UINTN) VbeFrameBuffer + (DestinationY * BytesPerScanLine) + DestinationX * VbePixelWidth);
    Blt       = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) BltUint8;
    //
    // Shuffle the RGB fields in EFI_GRAPHICS_OUTPUT_BLT_PIXEL to match the hardware buffer
    //
    Pixel = ((Blt->Red & Mode->Red.Mask) << Mode->Red.Position) |
      (
        (Blt->Green & Mode->Green.Mask) <<
        Mode->Green.Position
      ) |
          ((Blt->Blue & Mode->Blue.Mask) << Mode->Blue.Position);

    for (Index = 0; Index < Width; Index++) {
      gBS->CopyMem (
            VbeBuffer,
            &Pixel,
            VbePixelWidth
            );
      VbeBuffer += VbePixelWidth;
    }

    VbeBuffer = (UINT8 *) ((UINTN) VbeFrameBuffer + (DestinationY * BytesPerScanLine) + DestinationX * VbePixelWidth);
    for (DstY = DestinationY + 1; DstY < (Height + DestinationY); DstY++) {
      gBS->CopyMem (
            (VOID *) ((UINTN) VbeFrameBuffer + (DstY * BytesPerScanLine) + DestinationX * VbePixelWidth),
            VbeBuffer,
            TotalBytes
            );
    }

    for (DstY = DestinationY; DstY < (Height + DestinationY); DstY++) {
      //
      // Update physical frame buffer.
      //
      CopyVideoBuffer (
        PciIo,
        VbeBuffer,
        MemAddress,
        DestinationX,
        DstY,
        TotalBytes,
        VbePixelWidth,
        BytesPerScanLine
        );
    }
    break;

  case EfiBltBufferToVideo:
    for (SrcY = SourceY, DstY = DestinationY; SrcY < (Height + SourceY); SrcY++, DstY++) {
      Blt       = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) (BltUint8 + (SrcY * Delta) + (SourceX) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      VbeBuffer = ((UINT8 *) VbeFrameBuffer + (DstY * BytesPerScanLine + DestinationX * VbePixelWidth));
      for (DstX = DestinationX; DstX < (Width + DestinationX); DstX++) {
        //
        // Shuffle the RGB fields in EFI_GRAPHICS_OUTPUT_BLT_PIXEL to match the hardware buffer
        //
        Pixel = ((Blt->Red & Mode->Red.Mask) << Mode->Red.Position) |
          ((Blt->Green & Mode->Green.Mask) << Mode->Green.Position) |
            ((Blt->Blue & Mode->Blue.Mask) << Mode->Blue.Position);
        gBS->CopyMem (
              VbeBuffer,
              &Pixel,
              VbePixelWidth
              );
        Blt++;
        VbeBuffer += VbePixelWidth;
      }

      VbeBuffer = ((UINT8 *) VbeFrameBuffer + (DstY * BytesPerScanLine + DestinationX * VbePixelWidth));

      //
      // Update physical frame buffer.
      //
      CopyVideoBuffer (
        PciIo,
        VbeBuffer,
        MemAddress,
        DestinationX,
        DstY,
        TotalBytes,
        VbePixelWidth,
        BytesPerScanLine
        );
    }
    break;

    default: ;
  }

  gBS->RestoreTPL (OriginalTPL);

  return EFI_SUCCESS;
}

/**
  Graphics Output protocol instance to block transfer for VBE device.

  @param  This                   Pointer to Graphics Output protocol instance
  @param  BltBuffer              The data to transfer to screen
  @param  BltOperation           The operation to perform
  @param  SourceX                The X coordinate of the source for BltOperation
  @param  SourceY                The Y coordinate of the source for BltOperation
  @param  DestinationX           The X coordinate of the destination for
                                 BltOperation
  @param  DestinationY           The Y coordinate of the destination for
                                 BltOperation
  @param  Width                  The width of a rectangle in the blt rectangle in
                                 pixels
  @param  Height                 The height of a rectangle in the blt rectangle in
                                 pixels
  @param  Delta                  Not used for EfiBltVideoFill and
                                 EfiBltVideoToVideo operation. If a Delta of 0 is
                                 used, the entire BltBuffer will be operated on. If
                                 a subrectangle of the BltBuffer is used, then
                                 Delta represents the number of bytes in a row of
                                 the BltBuffer.

  @retval EFI_INVALID_PARAMETER  Invalid parameter passed in
  @retval EFI_SUCCESS            Blt operation success

**/
EFI_STATUS
EFIAPI
FbGopGraphicsOutputVbeBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer, OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta
  )
{
  FB_VIDEO_DEV                 *FbGopPrivate;
  FB_VIDEO_MODE_DATA           *Mode;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FbGopPrivate  = FB_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (This);
  Mode          = &FbGopPrivate->ModeData[This->Mode->Mode];

  return FbGopVbeBltWorker (
           FbGopPrivate,
           BltBuffer,
           BltOperation,
           SourceX,
           SourceY,
           DestinationX,
           DestinationY,
           Width,
           Height,
           Delta,
           Mode
           );
}


/**
  The user Entry Point for module UefiFbGop. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
FbGopEntryPoint(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HOB_GUID_TYPE  *GuidHob;
  
  //
  // Find the frame buffer information guid hob
  //
  GuidHob = GetFirstGuidHob (&gUefiFrameBufferInfoGuid);
  if (GuidHob != NULL) {   
    //
    // Install driver model protocol(s).
    //
    Status = EfiLibInstallDriverBindingComponentName2 (
               ImageHandle,
               SystemTable,
               &gFbGopDriverBinding,
               ImageHandle,
               &gFbGopComponentName,
               &gFbGopComponentName2
               );
    ASSERT_EFI_ERROR (Status);
  } else {
    DEBUG ((EFI_D_ERROR, "No FrameBuffer information from coreboot.  NO GOP driver !!!\n"));
    Status = EFI_ABORTED;
  }
  return Status;  
}

