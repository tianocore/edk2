/** @file

 Copyright (c) 2011-2014, ARM Ltd. All rights reserved.<BR>
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LcdGraphicsOutputDxe.h"

BOOLEAN mDisplayInitialized = FALSE;

LCD_MODE LcdModes[] = {
  {
    0, 640, 480,
    9, 4,
    96, 16, 48,
    2, 10, 33
  },
  {
    1, 800, 600,
    11, 2,
    120, 56, 64,
    5, 37, 22
  },
  {
    2, 1024, 768,
    6, 2,
    96, 16, 48,
    2, 10, 33
  },
};

LCD_INSTANCE mLcdTemplate = {
  LCD_INSTANCE_SIGNATURE,
  NULL, // Handle
  { // ModeInfo
    0, // Version
    0, // HorizontalResolution
    0, // VerticalResolution
    PixelBltOnly, // PixelFormat
    {
      0xF800, //RedMask;
      0x7E0, //GreenMask;
      0x1F, //BlueMask;
      0x0//ReservedMask
    }, // PixelInformation
    0, // PixelsPerScanLine
  },
  { // Mode
    3, // MaxMode;
    0, // Mode;
    NULL, // Info;
    0, // SizeOfInfo;
    0, // FrameBufferBase;
    0 // FrameBufferSize;
  },
  { // Gop
    LcdGraphicsQueryMode,  // QueryMode
    LcdGraphicsSetMode,    // SetMode
    LcdGraphicsBlt,        // Blt
    NULL                     // *Mode
  },
  { // DevicePath
    {
      {
        HARDWARE_DEVICE_PATH, HW_VENDOR_DP,
        { (UINT8) (sizeof(VENDOR_DEVICE_PATH)), (UINT8) ((sizeof(VENDOR_DEVICE_PATH)) >> 8) },
      },
      // Hardware Device Path for Lcd
      EFI_CALLER_ID_GUID // Use the driver's GUID
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      { sizeof(EFI_DEVICE_PATH_PROTOCOL), 0}
    }
  }
};

EFI_STATUS
LcdInstanceContructor (
  OUT LCD_INSTANCE** NewInstance
  )
{
  LCD_INSTANCE* Instance;

  Instance = AllocateCopyPool (sizeof(LCD_INSTANCE), &mLcdTemplate);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->Gop.Mode          = &Instance->Mode;
  Instance->Mode.Info         = &Instance->ModeInfo;

  *NewInstance = Instance;
  return EFI_SUCCESS;
}

EFI_STATUS
LcdPlatformGetVram (
  OUT EFI_PHYSICAL_ADDRESS*  VramBaseAddress,
  OUT UINTN*                 VramSize
  )
{
  EFI_STATUS             Status;
  EFI_CPU_ARCH_PROTOCOL  *Cpu;
  UINTN                  MaxSize;

  MaxSize = 0x500000;
  *VramSize = MaxSize;

  // Allocate VRAM from DRAM
  Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES((MaxSize)), VramBaseAddress);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Ensure the Cpu architectural protocol is already installed
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&Cpu);
  ASSERT_EFI_ERROR(Status);

  // Mark the VRAM as un-cacheable. The VRAM is inside the DRAM, which is cacheable.
  Status = Cpu->SetMemoryAttributes (Cpu, *VramBaseAddress, *VramSize, EFI_MEMORY_UC);
  if (EFI_ERROR(Status)) {
    gBS->FreePool (VramBaseAddress);
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
DssSetMode (
  UINT32 VramBaseAddress,
  UINTN  ModeNumber
  )
{
  // Make sure the interface clock is running
  MmioWrite32 (CM_ICLKEN_DSS, EN_DSS);

  // Stop the functional clocks
  MmioAnd32 (CM_FCLKEN_DSS, ~(EN_DSS1 | EN_DSS2 | EN_TV));

  // Program the DSS clock divisor
  MmioWrite32 (CM_CLKSEL_DSS, 0x1000 | (LcdModes[ModeNumber].DssDivisor));

  // Start the functional clocks
  MmioOr32 (CM_FCLKEN_DSS, (EN_DSS1 | EN_DSS2 | EN_TV));

  // Wait for DSS to stabilize
  gBS->Stall(1);

  // Reset the subsystem
  MmioWrite32(DSS_SYSCONFIG, DSS_SOFTRESET);
  while (!(MmioRead32 (DSS_SYSSTATUS) & DSS_RESETDONE));

  // Configure LCD parameters
  MmioWrite32 (DISPC_SIZE_LCD,
               ((LcdModes[ModeNumber].HorizontalResolution - 1)
               | ((LcdModes[ModeNumber].VerticalResolution - 1) << 16))
              );
  MmioWrite32 (DISPC_TIMING_H,
               ( (LcdModes[ModeNumber].HSync - 1)
               | ((LcdModes[ModeNumber].HFrontPorch - 1) << 8)
               | ((LcdModes[ModeNumber].HBackPorch - 1) << 20))
              );
  MmioWrite32 (DISPC_TIMING_V,
               ( (LcdModes[ModeNumber].VSync - 1)
               | ((LcdModes[ModeNumber].VFrontPorch - 1) << 8)
               | ((LcdModes[ModeNumber].VBackPorch - 1) << 20))
              );

  // Set the framebuffer to only load frames (no gamma tables)
  MmioAnd32 (DISPC_CONFIG, CLEARLOADMODE);
  MmioOr32  (DISPC_CONFIG, LOAD_FRAME_ONLY);

  // Divisor for the pixel clock
  MmioWrite32(DISPC_DIVISOR, ((1 << 16) | LcdModes[ModeNumber].DispcDivisor) );

  // Set up the graphics layer
  MmioWrite32 (DISPC_GFX_PRELD, 0x2D8);
  MmioWrite32 (DISPC_GFX_BA0, VramBaseAddress);
  MmioWrite32 (DISPC_GFX_SIZE,
               ((LcdModes[ModeNumber].HorizontalResolution - 1)
               | ((LcdModes[ModeNumber].VerticalResolution - 1) << 16))
              );

  MmioWrite32(DISPC_GFX_ATTR, (GFXENABLE | RGB16 | BURSTSIZE16));

  // Start it all
  MmioOr32 (DISPC_CONTROL, (LCDENABLE | ACTIVEMATRIX | DATALINES24 | BYPASS_MODE | LCDENABLESIGNAL));
  MmioOr32 (DISPC_CONTROL, GOLCD);

  return EFI_SUCCESS;
}

EFI_STATUS
HwInitializeDisplay (
  UINTN VramBaseAddress,
  UINTN VramSize
  )
{
  EFI_STATUS    Status;
  UINT8         Data;
  EFI_TPL       OldTpl;
  EMBEDDED_EXTERNAL_DEVICE   *gTPS65950;

  // Enable power lines used by TFP410
  Status = gBS->LocateProtocol (&gEmbeddedExternalDeviceProtocolGuid, NULL, (VOID **)&gTPS65950);
  ASSERT_EFI_ERROR (Status);

  OldTpl = gBS->RaiseTPL(TPL_NOTIFY);
  Data = VAUX_DEV_GRP_P1;
  Status = gTPS65950->Write (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID4, VPLL2_DEV_GRP), 1, &Data);
  ASSERT_EFI_ERROR(Status);

  Data = VAUX_DEDICATED_18V;
  Status = gTPS65950->Write (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID4, VPLL2_DEDICATED), 1, &Data);
  ASSERT_EFI_ERROR (Status);

  // Power up TFP410 (set GPIO2 on TPS - for BeagleBoard-xM)
  Status = gTPS65950->Read (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID2, GPIODATADIR1), 1, &Data);
  ASSERT_EFI_ERROR (Status);
  Data |= BIT2;
  Status = gTPS65950->Write (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID2, GPIODATADIR1), 1, &Data);
  ASSERT_EFI_ERROR (Status);

  Data = BIT2;
  Status = gTPS65950->Write (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID2, SETGPIODATAOUT1), 1, &Data);
  ASSERT_EFI_ERROR (Status);

  gBS->RestoreTPL(OldTpl);

  // Power up TFP410 (set GPIO 170 - for older BeagleBoards)
  MmioAnd32 (GPIO6_BASE + GPIO_OE, ~BIT10);
  MmioOr32  (GPIO6_BASE + GPIO_SETDATAOUT, BIT10);

  return EFI_SUCCESS;
}

EFI_STATUS
InitializeDisplay (
  IN LCD_INSTANCE* Instance
  )
{
  EFI_STATUS           Status;
  UINTN                VramSize;
  EFI_PHYSICAL_ADDRESS VramBaseAddress;

  Status = LcdPlatformGetVram (&VramBaseAddress, &VramSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Instance->Mode.FrameBufferBase = VramBaseAddress;
  Instance->Mode.FrameBufferSize = VramSize;

  Status = HwInitializeDisplay((UINTN)VramBaseAddress, VramSize);
  if (!EFI_ERROR (Status)) {
    mDisplayInitialized = TRUE;
  }

  return Status;
}

EFI_STATUS
EFIAPI
LcdGraphicsQueryMode (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL            *This,
  IN UINT32                                  ModeNumber,
  OUT UINTN                                  *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION   **Info
  )
{
  LCD_INSTANCE  *Instance;

  Instance = LCD_INSTANCE_FROM_GOP_THIS(This);

  if (!mDisplayInitialized) {
    InitializeDisplay (Instance);
  }

  // Error checking
  if ( (This == NULL) || (Info == NULL) || (SizeOfInfo == NULL) || (ModeNumber >= This->Mode->MaxMode) ) {
    DEBUG((DEBUG_ERROR, "LcdGraphicsQueryMode: ERROR - For mode number %d : Invalid Parameter.\n", ModeNumber ));
    return EFI_INVALID_PARAMETER;
  }

  *Info = AllocateCopyPool(sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION), &Instance->ModeInfo);
  if (*Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

  (*Info)->Version = 0;
  (*Info)->HorizontalResolution = LcdModes[ModeNumber].HorizontalResolution;
  (*Info)->VerticalResolution = LcdModes[ModeNumber].VerticalResolution;
  (*Info)->PixelFormat = PixelBltOnly;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LcdGraphicsSetMode (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL   *This,
  IN UINT32                         ModeNumber
  )
{
  LCD_INSTANCE  *Instance;

  Instance = LCD_INSTANCE_FROM_GOP_THIS(This);

  if (ModeNumber >= Instance->Mode.MaxMode) {
    return EFI_UNSUPPORTED;
  }

  if (!mDisplayInitialized) {
    InitializeDisplay (Instance);
  }

  DssSetMode((UINT32)Instance->Mode.FrameBufferBase, ModeNumber);

  Instance->Mode.Mode = ModeNumber;
  Instance->ModeInfo.HorizontalResolution = LcdModes[ModeNumber].HorizontalResolution;
  Instance->ModeInfo.VerticalResolution = LcdModes[ModeNumber].VerticalResolution;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LcdGraphicsOutputDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  LCD_INSTANCE* Instance;

  Status = LcdInstanceContructor (&Instance);
  if (EFI_ERROR(Status)) {
    goto EXIT;
  }

  // Install the Graphics Output Protocol and the Device Path
  Status = gBS->InstallMultipleProtocolInterfaces(
             &Instance->Handle,
             &gEfiGraphicsOutputProtocolGuid, &Instance->Gop,
             &gEfiDevicePathProtocolGuid,     &Instance->DevicePath,
             NULL
             );

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "GraphicsOutputDxeInitialize: Can not install the protocol. Exit Status=%r\n", Status));
    goto EXIT;
  }

  // Register for an ExitBootServicesEvent
  // When ExitBootServices starts, this function here will make sure that the graphics driver will shut down properly,
  // i.e. it will free up all allocated memory and perform any necessary hardware re-configuration.
  /*Status = gBS->CreateEvent (
               EVT_SIGNAL_EXIT_BOOT_SERVICES,
               TPL_NOTIFY,
               LcdGraphicsExitBootServicesEvent, NULL,
               &Instance->ExitBootServicesEvent
               );

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "GraphicsOutputDxeInitialize: Can not install the ExitBootServicesEvent handler. Exit Status=%r\n", Status));
    goto EXIT_ERROR_UNINSTALL_PROTOCOL;
  }*/

  // To get here, everything must be fine, so just exit
  goto EXIT;

//EXIT_ERROR_UNINSTALL_PROTOCOL:
  /* The following function could return an error message,
   * however, to get here something must have gone wrong already,
   * so preserve the original error, i.e. don't change
   * the Status variable, even it fails to uninstall the protocol.
   */
  /*  gBS->UninstallMultipleProtocolInterfaces (
        Instance->Handle,
        &gEfiGraphicsOutputProtocolGuid, &Instance->Gop, // Uninstall Graphics Output protocol
        &gEfiDevicePathProtocolGuid,     &Instance->DevicePath,     // Uninstall device path
        NULL
        );*/

EXIT:
  return Status;

}
