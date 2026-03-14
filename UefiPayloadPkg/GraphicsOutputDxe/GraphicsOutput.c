/** @file
  Implementation for a generic GOP driver.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "GraphicsOutput.h"

#include <Guid/EventGroup.h>

#include <Library/PcdLib.h>

#define GRAPHICS_OUTPUT_MODE_PHYSICAL  0
#define GRAPHICS_OUTPUT_MODE_HIDPI     1

CONST ACPI_ADR_DEVICE_PATH  mGraphicsOutputAdrNode = {
  {
    ACPI_DEVICE_PATH,
    ACPI_ADR_DP,
    { sizeof (ACPI_ADR_DEVICE_PATH), 0 },
  },
  ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, 0, 0)
};

EFI_PEI_GRAPHICS_DEVICE_INFO_HOB  mDefaultGraphicsDeviceInfo = {
  MAX_UINT16, MAX_UINT16, MAX_UINT16, MAX_UINT16, MAX_UINT8, MAX_UINT8
};

//
// The driver should only start on one graphics controller.
// So a global flag is used to remember that the driver is already started.
//
BOOLEAN  mDriverStarted = FALSE;

/**
  Returns information for an available graphics mode that the graphics device
  and the set of active video output devices supports.

  @param  This                  The EFI_GRAPHICS_OUTPUT_PROTOCOL instance.
  @param  ModeNumber            The mode number to return information on.
  @param  SizeOfInfo            A pointer to the size, in bytes, of the Info buffer.
  @param  Info                  A pointer to callee allocated buffer that returns information about ModeNumber.

  @retval EFI_SUCCESS           Valid mode information was returned.
  @retval EFI_DEVICE_ERROR      A hardware error occurred trying to retrieve the video mode.
  @retval EFI_INVALID_PARAMETER ModeNumber is not valid.

**/
EFI_STATUS
EFIAPI
GraphicsOutputQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
{
  GRAPHICS_OUTPUT_PRIVATE_DATA                *Private;
  CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *ModeInfo;

  if ((This == NULL) || (Info == NULL) || (SizeOfInfo == NULL) || (ModeNumber >= This->Mode->MaxMode)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = GRAPHICS_OUTPUT_PRIVATE_FROM_THIS (This);

  ModeInfo = NULL;
  if (ModeNumber == GRAPHICS_OUTPUT_MODE_PHYSICAL) {
    ModeInfo = &Private->PhysicalModeInfo;
  } else if ((ModeNumber == GRAPHICS_OUTPUT_MODE_HIDPI) && Private->HasHiDpiMode) {
    ModeInfo = &Private->LogicalModeInfo;
  }

  if (ModeInfo == NULL) {
    return EFI_UNSUPPORTED;
  }

  *SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  *Info       = AllocateCopyPool (*SizeOfInfo, ModeInfo);
  return (*Info != NULL) ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;
}

/**
  Set the video device into the specified mode.

  @param  This        The EFI_GRAPHICS_OUTPUT_PROTOCOL instance.
  @param  ModeNumber  Abstraction that defines the current video mode.
  @param  ClearScreen TRUE to clear the screen to black in physical mode.

  @retval EFI_SUCCESS           The graphics mode specified by ModeNumber was selected.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       ModeNumber is not supported by this device.

**/
static EFI_STATUS
GraphicsOutputSetModeInternal (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN UINT32                        ModeNumber,
  IN BOOLEAN                       ClearScreen
  )
{
  GRAPHICS_OUTPUT_PRIVATE_DATA                *Private;
  CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *ModeInfo;
  UINT32                                      Scale;

  Private = GRAPHICS_OUTPUT_PRIVATE_FROM_THIS (This);

  ModeInfo = NULL;
  Scale    = 1;
  if (ModeNumber == GRAPHICS_OUTPUT_MODE_PHYSICAL) {
    ModeInfo = &Private->PhysicalModeInfo;
    Scale    = 1;
  } else if ((ModeNumber == GRAPHICS_OUTPUT_MODE_HIDPI) && Private->HasHiDpiMode) {
    ModeInfo = &Private->LogicalModeInfo;
    Scale    = 2;
  }

  if (ModeInfo == NULL) {
    return EFI_UNSUPPORTED;
  }

  This->Mode->Mode       = ModeNumber;
  This->Mode->Info       = (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *)ModeInfo;
  This->Mode->SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

  if (ModeNumber == GRAPHICS_OUTPUT_MODE_PHYSICAL) {
    This->Mode->FrameBufferBase = Private->PhysicalFrameBufferBase;
    This->Mode->FrameBufferSize = Private->PhysicalFrameBufferSize;
  } else {
    This->Mode->FrameBufferBase = 0;
    This->Mode->FrameBufferSize = 0;
  }

  Private->FrameBufferScale = Scale;

  if (ClearScreen) {
    RETURN_STATUS                  Status;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Black;

    Black.Blue     = 0;
    Black.Green    = 0;
    Black.Red      = 0;
    Black.Reserved = 0;

    Status = FrameBufferBlt (
               Private->FrameBufferBltLibConfigure,
               &Black,
               EfiBltVideoFill,
               0,
               0,
               0,
               0,
               Private->PhysicalModeInfo.HorizontalResolution,
               Private->PhysicalModeInfo.VerticalResolution,
               0
               );
    return RETURN_ERROR (Status) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
  }

  return EFI_SUCCESS;
}

/**
  Set the video device into the specified mode and clears the visible portions of
  the output display to black.

  @param  This              The EFI_GRAPHICS_OUTPUT_PROTOCOL instance.
  @param  ModeNumber        Abstraction that defines the current video mode.

  @retval EFI_SUCCESS       The graphics mode specified by ModeNumber was selected.
  @retval EFI_DEVICE_ERROR  The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED   ModeNumber is not supported by this device.

**/
EFI_STATUS
EFIAPI
GraphicsOutputSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN  UINT32                        ModeNumber
  )
{
  if ((This == NULL) || (ModeNumber >= This->Mode->MaxMode)) {
    return EFI_UNSUPPORTED;
  }

  return GraphicsOutputSetModeInternal (This, ModeNumber, TRUE);
}

/**
  ExitBootServices notification.

  Switch back to a mode that provides a direct framebuffer before launching the
  OS (e.g. for users that write directly to the framebuffer).

  @param  Event    The event signaled.
  @param  Context  The event context.

**/
static VOID
EFIAPI
GraphicsOutputExitBootServices (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  GRAPHICS_OUTPUT_PRIVATE_DATA  *Private;
  EFI_STATUS                    Status;

  (VOID)Event;

  Private = (GRAPHICS_OUTPUT_PRIVATE_DATA *)Context;
  if ((Private == NULL) || !Private->HasHiDpiMode) {
    return;
  }

  if (Private->GraphicsOutput.Mode->Mode == GRAPHICS_OUTPUT_MODE_HIDPI) {
    //
    // Switch back to a mode that provides a direct framebuffer before
    // launching the OS (e.g. for efifb or other direct-writes users).
    //
    GraphicsOutputSetModeInternal (&Private->GraphicsOutput, GRAPHICS_OUTPUT_MODE_PHYSICAL, FALSE);

    //
    // Restore the physical-mode resolution PCDs so late DXE text/graphics code
    // sees the real framebuffer geometry after the HiDPI GOP mode is dropped.
    //
    Status = PcdSet32S (PcdVideoHorizontalResolution, Private->PhysicalModeInfo.HorizontalResolution);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_VERBOSE, "[%a]: PcdVideoHorizontalResolution update failed: %r\n", gEfiCallerBaseName, Status));
    }

    Status = PcdSet32S (PcdVideoVerticalResolution, Private->PhysicalModeInfo.VerticalResolution);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_VERBOSE, "[%a]: PcdVideoVerticalResolution update failed: %r\n", gEfiCallerBaseName, Status));
    }

    Status = PcdSet32S (PcdSetupVideoHorizontalResolution, Private->PhysicalModeInfo.HorizontalResolution);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_VERBOSE, "[%a]: PcdSetupVideoHorizontalResolution update failed: %r\n", gEfiCallerBaseName, Status));
    }

    Status = PcdSet32S (PcdSetupVideoVerticalResolution, Private->PhysicalModeInfo.VerticalResolution);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_VERBOSE, "[%a]: PcdSetupVideoVerticalResolution update failed: %r\n", gEfiCallerBaseName, Status));
    }
  }
}

/**
  Scale a BLT buffer by 2x.

  @param  Src            Pointer to the source buffer.
  @param  SrcWidth       Source buffer width in pixels.
  @param  SrcHeight      Source buffer height in pixels.
  @param  SrcDeltaBytes  Source buffer stride in bytes.
  @param  Dst            Pointer to the destination buffer.

  @retval EFI_SUCCESS           The buffer was scaled successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
static EFI_STATUS
ScaleBltBuffer2x (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Src,
  IN  UINTN                          SrcWidth,
  IN  UINTN                          SrcHeight,
  IN  UINTN                          SrcDeltaBytes,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Dst
  )
{
  UINTN  Row;
  UINTN  Col;

  if ((Src == NULL) || (Dst == NULL) || (SrcWidth == 0) || (SrcHeight == 0) || (SrcDeltaBytes == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Row = 0; Row < SrcHeight; Row++) {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *SrcRow;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *DstRow0;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *DstRow1;

    SrcRow  = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)((UINT8 *)Src + Row * SrcDeltaBytes);
    DstRow0 = Dst + (Row * 2) * (SrcWidth * 2);
    DstRow1 = DstRow0 + (SrcWidth * 2);

    for (Col = 0; Col < SrcWidth; Col++) {
      EFI_GRAPHICS_OUTPUT_BLT_PIXEL  P;
      UINTN                          DstX;

      P    = SrcRow[Col];
      DstX = Col * 2;

      DstRow0[DstX]     = P;
      DstRow0[DstX + 1] = P;
      DstRow1[DstX]     = P;
      DstRow1[DstX + 1] = P;
    }
  }

  return EFI_SUCCESS;
}

/**
  Downscale a BLT buffer by 2x.

  @param  Src            Pointer to the source buffer.
  @param  SrcWidth       Source buffer width in pixels.
  @param  SrcHeight      Source buffer height in pixels.
  @param  SrcDeltaBytes  Source buffer stride in bytes.
  @param  Dst            Pointer to the destination buffer.
  @param  DstWidth       Destination buffer width in pixels.
  @param  DstHeight      Destination buffer height in pixels.
  @param  DstDeltaBytes  Destination buffer stride in bytes.

  @retval EFI_SUCCESS           The buffer was downscaled successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
static EFI_STATUS
DownscaleBltBuffer2x (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Src,
  IN  UINTN                          SrcWidth,
  IN  UINTN                          SrcHeight,
  IN  UINTN                          SrcDeltaBytes,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Dst,
  IN  UINTN                          DstWidth,
  IN  UINTN                          DstHeight,
  IN  UINTN                          DstDeltaBytes
  )
{
  UINTN  Row;
  UINTN  Col;

  if ((Src == NULL) || (Dst == NULL) || (DstWidth == 0) || (DstHeight == 0) ||
      (SrcDeltaBytes == 0) || (DstDeltaBytes == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((SrcWidth < DstWidth * 2) || (SrcHeight < DstHeight * 2)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Row = 0; Row < DstHeight; Row++) {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *DstRow;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *SrcRow;

    DstRow = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)((UINT8 *)Dst + Row * DstDeltaBytes);
    SrcRow = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)((UINT8 *)Src + (Row * 2) * SrcDeltaBytes);

    for (Col = 0; Col < DstWidth; Col++) {
      DstRow[Col] = SrcRow[Col * 2];
    }
  }

  return EFI_SUCCESS;
}

/**
  Blt a rectangle of pixels on the graphics screen. Blt stands for BLock Transfer.

  @param  This         Protocol instance pointer.
  @param  BltBuffer    The data to transfer to the graphics screen.
                       Size is at least Width*Height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL).
  @param  BltOperation The operation to perform when copying BltBuffer on to the graphics screen.
  @param  SourceX      The X coordinate of source for the BltOperation.
  @param  SourceY      The Y coordinate of source for the BltOperation.
  @param  DestinationX The X coordinate of destination for the BltOperation.
  @param  DestinationY The Y coordinate of destination for the BltOperation.
  @param  Width        The width of a rectangle in the blt rectangle in pixels.
  @param  Height       The height of a rectangle in the blt rectangle in pixels.
  @param  Delta        Not used for EfiBltVideoFill or the EfiBltVideoToVideo operation.
                       If a Delta of zero is used, the entire BltBuffer is being operated on.
                       If a subrectangle of the BltBuffer is being used then Delta
                       represents the number of bytes in a row of the BltBuffer.

  @retval EFI_SUCCESS           BltBuffer was drawn to the graphics screen.
  @retval EFI_INVALID_PARAMETER BltOperation is not valid.
  @retval EFI_DEVICE_ERROR      The device had an error and could not complete the request.

**/
EFI_STATUS
EFIAPI
GraphicsOutputBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer  OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta         OPTIONAL
  )
{
  RETURN_STATUS                 Status;
  EFI_TPL                       Tpl;
  GRAPHICS_OUTPUT_PRIVATE_DATA  *Private;
  UINT32                        Scale;

  Private = GRAPHICS_OUTPUT_PRIVATE_FROM_THIS (This);
  Scale   = Private->FrameBufferScale;

  if (Scale == 0) {
    Scale = 1;
  }

  if ((Scale != 1) && (Scale != 2)) {
    return EFI_UNSUPPORTED;
  }

  //
  // We have to raise to TPL_NOTIFY, so we make an atomic write to the frame buffer.
  // We would not want a timer based event (Cursor, ...) to come in while we are
  // doing this operation.
  //
  Tpl = gBS->RaiseTPL (TPL_NOTIFY);
  if (Scale == 1) {
    Status = FrameBufferBlt (
               Private->FrameBufferBltLibConfigure,
               BltBuffer,
               BltOperation,
               SourceX,
               SourceY,
               DestinationX,
               DestinationY,
               Width,
               Height,
               Delta
               );
  } else {
    switch (BltOperation) {
      case EfiBltVideoFill:
      case EfiBltVideoToVideo:
        Status = FrameBufferBlt (
                   Private->FrameBufferBltLibConfigure,
                   BltBuffer,
                   BltOperation,
                   SourceX * Scale,
                   SourceY * Scale,
                   DestinationX * Scale,
                   DestinationY * Scale,
                   Width * Scale,
                   Height * Scale,
                   Delta
                   );
        break;

      case EfiBltBufferToVideo:
      {
        EFI_STATUS                     EfiStatus;
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Temp;
        UINTN                          PixelSize;
        UINTN                          SrcDeltaBytes;
        UINTN                          TempWidth;
        UINTN                          TempHeight;
        UINTN                          TempSize;
        UINTN                          TempDeltaBytes;
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *SrcBase;

        if (BltBuffer == NULL) {
          Status = RETURN_INVALID_PARAMETER;
          break;
        }

        PixelSize  = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
        TempWidth  = Width * Scale;
        TempHeight = Height * Scale;

        if ((TempWidth / Scale != Width) || (TempHeight / Scale != Height)) {
          Status = RETURN_INVALID_PARAMETER;
          break;
        }

        SrcDeltaBytes = Delta;
        if (SrcDeltaBytes == 0) {
          if ((SourceX != 0) || (SourceY != 0)) {
            Status = RETURN_INVALID_PARAMETER;
            break;
          }

          SrcDeltaBytes = Width * PixelSize;
        }

        TempDeltaBytes = TempWidth * PixelSize;
        TempSize       = TempHeight * TempDeltaBytes;
        Temp           = AllocatePool (TempSize);
        if (Temp == NULL) {
          Status = RETURN_OUT_OF_RESOURCES;
          break;
        }

        SrcBase   = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)((UINT8 *)BltBuffer + SourceY * SrcDeltaBytes + SourceX * PixelSize);
        EfiStatus = ScaleBltBuffer2x (
                      SrcBase,
                      Width,
                      Height,
                      SrcDeltaBytes,
                      Temp
                      );
        if (EFI_ERROR (EfiStatus)) {
          FreePool (Temp);
          Status = RETURN_INVALID_PARAMETER;
          break;
        }

        Status = FrameBufferBlt (
                   Private->FrameBufferBltLibConfigure,
                   Temp,
                   EfiBltBufferToVideo,
                   0,
                   0,
                   DestinationX * Scale,
                   DestinationY * Scale,
                   TempWidth,
                   TempHeight,
                   TempDeltaBytes
                   );

        FreePool (Temp);
        break;
      }

      case EfiBltVideoToBltBuffer:
      {
        EFI_STATUS                     EfiStatus;
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Temp;
        UINTN                          PixelSize;
        UINTN                          DstDeltaBytes;
        UINTN                          TempWidth;
        UINTN                          TempHeight;
        UINTN                          TempSize;
        UINTN                          TempDeltaBytes;
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *DstBase;

        if (BltBuffer == NULL) {
          Status = RETURN_INVALID_PARAMETER;
          break;
        }

        PixelSize  = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
        TempWidth  = Width * Scale;
        TempHeight = Height * Scale;

        if ((TempWidth / Scale != Width) || (TempHeight / Scale != Height)) {
          Status = RETURN_INVALID_PARAMETER;
          break;
        }

        DstDeltaBytes = Delta;
        if (DstDeltaBytes == 0) {
          if ((DestinationX != 0) || (DestinationY != 0)) {
            Status = RETURN_INVALID_PARAMETER;
            break;
          }

          DstDeltaBytes = Width * PixelSize;
        }

        TempDeltaBytes = TempWidth * PixelSize;
        TempSize       = TempHeight * TempDeltaBytes;
        Temp           = AllocatePool (TempSize);
        if (Temp == NULL) {
          Status = RETURN_OUT_OF_RESOURCES;
          break;
        }

        Status = FrameBufferBlt (
                   Private->FrameBufferBltLibConfigure,
                   Temp,
                   EfiBltVideoToBltBuffer,
                   SourceX * Scale,
                   SourceY * Scale,
                   0,
                   0,
                   TempWidth,
                   TempHeight,
                   TempDeltaBytes
                   );
        if (RETURN_ERROR (Status)) {
          FreePool (Temp);
          break;
        }

        DstBase   = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)((UINT8 *)BltBuffer + DestinationY * DstDeltaBytes + DestinationX * PixelSize);
        EfiStatus = DownscaleBltBuffer2x (
                      Temp,
                      TempWidth,
                      TempHeight,
                      TempDeltaBytes,
                      DstBase,
                      Width,
                      Height,
                      DstDeltaBytes
                      );
        FreePool (Temp);
        Status = EFI_ERROR (EfiStatus) ? RETURN_INVALID_PARAMETER : RETURN_SUCCESS;
        break;
      }

      default:
        Status = RETURN_INVALID_PARAMETER;
        break;
    }
  }

  gBS->RestoreTPL (Tpl);

  return RETURN_ERROR (Status) ? EFI_INVALID_PARAMETER : EFI_SUCCESS;
}

CONST GRAPHICS_OUTPUT_PRIVATE_DATA  mGraphicsOutputInstanceTemplate = {
  GRAPHICS_OUTPUT_PRIVATE_DATA_SIGNATURE,          // Signature
  NULL,                                            // GraphicsOutputHandle
  {
    GraphicsOutputQueryMode,
    GraphicsOutputSetMode,
    GraphicsOutputBlt,
    NULL                                           // Mode
  },
  {
    1,                                             // MaxMode
    0,                                             // Mode
    NULL,                                          // Info
    sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION), // SizeOfInfo
    0,                                             // FrameBufferBase
    0                                              // FrameBufferSize
  },
  { 0 },                                           // LogicalModeInfo
  { 0 },                                           // PhysicalModeInfo
  0,                                               // FrameBufferScale
  FALSE,                                           // HasHiDpiMode
  NULL,                                            // ExitBootServicesEvent
  0,                                               // PhysicalFrameBufferBase
  0,                                               // PhysicalFrameBufferSize
  NULL,                                            // DevicePath
  NULL,                                            // PciIo
  0,                                               // PciAttributes
  NULL,                                            // FrameBufferBltLibConfigure
  0                                                // FrameBufferBltLibConfigureSize
};

/**
  Test whether the Controller can be managed by the driver.

  @param  This                 Driver Binding protocol instance pointer.
  @param  Controller           The PCI controller.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          The driver can manage the video device.
  @retval other                The driver cannot manage the video device.
**/
EFI_STATUS
EFIAPI
GraphicsOutputDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  //
  // Since there is only one GraphicsInfo HOB, the driver only manages one video device.
  //
  if (mDriverStarted) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Test the PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    Status = EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Test the DevicePath protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    Status = EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  if ((RemainingDevicePath == NULL) ||
      IsDevicePathEnd (RemainingDevicePath) ||
      (CompareMem (RemainingDevicePath, &mGraphicsOutputAdrNode, sizeof (mGraphicsOutputAdrNode)) == 0))
  {
    return EFI_SUCCESS;
  } else {
    return EFI_INVALID_PARAMETER;
  }
}

/**
  Start the video controller.

  @param  This                 Driver Binding protocol instance pointer.
  @param  ControllerHandle     The PCI controller.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          The driver starts to manage the video device.
  @retval other                The driver cannot manage the video device.
**/
EFI_STATUS
EFIAPI
GraphicsOutputDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                         Status;
  RETURN_STATUS                      ReturnStatus;
  GRAPHICS_OUTPUT_PRIVATE_DATA       *Private;
  EFI_PCI_IO_PROTOCOL                *PciIo;
  EFI_DEVICE_PATH                    *PciDevicePath;
  PCI_TYPE00                         Pci;
  UINT8                              Index;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Resources;
  VOID                               *HobStart;
  EFI_PEI_GRAPHICS_INFO_HOB          *GraphicsInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB   *DeviceInfo;
  EFI_PHYSICAL_ADDRESS               FrameBufferBase;

  FrameBufferBase = 0;

  HobStart = GetFirstGuidHob (&gEfiGraphicsInfoHobGuid);
  ASSERT ((HobStart != NULL) && (GET_GUID_HOB_DATA_SIZE (HobStart) == sizeof (EFI_PEI_GRAPHICS_INFO_HOB)));
  GraphicsInfo = (EFI_PEI_GRAPHICS_INFO_HOB *)(GET_GUID_HOB_DATA (HobStart));

  HobStart = GetFirstGuidHob (&gEfiGraphicsDeviceInfoHobGuid);
  if ((HobStart == NULL) || (GET_GUID_HOB_DATA_SIZE (HobStart) < sizeof (*DeviceInfo))) {
    //
    // Use default device infomation when the device info HOB doesn't exist
    //
    DeviceInfo = &mDefaultGraphicsDeviceInfo;
    DEBUG ((DEBUG_INFO, "[%a]: GraphicsDeviceInfo HOB doesn't exist!\n", gEfiCallerBaseName));
  } else {
    DeviceInfo = (EFI_PEI_GRAPHICS_DEVICE_INFO_HOB *)(GET_GUID_HOB_DATA (HobStart));
    DEBUG ((
      DEBUG_INFO,
      "[%a]: GraphicsDeviceInfo HOB:\n"
      "  VendorId = %04x, DeviceId = %04x,\n"
      "  RevisionId = %02x, BarIndex = %x,\n"
      "  SubsystemVendorId = %04x, SubsystemId = %04x\n",
      gEfiCallerBaseName,
      DeviceInfo->VendorId,
      DeviceInfo->DeviceId,
      DeviceInfo->RevisionId,
      DeviceInfo->BarIndex,
      DeviceInfo->SubsystemVendorId,
      DeviceInfo->SubsystemId
      ));
  }

  //
  // Open the PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    Status = EFI_SUCCESS;
  }

  ASSERT_EFI_ERROR (Status);

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&PciDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    Status = EFI_SUCCESS;
  }

  ASSERT_EFI_ERROR (Status);

  //
  // Read the PCI Class Code from the PCI Device
  //
  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0, sizeof (Pci), &Pci);
  if (!EFI_ERROR (Status)) {
    if (!IS_PCI_DISPLAY (&Pci) || (
                                   ((DeviceInfo->VendorId != MAX_UINT16) && (DeviceInfo->VendorId != Pci.Hdr.VendorId)) ||
                                   ((DeviceInfo->DeviceId != MAX_UINT16) && (DeviceInfo->DeviceId != Pci.Hdr.DeviceId)) ||
                                   ((DeviceInfo->RevisionId != MAX_UINT8) && (DeviceInfo->RevisionId != Pci.Hdr.RevisionID)) ||
                                   ((DeviceInfo->SubsystemVendorId != MAX_UINT16) && (DeviceInfo->SubsystemVendorId != Pci.Device.SubsystemVendorID)) ||
                                   ((DeviceInfo->SubsystemId != MAX_UINT16) && (DeviceInfo->SubsystemId != Pci.Device.SubsystemID))
                                   )
        )
    {
      //
      // It's not a video device, or device infomation doesn't match.
      //
      Status = EFI_UNSUPPORTED;
    } else {
      //
      // If it's a video device and device information matches, use the BarIndex
      // from device information, or any BAR if BarIndex is not specified
      // whose size >= the frame buffer size from GraphicsInfo HOB.
      // Store the new frame buffer base.
      //
      for (Index = 0; Index < MAX_PCI_BAR; Index++) {
        if ((DeviceInfo->BarIndex != MAX_UINT8) && (DeviceInfo->BarIndex != Index)) {
          continue;
        }

        Status = PciIo->GetBarAttributes (PciIo, Index, NULL, (VOID **)&Resources);
        if (!EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_INFO,
            "[%a]: BAR[%d]: Base = %lx, Length = %lx\n",
            gEfiCallerBaseName,
            Index,
            Resources->AddrRangeMin,
            Resources->AddrLen
            ));
          if ((Resources->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) &&
              (Resources->Len == (UINT16)(sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3)) &&
              (Resources->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) &&
              (Resources->AddrLen >= GraphicsInfo->FrameBufferSize)
              )
          {
            if (FrameBufferBase == 0) {
              FrameBufferBase = Resources->AddrRangeMin;
            }

            if (DeviceInfo->BarIndex == MAX_UINT8) {
              if (Resources->AddrRangeMin == GraphicsInfo->FrameBufferBase) {
                FrameBufferBase = Resources->AddrRangeMin;
                break;
              }
            } else {
              break;
            }
          }
        }
      }

      if (Index == MAX_PCI_BAR) {
        Status = EFI_UNSUPPORTED;
      } else {
        DEBUG ((DEBUG_INFO, "[%a]: ... matched!\n", gEfiCallerBaseName));
      }
    }
  }

  if (EFI_ERROR (Status)) {
    goto CloseProtocols;
  }

  if ((RemainingDevicePath != NULL) && IsDevicePathEnd (RemainingDevicePath)) {
    return EFI_SUCCESS;
  }

  Private = AllocateCopyPool (sizeof (mGraphicsOutputInstanceTemplate), &mGraphicsOutputInstanceTemplate);
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CloseProtocols;
  }

  Private->PhysicalFrameBufferBase = FrameBufferBase;
  Private->PhysicalFrameBufferSize = GraphicsInfo->FrameBufferSize;

  CopyMem (&Private->PhysicalModeInfo, &GraphicsInfo->GraphicsMode, sizeof (Private->PhysicalModeInfo));
  CopyMem (&Private->LogicalModeInfo, &Private->PhysicalModeInfo, sizeof (Private->LogicalModeInfo));

  Private->HasHiDpiMode     = FALSE;
  Private->FrameBufferScale = 1;
  if (FeaturePcdGet (PcdFspGopBasicHiDpiSupport)) {
    UINT32  ThresholdH;
    UINT32  ThresholdV;

    ThresholdH = PcdGet32 (PcdFspGopBasicHiDpiScaleThresholdHorizontal);
    ThresholdV = PcdGet32 (PcdFspGopBasicHiDpiScaleThresholdVertical);

    if ((Private->PhysicalModeInfo.HorizontalResolution >= ThresholdH) &&
        (Private->PhysicalModeInfo.VerticalResolution >= ThresholdV) &&
        ((Private->PhysicalModeInfo.HorizontalResolution % 2) == 0) &&
        ((Private->PhysicalModeInfo.VerticalResolution % 2) == 0))
    {
      Private->HasHiDpiMode                         = TRUE;
      Private->LogicalModeInfo.HorizontalResolution = Private->PhysicalModeInfo.HorizontalResolution / 2;
      Private->LogicalModeInfo.VerticalResolution   = Private->PhysicalModeInfo.VerticalResolution / 2;
      Private->LogicalModeInfo.PixelsPerScanLine    = Private->LogicalModeInfo.HorizontalResolution;
      Private->LogicalModeInfo.PixelFormat          = PixelBltOnly;
      ZeroMem (&Private->LogicalModeInfo.PixelInformation, sizeof (Private->LogicalModeInfo.PixelInformation));

      DEBUG ((
        DEBUG_INFO,
        "[%a]: Enabling GOP 2x framebuffer scaling (threshold %ux%u, physical %ux%u, logical %ux%u)\n",
        gEfiCallerBaseName,
        ThresholdH,
        ThresholdV,
        Private->PhysicalModeInfo.HorizontalResolution,
        Private->PhysicalModeInfo.VerticalResolution,
        Private->LogicalModeInfo.HorizontalResolution,
        Private->LogicalModeInfo.VerticalResolution
        ));
    }
  }

  Private->GraphicsOutputMode.MaxMode = Private->HasHiDpiMode ? 2 : 1;

  //
  // Fix up Mode pointer in GraphicsOutput
  //
  Private->GraphicsOutput.Mode = &Private->GraphicsOutputMode;

  Status = GraphicsOutputSetModeInternal (
             &Private->GraphicsOutput,
             Private->HasHiDpiMode ? GRAPHICS_OUTPUT_MODE_HIDPI : GRAPHICS_OUTPUT_MODE_PHYSICAL,
             FALSE
             );
  if (EFI_ERROR (Status)) {
    goto FreeMemory;
  }

  //
  // Set attributes
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationGet,
                    0,
                    &Private->PciAttributes
                    );
  if (!EFI_ERROR (Status)) {
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationEnable,
                      EFI_PCI_DEVICE_ENABLE,
                      NULL
                      );
  }

  if (EFI_ERROR (Status)) {
    goto FreeMemory;
  }

  //
  // Create the FrameBufferBltLib configuration.
  //
  ReturnStatus = FrameBufferBltConfigure (
                   (VOID *)(UINTN)Private->PhysicalFrameBufferBase,
                   &Private->PhysicalModeInfo,
                   Private->FrameBufferBltLibConfigure,
                   &Private->FrameBufferBltLibConfigureSize
                   );
  if (ReturnStatus == RETURN_BUFFER_TOO_SMALL) {
    Private->FrameBufferBltLibConfigure = AllocatePool (Private->FrameBufferBltLibConfigureSize);
    if (Private->FrameBufferBltLibConfigure != NULL) {
      ReturnStatus = FrameBufferBltConfigure (
                       (VOID *)(UINTN)Private->PhysicalFrameBufferBase,
                       &Private->PhysicalModeInfo,
                       Private->FrameBufferBltLibConfigure,
                       &Private->FrameBufferBltLibConfigureSize
                       );
    }
  }

  if (RETURN_ERROR (ReturnStatus)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto RestorePciAttributes;
  }

  Private->DevicePath = AppendDevicePathNode (PciDevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&mGraphicsOutputAdrNode);
  if (Private->DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto RestorePciAttributes;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->GraphicsOutputHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  &Private->GraphicsOutput,
                  &gEfiDevicePathProtocolGuid,
                  Private->DevicePath,
                  NULL
                  );

  if (!EFI_ERROR (Status)) {
    if (Private->HasHiDpiMode) {
      Status = gBS->CreateEventEx (
                      EVT_NOTIFY_SIGNAL,
                      TPL_CALLBACK,
                      GraphicsOutputExitBootServices,
                      Private,
                      &gEfiEventExitBootServicesGuid,
                      &Private->ExitBootServicesEvent
                      );
      if (EFI_ERROR (Status)) {
        gBS->UninstallMultipleProtocolInterfaces (
               Private->GraphicsOutputHandle,
               &gEfiGraphicsOutputProtocolGuid,
               &Private->GraphicsOutput,
               &gEfiDevicePathProtocolGuid,
               Private->DevicePath,
               NULL
               );
        goto RestorePciAttributes;
      }
    }

    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&Private->PciIo,
                    This->DriverBindingHandle,
                    Private->GraphicsOutputHandle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    if (!EFI_ERROR (Status)) {
      mDriverStarted = TRUE;
    } else {
      if (Private->ExitBootServicesEvent != NULL) {
        gBS->CloseEvent (Private->ExitBootServicesEvent);
        Private->ExitBootServicesEvent = NULL;
      }

      gBS->UninstallMultipleProtocolInterfaces (
             Private->GraphicsOutputHandle,
             &gEfiGraphicsOutputProtocolGuid,
             &Private->GraphicsOutput,
             &gEfiDevicePathProtocolGuid,
             Private->DevicePath,
             NULL
             );
    }
  }

RestorePciAttributes:
  if (EFI_ERROR (Status)) {
    //
    // Restore original PCI attributes
    //
    PciIo->Attributes (
             PciIo,
             EfiPciIoAttributeOperationSet,
             Private->PciAttributes,
             NULL
             );
  }

FreeMemory:
  if (EFI_ERROR (Status)) {
    if (Private != NULL) {
      if (Private->DevicePath != NULL) {
        FreePool (Private->DevicePath);
      }

      if (Private->FrameBufferBltLibConfigure != NULL) {
        FreePool (Private->FrameBufferBltLibConfigure);
      }

      FreePool (Private);
    }
  }

CloseProtocols:
  if (EFI_ERROR (Status)) {
    //
    // Close the PCI I/O Protocol
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    //
    // Close the PCI I/O Protocol
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
  Stop the video controller.

  @param  This                 Driver Binding protocol instance pointer.
  @param  Controller           The PCI controller.
  @param  NumberOfChildren     The number of child device handles in ChildHandleBuffer.
  @param  ChildHandleBuffer    An array of child handles to be freed. May be NULL
                               if NumberOfChildren is 0.

  @retval EFI_SUCCESS          The device was stopped.
  @retval EFI_DEVICE_ERROR     The device could not be stopped due to a device error.
**/
EFI_STATUS
EFIAPI
GraphicsOutputDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;
  GRAPHICS_OUTPUT_PRIVATE_DATA  *Private;

  if (NumberOfChildren == 0) {
    //
    // Close the PCI I/O Protocol
    //
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiPciIoProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );
    ASSERT_EFI_ERROR (Status);

    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiDevicePathProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );
    ASSERT_EFI_ERROR (Status);
    return EFI_SUCCESS;
  }

  ASSERT (NumberOfChildren == 1);
  Status = gBS->OpenProtocol (
                  ChildHandleBuffer[0],
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&Gop,
                  This->DriverBindingHandle,
                  ChildHandleBuffer[0],
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private = GRAPHICS_OUTPUT_PRIVATE_FROM_THIS (Gop);

  if (Private->ExitBootServicesEvent != NULL) {
    gBS->CloseEvent (Private->ExitBootServicesEvent);
    Private->ExitBootServicesEvent = NULL;
  }

  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  This->DriverBindingHandle,
                  Private->GraphicsOutputHandle
                  );
  ASSERT_EFI_ERROR (Status);
  //
  // Remove the GOP protocol interface from the system
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Private->GraphicsOutputHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  &Private->GraphicsOutput,
                  &gEfiDevicePathProtocolGuid,
                  Private->DevicePath,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Restore original PCI attributes
    //
    Status = Private->PciIo->Attributes (
                               Private->PciIo,
                               EfiPciIoAttributeOperationSet,
                               Private->PciAttributes,
                               NULL
                               );
    ASSERT_EFI_ERROR (Status);

    FreePool (Private->DevicePath);
    FreePool (Private->FrameBufferBltLibConfigure);
    mDriverStarted = FALSE;
  } else {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&Private->PciIo,
                    This->DriverBindingHandle,
                    Private->GraphicsOutputHandle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

EFI_DRIVER_BINDING_PROTOCOL  mGraphicsOutputDriverBinding = {
  GraphicsOutputDriverBindingSupported,
  GraphicsOutputDriverBindingStart,
  GraphicsOutputDriverBindingStop,
  0x10,
  NULL,
  NULL
};

/**
  The Entry Point for GraphicsOutput driver.

  It installs DriverBinding, ComponentName and ComponentName2 protocol if there is
  GraphicsInfo HOB passed from Graphics PEIM.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeGraphicsOutput (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *HobStart;

  HobStart = GetFirstGuidHob (&gEfiGraphicsInfoHobGuid);

  if ((HobStart == NULL) || (GET_GUID_HOB_DATA_SIZE (HobStart) < sizeof (EFI_PEI_GRAPHICS_INFO_HOB))) {
    return EFI_NOT_FOUND;
  }

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &mGraphicsOutputDriverBinding,
             ImageHandle,
             &mGraphicsOutputComponentName,
             &mGraphicsOutputComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
