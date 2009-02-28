/*++

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BiosVideo.c
    
Abstract:

  ConsoleOut Routines that speak VGA.

Revision History

--*/

#include "BiosVideo.h"


//
// EFI Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL gBiosVideoDriverBinding = {
  BiosVideoDriverBindingSupported,
  BiosVideoDriverBindingStart,
  BiosVideoDriverBindingStop,
  0x3,
  NULL,
  NULL
};

//
// Global lookup tables for VGA graphics modes
//
UINT8                       mVgaLeftMaskTable[]   = { 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01 };

UINT8                       mVgaRightMaskTable[]  = { 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };

UINT8                       mVgaBitMaskTable[]    = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

THUNK_CONTEXT               mThunkContext;

EFI_LEGACY_8259_PROTOCOL    *mLegacy8259 = NULL;

#define EFI_CPU_EFLAGS_IF 0x200

EFI_UGA_PIXEL               mVgaColorToUgaColor[] = {
  {
    0x00,
    0x00,
    0x00,
    0x00
  },
  {
    0x98,
    0x00,
    0x00,
    0x00
  },
  {
    0x00,
    0x98,
    0x00,
    0x00
  },
  {
    0x98,
    0x98,
    0x00,
    0x00
  },
  {
    0x00,
    0x00,
    0x98,
    0x00
  },
  {
    0x98,
    0x00,
    0x98,
    0x00
  },
  {
    0x00,
    0x98,
    0x98,
    0x00
  },
  {
    0x98,
    0x98,
    0x98,
    0x00
  },
  {
    0x10,
    0x10,
    0x10,
    0x00
  },
  {
    0xff,
    0x10,
    0x10,
    0x00
  },
  {
    0x10,
    0xff,
    0x10,
    0x00
  },
  {
    0xff,
    0xff,
    0x10,
    0x00
  },
  {
    0x10,
    0x10,
    0xff,
    0x00
  },
  {
    0xf0,
    0x10,
    0xff,
    0x00
  },
  {
    0x10,
    0xff,
    0xff,
    0x00
  },
  {
    0xff,
    0xff,
    0xff,
    0x00
  }
};

VOID
InitializeBiosIntCaller (
  VOID
  );
  
VOID
InitializeInterruptRedirection (
  VOID
  );
    
BOOLEAN
EFIAPI
LegacyBiosInt86 (
  IN  UINT8                           BiosInt,
  IN  EFI_IA32_REGISTER_SET           *Regs
  );
    
EFI_STATUS
EFIAPI
BiosVideoDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++
  
  Routine Description:
  
    Driver Entry Point.
        
  Arguments:
  
  ImageHandle - Handle of driver image.
  SystemTable - Pointer to system table.
  
  Returns:
  
    EFI_STATUS
    
--*/
{
  EFI_STATUS  Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
            ImageHandle,
            SystemTable,
            &gBiosVideoDriverBinding,
            ImageHandle,
            &gBiosVideoComponentName,
            &gBiosVideoComponentName2
            );

  return Status;
}

VOID
EFIAPI
BiosVideoExitBootServices (
  EFI_EVENT  Event,
  VOID       *Context
  )
/*++

Routine Description:

  Callback function for exit boot service event

Arguments:

  Event   - EFI_EVENT structure
  Context - Event context

Returns:

  None

--*/
{
/*
  BIOS_VIDEO_DEV        *BiosVideoPrivate;
  EFI_IA32_REGISTER_SET Regs;

  //
  // Get our context
  //
  BiosVideoPrivate = (BIOS_VIDEO_DEV *) Context;

  //
  // Set the 80x25 Text VGA Mode
  //
  Regs.H.AH = 0x00;
  Regs.H.AL = 0x83;
  BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);

  Regs.H.AH = 0x11;
  Regs.H.AL = 0x14;
  Regs.H.BL = 0;
  BiosVideoPrivate->LegacyBios->Int86 (BiosVideoPrivate->LegacyBios, 0x10, &Regs);
*/
}

EFI_STATUS
EFIAPI
BiosVideoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++
  
  Routine Description:

    Supported.
    
  Arguments:

  This - Pointer to driver binding protocol
  Controller - Controller handle to connect
  RemainingDevicePath - A pointer to the remaining portion of a device path
    
    
  Returns:

  EFI_STATUS - EFI_SUCCESS:This controller can be managed by this driver,
               Otherwise, this controller cannot be managed by this driver
  
--*/
{
  EFI_STATUS                      Status;
  EFI_LEGACY_8259_PROTOCOL        *Legacy8259;
  EFI_PCI_IO_PROTOCOL             *PciIo;
  
  DEBUG ((EFI_D_INFO, "BiosVideoDriverBindingSupported\n"));

  //
  // See if the Legacy BIOS Protocol is available
  //
  Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **) &Legacy8259);
  if (EFI_ERROR (Status)) {
    return Status;
  }
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
    DEBUG ((EFI_D_INFO, "BiosVideoDriverBindingSupported: Fail to open PciIo protocol!\n"));
    return Status;
  }
  
  if (!BiosVideoIsVga (PciIo)) {
    DEBUG ((EFI_D_INFO, "BiosVideoDriverBindingSupported: Is not VGA!\n"));
    Status = EFI_UNSUPPORTED;
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return Status;
}

EFI_STATUS
EFIAPI
BiosVideoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++
  
  Routine Description:

    Install UGA Draw Protocol onto VGA device handles
  
  Arguments:

  This - Pointer to driver binding protocol
  Controller - Controller handle to connect
  RemainingDevicePath - A pointer to the remaining portion of a device path
    
  Returns:

    EFI_STATUS
    
--*/
{
  EFI_STATUS      Status;
  BIOS_VIDEO_DEV  *BiosVideoPrivate;

  DEBUG ((EFI_D_INFO, "BiosVideoDriverBindingStart\n"));
  //
  // Initialize local variables
  //
  BiosVideoPrivate = NULL;

  //
  // Allocate the private device structure
  //
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (BIOS_VIDEO_DEV),
                  (VOID**)&BiosVideoPrivate
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  ZeroMem (BiosVideoPrivate, sizeof (BIOS_VIDEO_DEV));

  //
  // See if the Legacy BIOS Protocol is available
  //
  Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **) &mLegacy8259);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  //
  // Prepare for status code
  //
  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID**)&BiosVideoPrivate->DevicePath
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Open the IO Abstraction(s) needed
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &(BiosVideoPrivate->PciIo),
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  DEBUG ((EFI_D_INFO, "InitializeBiosIntCaller\n"));
  InitializeBiosIntCaller();
  InitializeInterruptRedirection();
  DEBUG ((EFI_D_INFO, "InitializeBiosIntCaller Finished!\n"));
  
  if (!BiosVideoIsVga (BiosVideoPrivate->PciIo)) {
    DEBUG ((EFI_D_INFO, "BiosVideoDriverBindingStart: not VGA\n"));
    Status = EFI_UNSUPPORTED;
    goto Done;
  }
  
  BiosVideoPrivate->VgaCompatible = TRUE; 
  //
  // Initialize the private device structure
  //
  BiosVideoPrivate->Signature = BIOS_VIDEO_DEV_SIGNATURE;
  BiosVideoPrivate->Handle    = Controller;

  /**
  Status = gBS->CreateEvent (
                  EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES,
                  EFI_TPL_NOTIFY,
                  BiosVideoExitBootServices,
                  BiosVideoPrivate,
                  &BiosVideoPrivate->ExitBootServicesEvent
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  **/
  
  //
  // Fill in UGA Draw specific mode structures
  //
  BiosVideoPrivate->HardwareNeedsStarting = TRUE;
  BiosVideoPrivate->CurrentMode           = 0;
  BiosVideoPrivate->MaxMode               = 0;
  BiosVideoPrivate->ModeData              = NULL;
  BiosVideoPrivate->LineBuffer            = NULL;
  BiosVideoPrivate->VgaFrameBuffer        = NULL;
  BiosVideoPrivate->VbeFrameBuffer        = NULL;

  //
  // Fill in the VGA Mini Port Protocol fields
  //
  BiosVideoPrivate->VgaMiniPort.SetMode                   = BiosVideoVgaMiniPortSetMode;
  BiosVideoPrivate->VgaMiniPort.VgaMemoryOffset           = 0xb8000;
  BiosVideoPrivate->VgaMiniPort.CrtcAddressRegisterOffset = 0x3d4;
  BiosVideoPrivate->VgaMiniPort.CrtcDataRegisterOffset    = 0x3d5;
  BiosVideoPrivate->VgaMiniPort.VgaMemoryBar              = EFI_PCI_IO_PASS_THROUGH_BAR;
  BiosVideoPrivate->VgaMiniPort.CrtcAddressRegisterBar    = EFI_PCI_IO_PASS_THROUGH_BAR;
  BiosVideoPrivate->VgaMiniPort.CrtcDataRegisterBar       = EFI_PCI_IO_PASS_THROUGH_BAR;

  //
  // Assume that UGA Draw will be produced until proven otherwise
  //
  BiosVideoPrivate->ProduceUgaDraw = TRUE;

  //
  // Check for VESA BIOS Extensions for modes that are compatible with UGA Draw
  //
  //CpuDeadLoop();
  DEBUG ((EFI_D_INFO, "BiosVideoDriverBindingStart: Before check VBE!\n"));
  Status = BiosVideoCheckForVbe (BiosVideoPrivate);
  DEBUG ((EFI_D_INFO, "BiosVideoDriverBindingStart: check VBE status=%r!\n", Status));
  
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "BiosVideoDriverBindingStart: Fail to check VBE!\n"));
    //
    // The VESA BIOS Extensions are not compatible with UGA Draw, so check for support
    // for the standard 640x480 16 color VGA mode
    //
    if (BiosVideoPrivate->VgaCompatible) {
      Status = BiosVideoCheckForVga (BiosVideoPrivate);
    }

    if (EFI_ERROR (Status)) {
      //
      // Neither VBE nor the standard 640x480 16 color VGA mode are supported, so do
      // not produce the UGA Draw protocol.  Instead, produce the VGA MiniPort Protocol.
      //
      BiosVideoPrivate->ProduceUgaDraw = FALSE;

      //
      // INT services are available, so on the 80x25 and 80x50 text mode are supported
      //
      BiosVideoPrivate->VgaMiniPort.MaxMode = 2;
    }
  }

  if (BiosVideoPrivate->ProduceUgaDraw) {
    DEBUG ((EFI_D_INFO, "BiosVideoDriverBindingStart: Produce Uga Draw!\n"));
    //
    // Install UGA Draw Protocol
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Controller,
                    &gEfiUgaDrawProtocolGuid,
                    &BiosVideoPrivate->UgaDraw,
                    NULL
                    );
  } else {
    //
    // Install VGA Mini Port Protocol
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Controller,
                    &gEfiVgaMiniPortProtocolGuid,
                    &BiosVideoPrivate->VgaMiniPort,
                    NULL
                    );
  }

Done:
  if (EFI_ERROR (Status)) {
    if (BiosVideoPrivate != NULL) {
      //
      // Free mode data
      //
      if (BiosVideoPrivate->ModeData != NULL) {
        gBS->FreePool (BiosVideoPrivate->ModeData);
      }
      //
      // Free memory allocated below 1MB
      //
      if (BiosVideoPrivate->PagesBelow1MB != 0) {
        gBS->FreePages (BiosVideoPrivate->PagesBelow1MB, BiosVideoPrivate->NumberOfPagesBelow1MB);
      }

      if (BiosVideoPrivate->PciIo != NULL) {
        //
        // Release PCI I/O and UGA Draw Protocols on the controller handle.
        //
        gBS->CloseProtocol (
              Controller,
              &gEfiPciIoProtocolGuid,
              This->DriverBindingHandle,
              Controller
              );
      }
      //
      // Close the ExitBootServices event
      //
      if (BiosVideoPrivate->ExitBootServicesEvent != NULL) {
        gBS->CloseEvent (BiosVideoPrivate->ExitBootServicesEvent);
      }
      //
      // Free private data structure
      //
      gBS->FreePool (BiosVideoPrivate);
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
BiosVideoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
/*++
  
  Routine Description:

    Stop.
  
  Arguments:

  This - Pointer to driver binding protocol
  Controller - Controller handle to connect
  NumberOfChilren - Number of children handle created by this driver
  ChildHandleBuffer - Buffer containing child handle created
  
  Returns:

  EFI_SUCCESS - Driver disconnected successfully from controller
  EFI_UNSUPPORTED - Cannot find BIOS_VIDEO_DEV structure
  
--*/
{
  EFI_STATUS                  Status;
  EFI_UGA_DRAW_PROTOCOL       *Uga;
  EFI_VGA_MINI_PORT_PROTOCOL  *VgaMiniPort;
  BIOS_VIDEO_DEV              *BiosVideoPrivate;
  EFI_IA32_REGISTER_SET       Regs;

  BiosVideoPrivate = NULL;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUgaDrawProtocolGuid,
                  (VOID **) &Uga,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_UGA_DRAW_THIS (Uga);
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiVgaMiniPortProtocolGuid,
                  (VOID **) &VgaMiniPort,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_VGA_MINI_PORT_THIS (VgaMiniPort);
  }

  if (BiosVideoPrivate == NULL) {
    return EFI_UNSUPPORTED;
  }

  if (BiosVideoPrivate->ProduceUgaDraw) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    Controller,
                    &gEfiUgaDrawProtocolGuid,
                    &BiosVideoPrivate->UgaDraw,
                    NULL
                    );
  } else {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    Controller,
                    &gEfiVgaMiniPortProtocolGuid,
                    &BiosVideoPrivate->VgaMiniPort,
                    NULL
                    );
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  gBS->SetMem (&Regs, sizeof (Regs), 0);  
  
  //
  // Set the 80x25 Text VGA Mode
  //
  Regs.H.AH = 0x00;
  Regs.H.AL = 0x03;
  LegacyBiosInt86 (0x10, &Regs);

  Regs.H.AH = 0x11;
  Regs.H.AL = 0x14;
  Regs.H.BL = 0;
  LegacyBiosInt86 (0x10, &Regs);

  //
  // Do not disable IO/memory decode since that would prevent legacy ROM from working
  //
  //
  // Free VGA Frame Buffer
  //
  if (BiosVideoPrivate->VgaFrameBuffer != NULL) {
    gBS->FreePool (BiosVideoPrivate->VgaFrameBuffer);
  }
  //
  // Free VBE Frame Buffer
  //
  if (BiosVideoPrivate->VbeFrameBuffer != NULL) {
    gBS->FreePool (BiosVideoPrivate->VbeFrameBuffer);
  }
  //
  // Free line buffer
  //
  if (BiosVideoPrivate->LineBuffer != NULL) {
    gBS->FreePool (BiosVideoPrivate->LineBuffer);
  }
  //
  // Free mode data
  //
  if (BiosVideoPrivate->ModeData != NULL) {
    gBS->FreePool (BiosVideoPrivate->ModeData);
  }
  //
  // Free memory allocated below 1MB
  //
  if (BiosVideoPrivate->PagesBelow1MB != 0) {
    gBS->FreePages (BiosVideoPrivate->PagesBelow1MB, BiosVideoPrivate->NumberOfPagesBelow1MB);
  }

  if (BiosVideoPrivate->VbeSaveRestorePages != 0) {
    gBS->FreePages (BiosVideoPrivate->VbeSaveRestoreBuffer, BiosVideoPrivate->VbeSaveRestorePages);
  }
  //
  // Release PCI I/O and UGA Draw Protocols on the controller handle.
  //
  gBS->CloseProtocol (
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  //
  // Close the ExitBootServices event
  //
  gBS->CloseEvent (BiosVideoPrivate->ExitBootServicesEvent);

  //
  // Free private data structure
  //
  gBS->FreePool (BiosVideoPrivate);

  return EFI_SUCCESS;
}

#define PCI_DEVICE_ENABLED  (EFI_PCI_COMMAND_IO_SPACE | EFI_PCI_COMMAND_MEMORY_SPACE)


BOOLEAN
BiosVideoIsVga (
  IN  EFI_PCI_IO_PROTOCOL       *PciIo
  )
{
  EFI_STATUS    Status;
  BOOLEAN       VgaCompatible;
  PCI_TYPE00    Pci;

  VgaCompatible = FALSE;

  //
  // Read the PCI Configuration Header
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (Pci) / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    return VgaCompatible;
  }

  //
  // See if this is a VGA compatible controller or not
  //
  if ((Pci.Hdr.Command & PCI_DEVICE_ENABLED) == PCI_DEVICE_ENABLED) {
    if (Pci.Hdr.ClassCode[2] == PCI_CLASS_OLD && Pci.Hdr.ClassCode[1] == PCI_CLASS_OLD_VGA) {
      //
      // Base Class 0x00 Sub-Class 0x01 - Backward compatible VGA device
      //
      VgaCompatible = TRUE;
    }

    if (Pci.Hdr.ClassCode[2] == PCI_CLASS_DISPLAY && Pci.Hdr.ClassCode[1] == PCI_CLASS_DISPLAY_VGA && Pci.Hdr.ClassCode[0] == 0x00) {
      //
      // Base Class 3 Sub-Class 0 Programming interface 0 - VGA compatible Display controller
      //
      VgaCompatible = TRUE;
    }
  }

  return VgaCompatible;
}


EFI_STATUS
BiosVideoCheckForVbe (
  IN OUT BIOS_VIDEO_DEV  *BiosVideoPrivate  
  )
/*++
  
  Routine Description:

  Check for VBE device   
  
  Arguments:
  
  BiosVideoPrivate - Pointer to BIOS_VIDEO_DEV structure
  
  Returns:
  
  EFI_SUCCESS - VBE device found
  
--*/
{
  EFI_STATUS            Status;
  EFI_IA32_REGISTER_SET Regs;
  UINT16                *ModeNumberPtr;
  BOOLEAN               ModeFound;
  BIOS_VIDEO_MODE_DATA  *ModeBuffer;
  UINTN                 Index;

  //
  // Allocate buffer under 1MB for VBE data structures
  //
  BiosVideoPrivate->NumberOfPagesBelow1MB = EFI_SIZE_TO_PAGES (
                                              sizeof (VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK) + sizeof (VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK) +
                                              sizeof (VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK)
                                              );

  BiosVideoPrivate->PagesBelow1MB = 0x00100000 - 1;

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiBootServicesData,
                  BiosVideoPrivate->NumberOfPagesBelow1MB,
                  &BiosVideoPrivate->PagesBelow1MB
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Fill in the UGA Draw Protocol
  //
  BiosVideoPrivate->UgaDraw.GetMode = BiosVideoUgaDrawGetMode;
  BiosVideoPrivate->UgaDraw.SetMode = BiosVideoUgaDrawSetMode;
  BiosVideoPrivate->UgaDraw.Blt     = BiosVideoUgaDrawVbeBlt;

  //
  // Fill in the VBE related data structures
  //
  BiosVideoPrivate->VbeInformationBlock = (VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK *) (UINTN) (BiosVideoPrivate->PagesBelow1MB);
  BiosVideoPrivate->VbeModeInformationBlock = (VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK *) (BiosVideoPrivate->VbeInformationBlock + 1);
  BiosVideoPrivate->VbeCrtcInformationBlock = (VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK *) (BiosVideoPrivate->VbeModeInformationBlock + 1);
  BiosVideoPrivate->VbeSaveRestorePages   = 0;
  BiosVideoPrivate->VbeSaveRestoreBuffer  = 0;

  //
  // Test to see if the Video Adapter is compliant with VBE 3.0
  //
  gBS->SetMem (&Regs, sizeof (Regs), 0);
  Regs.X.AX = VESA_BIOS_EXTENSIONS_RETURN_CONTROLLER_INFORMATION;
  gBS->SetMem (BiosVideoPrivate->VbeInformationBlock, sizeof (VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK), 0);
  BiosVideoPrivate->VbeInformationBlock->VESASignature  = VESA_BIOS_EXTENSIONS_VBE2_SIGNATURE;
  Regs.X.ES = EFI_SEGMENT ((UINTN)BiosVideoPrivate->VbeInformationBlock);
  Regs.X.DI = EFI_OFFSET ((UINTN)BiosVideoPrivate->VbeInformationBlock);

  LegacyBiosInt86 (0x10, &Regs);

  Status = EFI_DEVICE_ERROR;

  //
  // See if the VESA call succeeded
  //
  if (Regs.X.AX != VESA_BIOS_EXTENSIONS_STATUS_SUCCESS) {
    return Status;
  }
  //
  // Check for 'VESA' signature
  //
  if (BiosVideoPrivate->VbeInformationBlock->VESASignature != VESA_BIOS_EXTENSIONS_VESA_SIGNATURE) {
    return Status;
  }
  //
  // Check to see if this is VBE 2.0 or higher
  //
  if (BiosVideoPrivate->VbeInformationBlock->VESAVersion < VESA_BIOS_EXTENSIONS_VERSION_2_0) {
    return Status;
  }
  //
  // Walk through the mode list to see if there is at least one mode the is compatible with the UGA_DRAW protocol
  //
  ModeNumberPtr = (UINT16 *)
    (
      (((UINTN) BiosVideoPrivate->VbeInformationBlock->VideoModePtr & 0xffff0000) >> 12) |
        ((UINTN) BiosVideoPrivate->VbeInformationBlock->VideoModePtr & 0x0000ffff)
    );
  for (; *ModeNumberPtr != VESA_BIOS_EXTENSIONS_END_OF_MODE_LIST; ModeNumberPtr++) {
    //
    // Make sure this is a mode number defined by the VESA VBE specification.  If it isn'tm then skip this mode number.
    //
    if ((*ModeNumberPtr & VESA_BIOS_EXTENSIONS_MODE_NUMBER_VESA) == 0) {
      continue;
    }
    //
    // Get the information about the mode
    //
    gBS->SetMem (&Regs, sizeof (Regs), 0);
    Regs.X.AX = VESA_BIOS_EXTENSIONS_RETURN_MODE_INFORMATION;
    Regs.X.CX = *ModeNumberPtr;
    gBS->SetMem (BiosVideoPrivate->VbeModeInformationBlock, sizeof (VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK), 0);
    Regs.X.ES = EFI_SEGMENT ((UINTN)BiosVideoPrivate->VbeModeInformationBlock);
    Regs.X.DI = EFI_OFFSET ((UINTN)BiosVideoPrivate->VbeModeInformationBlock);

    LegacyBiosInt86 (0x10, &Regs);

    //
    // See if the call succeeded.  If it didn't, then try the next mode.
    //
    if (Regs.X.AX != VESA_BIOS_EXTENSIONS_STATUS_SUCCESS) {
      continue;
    }
    //
    // See if the mode supports color.  If it doesn't then try the next mode.
    //
    if ((BiosVideoPrivate->VbeModeInformationBlock->ModeAttributes & VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_COLOR) == 0) {
      continue;
    }
    //
    // See if the mode supports graphics.  If it doesn't then try the next mode.
    //
    if ((BiosVideoPrivate->VbeModeInformationBlock->ModeAttributes & VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_GRAPHICS) == 0) {
      continue;
    }
    //
    // See if the mode supports a linear frame buffer.  If it doesn't then try the next mode.
    //
    if ((BiosVideoPrivate->VbeModeInformationBlock->ModeAttributes & VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_LINEAR_FRAME_BUFFER) == 0) {
      continue;
    }
    //
    // See if the mode supports 32 bit color.  If it doesn't then try the next mode.
    // 32 bit mode can be implemented by 24 Bits Per Pixels. Also make sure the
    // number of bits per pixel is a multiple of 8 or more than 32 bits per pixel
    //
    if (BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel < 24) {
      continue;
    }

    if (BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel > 32) {
      continue;
    }

    if ((BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel % 8) != 0) {
      continue;
    }
    //
    // See if the physical base pointer for the linear mode is valid.  If it isn't then try the next mode.
    //
    if (BiosVideoPrivate->VbeModeInformationBlock->PhysBasePtr == 0) {
      continue;
    }
    //
    // See if the resolution is 1024x768, 800x600, or 640x480
    //
    ModeFound = FALSE;
    if (BiosVideoPrivate->VbeModeInformationBlock->XResolution == 1024 &&
        BiosVideoPrivate->VbeModeInformationBlock->YResolution == 768
        ) {
      ModeFound = TRUE;
    }

    if (BiosVideoPrivate->VbeModeInformationBlock->XResolution == 800 &&
        BiosVideoPrivate->VbeModeInformationBlock->YResolution == 600
        ) {
      ModeFound = TRUE;
    }

    if (BiosVideoPrivate->VbeModeInformationBlock->XResolution == 640 &&
        BiosVideoPrivate->VbeModeInformationBlock->YResolution == 480
        ) {
      ModeFound = TRUE;
    }

    if (!ModeFound) {
      continue;
    }
    //
    // Add mode to the list of available modes
    //
    BiosVideoPrivate->MaxMode++;
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    BiosVideoPrivate->MaxMode * sizeof (BIOS_VIDEO_MODE_DATA),
                    (VOID **) &ModeBuffer
                    );
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    if (BiosVideoPrivate->MaxMode > 1) {
      gBS->CopyMem (
            ModeBuffer,
            BiosVideoPrivate->ModeData,
            (BiosVideoPrivate->MaxMode - 1) * sizeof (BIOS_VIDEO_MODE_DATA)
            );
    }

    if (BiosVideoPrivate->ModeData != NULL) {
      gBS->FreePool (BiosVideoPrivate->ModeData);
    }

    ModeBuffer[BiosVideoPrivate->MaxMode - 1].VbeModeNumber = *ModeNumberPtr;
    if (BiosVideoPrivate->VbeInformationBlock->VESAVersion >= VESA_BIOS_EXTENSIONS_VERSION_3_0) {
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].BytesPerScanLine = BiosVideoPrivate->VbeModeInformationBlock->LinBytesPerScanLine;
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].Red.Position = BiosVideoPrivate->VbeModeInformationBlock->LinRedFieldPosition;
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].Red.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->LinRedMaskSize) - 1);
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].Blue.Position = BiosVideoPrivate->VbeModeInformationBlock->LinBlueFieldPosition;
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].Blue.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->LinBlueMaskSize) - 1);
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].Green.Position = BiosVideoPrivate->VbeModeInformationBlock->LinGreenFieldPosition;
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].Green.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->LinGreenMaskSize) - 1);

    } else {
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].BytesPerScanLine = BiosVideoPrivate->VbeModeInformationBlock->BytesPerScanLine;
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].Red.Position = BiosVideoPrivate->VbeModeInformationBlock->RedFieldPosition;
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].Red.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->RedMaskSize) - 1);
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].Blue.Position = BiosVideoPrivate->VbeModeInformationBlock->BlueFieldPosition;
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].Blue.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->BlueMaskSize) - 1);
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].Green.Position = BiosVideoPrivate->VbeModeInformationBlock->GreenFieldPosition;
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].Green.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->GreenMaskSize) - 1);

    }

    ModeBuffer[BiosVideoPrivate->MaxMode - 1].LinearFrameBuffer = (VOID *) (UINTN)BiosVideoPrivate->VbeModeInformationBlock->PhysBasePtr;
    ModeBuffer[BiosVideoPrivate->MaxMode - 1].HorizontalResolution = BiosVideoPrivate->VbeModeInformationBlock->XResolution;
    ModeBuffer[BiosVideoPrivate->MaxMode - 1].VerticalResolution = BiosVideoPrivate->VbeModeInformationBlock->YResolution;

    if (BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel >= 24) {
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].ColorDepth = 32;
    } else {
      ModeBuffer[BiosVideoPrivate->MaxMode - 1].ColorDepth = BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel;
    }

    ModeBuffer[BiosVideoPrivate->MaxMode - 1].BitsPerPixel  = BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel;

    ModeBuffer[BiosVideoPrivate->MaxMode - 1].RefreshRate   = 60;

    BiosVideoPrivate->ModeData = ModeBuffer;
  }
  //
  // Check to see if we found any modes that are compatible with UGA DRAW
  //
  if (BiosVideoPrivate->MaxMode == 0) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }
  //
  // Find the best mode to initialize
  //
  Status = BiosVideoUgaDrawSetMode (&BiosVideoPrivate->UgaDraw, 1024, 768, 32, 60);
  //Status = BiosVideoUgaDrawSetMode (&BiosVideoPrivate->UgaDraw, 800, 600, 32, 60);
  if (EFI_ERROR (Status)) {
    Status = BiosVideoUgaDrawSetMode (&BiosVideoPrivate->UgaDraw, 800, 600, 32, 60);
    //Status = BiosVideoUgaDrawSetMode (&BiosVideoPrivate->UgaDraw, 1024, 768, 32, 60);
    if (EFI_ERROR (Status)) {
      Status = BiosVideoUgaDrawSetMode (&BiosVideoPrivate->UgaDraw, 640, 480, 32, 60);
      for (Index = 0; EFI_ERROR (Status) && Index < BiosVideoPrivate->MaxMode; Index++) {
        Status = BiosVideoUgaDrawSetMode (
                  &BiosVideoPrivate->UgaDraw,
                  BiosVideoPrivate->ModeData[Index].HorizontalResolution,
                  BiosVideoPrivate->ModeData[Index].VerticalResolution,
                  BiosVideoPrivate->ModeData[Index].ColorDepth,
                  BiosVideoPrivate->ModeData[Index].RefreshRate
                  );
      }
    }
  }

Done:
  //
  // If there was an error, then free the mode structure
  //
  if (EFI_ERROR (Status)) {
    if (BiosVideoPrivate->ModeData != NULL) {
      gBS->FreePool (BiosVideoPrivate->ModeData);
      BiosVideoPrivate->ModeData  = NULL;
      BiosVideoPrivate->MaxMode   = 0;
    }
  }

  return Status;
}

EFI_STATUS
BiosVideoCheckForVga (
  IN OUT BIOS_VIDEO_DEV  *BiosVideoPrivate  
  )
/*++
  
  Routine Description:

    Check for VGA device
  
  Arguments:
  
    BiosVideoPrivate - Pointer to BIOS_VIDEO_DEV structure
  
  Returns:
  
    EFI_SUCCESS - Standard VGA device found
  
--*/
{
  EFI_STATUS            Status;
  BIOS_VIDEO_MODE_DATA  *ModeBuffer;

  //
  // Fill in the UGA Draw Protocol
  //
  BiosVideoPrivate->UgaDraw.GetMode = BiosVideoUgaDrawGetMode;
  BiosVideoPrivate->UgaDraw.SetMode = BiosVideoUgaDrawSetMode;
  BiosVideoPrivate->UgaDraw.Blt     = BiosVideoUgaDrawVgaBlt;

  //
  // Add mode to the list of available modes
  //
  BiosVideoPrivate->MaxMode++;
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  BiosVideoPrivate->MaxMode * sizeof (BIOS_VIDEO_MODE_DATA),
                  (VOID **) &ModeBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (BiosVideoPrivate->MaxMode > 1) {
    gBS->CopyMem (
          ModeBuffer,
          BiosVideoPrivate->ModeData,
          (BiosVideoPrivate->MaxMode - 1) * sizeof (BIOS_VIDEO_MODE_DATA)
          );
  }

  if (BiosVideoPrivate->ModeData != NULL) {
    gBS->FreePool (BiosVideoPrivate->ModeData);
  }

  ModeBuffer[BiosVideoPrivate->MaxMode - 1].VbeModeNumber         = 0x0012;
  ModeBuffer[BiosVideoPrivate->MaxMode - 1].BytesPerScanLine      = 640;
  ModeBuffer[BiosVideoPrivate->MaxMode - 1].LinearFrameBuffer     = (VOID *) (UINTN)(0xa0000);
  ModeBuffer[BiosVideoPrivate->MaxMode - 1].HorizontalResolution  = 640;
  ModeBuffer[BiosVideoPrivate->MaxMode - 1].VerticalResolution    = 480;
  ModeBuffer[BiosVideoPrivate->MaxMode - 1].ColorDepth            = 32;
  ModeBuffer[BiosVideoPrivate->MaxMode - 1].RefreshRate           = 60;

  BiosVideoPrivate->ModeData = ModeBuffer;

  //
  // Test to see if the Video Adapter support the 640x480 16 color mode
  //
  Status = BiosVideoUgaDrawSetMode (&BiosVideoPrivate->UgaDraw, 640, 480, 32, 60);

  //
  // If there was an error, then free the mode structure
  //
  if (EFI_ERROR (Status)) {
    BiosVideoPrivate->MaxMode = 0;
    if (BiosVideoPrivate->ModeData != NULL) {
      gBS->FreePool (BiosVideoPrivate->ModeData);
    }
  }

  return Status;
}
//
// UGA Protocol Member Functions for VESA BIOS Extensions
//
EFI_STATUS
EFIAPI
BiosVideoUgaDrawGetMode (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  OUT UINT32                 *HorizontalResolution,
  OUT UINT32                 *VerticalResolution,
  OUT UINT32                 *ColorDepth,
  OUT UINT32                 *RefreshRate
  )
/*++

Routine Description:

  UGA protocol interface to get video mode

Arguments:

  This                  - Pointer to UGA draw protocol instance
  HorizontalResolution  - Horizontal Resolution, in pixels
  VerticalResolution    - Vertical Resolution, in pixels
  ColorDepth            - Bit number used to represent color value of a pixel 
  RefreshRate           - Refresh rate, in Hertz

Returns:

  EFI_DEVICE_ERROR - Hardware need starting
  EFI_INVALID_PARAMETER - Invalid parameter passed in
  EFI_SUCCESS - Video mode query successfully

--*/
{
  BIOS_VIDEO_DEV  *BiosVideoPrivate;

  BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_UGA_DRAW_THIS (This);

  if (BiosVideoPrivate->HardwareNeedsStarting) {
    return EFI_DEVICE_ERROR;
  }

  if (HorizontalResolution == NULL || VerticalResolution == NULL || ColorDepth == NULL || RefreshRate == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *HorizontalResolution = BiosVideoPrivate->ModeData[BiosVideoPrivate->CurrentMode].HorizontalResolution;
  *VerticalResolution   = BiosVideoPrivate->ModeData[BiosVideoPrivate->CurrentMode].VerticalResolution;
  *ColorDepth           = BiosVideoPrivate->ModeData[BiosVideoPrivate->CurrentMode].ColorDepth;
  *RefreshRate          = BiosVideoPrivate->ModeData[BiosVideoPrivate->CurrentMode].RefreshRate;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BiosVideoUgaDrawSetMode (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  IN  UINT32                 HorizontalResolution,
  IN  UINT32                 VerticalResolution,
  IN  UINT32                 ColorDepth,
  IN  UINT32                 RefreshRate
  )
/*++

Routine Description:

  UGA draw protocol interface to set video mode

Arguments:

  This                  - Pointer to UGA draw protocol instance
  HorizontalResolution  - Horizontal Resolution, in pixels
  VerticalResolution    - Vertical Resolution, in pixels
  ColorDepth            - Bit number used to represent color value of a pixel 
  RefreshRate           - Refresh rate, in Hertz

Returns:

  EFI_DEVICE_ERROR - Device error
  EFI_SUCCESS - Video mode set successfully
  EFI_UNSUPPORTED - Cannot support this video mode

--*/
{
  EFI_STATUS            Status;
  BIOS_VIDEO_DEV        *BiosVideoPrivate;
  UINTN                 Index;
  EFI_IA32_REGISTER_SET Regs;

  BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_UGA_DRAW_THIS (This);

  for (Index = 0; Index < BiosVideoPrivate->MaxMode; Index++) {

    if (HorizontalResolution != BiosVideoPrivate->ModeData[Index].HorizontalResolution) {
      continue;
    }

    if (VerticalResolution != BiosVideoPrivate->ModeData[Index].VerticalResolution) {
      continue;
    }

    if (ColorDepth != BiosVideoPrivate->ModeData[Index].ColorDepth) {
      continue;
    }

    if (RefreshRate != BiosVideoPrivate->ModeData[Index].RefreshRate) {
      continue;
    }

    if (BiosVideoPrivate->LineBuffer) {
      gBS->FreePool (BiosVideoPrivate->LineBuffer);
    }

    if (BiosVideoPrivate->VgaFrameBuffer) {
      gBS->FreePool (BiosVideoPrivate->VgaFrameBuffer);
    }

    if (BiosVideoPrivate->VbeFrameBuffer) {
      gBS->FreePool (BiosVideoPrivate->VbeFrameBuffer);
    }

    BiosVideoPrivate->LineBuffer = NULL;
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    BiosVideoPrivate->ModeData[Index].BytesPerScanLine,
                    (VOID**)&BiosVideoPrivate->LineBuffer
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Clear all registers
    //
    gBS->SetMem (&Regs, sizeof (Regs), 0);

    if (BiosVideoPrivate->ModeData[Index].VbeModeNumber < 0x100) {
      //
      // Allocate a working buffer for BLT operations to the VGA frame buffer
      //
      BiosVideoPrivate->VgaFrameBuffer = NULL;
      Status = gBS->AllocatePool (
                      EfiBootServicesData,
                      4 * 480 * 80,
                      (VOID**)&BiosVideoPrivate->VgaFrameBuffer
                      );
      if (EFI_ERROR (Status)) {
        return Status;
      }
      //
      // Set VGA Mode
      //
      Regs.X.AX = BiosVideoPrivate->ModeData[Index].VbeModeNumber;
      LegacyBiosInt86 (0x10, &Regs);

    } else {
      //
      // Allocate a working buffer for BLT operations to the VBE frame buffer
      //
      BiosVideoPrivate->VbeFrameBuffer = NULL;
      Status = gBS->AllocatePool (
                      EfiBootServicesData,
                      BiosVideoPrivate->ModeData[Index].BytesPerScanLine * BiosVideoPrivate->ModeData[Index].VerticalResolution,
                      (VOID**)&BiosVideoPrivate->VbeFrameBuffer
                      );
      if (EFI_ERROR (Status)) {
        return Status;
      }
      //
      // Set VBE mode
      //
      Regs.X.AX = VESA_BIOS_EXTENSIONS_SET_MODE;
      Regs.X.BX = (UINT16) (BiosVideoPrivate->ModeData[Index].VbeModeNumber | VESA_BIOS_EXTENSIONS_MODE_NUMBER_LINEAR_FRAME_BUFFER);
      gBS->SetMem (BiosVideoPrivate->VbeCrtcInformationBlock, sizeof (VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK), 0);
      Regs.X.ES = EFI_SEGMENT ((UINTN)BiosVideoPrivate->VbeCrtcInformationBlock);
      Regs.X.DI = EFI_OFFSET ((UINTN)BiosVideoPrivate->VbeCrtcInformationBlock);
      LegacyBiosInt86 (0x10, &Regs);

      //
      // Check to see if the call succeeded
      //
      if (Regs.X.AX != VESA_BIOS_EXTENSIONS_STATUS_SUCCESS) {
        return EFI_DEVICE_ERROR;
      }
      //
      // Initialize the state of the VbeFrameBuffer
      //
      Status = BiosVideoPrivate->PciIo->Mem.Read (
                                              BiosVideoPrivate->PciIo,
                                              EfiPciIoWidthUint32,
                                              EFI_PCI_IO_PASS_THROUGH_BAR,
                                              (UINT64) (UINTN) BiosVideoPrivate->ModeData[Index].LinearFrameBuffer,
                                              (BiosVideoPrivate->ModeData[Index].BytesPerScanLine * BiosVideoPrivate->ModeData[Index].VerticalResolution) >> 2,
                                              BiosVideoPrivate->VbeFrameBuffer
                                              );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    BiosVideoPrivate->CurrentMode           = Index;

    BiosVideoPrivate->HardwareNeedsStarting = FALSE;

    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

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
/*++

Routine Description:

  Update physical frame buffer, copy 4 bytes block, then copy remaining bytes.

Arguments:

  PciIo             - The pointer of EFI_PCI_IO_PROTOCOL
  VbeBuffer         - The data to transfer to screen
  MemAddress        - Physical frame buffer base address
  DestinationX      - The X coordinate of the destination for BltOperation
  DestinationY      - The Y coordinate of the destination for BltOperation
  TotalBytes        - The total bytes of copy
  VbePixelWidth     - Bytes per pixel
  BytesPerScanLine  - Bytes per scan line

Returns:

  None.

--*/
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

//
// BUGBUG : Add Blt for 16 bit color, 15 bit color, and 8 bit color modes
//
EFI_STATUS
EFIAPI
BiosVideoUgaDrawVbeBlt (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  IN  EFI_UGA_PIXEL          *BltBuffer, OPTIONAL
  IN  EFI_UGA_BLT_OPERATION  BltOperation,
  IN  UINTN                  SourceX,
  IN  UINTN                  SourceY,
  IN  UINTN                  DestinationX,
  IN  UINTN                  DestinationY,
  IN  UINTN                  Width,
  IN  UINTN                  Height,
  IN  UINTN                  Delta
  )
/*++

Routine Description:

  UGA draw protocol instance to block transfer for VBE device

Arguments:

  This          - Pointer to UGA draw protocol instance
  BltBuffer     - The data to transfer to screen
  BltOperation  - The operation to perform
  SourceX       - The X coordinate of the source for BltOperation
  SourceY       - The Y coordinate of the source for BltOperation
  DestinationX  - The X coordinate of the destination for BltOperation
  DestinationY  - The Y coordinate of the destination for BltOperation
  Width         - The width of a rectangle in the blt rectangle in pixels
  Height        - The height of a rectangle in the blt rectangle in pixels
  Delta         - Not used for EfiUgaVideoFill and EfiUgaVideoToVideo operation.
                  If a Delta of 0 is used, the entire BltBuffer will be operated on.
                  If a subrectangle of the BltBuffer is used, then Delta represents 
                  the number of bytes in a row of the BltBuffer.

Returns:

  EFI_INVALID_PARAMETER - Invalid parameter passed in
  EFI_SUCCESS - Blt operation success

--*/
{
  BIOS_VIDEO_DEV        *BiosVideoPrivate;
  BIOS_VIDEO_MODE_DATA  *Mode;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  EFI_TPL               OriginalTPL;
  UINTN                 DstY;
  UINTN                 SrcY;
  UINTN                 DstX;
  EFI_UGA_PIXEL         *Blt;
  VOID                  *MemAddress;
  EFI_UGA_PIXEL         *VbeFrameBuffer;
  UINTN                 BytesPerScanLine;
  UINTN                 Index;
  UINT8                 *VbeBuffer;
  UINT8                 *VbeBuffer1;
  UINT8                 *BltUint8;
  UINT32                VbePixelWidth;
  UINT32                Pixel;
  UINTN                 TotalBytes;

  BiosVideoPrivate  = BIOS_VIDEO_DEV_FROM_UGA_DRAW_THIS (This);
  Mode              = &BiosVideoPrivate->ModeData[BiosVideoPrivate->CurrentMode];
  PciIo             = BiosVideoPrivate->PciIo;

  VbeFrameBuffer    = BiosVideoPrivate->VbeFrameBuffer;
  MemAddress        = Mode->LinearFrameBuffer;
  BytesPerScanLine  = Mode->BytesPerScanLine;
  VbePixelWidth     = Mode->BitsPerPixel / 8;
  BltUint8          = (UINT8 *) BltBuffer;
  TotalBytes        = Width * VbePixelWidth;

  if ((BltOperation < EfiUgaVideoFill) || (BltOperation >= EfiUgaBltMax)) {
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
  if (BltOperation == EfiUgaVideoToBltBuffer) {
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
    Delta = Width * sizeof (EFI_UGA_PIXEL);
  }
  //
  // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
  // We would not want a timer based event (Cursor, ...) to come in while we are
  // doing this operation.
  //
  OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

  switch (BltOperation) {
  case EfiUgaVideoToBltBuffer:
    for (SrcY = SourceY, DstY = DestinationY; DstY < (Height + DestinationY); SrcY++, DstY++) {
      Blt = (EFI_UGA_PIXEL *) (BltUint8 + DstY * Delta + DestinationX * sizeof (EFI_UGA_PIXEL));
      //
      // Shuffle the packed bytes in the hardware buffer to match EFI_UGA_PIXEL
      //
      VbeBuffer = ((UINT8 *) VbeFrameBuffer + (SrcY * BytesPerScanLine + SourceX * VbePixelWidth));
      for (DstX = DestinationX; DstX < (Width + DestinationX); DstX++) {
        Pixel         = *(UINT32 *) (VbeBuffer);
        Blt->Red      = (UINT8) ((Pixel >> Mode->Red.Position) & Mode->Red.Mask);
        Blt->Blue     = (UINT8) ((Pixel >> Mode->Blue.Position) & Mode->Blue.Mask);
        Blt->Green    = (UINT8) ((Pixel >> Mode->Green.Position) & Mode->Green.Mask);
        Blt->Reserved = 0;
        Blt++;
        VbeBuffer += VbePixelWidth;
      }

    }
    break;

  case EfiUgaVideoToVideo:
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

  case EfiUgaVideoFill:
    VbeBuffer = (UINT8 *) ((UINTN) VbeFrameBuffer + (DestinationY * BytesPerScanLine) + DestinationX * VbePixelWidth);
    Blt       = (EFI_UGA_PIXEL *) BltUint8;
    //
    // Shuffle the RGB fields in EFI_UGA_PIXEL to match the hardware buffer
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

  case EfiUgaBltBufferToVideo:
    for (SrcY = SourceY, DstY = DestinationY; SrcY < (Height + SourceY); SrcY++, DstY++) {
      Blt       = (EFI_UGA_PIXEL *) (BltUint8 + (SrcY * Delta) + (SourceX) * sizeof (EFI_UGA_PIXEL));
      VbeBuffer = ((UINT8 *) VbeFrameBuffer + (DstY * BytesPerScanLine + DestinationX * VbePixelWidth));
      for (DstX = DestinationX; DstX < (Width + DestinationX); DstX++) {
        //
        // Shuffle the RGB fields in EFI_UGA_PIXEL to match the hardware buffer
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
  default:
    break;
  }

  gBS->RestoreTPL (OriginalTPL);

  return EFI_SUCCESS;
}

STATIC
VOID
WriteGraphicsController (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINTN                Address,
  IN  UINTN                Data
  )
/*++

Routine Description:

  Write graphics controller registers

Arguments:

  PciIo   - Pointer to PciIo protocol instance of the controller
  Address - Register address
  Data    - Data to be written to register

Returns:

  None

--*/
{
  Address = Address | (Data << 8);
  PciIo->Io.Write (
              PciIo,
              EfiPciIoWidthUint16,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              VGA_GRAPHICS_CONTROLLER_ADDRESS_REGISTER,
              1,
              &Address
              );
}

VOID
VgaReadBitPlanes (
  EFI_PCI_IO_PROTOCOL  *PciIo,
  UINT8                *HardwareBuffer,
  UINT8                *MemoryBuffer,
  UINTN                WidthInBytes,
  UINTN                Height
  )
/*++

Routine Description:

  Read the four bit plane of VGA frame buffer

Arguments:

  PciIo           - Pointer to PciIo protocol instance of the controller
  HardwareBuffer  - Hardware VGA frame buffer address
  MemoryBuffer    - Memory buffer address
  WidthInBytes    - Number of bytes in a line to read
  Height          - Height of the area to read

Returns:

  None

--*/
{
  UINTN BitPlane;
  UINTN Rows;
  UINTN FrameBufferOffset;
  UINT8 *Source;
  UINT8 *Destination;

  //
  // Program the Mode Register Write mode 0, Read mode 0
  //
  WriteGraphicsController (
    PciIo,
    VGA_GRAPHICS_CONTROLLER_MODE_REGISTER,
    VGA_GRAPHICS_CONTROLLER_READ_MODE_0 | VGA_GRAPHICS_CONTROLLER_WRITE_MODE_0
    );

  for (BitPlane = 0, FrameBufferOffset = 0;
       BitPlane < VGA_NUMBER_OF_BIT_PLANES;
       BitPlane++, FrameBufferOffset += VGA_BYTES_PER_BIT_PLANE
      ) {
    //
    // Program the Read Map Select Register to select the correct bit plane
    //
    WriteGraphicsController (
      PciIo,
      VGA_GRAPHICS_CONTROLLER_READ_MAP_SELECT_REGISTER,
      BitPlane
      );

    Source      = HardwareBuffer;
    Destination = MemoryBuffer + FrameBufferOffset;

    for (Rows = 0; Rows < Height; Rows++, Source += VGA_BYTES_PER_SCAN_LINE, Destination += VGA_BYTES_PER_SCAN_LINE) {
      PciIo->Mem.Read (
                  PciIo,
                  EfiPciIoWidthUint8,
                  EFI_PCI_IO_PASS_THROUGH_BAR,
                  (UINT64) (UINTN)Source,
                  WidthInBytes,
                  (VOID *) Destination
                  );
    }
  }
}

VOID
VgaConvertToUgaColor (
  UINT8          *MemoryBuffer,
  UINTN          X,
  UINTN          Y,
  EFI_UGA_PIXEL  *BltBuffer
  )
/*++

Routine Description:

  Internal routine to convert VGA color to UGA color

Arguments:

  MemoryBuffer  - Buffer containing VGA color
  X             - The X coordinate of pixel on screen
  Y             - The Y coordinate of pixel on screen
  BltBuffer     - Buffer to contain converted UGA color

Returns:

  None

--*/
{
  UINTN Mask;
  UINTN Bit;
  UINTN Color;

  MemoryBuffer += ((Y << 6) + (Y << 4) + (X >> 3));
  Mask = mVgaBitMaskTable[X & 0x07];
  for (Bit = 0x01, Color = 0; Bit < 0x10; Bit <<= 1, MemoryBuffer += VGA_BYTES_PER_BIT_PLANE) {
    if (*MemoryBuffer & Mask) {
      Color |= Bit;
    }
  }

  *BltBuffer = mVgaColorToUgaColor[Color];
}

UINT8
VgaConvertColor (
  IN  EFI_UGA_PIXEL          *BltBuffer
  )
/*++

Routine Description:

  Internal routine to convert UGA color to VGA color

Arguments:

  BltBuffer - buffer containing UGA color

Returns:

  Converted VGA color

--*/
{
  UINT8 Color;

  Color = (UINT8) ((BltBuffer->Blue >> 7) | ((BltBuffer->Green >> 6) & 0x02) | ((BltBuffer->Red >> 5) & 0x04));
  if ((BltBuffer->Red + BltBuffer->Green + BltBuffer->Blue) > 0x180) {
    Color |= 0x08;
  }

  return Color;
}

EFI_STATUS
EFIAPI
BiosVideoUgaDrawVgaBlt (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  IN  EFI_UGA_PIXEL          *BltBuffer, OPTIONAL
  IN  EFI_UGA_BLT_OPERATION  BltOperation,
  IN  UINTN                  SourceX,
  IN  UINTN                  SourceY,
  IN  UINTN                  DestinationX,
  IN  UINTN                  DestinationY,
  IN  UINTN                  Width,
  IN  UINTN                  Height,
  IN  UINTN                  Delta
  )
/*++

Routine Description:

  UGA draw protocol instance to block transfer for VGA device

Arguments:

  This          - Pointer to UGA draw protocol instance
  BltBuffer     - The data to transfer to screen
  BltOperation  - The operation to perform
  SourceX       - The X coordinate of the source for BltOperation
  SourceY       - The Y coordinate of the source for BltOperation
  DestinationX  - The X coordinate of the destination for BltOperation
  DestinationY  - The Y coordinate of the destination for BltOperation
  Width         - The width of a rectangle in the blt rectangle in pixels
  Height        - The height of a rectangle in the blt rectangle in pixels
  Delta         - Not used for EfiUgaVideoFill and EfiUgaVideoToVideo operation.
                  If a Delta of 0 is used, the entire BltBuffer will be operated on.
                  If a subrectangle of the BltBuffer is used, then Delta represents 
                  the number of bytes in a row of the BltBuffer.

Returns:

  EFI_INVALID_PARAMETER - Invalid parameter passed in
  EFI_SUCCESS - Blt operation success

--*/
{
  BIOS_VIDEO_DEV      *BiosVideoPrivate;
  EFI_TPL             OriginalTPL;
  UINT8               *MemAddress;
  UINTN               BytesPerScanLine;
  UINTN               BytesPerBitPlane;
  UINTN               Bit;
  UINTN               Index;
  UINTN               Index1;
  UINTN               StartAddress;
  UINTN               Bytes;
  UINTN               Offset;
  UINT8               LeftMask;
  UINT8               RightMask;
  UINTN               Address;
  UINTN               AddressFix;
  UINT8               *Address1;
  UINT8               *SourceAddress;
  UINT8               *DestinationAddress;
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT8               Data;
  UINT8               PixelColor;
  UINT8               *VgaFrameBuffer;
  UINTN               SourceOffset;
  UINTN               SourceWidth;
  UINTN               Rows;
  UINTN               Columns;
  UINTN               X;
  UINTN               Y;

  BiosVideoPrivate  = BIOS_VIDEO_DEV_FROM_UGA_DRAW_THIS (This);

  PciIo             = BiosVideoPrivate->PciIo;
  MemAddress        = BiosVideoPrivate->ModeData[BiosVideoPrivate->CurrentMode].LinearFrameBuffer;
  BytesPerScanLine  = BiosVideoPrivate->ModeData[BiosVideoPrivate->CurrentMode].BytesPerScanLine >> 3;
  BytesPerBitPlane  = BytesPerScanLine * BiosVideoPrivate->ModeData[BiosVideoPrivate->CurrentMode].VerticalResolution;
  VgaFrameBuffer    = BiosVideoPrivate->VgaFrameBuffer;

  if ((BltOperation < EfiUgaVideoFill) || (BltOperation >= EfiUgaBltMax)) {
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
  if (BltOperation == EfiUgaVideoToBltBuffer) {
    //
    // Video to BltBuffer: Source is Video, destination is BltBuffer
    //
    if (SourceY + Height > BiosVideoPrivate->ModeData[BiosVideoPrivate->CurrentMode].VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (SourceX + Width > BiosVideoPrivate->ModeData[BiosVideoPrivate->CurrentMode].HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // BltBuffer to Video: Source is BltBuffer, destination is Video
    //
    if (DestinationY + Height > BiosVideoPrivate->ModeData[BiosVideoPrivate->CurrentMode].VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestinationX + Width > BiosVideoPrivate->ModeData[BiosVideoPrivate->CurrentMode].HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // If Delta is zero, then the entire BltBuffer is being used, so Delta
  // is the number of bytes in each row of BltBuffer.  Since BltBuffer is Width pixels size,
  // the number of bytes in each row can be computed.
  //
  if (Delta == 0) {
    Delta = Width * sizeof (EFI_UGA_PIXEL);
  }
  //
  // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
  // We would not want a timer based event (Cursor, ...) to come in while we are
  // doing this operation.
  //
  OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Compute some values we need for VGA
  //
  switch (BltOperation) {
  case EfiUgaVideoToBltBuffer:

    SourceOffset  = (SourceY << 6) + (SourceY << 4) + (SourceX >> 3);
    SourceWidth   = ((SourceX + Width - 1) >> 3) - (SourceX >> 3) + 1;

    //
    // Read all the pixels in the 4 bit planes into a memory buffer that looks like the VGA buffer
    //
    VgaReadBitPlanes (
      PciIo,
      MemAddress + SourceOffset,
      VgaFrameBuffer + SourceOffset,
      SourceWidth,
      Height
      );

    //
    // Convert VGA Bit Planes to a UGA 32-bit color value
    //
    BltBuffer += (DestinationY * (Delta >> 2) + DestinationX);
    for (Rows = 0, Y = SourceY; Rows < Height; Rows++, Y++, BltBuffer += (Delta >> 2)) {
      for (Columns = 0, X = SourceX; Columns < Width; Columns++, X++, BltBuffer++) {
        VgaConvertToUgaColor (VgaFrameBuffer, X, Y, BltBuffer);
      }

      BltBuffer -= Width;
    }

    break;

  case EfiUgaVideoToVideo:
    //
    // Check for an aligned Video to Video operation
    //
    if ((SourceX & 0x07) == 0x00 && (DestinationX & 0x07) == 0x00 && (Width & 0x07) == 0x00) {
      //
      // Program the Mode Register Write mode 1, Read mode 0
      //
      WriteGraphicsController (
        BiosVideoPrivate->PciIo,
        VGA_GRAPHICS_CONTROLLER_MODE_REGISTER,
        VGA_GRAPHICS_CONTROLLER_READ_MODE_0 | VGA_GRAPHICS_CONTROLLER_WRITE_MODE_1
        );

      SourceAddress       = (UINT8 *) (MemAddress + (SourceY << 6) + (SourceY << 4) + (SourceX >> 3));
      DestinationAddress  = (UINT8 *) (MemAddress + (DestinationY << 6) + (DestinationY << 4) + (DestinationX >> 3));
      Bytes               = Width >> 3;
      for (Index = 0, Offset = 0; Index < Height; Index++, Offset += BytesPerScanLine) {
        PciIo->CopyMem (
                PciIo,
                EfiPciIoWidthUint8,
                EFI_PCI_IO_PASS_THROUGH_BAR,
                (UINT64) ((UINTN)DestinationAddress + Offset),
                EFI_PCI_IO_PASS_THROUGH_BAR,
                (UINT64) ((UINTN)SourceAddress + Offset),
                Bytes
                );
      }
    } else {
      SourceOffset  = (SourceY << 6) + (SourceY << 4) + (SourceX >> 3);
      SourceWidth   = ((SourceX + Width - 1) >> 3) - (SourceX >> 3) + 1;

      //
      // Read all the pixels in the 4 bit planes into a memory buffer that looks like the VGA buffer
      //
      VgaReadBitPlanes (
        PciIo,
        MemAddress + SourceOffset,
        VgaFrameBuffer + SourceOffset,
        SourceWidth,
        Height
        );
    }

    break;

  case EfiUgaVideoFill:
    StartAddress  = (UINTN) (MemAddress + (DestinationY << 6) + (DestinationY << 4) + (DestinationX >> 3));
    Bytes         = ((DestinationX + Width - 1) >> 3) - (DestinationX >> 3);
    LeftMask      = mVgaLeftMaskTable[DestinationX & 0x07];
    RightMask     = mVgaRightMaskTable[(DestinationX + Width - 1) & 0x07];
    if (Bytes == 0) {
      LeftMask &= RightMask;
      RightMask = 0;
    }

    if (LeftMask == 0xff) {
      StartAddress--;
      Bytes++;
      LeftMask = 0;
    }

    if (RightMask == 0xff) {
      Bytes++;
      RightMask = 0;
    }

    PixelColor = VgaConvertColor (BltBuffer);

    //
    // Program the Mode Register Write mode 2, Read mode 0
    //
    WriteGraphicsController (
      BiosVideoPrivate->PciIo,
      VGA_GRAPHICS_CONTROLLER_MODE_REGISTER,
      VGA_GRAPHICS_CONTROLLER_READ_MODE_0 | VGA_GRAPHICS_CONTROLLER_WRITE_MODE_2
      );

    //
    // Program the Data Rotate/Function Select Register to replace
    //
    WriteGraphicsController (
      BiosVideoPrivate->PciIo,
      VGA_GRAPHICS_CONTROLLER_DATA_ROTATE_REGISTER,
      VGA_GRAPHICS_CONTROLLER_FUNCTION_REPLACE
      );

    if (LeftMask != 0) {
      //
      // Program the BitMask register with the Left column mask
      //
      WriteGraphicsController (
        BiosVideoPrivate->PciIo,
        VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER,
        LeftMask
        );

      for (Index = 0, Address = StartAddress; Index < Height; Index++, Address += BytesPerScanLine) {
        //
        // Read data from the bit planes into the latches
        //
        PciIo->Mem.Read (
                    PciIo,
                    EfiPciIoWidthUint8,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) Address,
                    1,
                    &Data
                    );
        //
        // Write the lower 4 bits of PixelColor to the bit planes in the pixels enabled by BitMask
        //
        PciIo->Mem.Write (
                    PciIo,
                    EfiPciIoWidthUint8,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) Address,
                    1,
                    &PixelColor
                    );
      }
    }

    if (Bytes > 1) {
      //
      // Program the BitMask register with the middle column mask of 0xff
      //
      WriteGraphicsController (
        BiosVideoPrivate->PciIo,
        VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER,
        0xff
        );

      for (Index = 0, Address = StartAddress + 1; Index < Height; Index++, Address += BytesPerScanLine) {
        PciIo->Mem.Write (
                    PciIo,
                    EfiPciIoWidthFillUint8,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) Address,
                    Bytes - 1,
                    &PixelColor
                    );
      }
    }

    if (RightMask != 0) {
      //
      // Program the BitMask register with the Right column mask
      //
      WriteGraphicsController (
        BiosVideoPrivate->PciIo,
        VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER,
        RightMask
        );

      for (Index = 0, Address = StartAddress + Bytes; Index < Height; Index++, Address += BytesPerScanLine) {
        //
        // Read data from the bit planes into the latches
        //
        PciIo->Mem.Read (
                    PciIo,
                    EfiPciIoWidthUint8,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) Address,
                    1,
                    &Data
                    );
        //
        // Write the lower 4 bits of PixelColor to the bit planes in the pixels enabled by BitMask
        //
        PciIo->Mem.Write (
                    PciIo,
                    EfiPciIoWidthUint8,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) Address,
                    1,
                    &PixelColor
                    );
      }
    }
    break;

  case EfiUgaBltBufferToVideo:
    StartAddress  = (UINTN) (MemAddress + (DestinationY << 6) + (DestinationY << 4) + (DestinationX >> 3));
    LeftMask      = mVgaBitMaskTable[DestinationX & 0x07];

    //
    // Program the Mode Register Write mode 2, Read mode 0
    //
    WriteGraphicsController (
      BiosVideoPrivate->PciIo,
      VGA_GRAPHICS_CONTROLLER_MODE_REGISTER,
      VGA_GRAPHICS_CONTROLLER_READ_MODE_0 | VGA_GRAPHICS_CONTROLLER_WRITE_MODE_2
      );

    //
    // Program the Data Rotate/Function Select Register to replace
    //
    WriteGraphicsController (
      BiosVideoPrivate->PciIo,
      VGA_GRAPHICS_CONTROLLER_DATA_ROTATE_REGISTER,
      VGA_GRAPHICS_CONTROLLER_FUNCTION_REPLACE
      );

    for (Index = 0, Address = StartAddress; Index < Height; Index++, Address += BytesPerScanLine) {
      for (Index1 = 0; Index1 < Width; Index1++) {
        BiosVideoPrivate->LineBuffer[Index1] = VgaConvertColor (&BltBuffer[(SourceY + Index) * (Delta >> 2) + SourceX + Index1]);
      }
      AddressFix = Address;

      for (Bit = 0; Bit < 8; Bit++) {
        //
        // Program the BitMask register with the Left column mask
        //
        WriteGraphicsController (
          BiosVideoPrivate->PciIo,
          VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER,
          LeftMask
          );

        for (Index1 = Bit, Address1 = (UINT8 *) AddressFix; Index1 < Width; Index1 += 8, Address1++) {
          //
          // Read data from the bit planes into the latches
          //
          PciIo->Mem.Read (
                      PciIo,
                      EfiPciIoWidthUint8,
                      EFI_PCI_IO_PASS_THROUGH_BAR,
                      (UINT64) (UINTN) Address1,
                      1,
                      &Data
                      );

          PciIo->Mem.Write (
                      PciIo,
                      EfiPciIoWidthUint8,
                      EFI_PCI_IO_PASS_THROUGH_BAR,
                      (UINT64) (UINTN) Address1,
                      1,
                      &BiosVideoPrivate->LineBuffer[Index1]
                      );
        }

        LeftMask = (UINT8) (LeftMask >> 1);
        if (LeftMask == 0) {
          LeftMask = 0x80;
          AddressFix++;
        }
      }
    }

    break;
  default:
    break;
  }

  gBS->RestoreTPL (OriginalTPL);

  return EFI_SUCCESS;
}
//
// VGA Mini Port Protocol Functions
//
EFI_STATUS
EFIAPI
BiosVideoVgaMiniPortSetMode (
  IN  EFI_VGA_MINI_PORT_PROTOCOL  *This,
  IN  UINTN                       ModeNumber
  )
/*++

Routine Description:

  VgaMiniPort protocol interface to set mode

Arguments:

  This        - Pointer to VgaMiniPort protocol instance
  ModeNumber  - The index of the mode

Returns:

  EFI_UNSUPPORTED - The requested mode is not supported
  EFI_SUCCESS - The requested mode is set successfully

--*/
{
  BIOS_VIDEO_DEV        *BiosVideoPrivate;
  EFI_IA32_REGISTER_SET Regs;

  //
  // Make sure the ModeNumber is a valid value
  //
  if (ModeNumber >= This->MaxMode) {
    return EFI_UNSUPPORTED;
  }
  //
  // Get the device structure for this device
  //
  BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_VGA_MINI_PORT_THIS (This);

  gBS->SetMem (&Regs, sizeof (Regs), 0);

  switch (ModeNumber) {
  case 0:
    //
    // Set the 80x25 Text VGA Mode
    //
    Regs.H.AH = 0x00;
    Regs.H.AL = 0x83;
    LegacyBiosInt86 (0x10, &Regs);

    Regs.H.AH = 0x11;
    Regs.H.AL = 0x14;
    Regs.H.BL = 0;
    LegacyBiosInt86 (0x10, &Regs);
    break;

  case 1:
    //
    // Set the 80x50 Text VGA Mode
    //
    Regs.H.AH = 0x00;
    Regs.H.AL = 0x83;
    LegacyBiosInt86 (0x10, &Regs);
    Regs.H.AH = 0x11;
    Regs.H.AL = 0x12;
    Regs.H.BL = 0;
    LegacyBiosInt86 (0x10, &Regs);
    break;

  default:
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

VOID
InitializeBiosIntCaller (
  VOID
  )
{
  EFI_STATUS            Status;
  UINT32                RealModeBufferSize;
  UINT32                ExtraStackSize;
  EFI_PHYSICAL_ADDRESS  LegacyRegionBase;
  
  //
  // Get LegacyRegion
  //
  AsmGetThunk16Properties (&RealModeBufferSize, &ExtraStackSize);

  LegacyRegionBase = 0x100000;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES(RealModeBufferSize + ExtraStackSize + 200),
                  &LegacyRegionBase
                  );
  ASSERT_EFI_ERROR (Status);
  
  mThunkContext.RealModeBuffer     = (VOID*)(UINTN)LegacyRegionBase;
  mThunkContext.RealModeBufferSize = EFI_PAGES_TO_SIZE (RealModeBufferSize);
  mThunkContext.ThunkAttributes    = 3;
  AsmPrepareThunk16(&mThunkContext);
  
  //Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **) &mLegacy8259);
  //ASSERT_EFI_ERROR (Status);  
}

VOID
InitializeInterruptRedirection (
  VOID
  )
/*++

  Routine Description:
    Initialize interrupt redirection code and entries, because
    IDT Vectors 0x68-0x6f must be redirected to IDT Vectors 0x08-0x0f.
    Or the interrupt will lost when we do thunk.
    NOTE: We do not reset 8259 vector base, because it will cause pending
    interrupt lost.

  Arguments:
    NONE

  Returns:
    NONE
--*/
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  LegacyRegionBase;
  UINTN                 LegacyRegionLength;
  UINT32                *IdtArray;
  UINTN                 Index;
  UINT8                 ProtectedModeBaseVector;
  UINT32                InterruptRedirectionCode[] = {
    0x90CF08CD, // INT8; IRET; NOP
    0x90CF09CD, // INT9; IRET; NOP
    0x90CF0ACD, // INTA; IRET; NOP
    0x90CF0BCD, // INTB; IRET; NOP
    0x90CF0CCD, // INTC; IRET; NOP
    0x90CF0DCD, // INTD; IRET; NOP
    0x90CF0ECD, // INTE; IRET; NOP
    0x90CF0FCD  // INTF; IRET; NOP
  };

  //
  // Get LegacyRegion
  //
  LegacyRegionLength = sizeof(InterruptRedirectionCode);
  LegacyRegionBase = 0x100000;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES(LegacyRegionLength),
                  &LegacyRegionBase
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Copy code to legacy region
  //
  CopyMem ((VOID *)(UINTN)LegacyRegionBase, InterruptRedirectionCode, sizeof (InterruptRedirectionCode));

  //
  // Get VectorBase, it should be 0x68
  //
  Status = mLegacy8259->GetVector (mLegacy8259, Efi8259Irq0, &ProtectedModeBaseVector);
  ASSERT_EFI_ERROR (Status);

  //
  // Patch IVT 0x68 ~ 0x6f
  //
  IdtArray = (UINT32 *) 0;
  for (Index = 0; Index < 8; Index++) {
    IdtArray[ProtectedModeBaseVector + Index] = ((EFI_SEGMENT (LegacyRegionBase + Index * 4)) << 16) | (EFI_OFFSET (LegacyRegionBase + Index * 4));
  }

  return ;
}

BOOLEAN
EFIAPI
LegacyBiosInt86 (
  IN  UINT8                           BiosInt,
  IN  EFI_IA32_REGISTER_SET           *Regs
  )
/*++

  Routine Description:
    Thunk to 16-bit real mode and execute a software interrupt with a vector 
    of BiosInt. Regs will contain the 16-bit register context on entry and 
    exit.

  Arguments:
    This    - Protocol instance pointer.
    BiosInt - Processor interrupt vector to invoke
    Reg     - Register contexted passed into (and returned) from thunk to 
              16-bit mode

  Returns:
    FALSE   - Thunk completed, and there were no BIOS errors in the target code.
              See Regs for status.
    TRUE    - There was a BIOS erro in the target code.

--*/
{
  UINTN                 Status;
  UINTN                 Eflags;
  IA32_REGISTER_SET     ThunkRegSet;
  BOOLEAN               Ret;
  UINT16                *Stack16;
  
  Regs->X.Flags.Reserved1 = 1;
  Regs->X.Flags.Reserved2 = 0;
  Regs->X.Flags.Reserved3 = 0;
  Regs->X.Flags.Reserved4 = 0;
  Regs->X.Flags.IOPL      = 3;
  Regs->X.Flags.NT        = 0;
  Regs->X.Flags.IF        = 1;
  Regs->X.Flags.TF        = 0;
  Regs->X.Flags.CF        = 0;

  ZeroMem (&ThunkRegSet, sizeof (ThunkRegSet));
  ThunkRegSet.E.EDI  = Regs->E.EDI;
  ThunkRegSet.E.ESI  = Regs->E.ESI;
  ThunkRegSet.E.EBP  = Regs->E.EBP;
  ThunkRegSet.E.EBX  = Regs->E.EBX;
  ThunkRegSet.E.EDX  = Regs->E.EDX;
  ThunkRegSet.E.ECX  = Regs->E.ECX;
  ThunkRegSet.E.EAX  = Regs->E.EAX;
  ThunkRegSet.E.DS   = Regs->E.DS;
  ThunkRegSet.E.ES   = Regs->E.ES;

  CopyMem (&(ThunkRegSet.E.EFLAGS), &(Regs->E.EFlags), sizeof (UINT32));
 
  //
  // The call to Legacy16 is a critical section to EFI
  //
  Eflags = AsmReadEflags ();
  if ((Eflags | EFI_CPU_EFLAGS_IF) != 0) {
    DisableInterrupts ();
  }

  //
  // Set Legacy16 state. 0x08, 0x70 is legacy 8259 vector bases.
  //
  Status = mLegacy8259->SetMode (mLegacy8259, Efi8259LegacyMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);
  
  Stack16 = (UINT16 *)((UINT8 *) mThunkContext.RealModeBuffer + mThunkContext.RealModeBufferSize - sizeof (UINT16));
  Stack16 -= sizeof (ThunkRegSet.E.EFLAGS) / sizeof (UINT16);
  CopyMem (Stack16, &ThunkRegSet.E.EFLAGS, sizeof (ThunkRegSet.E.EFLAGS));

  ThunkRegSet.E.SS   = (UINT16) (((UINTN) Stack16 >> 16) << 12);
  ThunkRegSet.E.ESP  = (UINT16) (UINTN) Stack16;
  ThunkRegSet.E.Eip  = (UINT16)((UINT32 *)NULL)[BiosInt];
  ThunkRegSet.E.CS   = (UINT16)(((UINT32 *)NULL)[BiosInt] >> 16);
  mThunkContext.RealModeState = &ThunkRegSet;
  AsmThunk16 (&mThunkContext);

  //
  // Restore protected mode interrupt state
  //
  Status = mLegacy8259->SetMode (mLegacy8259, Efi8259ProtectedMode, NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // End critical section
  //
  if ((Eflags | EFI_CPU_EFLAGS_IF) != 0) {
    EnableInterrupts ();
  }

  Regs->E.EDI      = ThunkRegSet.E.EDI;      
  Regs->E.ESI      = ThunkRegSet.E.ESI;  
  Regs->E.EBP      = ThunkRegSet.E.EBP;  
  Regs->E.EBX      = ThunkRegSet.E.EBX;  
  Regs->E.EDX      = ThunkRegSet.E.EDX;  
  Regs->E.ECX      = ThunkRegSet.E.ECX;  
  Regs->E.EAX      = ThunkRegSet.E.EAX;
  Regs->E.SS       = ThunkRegSet.E.SS;
  Regs->E.CS       = ThunkRegSet.E.CS;  
  Regs->E.DS       = ThunkRegSet.E.DS;  
  Regs->E.ES       = ThunkRegSet.E.ES;

  CopyMem (&(Regs->E.EFlags), &(ThunkRegSet.E.EFLAGS), sizeof (UINT32));

  Ret = (BOOLEAN) (Regs->E.EFlags.CF == 1);

  return Ret;
}


