/** @file

Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    WinNtGopScreen.c

Abstract:

  This file produces the graphics abstration of GOP. It is called by
  WinNtGopDriver.c file which deals with the UEFI 2.0 driver model.
  This file just does graphics.


**/

#include "WinNtGop.h"

EFI_WIN_NT_THUNK_PROTOCOL *mWinNt;
DWORD                     mTlsIndex         = TLS_OUT_OF_INDEXES;
DWORD                     mTlsIndexUseCount = 0;  // lets us know when we can free mTlsIndex.
EFI_EVENT          mGopScreenExitBootServicesEvent;
GOP_MODE_DATA mGopModeData[] = {
    {800, 600, 0, 0},
    {640, 480, 0, 0},
    {720, 400, 0, 0},
    {1024, 768, 0, 0},
    {1280, 1024, 0, 0}
    };

EFI_STATUS
WinNtGopStartWindow (
  IN  GOP_PRIVATE_DATA    *Private,
  IN  UINT32              HorizontalResolution,
  IN  UINT32              VerticalResolution,
  IN  UINT32              ColorDepth,
  IN  UINT32              RefreshRate
  );

VOID
EFIAPI
KillNtGopThread (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

BOOLEAN
WinNtGopConvertParamToEfiKeyShiftState (
  IN  GOP_PRIVATE_DATA  *Private,
  IN  WPARAM            *wParam,
  IN  LPARAM            *lParam,
  IN  BOOLEAN           Flag
  )
{
  switch (*wParam) {
  //
  // BUGBUG: Only GetAsyncKeyState() and GetKeyState() can distinguish
  // left and right Ctrl, and Shift key.
  // Neither of the two is defined in EFI_WIN_NT_THUNK_PROTOCOL.
  // Therefor, we can not set the correct Shift state here.
  //
  case VK_SHIFT:
    if ((*lParam & GOP_EXTENDED_KEY) == GOP_EXTENDED_KEY) {
        Private->RightShift = Flag;
      } else {
        Private->LeftShift = Flag;
      }
    return TRUE;

  case VK_LSHIFT:
    Private->LeftShift = Flag;
    return TRUE;

  case VK_RSHIFT:
    Private->RightShift = Flag;
    return TRUE;

  case VK_CONTROL:
    if ((*lParam & GOP_EXTENDED_KEY) == GOP_EXTENDED_KEY) {
        Private->RightCtrl= Flag;
      } else {
        Private->LeftCtrl = Flag;
      }
    return TRUE;

  case VK_LCONTROL:
    Private->LeftCtrl = Flag;
    return TRUE;

  case VK_RCONTROL:
    Private->RightCtrl = Flag;
    return TRUE;

  case VK_LWIN:
    Private->LeftLogo   = Flag;
    return TRUE;

  case VK_RWIN:
    Private->RightLogo  = Flag;
    return TRUE;

  case VK_APPS:
    Private->Menu       = Flag;
    return TRUE;
  //
  // BUGBUG: PrintScreen/SysRq can not trigger WM_KEYDOWN message,
  // so SySReq shift state is not supported here.
  //
  case VK_PRINT:
    Private->SysReq     = Flag;
    return TRUE;
  //
  // For Alt Keystroke.
  //
  case VK_MENU:
    if ((*lParam & GOP_EXTENDED_KEY) == GOP_EXTENDED_KEY) {
        Private->RightAlt = Flag;
      } else {
        Private->LeftAlt = Flag;
      }
    return TRUE;

  default:
    return FALSE;
  }
}

BOOLEAN
WinNtGopConvertParamToEfiKey (
  IN  GOP_PRIVATE_DATA  *Private,
  IN  WPARAM            *wParam,
  IN  LPARAM            *lParam,
  IN  EFI_INPUT_KEY     *Key
  )
{
  BOOLEAN Flag;
  Flag = FALSE;
  switch (*wParam) {
  case VK_HOME:       Key->ScanCode = SCAN_HOME;      Flag = TRUE; break;
  case VK_END:        Key->ScanCode = SCAN_END;       Flag = TRUE; break;
  case VK_LEFT:       Key->ScanCode = SCAN_LEFT;      Flag = TRUE; break;
  case VK_RIGHT:      Key->ScanCode = SCAN_RIGHT;     Flag = TRUE; break;
  case VK_UP:         Key->ScanCode = SCAN_UP;        Flag = TRUE; break;
  case VK_DOWN:       Key->ScanCode = SCAN_DOWN;      Flag = TRUE; break;
  case VK_DELETE:     Key->ScanCode = SCAN_DELETE;    Flag = TRUE; break;
  case VK_INSERT:     Key->ScanCode = SCAN_INSERT;    Flag = TRUE; break;
  case VK_PRIOR:      Key->ScanCode = SCAN_PAGE_UP;   Flag = TRUE; break;
  case VK_NEXT:       Key->ScanCode = SCAN_PAGE_DOWN; Flag = TRUE; break;
  case VK_ESCAPE:     Key->ScanCode = SCAN_ESC;       Flag = TRUE; break;

  case VK_F1:         Key->ScanCode = SCAN_F1;    Flag = TRUE;     break;
  case VK_F2:         Key->ScanCode = SCAN_F2;    Flag = TRUE;     break;
  case VK_F3:         Key->ScanCode = SCAN_F3;    Flag = TRUE;     break;
  case VK_F4:         Key->ScanCode = SCAN_F4;    Flag = TRUE;     break;
  case VK_F5:         Key->ScanCode = SCAN_F5;    Flag = TRUE;     break;
  case VK_F6:         Key->ScanCode = SCAN_F6;    Flag = TRUE;     break;
  case VK_F7:         Key->ScanCode = SCAN_F7;    Flag = TRUE;     break;
  case VK_F8:         Key->ScanCode = SCAN_F8;    Flag = TRUE;     break;
  case VK_F9:         Key->ScanCode = SCAN_F9;    Flag = TRUE;     break;
  case VK_F11:        Key->ScanCode = SCAN_F11;   Flag = TRUE;     break;
  case VK_F12:        Key->ScanCode = SCAN_F12;   Flag = TRUE;     break;

  case VK_F13:        Key->ScanCode = SCAN_F13;   Flag = TRUE;     break;
  case VK_F14:        Key->ScanCode = SCAN_F14;   Flag = TRUE;     break;
  case VK_F15:        Key->ScanCode = SCAN_F15;   Flag = TRUE;     break;
  case VK_F16:        Key->ScanCode = SCAN_F16;   Flag = TRUE;     break;
  case VK_F17:        Key->ScanCode = SCAN_F17;   Flag = TRUE;     break;
  case VK_F18:        Key->ScanCode = SCAN_F18;   Flag = TRUE;     break;
  case VK_F19:        Key->ScanCode = SCAN_F19;   Flag = TRUE;     break;
  case VK_F20:        Key->ScanCode = SCAN_F20;   Flag = TRUE;     break;
  case VK_F21:        Key->ScanCode = SCAN_F21;   Flag = TRUE;     break;
  case VK_F22:        Key->ScanCode = SCAN_F22;   Flag = TRUE;     break;
  case VK_F23:        Key->ScanCode = SCAN_F23;   Flag = TRUE;     break;
  case VK_F24:        Key->ScanCode = SCAN_F24;   Flag = TRUE;     break;
  case VK_PAUSE:      Key->ScanCode = SCAN_PAUSE; Flag = TRUE;     break;

  //
  // Set toggle state
  //
  case VK_NUMLOCK:
    Private->NumLock    = (BOOLEAN)(!Private->NumLock);
    Flag = TRUE;
    break;
  case VK_SCROLL:
    Private->ScrollLock = (BOOLEAN)(!Private->ScrollLock);
    Flag = TRUE;
    break;
  case VK_CAPITAL:
    Private->CapsLock   = (BOOLEAN)(!Private->CapsLock);
    Flag = TRUE;
    break;
  }
  
  return (WinNtGopConvertParamToEfiKeyShiftState (Private, wParam, lParam, TRUE)) == TRUE ? TRUE : Flag;
}


//
// GOP Protocol Member Functions
//


/**
  Graphics Output protocol interface to get video mode

  @param  This                   Protocol instance pointer.
  @param  ModeNumber             The mode number to return information on.
  @param  Info                   Caller allocated buffer that returns information
                                 about ModeNumber.
  @param  SizeOfInfo             A pointer to the size, in bytes, of the Info
                                 buffer.

  @retval EFI_SUCCESS            Mode information returned.
  @retval EFI_BUFFER_TOO_SMALL   The Info buffer was too small.
  @retval EFI_DEVICE_ERROR       A hardware error occurred trying to retrieve the
                                 video mode.
  @retval EFI_NOT_STARTED        Video display is not initialized. Call SetMode ()
  @retval EFI_INVALID_PARAMETER  One of the input args was NULL.

**/
EFI_STATUS
EFIAPI
WinNtGopQuerytMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
{
  GOP_PRIVATE_DATA  *Private;

  Private = GOP_PRIVATE_DATA_FROM_THIS (This);

  if (Info == NULL || SizeOfInfo == NULL || (UINTN) ModeNumber >= This->Mode->MaxMode) {
    return EFI_INVALID_PARAMETER;
  }

  *Info = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
  if (*Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

  (*Info)->Version = 0;
  (*Info)->HorizontalResolution = Private->ModeData[ModeNumber].HorizontalResolution;
  (*Info)->VerticalResolution   = Private->ModeData[ModeNumber].VerticalResolution;
  (*Info)->PixelFormat = PixelBltOnly;
  (*Info)->PixelsPerScanLine = (*Info)->HorizontalResolution;

  return EFI_SUCCESS;
}


/**
  Graphics Output protocol interface to set video mode

  @param  This                   Protocol instance pointer.
  @param  ModeNumber             The mode number to be set.

  @retval EFI_SUCCESS            Graphics mode was changed.
  @retval EFI_DEVICE_ERROR       The device had an error and could not complete the
                                 request.
  @retval EFI_UNSUPPORTED        ModeNumber is not supported by this device.

**/
EFI_STATUS
EFIAPI
WinNtGopSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL * This,
  IN  UINT32                       ModeNumber
  )
{
  EFI_STATUS                    Status;
  GOP_PRIVATE_DATA              *Private;
  GOP_MODE_DATA *ModeData;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Fill;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *NewFillLine;
  RECT                          Rect;
  UINTN                         Size;
  UINTN                         Width;
  UINTN                         Height;

  Private = GOP_PRIVATE_DATA_FROM_THIS (This);

  if (ModeNumber >= This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  ModeData = &Private->ModeData[ModeNumber];
  This->Mode->Mode = ModeNumber;
  Private->GraphicsOutput.Mode->Info->HorizontalResolution = ModeData->HorizontalResolution;
  Private->GraphicsOutput.Mode->Info->VerticalResolution = ModeData->VerticalResolution;
  Private->GraphicsOutput.Mode->Info->PixelsPerScanLine = ModeData->HorizontalResolution;

  if (Private->HardwareNeedsStarting) {
    Status = WinNtGopStartWindow (
              Private,
              ModeData->HorizontalResolution,
              ModeData->VerticalResolution,
              ModeData->ColorDepth,
              ModeData->RefreshRate
              );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Private->HardwareNeedsStarting = FALSE;
  } else {
    //
    // Change the resolution and resize of the window
    //

    //
    // Free the old buffer. We do not save the content of the old buffer since the
    // screen is to be cleared anyway. Clearing the screen is required by the EFI spec.
    // See UEFI spec -EFI_GRAPHICS_OUTPUT_PROTOCOL.SetMode()
    //
    Private->WinNtThunk->HeapFree (Private->WinNtThunk->GetProcessHeap (), 0, Private->VirtualScreenInfo);

    //
    // Allocate DIB frame buffer directly from NT for performance enhancement
    // This buffer is the virtual screen/frame buffer. This buffer is not the
    // same a a frame buffer. The first row of this buffer will be the bottom
    // line of the image. This is an artifact of the way we draw to the screen.
    //
    Size = ModeData->HorizontalResolution * ModeData->VerticalResolution * sizeof (RGBQUAD) + sizeof (BITMAPV4HEADER);
    Private->VirtualScreenInfo = Private->WinNtThunk->HeapAlloc (
                                                        Private->WinNtThunk->GetProcessHeap (),
                                                        HEAP_ZERO_MEMORY,
                                                        Size
                                                        );

    //
    // Update the virtual screen info data structure
    //
    Private->VirtualScreenInfo->bV4Size           = sizeof (BITMAPV4HEADER);
    Private->VirtualScreenInfo->bV4Width          = ModeData->HorizontalResolution;
    Private->VirtualScreenInfo->bV4Height         = ModeData->VerticalResolution;
    Private->VirtualScreenInfo->bV4Planes         = 1;
    Private->VirtualScreenInfo->bV4BitCount       = 32;
    //
    // uncompressed
    //
    Private->VirtualScreenInfo->bV4V4Compression  = BI_RGB;

    //
    // The rest of the allocated memory block is the virtual screen buffer
    //
    Private->VirtualScreen = (RGBQUAD *) (Private->VirtualScreenInfo + 1);

    //
    // Use the AdjuctWindowRect fuction to calculate the real width and height
    // of the new window including the border and caption
    //
    Rect.left   = 0;
    Rect.top    = 0;
    Rect.right  = ModeData->HorizontalResolution;
    Rect.bottom = ModeData->VerticalResolution;

    Private->WinNtThunk->AdjustWindowRect (&Rect, WS_OVERLAPPEDWINDOW, 0);

    Width   = Rect.right - Rect.left;
    Height  = Rect.bottom - Rect.top;

    //
    // Retrieve the original window position information
    //
    Private->WinNtThunk->GetWindowRect (Private->WindowHandle, &Rect);

    //
    // Adjust the window size
    //
    Private->WinNtThunk->MoveWindow (Private->WindowHandle, Rect.left, Rect.top, (INT32)Width, (INT32)Height, TRUE);

  }

  NewFillLine = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * ModeData->HorizontalResolution);
  if (NewFillLine == NULL) {
    return EFI_DEVICE_ERROR;
  }

  if (Private->FillLine != NULL) {
    FreePool (Private->FillLine);
  }

  Private->FillLine             = NewFillLine;

  Fill.Red                      = 0x00;
  Fill.Green                    = 0x00;
  Fill.Blue                     = 0x00;
  This->Blt (
          This,
          &Fill,
          EfiBltVideoFill,
          0,
          0,
          0,
          0,
          ModeData->HorizontalResolution,
          ModeData->VerticalResolution,
          ModeData->HorizontalResolution * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
          );
  return EFI_SUCCESS;
}


/**
  Blt pixels from the rectangle (Width X Height) formed by the BltBuffer
  onto the graphics screen starting a location (X, Y). (0, 0) is defined as
  the upper left hand side of the screen. (X, Y) can be outside of the
  current screen geometry and the BltBuffer will be cliped when it is
  displayed. X and Y can be negative or positive. If Width or Height is
  bigger than the current video screen the image will be clipped.

  @param  This                   Protocol instance pointer.
  @param  X                      X location on graphics screen.
  @param  Y                      Y location on the graphics screen.
  @param  Width                  Width of BltBuffer.
  @param  Height                 Hight of BltBuffer
  @param  BltOperation           Operation to perform on BltBuffer and video memory
  @param  BltBuffer              Buffer containing data to blt into video buffer.
                                 This  buffer has a size of
                                 Width*Height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
  @param  SourceX                If the BltOperation is a EfiCopyBlt this is the
                                 source of the copy. For other BLT operations this
                                 argument is not used.
  @param  SourceX                If the BltOperation is a EfiCopyBlt this is the
                                 source of the copy. For other BLT operations this
                                 argument is not used.

  @retval EFI_SUCCESS            The palette is updated with PaletteArray.
  @retval EFI_INVALID_PARAMETER  BltOperation is not valid.
  @retval EFI_DEVICE_ERROR       A hardware error occured writting to the video
                                 buffer.

**/
// TODO:    SourceY - add argument and description to function comment
// TODO:    DestinationX - add argument and description to function comment
// TODO:    DestinationY - add argument and description to function comment
// TODO:    Delta - add argument and description to function comment
EFI_STATUS
EFIAPI
WinNtGopBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL                   *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL                           *BltBuffer, OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION                   BltOperation,
  IN  UINTN                                   SourceX,
  IN  UINTN                                   SourceY,
  IN  UINTN                                   DestinationX,
  IN  UINTN                                   DestinationY,
  IN  UINTN                                   Width,
  IN  UINTN                                   Height,
  IN  UINTN                                   Delta         OPTIONAL
  )
{
  GOP_PRIVATE_DATA              *Private;
  EFI_TPL                       OriginalTPL;
  UINTN                         DstY;
  UINTN                         SrcY;
  RGBQUAD                       *VScreen;
  RGBQUAD                       *VScreenSrc;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  UINTN                         Index;
  RECT                          Rect;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *FillPixel;
  UINT32                        VerticalResolution;
  UINT32                        HorizontalResolution;

  Private = GOP_PRIVATE_DATA_FROM_THIS (This);

  if ((BltOperation < 0) || (BltOperation >= EfiGraphicsOutputBltOperationMax)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    return EFI_INVALID_PARAMETER;
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
  // We need to fill the Virtual Screen buffer with the blt data.
  // The virtual screen is upside down, as the first row is the bootom row of
  // the image.
  //
  VerticalResolution = This->Mode->Info->VerticalResolution;
  HorizontalResolution = This->Mode->Info->HorizontalResolution;
  if (BltOperation == EfiBltVideoToBltBuffer) {

    //
    // Video to BltBuffer: Source is Video, destination is BltBuffer
    //
    if (SourceY + Height > VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (SourceX + Width > HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
    // We would not want a timer based event (Cursor, ...) to come in while we are
    // doing this operation.
    //
    OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

    for (SrcY = SourceY, DstY = DestinationY; DstY < (Height + DestinationY); SrcY++, DstY++) {
      Blt = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) ((UINT8 *) BltBuffer + (DstY * Delta) + DestinationX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      VScreen = &Private->VirtualScreen[(VerticalResolution - SrcY - 1) * HorizontalResolution + SourceX];
      CopyMem (Blt, VScreen, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * Width);
    }
  } else {
    //
    // BltBuffer to Video: Source is BltBuffer, destination is Video
    //
    if (DestinationY + Height > VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestinationX + Width > HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
    // We would not want a timer based event (Cursor, ...) to come in while we are
    // doing this operation.
    //
    OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

    if (BltOperation == EfiBltVideoFill) {
      FillPixel = BltBuffer;
      for (Index = 0; Index < Width; Index++) {
        Private->FillLine[Index] = *FillPixel;
      }
    }

    for (Index = 0; Index < Height; Index++) {
      if (DestinationY <= SourceY) {
        SrcY  = SourceY + Index;
        DstY  = DestinationY + Index;
      } else {
        SrcY  = SourceY + Height - Index - 1;
        DstY  = DestinationY + Height - Index - 1;
      }

      VScreen = &Private->VirtualScreen[(VerticalResolution - DstY - 1) * HorizontalResolution + DestinationX];
      switch (BltOperation) {
      case EfiBltBufferToVideo:
        Blt = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) ((UINT8 *) BltBuffer + (SrcY * Delta) + SourceX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
        CopyMem (VScreen, Blt, Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
        break;

      case EfiBltVideoToVideo:
        VScreenSrc = &Private->VirtualScreen[(VerticalResolution - SrcY - 1) * HorizontalResolution + SourceX];
        CopyMem (VScreen, VScreenSrc, Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
        break;

      case EfiBltVideoFill:
        CopyMem (VScreen, Private->FillLine, Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
        break;
      }
    }
  }

  if (BltOperation != EfiBltVideoToBltBuffer) {
    //
    // Mark the area we just blted as Invalid so WM_PAINT will update.
    //
    Rect.left   = (LONG)DestinationX;
    Rect.top    = (LONG)DestinationY;
    Rect.right  = (LONG)(DestinationX + Width);
    Rect.bottom = (LONG)(DestinationY + Height);
    Private->WinNtThunk->InvalidateRect (Private->WindowHandle, &Rect, FALSE);

    //
    // Send the WM_PAINT message to the thread that is drawing the window. We
    // are in the main thread and the window drawing is in a child thread.
    // There is a child thread per window. We have no CriticalSection or Mutex
    // since we write the data and the other thread displays the data. While
    // we may miss some data for a short period of time this is no different than
    // a write combining on writes to a frame buffer.
    //

    Private->WinNtThunk->UpdateWindow (Private->WindowHandle);
  }

  gBS->RestoreTPL (OriginalTPL);

  return EFI_SUCCESS;
}

//
// Construction and Destruction functions
//


/**


  @return None

**/
// TODO:    WinNtIo - add argument and description to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
EFI_STATUS
WinNtGopSupported (
  IN  EFI_WIN_NT_IO_PROTOCOL  *WinNtIo
  )
{
  //
  // Check to see if the IO abstraction represents a device type we support.
  //
  // This would be replaced a check of PCI subsystem ID, etc.
  //
  if (!CompareGuid (WinNtIo->TypeGuid, &gEfiWinNtGopGuid)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}


/**
  Win32 Windows event handler.

  See Win32 Book

  @return See Win32 Book

**/
// TODO:    hwnd - add argument and description to function comment
// TODO:    iMsg - add argument and description to function comment
// TODO:    wParam - add argument and description to function comment
// TODO:    lParam - add argument and description to function comment
LRESULT
CALLBACK
WinNtGopThreadWindowProc (
  IN  HWND    hwnd,
  IN  UINT    iMsg,
  IN  WPARAM  wParam,
  IN  LPARAM  lParam
  )
{
  GOP_PRIVATE_DATA  *Private;
  UINTN             Size;
  HDC               Handle;
  PAINTSTRUCT       PaintStruct;
  LPARAM            Index;
  EFI_INPUT_KEY     Key;
  BOOLEAN           AltIsPress;

  //
  // BugBug - if there are two instances of this DLL in memory (such as is
  // the case for ERM), the correct instance of this function may not be called.
  // This also means that the address of the mTlsIndex value will be wrong, and
  // the value may be wrong too.
  //


  //
  // Use mTlsIndex global to get a Thread Local Storage version of Private.
  // This works since each Gop protocol has a unique Private data instance and
  // a unique thread.
  //
  AltIsPress = FALSE;
  Private = mWinNt->TlsGetValue (mTlsIndex);
  ASSERT (NULL != Private);

  switch (iMsg) {
  case WM_CREATE:
    Size = Private->GraphicsOutput.Mode->Info->HorizontalResolution * Private->GraphicsOutput.Mode->Info->VerticalResolution * sizeof (RGBQUAD);

    //
    // Allocate DIB frame buffer directly from NT for performance enhancement
    // This buffer is the virtual screen/frame buffer. This buffer is not the
    // same a a frame buffer. The first fow of this buffer will be the bottom
    // line of the image. This is an artifact of the way we draw to the screen.
    //
    Private->VirtualScreenInfo = Private->WinNtThunk->HeapAlloc (
                                                        Private->WinNtThunk->GetProcessHeap (),
                                                        HEAP_ZERO_MEMORY,
                                                        Size
                                                        );

    Private->VirtualScreenInfo->bV4Size           = sizeof (BITMAPV4HEADER);
    Private->VirtualScreenInfo->bV4Width          = Private->GraphicsOutput.Mode->Info->HorizontalResolution;
    Private->VirtualScreenInfo->bV4Height         = Private->GraphicsOutput.Mode->Info->VerticalResolution;
    Private->VirtualScreenInfo->bV4Planes         = 1;
    Private->VirtualScreenInfo->bV4BitCount       = 32;
    //
    // uncompressed
    //
    Private->VirtualScreenInfo->bV4V4Compression  = BI_RGB;
    Private->VirtualScreen = (RGBQUAD *) (Private->VirtualScreenInfo + 1);
    return 0;

  case WM_PAINT:
    //
    // I have not found a way to convert hwnd into a Private context. So for
    // now we use this API to convert hwnd to Private data.
    //

    Handle = mWinNt->BeginPaint (hwnd, &PaintStruct);

    mWinNt->SetDIBitsToDevice (
              Handle,                                     // Destination Device Context
              0,                                          // Destination X - 0
              0,                                          // Destination Y - 0
              Private->GraphicsOutput.Mode->Info->HorizontalResolution,              // Width
              Private->GraphicsOutput.Mode->Info->VerticalResolution,                // Height
              0,                                          // Source X
              0,                                          // Source Y
              0,                                          // DIB Start Scan Line
              Private->GraphicsOutput.Mode->Info->VerticalResolution,                // Number of scan lines
              Private->VirtualScreen,                     // Address of array of DIB bits
              (BITMAPINFO *) Private->VirtualScreenInfo,  // Address of structure with bitmap info
              DIB_RGB_COLORS                              // RGB or palette indexes
              );

    mWinNt->EndPaint (hwnd, &PaintStruct);
    return 0;

  //
  // F10 and the ALT key do not create a WM_KEYDOWN message, thus this special case
  // WM_SYSKEYDOWN is posted when F10 is pressed or
  // holds down ALT key and then presses another key.
  //
  case WM_SYSKEYDOWN:

    Key.ScanCode    = 0;
    Key.UnicodeChar = CHAR_NULL;
    switch (wParam) {
    case VK_F10:
      Key.ScanCode    = SCAN_F10;
      Key.UnicodeChar = CHAR_NULL;
      GopPrivateAddKey (Private, Key);
      return 0;
    }

    //
    // If ALT or ALT + modifier key is pressed.
    //
    if (WinNtGopConvertParamToEfiKey (Private, &wParam, &lParam, &Key)) {
      if (Key.ScanCode != 0){
        //
        // If ALT is pressed with other ScanCode.
        // Always revers the left Alt for simple.
        //
        Private->LeftAlt = TRUE;
      }
      GopPrivateAddKey (Private, Key);
      //
      // When Alt is released there is no windoes message, so 
      // clean it after using it.
      //
      Private->RightAlt = FALSE;
      Private->LeftAlt  = FALSE;
      return 0;
    }
    AltIsPress = TRUE;

  case WM_CHAR:    
    //
    // The ESC key also generate WM_CHAR.
    //
    if (wParam == 0x1B) {
	    return 0;
    }    

    if (AltIsPress == TRUE) {
      //
      // If AltIsPress is true that means the Alt key is pressed.
      //
      Private->LeftAlt = TRUE;
    }
    for (Index = 0; Index < (lParam & 0xffff); Index++) {
      if (wParam != 0) {
        Key.UnicodeChar = (CHAR16) wParam;
        Key.ScanCode    = SCAN_NULL;     
        GopPrivateAddKey (Private, Key);    
      }
    }
    if (AltIsPress == TRUE) {
      //
      // When Alt is released there is no windoes message, so 
      // clean it after using it.
      //
      Private->LeftAlt  = FALSE;
      Private->RightAlt = FALSE;
    }
    return 0;

  case WM_SYSKEYUP:
    //
    // ALT is pressed with another key released
    //
    WinNtGopConvertParamToEfiKeyShiftState (Private, &wParam, &lParam, FALSE);
    return 0;

  case WM_KEYDOWN:
    Key.ScanCode    = SCAN_NULL;
    Key.UnicodeChar = CHAR_NULL;
    //
    // A value key press will cause a WM_KEYDOWN first, then cause a WM_CHAR
    // So if there is no modifier key updated, skip the WM_KEYDOWN even.
    //
    if (WinNtGopConvertParamToEfiKey (Private, &wParam, &lParam, &Key)) {
      //
      // Support the partial keystroke, add all keydown event into the queue.
      //
      GopPrivateAddKey (Private, Key);
    }
    return 0;

  case WM_KEYUP:
    WinNtGopConvertParamToEfiKeyShiftState (Private, &wParam, &lParam, FALSE);
    return 0;

  case WM_CLOSE:
    //
    // This close message is issued by user, core is not aware of this,
    // so don't release the window display resource, just hide the window.
    //
    Private->WinNtThunk->ShowWindow (Private->WindowHandle, SW_HIDE);
    return 0;

  case WM_DESTROY:
    mWinNt->DestroyWindow (hwnd);
    mWinNt->PostQuitMessage (0);

    mWinNt->HeapFree (Private->WinNtThunk->GetProcessHeap (), 0, Private->VirtualScreenInfo);

    mWinNt->ExitThread (0);
    return 0;

  default:
    break;
  };

  return mWinNt->DefWindowProc (hwnd, iMsg, wParam, lParam);
}


/**
  This thread simulates the end of WinMain () aplication. Each Winow nededs
  to process it's events. The messages are dispatched to
  WinNtGopThreadWindowProc ().
  Be very careful sine WinNtGopThreadWinMain () and WinNtGopThreadWindowProc ()
  are running in a seperate thread. We have to do this to process the events.

  @param  lpParameter            Handle of window to manage.

  @return if a WM_QUIT message is returned exit.

**/
DWORD
WINAPI
WinNtGopThreadWinMain (
  LPVOID    lpParameter
  )
{
  MSG               Message;
  GOP_PRIVATE_DATA  *Private;
  RECT              Rect;

  Private = (GOP_PRIVATE_DATA *) lpParameter;
  ASSERT (NULL != Private);

  //
  // Since each thread has unique private data, save the private data in Thread
  // Local Storage slot. Then the shared global mTlsIndex can be used to get
  // thread specific context.
  //
  Private->WinNtThunk->TlsSetValue (mTlsIndex, Private);

  Private->ThreadId                   = Private->WinNtThunk->GetCurrentThreadId ();

  Private->WindowsClass.cbSize        = sizeof (WNDCLASSEX);
  Private->WindowsClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  Private->WindowsClass.lpfnWndProc   = WinNtGopThreadWindowProc;
  Private->WindowsClass.cbClsExtra    = 0;
  Private->WindowsClass.cbWndExtra    = 0;
  Private->WindowsClass.hInstance     = NULL;
  Private->WindowsClass.hIcon         = Private->WinNtThunk->LoadIcon (NULL, IDI_APPLICATION);
  Private->WindowsClass.hCursor       = Private->WinNtThunk->LoadCursor (NULL, IDC_ARROW);
  Private->WindowsClass.hbrBackground = (HBRUSH)(UINTN)COLOR_WINDOW;
  Private->WindowsClass.lpszMenuName  = NULL;
  Private->WindowsClass.lpszClassName = WIN_NT_GOP_CLASS_NAME;
  Private->WindowsClass.hIconSm       = Private->WinNtThunk->LoadIcon (NULL, IDI_APPLICATION);

  //
  // This call will fail after the first time, but thats O.K. since we only need
  // WIN_NT_GOP_CLASS_NAME to exist to create the window.
  //
  // Note: Multiple instances of this DLL will use the same instance of this
  // Class, including the callback function, unless the Class is unregistered and
  // successfully registered again.
  //
  Private->WinNtThunk->RegisterClassEx (&Private->WindowsClass);

  //
  // Setting Rect values to allow for the AdjustWindowRect to provide
  // us the correct sizes for the client area when doing the CreateWindowEx
  //
  Rect.top    = 0;
  Rect.bottom = Private->GraphicsOutput.Mode->Info->VerticalResolution;
  Rect.left   = 0;
  Rect.right  = Private->GraphicsOutput.Mode->Info->HorizontalResolution;

  Private->WinNtThunk->AdjustWindowRect (&Rect, WS_OVERLAPPEDWINDOW, 0);

  Private->WindowHandle = Private->WinNtThunk->CreateWindowEx (
                                                0,
                                                WIN_NT_GOP_CLASS_NAME,
                                                Private->WindowName,
                                                WS_OVERLAPPEDWINDOW,
                                                CW_USEDEFAULT,
                                                CW_USEDEFAULT,
                                                Rect.right - Rect.left,
                                                Rect.bottom - Rect.top,
                                                NULL,
                                                NULL,
                                                NULL,
                                                (VOID **)&Private
                                                );

  //
  // The reset of this thread is the standard winows program. We need a sperate
  // thread since we must process the message loop to make windows act like
  // windows.
  //

  Private->WinNtThunk->ShowWindow (Private->WindowHandle, SW_SHOW);
  Private->WinNtThunk->UpdateWindow (Private->WindowHandle);

  //
  // Let the main thread get some work done
  //
  Private->WinNtThunk->ReleaseSemaphore (Private->ThreadInited, 1, NULL);

  //
  // This is the message loop that all Windows programs need.
  //
  while (Private->WinNtThunk->GetMessage (&Message, Private->WindowHandle, 0, 0)) {
    Private->WinNtThunk->TranslateMessage (&Message);
    Private->WinNtThunk->DispatchMessage (&Message);
  }

  return (DWORD)Message.wParam;
}


/**
  TODO: Add function description

  @param  Private                TODO: add argument description
  @param  HorizontalResolution   TODO: add argument description
  @param  VerticalResolution     TODO: add argument description
  @param  ColorDepth             TODO: add argument description
  @param  RefreshRate            TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
WinNtGopStartWindow (
  IN  GOP_PRIVATE_DATA    *Private,
  IN  UINT32              HorizontalResolution,
  IN  UINT32              VerticalResolution,
  IN  UINT32              ColorDepth,
  IN  UINT32              RefreshRate
  )
{
  EFI_STATUS          Status;
  DWORD               NewThreadId;

  mWinNt  = Private->WinNtThunk;

  //
  // Initialize a Thread Local Storge variable slot. We use TLS to get the
  // correct Private data instance into the windows thread.
  //
  if (mTlsIndex == TLS_OUT_OF_INDEXES) {
    ASSERT (0 == mTlsIndexUseCount);
    mTlsIndex = Private->WinNtThunk->TlsAlloc ();
  }

  //
  // always increase the use count!
  //
  mTlsIndexUseCount++;

  //
  // Register to be notified on exit boot services so we can destroy the window.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  KillNtGopThread,
                  Private,
                  &gEfiEventExitBootServicesGuid,
                  &mGopScreenExitBootServicesEvent
                  );

  Private->ThreadInited = Private->WinNtThunk->CreateSemaphore (NULL, 0, 1, NULL);
  Private->ThreadHandle = Private->WinNtThunk->CreateThread (
                                                NULL,
                                                0,
                                                WinNtGopThreadWinMain,
                                                (VOID *) Private,
                                                0,
                                                &NewThreadId
                                                );

  //
  // The other thread has entered the windows message loop so we can
  // continue our initialization.
  //
  Private->WinNtThunk->WaitForSingleObject (Private->ThreadInited, INFINITE);
  Private->WinNtThunk->CloseHandle (Private->ThreadInited);

  return Status;
}


/**


  @return None

**/
// TODO:    Private - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
EFI_STATUS
WinNtGopConstructor (
  GOP_PRIVATE_DATA    *Private
  )
{
  Private->ModeData = mGopModeData;

  Private->GraphicsOutput.QueryMode = WinNtGopQuerytMode;
  Private->GraphicsOutput.SetMode = WinNtGopSetMode;
  Private->GraphicsOutput.Blt            = WinNtGopBlt;

  //
  // Allocate buffer for Graphics Output Protocol mode information
  //
  Private->GraphicsOutput.Mode = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE));
  if (Private->GraphicsOutput.Mode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Private->GraphicsOutput.Mode->Info = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
  if (Private->GraphicsOutput.Mode->Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->GraphicsOutput.Mode->MaxMode = sizeof(mGopModeData) / sizeof(GOP_MODE_DATA);
  //
  // Till now, we have no idea about the window size.
  //
  Private->GraphicsOutput.Mode->Mode = GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER;
  Private->GraphicsOutput.Mode->Info->Version = 0;
  Private->GraphicsOutput.Mode->Info->HorizontalResolution = 0;
  Private->GraphicsOutput.Mode->Info->VerticalResolution = 0;
  Private->GraphicsOutput.Mode->Info->PixelFormat = PixelBltOnly;
  Private->GraphicsOutput.Mode->SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  Private->GraphicsOutput.Mode->FrameBufferBase = (EFI_PHYSICAL_ADDRESS) (UINTN) NULL;
  Private->GraphicsOutput.Mode->FrameBufferSize = 0;

  Private->HardwareNeedsStarting  = TRUE;
  Private->FillLine               = NULL;

  WinNtGopInitializeSimpleTextInForWindow (Private);

  return EFI_SUCCESS;
}


/**


  @return None

**/
// TODO:    Private - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
EFI_STATUS
WinNtGopDestructor (
  GOP_PRIVATE_DATA     *Private
  )
{
  if (!Private->HardwareNeedsStarting) {
    //
    // BugBug: Shutdown GOP Hardware and any child devices.
    //
    Private->WinNtThunk->SendMessage (Private->WindowHandle, WM_DESTROY, 0, 0);
    Private->WinNtThunk->CloseHandle (Private->ThreadHandle);

    mTlsIndexUseCount--;

    //
    // The callback function for another window could still be called,
    // so we need to make sure there are no more users of mTlsIndex.
    //
    if (0 == mTlsIndexUseCount) {
      ASSERT (TLS_OUT_OF_INDEXES != mTlsIndex);

      Private->WinNtThunk->TlsFree (mTlsIndex);
      mTlsIndex = TLS_OUT_OF_INDEXES;

      Private->WinNtThunk->UnregisterClass (
                              Private->WindowsClass.lpszClassName,
                              Private->WindowsClass.hInstance
                              );
    }

    WinNtGopDestroySimpleTextInForWindow (Private);
  }

  //
  // Free graphics output protocol occupied resource
  //
  if (Private->GraphicsOutput.Mode != NULL) {
    if (Private->GraphicsOutput.Mode->Info != NULL) {
      FreePool (Private->GraphicsOutput.Mode->Info);
    }
    FreePool (Private->GraphicsOutput.Mode);
  }

  return EFI_SUCCESS;
}


/**
  This is the GOP screen's callback notification function for exit-boot-services.
  All we do here is call WinNtGopDestructor().

  @param  Event                  not used
  @param  Context                pointer to the Private structure.

  @return None.

**/
VOID
EFIAPI
KillNtGopThread (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  WinNtGopDestructor (Context);
}
