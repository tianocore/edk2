/** @file
  Implementation for a generic GOP driver.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "GraphicsOutput.h"

#include <Guid/EventGroup.h>

#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>

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
  Return TRUE if the framebuffer is wider than the configured cap aspect and can
  be cropped to a centered viewport with left/right black bars.

  @param[in]  HorizontalResolution  Physical framebuffer width.
  @param[in]  VerticalResolution    Physical framebuffer height.
  @param[in]  CapAspectWidth        Maximum viewport aspect numerator.
  @param[in]  CapAspectHeight       Maximum viewport aspect denominator.
  @param[out] ViewportWidth         Cropped viewport width.

  @retval TRUE   A centered capped viewport can be created.
  @retval FALSE  No horizontal crop should be applied.
**/
STATIC
BOOLEAN
TryGetWideAspectCappedViewportWidth (
  IN  UINT32  HorizontalResolution,
  IN  UINT32  VerticalResolution,
  IN  UINT32  CapAspectWidth,
  IN  UINT32  CapAspectHeight,
  OUT UINT32  *ViewportWidth
  )
{
  UINT64  CandidateWidth;

  if ((ViewportWidth == NULL) ||
      (HorizontalResolution == 0) ||
      (VerticalResolution == 0) ||
      (CapAspectWidth == 0) ||
      (CapAspectHeight == 0))
  {
    return FALSE;
  }

  if (((UINT64)HorizontalResolution * CapAspectHeight) <= ((UINT64)VerticalResolution * CapAspectWidth)) {
    return FALSE;
  }

  CandidateWidth  = ((UINT64)VerticalResolution * CapAspectWidth) / CapAspectHeight;
  CandidateWidth &= ~1ULL;
  if ((CandidateWidth == 0) || (CandidateWidth >= HorizontalResolution)) {
    return FALSE;
  }

  *ViewportWidth = (UINT32)CandidateWidth;
  return TRUE;
}

/**
  Checks whether a non-empty rectangle fits within a bounding rectangle.

  @param[in] X              Rectangle X coordinate.
  @param[in] Y              Rectangle Y coordinate.
  @param[in] Width          Rectangle width.
  @param[in] Height         Rectangle height.
  @param[in] MaximumWidth   Bounding rectangle width.
  @param[in] MaximumHeight  Bounding rectangle height.

  @retval TRUE   The rectangle fits.
  @retval FALSE  The rectangle is empty or exceeds the bounds.
**/
STATIC
BOOLEAN
RectangleFits (
  IN UINTN  X,
  IN UINTN  Y,
  IN UINTN  Width,
  IN UINTN  Height,
  IN UINTN  MaximumWidth,
  IN UINTN  MaximumHeight
  )
{
  return (BOOLEAN)(
                   (Width != 0) &&
                   (Height != 0) &&
                   (X <= MaximumWidth) &&
                   (Y <= MaximumHeight) &&
                   (Width <= MaximumWidth - X) &&
                   (Height <= MaximumHeight - Y)
                   );
}

/**
  Calculates the byte range occupied by a BLT buffer rectangle.

  A zero Delta describes a tightly packed rectangle and is replaced with its
  calculated row size.

  @param[in]     X       Rectangle X coordinate in the BLT buffer.
  @param[in]     Y       Rectangle Y coordinate in the BLT buffer.
  @param[in]     Width   Rectangle width.
  @param[in]     Height  Rectangle height.
  @param[in,out] Delta   BLT buffer row size in bytes.
  @param[out]    Offset  Byte offset of the rectangle's first pixel.
  @param[out]    Extent  Byte offset immediately after its final pixel.

  @retval EFI_SUCCESS            The byte range was calculated.
  @retval EFI_INVALID_PARAMETER  Input is invalid or arithmetic overflowed.
**/
STATIC
EFI_STATUS
GetBltBufferOffset (
  IN     UINTN  X,
  IN     UINTN  Y,
  IN     UINTN  Width,
  IN     UINTN  Height,
  IN OUT UINTN  *Delta,
  OUT UINTN     *Offset,
  OUT UINTN     *Extent
  )
{
  RETURN_STATUS  Status;
  UINTN          ColumnOffset;
  UINTN          LastRow;
  UINTN          LastRowOffset;
  UINTN          RowBytes;
  UINTN          RowPixels;
  UINTN          WidthBytes;

  if ((Delta == NULL) || (Offset == NULL) || (Extent == NULL) || (Width == 0) || (Height == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = SafeUintnMult (Width, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), &WidthBytes);
  if (RETURN_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*Delta == 0) {
    if ((X != 0) || (Y != 0)) {
      return EFI_INVALID_PARAMETER;
    }

    *Delta = WidthBytes;
  } else {
    Status = SafeUintnAdd (X, Width, &RowPixels);
    if (RETURN_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }

    Status = SafeUintnMult (RowPixels, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), &RowBytes);
    if (RETURN_ERROR (Status) || (*Delta < RowBytes)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  Status = SafeUintnAdd (Y, Height - 1, &LastRow);
  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnMult (LastRow, *Delta, &LastRowOffset);
  }

  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnMult (X, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), &ColumnOffset);
  }

  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnAdd (LastRowOffset, ColumnOffset, &LastRowOffset);
  }

  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnAdd (LastRowOffset, WidthBytes, &LastRowOffset);
  }

  if (RETURN_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  *Extent = LastRowOffset;
  Status  = SafeUintnMult (Y, *Delta, Offset);
  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnAdd (*Offset, ColumnOffset, Offset);
  }

  return RETURN_ERROR (Status) ? EFI_INVALID_PARAMETER : EFI_SUCCESS;
}

/**
  Scales a coordinate and applies its physical viewport offset.

  @param[in]  Coordinate        Logical coordinate.
  @param[in]  Scale             Framebuffer scale.
  @param[in]  Offset            Physical viewport offset.
  @param[out] ScaledCoordinate  Resulting physical coordinate.

  @retval EFI_SUCCESS            The coordinate was converted.
  @retval EFI_INVALID_PARAMETER  Arithmetic overflowed.
**/
STATIC
EFI_STATUS
ScaleCoordinate (
  IN  UINTN   Coordinate,
  IN  UINT32  Scale,
  IN  UINT32  Offset,
  OUT UINTN   *ScaledCoordinate
  )
{
  RETURN_STATUS  Status;

  Status = SafeUintnMult (Coordinate, Scale, ScaledCoordinate);
  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnAdd (*ScaledCoordinate, Offset, ScaledCoordinate);
  }

  return RETURN_ERROR (Status) ? EFI_INVALID_PARAMETER : EFI_SUCCESS;
}

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
  ReadyToBoot notification.

  Switch back to a mode that provides a direct framebuffer before launching the
  OS (e.g. for users that write directly to the framebuffer).

  @param  Event    The event signaled.
  @param  Context  The event context.

**/
static VOID
EFIAPI
GraphicsOutputReadyToBoot (
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
  RETURN_STATUS  Status;
  UINTN          Col;
  UINTN          DstHeight;
  UINTN          DstSize;
  UINTN          DstWidth;
  UINTN          DstWidthBytes;
  UINTN          Row;
  UINTN          SrcWidthBytes;

  if ((Src == NULL) || (Dst == NULL) || (SrcWidth == 0) || (SrcHeight == 0) || (SrcDeltaBytes == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = SafeUintnMult (SrcWidth, sizeof (*Src), &SrcWidthBytes);
  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnMult (SrcWidth, 2, &DstWidth);
  }

  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnMult (SrcHeight, 2, &DstHeight);
  }

  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnMult (DstWidth, sizeof (*Dst), &DstWidthBytes);
  }

  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnMult (DstHeight, DstWidthBytes, &DstSize);
  }

  if (RETURN_ERROR (Status) || (SrcDeltaBytes < SrcWidthBytes) || (DstSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Row = 0; Row < SrcHeight; Row++) {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *SrcRow;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *DstRow0;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *DstRow1;

    SrcRow  = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)((UINT8 *)Src + Row * SrcDeltaBytes);
    DstRow0 = Dst + (Row * 2) * DstWidth;
    DstRow1 = DstRow0 + DstWidth;

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
  RETURN_STATUS  Status;
  UINTN          Col;
  UINTN          DstWidthBytes;
  UINTN          RequiredSrcHeight;
  UINTN          RequiredSrcWidth;
  UINTN          Row;
  UINTN          SrcWidthBytes;

  if ((Src == NULL) || (Dst == NULL) || (DstWidth == 0) || (DstHeight == 0) ||
      (SrcDeltaBytes == 0) || (DstDeltaBytes == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = SafeUintnMult (DstWidth, 2, &RequiredSrcWidth);
  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnMult (DstHeight, 2, &RequiredSrcHeight);
  }

  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnMult (SrcWidth, sizeof (*Src), &SrcWidthBytes);
  }

  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnMult (DstWidth, sizeof (*Dst), &DstWidthBytes);
  }

  if (RETURN_ERROR (Status) ||
      (SrcWidth < RequiredSrcWidth) ||
      (SrcHeight < RequiredSrcHeight) ||
      (SrcDeltaBytes < SrcWidthBytes) ||
      (DstDeltaBytes < DstWidthBytes))
  {
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
  UINTN                          BufferAddress;
  UINTN                          BufferExtent;
  UINTN                          BufferOffset;
  UINTN                          BufferEnd;
  UINTN                          BufferDelta;
  EFI_STATUS                     EfiStatus;
  UINTN                          PhysicalDestinationX;
  UINTN                          PhysicalDestinationY;
  UINTN                          PhysicalSourceX;
  UINTN                          PhysicalSourceY;
  GRAPHICS_OUTPUT_PRIVATE_DATA   *Private;
  UINT32                         Scale;
  UINTN                          ScaledHeight;
  UINTN                          ScaledWidth;
  RETURN_STATUS                  Status;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Temp;
  UINTN                          TempDeltaBytes;
  UINTN                          TempSize;
  EFI_TPL                        Tpl;

  if ((This == NULL) ||
      (This->Mode == NULL) ||
      (This->Mode->Info == NULL) ||
      (BltOperation >= EfiGraphicsOutputBltOperationMax))
  {
    return EFI_INVALID_PARAMETER;
  }

  Private = GRAPHICS_OUTPUT_PRIVATE_FROM_THIS (This);
  Scale   = Private->FrameBufferScale;
  if (Scale == 0) {
    Scale = 1;
  }

  if ((Scale != 1) && (Scale != 2)) {
    return EFI_UNSUPPORTED;
  }

  switch (BltOperation) {
    case EfiBltVideoFill:
    case EfiBltBufferToVideo:
      if (!RectangleFits (
             DestinationX,
             DestinationY,
             Width,
             Height,
             This->Mode->Info->HorizontalResolution,
             This->Mode->Info->VerticalResolution
             ))
      {
        return EFI_INVALID_PARAMETER;
      }

      break;

    case EfiBltVideoToBltBuffer:
      if (!RectangleFits (
             SourceX,
             SourceY,
             Width,
             Height,
             This->Mode->Info->HorizontalResolution,
             This->Mode->Info->VerticalResolution
             ))
      {
        return EFI_INVALID_PARAMETER;
      }

      break;

    case EfiBltVideoToVideo:
      if (!RectangleFits (
             SourceX,
             SourceY,
             Width,
             Height,
             This->Mode->Info->HorizontalResolution,
             This->Mode->Info->VerticalResolution
             ) ||
          !RectangleFits (
             DestinationX,
             DestinationY,
             Width,
             Height,
             This->Mode->Info->HorizontalResolution,
             This->Mode->Info->VerticalResolution
             ))
      {
        return EFI_INVALID_PARAMETER;
      }

      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  if (Scale == 1) {
    Tpl    = gBS->RaiseTPL (TPL_NOTIFY);
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
    gBS->RestoreTPL (Tpl);
    return RETURN_ERROR (Status) ? EFI_INVALID_PARAMETER : EFI_SUCCESS;
  }

  Status = SafeUintnMult (Width, Scale, &ScaledWidth);
  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnMult (Height, Scale, &ScaledHeight);
  }

  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnMult (ScaledWidth, sizeof (*Temp), &TempDeltaBytes);
  }

  if (!RETURN_ERROR (Status)) {
    Status = SafeUintnMult (ScaledHeight, TempDeltaBytes, &TempSize);
  }

  if (RETURN_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (BltOperation) {
    case EfiBltVideoFill:
      EfiStatus = ScaleCoordinate (
                    DestinationX,
                    Scale,
                    Private->ViewportOffsetX,
                    &PhysicalDestinationX
                    );
      if (!EFI_ERROR (EfiStatus)) {
        EfiStatus = ScaleCoordinate (
                      DestinationY,
                      Scale,
                      Private->ViewportOffsetY,
                      &PhysicalDestinationY
                      );
      }

      if (EFI_ERROR (EfiStatus)) {
        return EFI_INVALID_PARAMETER;
      }

      Tpl    = gBS->RaiseTPL (TPL_NOTIFY);
      Status = FrameBufferBlt (
                 Private->FrameBufferBltLibConfigure,
                 BltBuffer,
                 EfiBltVideoFill,
                 0,
                 0,
                 PhysicalDestinationX,
                 PhysicalDestinationY,
                 ScaledWidth,
                 ScaledHeight,
                 0
                 );
      gBS->RestoreTPL (Tpl);
      break;

    case EfiBltVideoToVideo:
      EfiStatus = ScaleCoordinate (SourceX, Scale, Private->ViewportOffsetX, &PhysicalSourceX);
      if (!EFI_ERROR (EfiStatus)) {
        EfiStatus = ScaleCoordinate (SourceY, Scale, Private->ViewportOffsetY, &PhysicalSourceY);
      }

      if (!EFI_ERROR (EfiStatus)) {
        EfiStatus = ScaleCoordinate (
                      DestinationX,
                      Scale,
                      Private->ViewportOffsetX,
                      &PhysicalDestinationX
                      );
      }

      if (!EFI_ERROR (EfiStatus)) {
        EfiStatus = ScaleCoordinate (
                      DestinationY,
                      Scale,
                      Private->ViewportOffsetY,
                      &PhysicalDestinationY
                      );
      }

      if (EFI_ERROR (EfiStatus)) {
        return EFI_INVALID_PARAMETER;
      }

      Tpl    = gBS->RaiseTPL (TPL_NOTIFY);
      Status = FrameBufferBlt (
                 Private->FrameBufferBltLibConfigure,
                 NULL,
                 EfiBltVideoToVideo,
                 PhysicalSourceX,
                 PhysicalSourceY,
                 PhysicalDestinationX,
                 PhysicalDestinationY,
                 ScaledWidth,
                 ScaledHeight,
                 0
                 );
      gBS->RestoreTPL (Tpl);
      break;

    case EfiBltBufferToVideo:
      if (BltBuffer == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      BufferDelta = Delta;
      EfiStatus   = GetBltBufferOffset (
                      SourceX,
                      SourceY,
                      Width,
                      Height,
                      &BufferDelta,
                      &BufferOffset,
                      &BufferExtent
                      );
      if (!EFI_ERROR (EfiStatus)) {
        Status = SafeUintnAdd ((UINTN)BltBuffer, BufferOffset, &BufferAddress);
      }

      if (!EFI_ERROR (EfiStatus) && !RETURN_ERROR (Status)) {
        Status = SafeUintnAdd ((UINTN)BltBuffer, BufferExtent, &BufferEnd);
      }

      if (EFI_ERROR (EfiStatus) || RETURN_ERROR (Status)) {
        return EFI_INVALID_PARAMETER;
      }

      Temp = AllocatePool (TempSize);
      if (Temp == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      EfiStatus = ScaleBltBuffer2x (
                    (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)BufferAddress,
                    Width,
                    Height,
                    BufferDelta,
                    Temp
                    );
      if (EFI_ERROR (EfiStatus)) {
        FreePool (Temp);
        return EfiStatus;
      }

      EfiStatus = ScaleCoordinate (
                    DestinationX,
                    Scale,
                    Private->ViewportOffsetX,
                    &PhysicalDestinationX
                    );
      if (!EFI_ERROR (EfiStatus)) {
        EfiStatus = ScaleCoordinate (
                      DestinationY,
                      Scale,
                      Private->ViewportOffsetY,
                      &PhysicalDestinationY
                      );
      }

      if (EFI_ERROR (EfiStatus)) {
        FreePool (Temp);
        return EFI_INVALID_PARAMETER;
      }

      Tpl    = gBS->RaiseTPL (TPL_NOTIFY);
      Status = FrameBufferBlt (
                 Private->FrameBufferBltLibConfigure,
                 Temp,
                 EfiBltBufferToVideo,
                 0,
                 0,
                 PhysicalDestinationX,
                 PhysicalDestinationY,
                 ScaledWidth,
                 ScaledHeight,
                 TempDeltaBytes
                 );
      gBS->RestoreTPL (Tpl);
      FreePool (Temp);
      break;

    case EfiBltVideoToBltBuffer:
      if (BltBuffer == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      BufferDelta = Delta;
      EfiStatus   = GetBltBufferOffset (
                      DestinationX,
                      DestinationY,
                      Width,
                      Height,
                      &BufferDelta,
                      &BufferOffset,
                      &BufferExtent
                      );
      if (!EFI_ERROR (EfiStatus)) {
        Status = SafeUintnAdd ((UINTN)BltBuffer, BufferOffset, &BufferAddress);
      }

      if (!EFI_ERROR (EfiStatus) && !RETURN_ERROR (Status)) {
        Status = SafeUintnAdd ((UINTN)BltBuffer, BufferExtent, &BufferEnd);
      }

      if (EFI_ERROR (EfiStatus) || RETURN_ERROR (Status)) {
        return EFI_INVALID_PARAMETER;
      }

      EfiStatus = ScaleCoordinate (SourceX, Scale, Private->ViewportOffsetX, &PhysicalSourceX);
      if (!EFI_ERROR (EfiStatus)) {
        EfiStatus = ScaleCoordinate (SourceY, Scale, Private->ViewportOffsetY, &PhysicalSourceY);
      }

      if (EFI_ERROR (EfiStatus)) {
        return EFI_INVALID_PARAMETER;
      }

      Temp = AllocatePool (TempSize);
      if (Temp == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Tpl    = gBS->RaiseTPL (TPL_NOTIFY);
      Status = FrameBufferBlt (
                 Private->FrameBufferBltLibConfigure,
                 Temp,
                 EfiBltVideoToBltBuffer,
                 PhysicalSourceX,
                 PhysicalSourceY,
                 0,
                 0,
                 ScaledWidth,
                 ScaledHeight,
                 TempDeltaBytes
                 );
      gBS->RestoreTPL (Tpl);
      if (RETURN_ERROR (Status)) {
        FreePool (Temp);
        break;
      }

      EfiStatus = DownscaleBltBuffer2x (
                    Temp,
                    ScaledWidth,
                    ScaledHeight,
                    TempDeltaBytes,
                    (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)BufferAddress,
                    Width,
                    Height,
                    BufferDelta
                    );
      FreePool (Temp);
      if (EFI_ERROR (EfiStatus)) {
        return EfiStatus;
      }

      Status = RETURN_SUCCESS;
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

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
  0,                                               // ViewportOffsetX
  0,                                               // ViewportOffsetY
  0,                                               // ViewportWidth
  0,                                               // ViewportHeight
  FALSE,                                           // HasHiDpiMode
  NULL,                                            // ReadyToBootEvent
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

  Private->ViewportOffsetX  = 0;
  Private->ViewportOffsetY  = 0;
  Private->ViewportWidth    = Private->PhysicalModeInfo.HorizontalResolution;
  Private->ViewportHeight   = Private->PhysicalModeInfo.VerticalResolution;
  Private->HasHiDpiMode     = FALSE;
  Private->FrameBufferScale = 1;
  if (FeaturePcdGet (PcdFspGopBasicHiDpiSupport)) {
    UINT32  ThresholdH;
    UINT32  ThresholdV;
    UINT32  ViewportWidth;

    ThresholdH = PcdGet32 (PcdFspGopBasicHiDpiScaleThresholdHorizontal);
    ThresholdV = PcdGet32 (PcdFspGopBasicHiDpiScaleThresholdVertical);

    if ((Private->PhysicalModeInfo.HorizontalResolution >= ThresholdH) &&
        (Private->PhysicalModeInfo.VerticalResolution >= ThresholdV) &&
        ((Private->PhysicalModeInfo.HorizontalResolution % 2) == 0) &&
        ((Private->PhysicalModeInfo.VerticalResolution % 2) == 0))
    {
      Private->HasHiDpiMode = TRUE;
      if (FeaturePcdGet (PcdFspGopBasicHiDpiWideAspectCapSupport) &&
          TryGetWideAspectCappedViewportWidth (
            Private->PhysicalModeInfo.HorizontalResolution,
            Private->PhysicalModeInfo.VerticalResolution,
            PcdGet32 (PcdFspGopBasicHiDpiWideAspectCapWidth),
            PcdGet32 (PcdFspGopBasicHiDpiWideAspectCapHeight),
            &ViewportWidth
            ))
      {
        Private->ViewportWidth   = ViewportWidth;
        Private->ViewportOffsetX = (Private->PhysicalModeInfo.HorizontalResolution - ViewportWidth) / 2;
      }

      Private->LogicalModeInfo.HorizontalResolution = Private->ViewportWidth / 2;
      Private->LogicalModeInfo.VerticalResolution   = Private->ViewportHeight / 2;
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

      if (Private->ViewportOffsetX != 0) {
        DEBUG ((
          DEBUG_INFO,
          "[%a]: Centering HiDPI viewport at +%u, viewport %ux%u inside physical %ux%u\n",
          gEfiCallerBaseName,
          Private->ViewportOffsetX,
          Private->ViewportWidth,
          Private->ViewportHeight,
          Private->PhysicalModeInfo.HorizontalResolution,
          Private->PhysicalModeInfo.VerticalResolution
          ));
      }
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
                      GraphicsOutputReadyToBoot,
                      Private,
                      &gEfiEventReadyToBootGuid,
                      &Private->ReadyToBootEvent
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
      if (Private->ReadyToBootEvent != NULL) {
        gBS->CloseEvent (Private->ReadyToBootEvent);
        Private->ReadyToBootEvent = NULL;
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

  if (Private->ReadyToBootEvent != NULL) {
    gBS->CloseEvent (Private->ReadyToBootEvent);
    Private->ReadyToBootEvent = NULL;
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
