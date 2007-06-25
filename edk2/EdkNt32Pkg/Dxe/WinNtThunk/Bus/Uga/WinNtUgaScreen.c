/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    WinNtUgaScreen.c

Abstract:

  This file produces the graphics abstration of UGA. It is called by
  WinNtUgaDriver.c file which deals with the EFI 1.1 driver model.
  This file just does graphics.

--*/

#include "WinNtUga.h"

EFI_WIN_NT_THUNK_PROTOCOL *mWinNt;
DWORD                     mTlsIndex         = TLS_OUT_OF_INDEXES;
DWORD                     mTlsIndexUseCount = 0;  // lets us know when we can free mTlsIndex.
static EFI_EVENT          mUgaScreenExitBootServicesEvent;

EFI_STATUS
WinNtUgaStartWindow (
  IN  UGA_PRIVATE_DATA    *Private,
  IN  UINT32              HorizontalResolution,
  IN  UINT32              VerticalResolution,
  IN  UINT32              ColorDepth,
  IN  UINT32              RefreshRate
  );

STATIC
VOID
EFIAPI
KillNtUgaThread (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

//
// UGA Protocol Member Functions
//

EFI_STATUS
EFIAPI
WinNtUgaGetMode (
  EFI_UGA_DRAW_PROTOCOL *This,
  UINT32                *HorizontalResolution,
  UINT32                *VerticalResolution,
  UINT32                *ColorDepth,
  UINT32                *RefreshRate
  )
/*++

  Routine Description:
    Return the current video mode information.

  Arguments:
    This                  - Protocol instance pointer.
    HorizontalResolution  - Current video horizontal resolution in pixels
    VerticalResolution    - Current video Vertical resolution in pixels
    ColorDepth            - Current video color depth in bits per pixel
    RefreshRate           - Current video refresh rate in Hz.

  Returns:
    EFI_SUCCESS     - Mode information returned.
    EFI_NOT_STARTED - Video display is not initialized. Call SetMode ()
    EFI_INVALID_PARAMETER - One of the input args was NULL.

--*/
// TODO:    ADD IN/OUT description here
{
  UGA_PRIVATE_DATA  *Private;

  Private = UGA_DRAW_PRIVATE_DATA_FROM_THIS (This);

  if (Private->HardwareNeedsStarting) {
    return EFI_NOT_STARTED;
  }

  if ((HorizontalResolution == NULL) ||
      (VerticalResolution   == NULL) ||
      (ColorDepth           == NULL) ||
      (RefreshRate          == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *HorizontalResolution = Private->HorizontalResolution;
  *VerticalResolution   = Private->VerticalResolution;
  *ColorDepth           = Private->ColorDepth;
  *RefreshRate          = Private->RefreshRate;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtUgaSetMode (
  EFI_UGA_DRAW_PROTOCOL *This,
  UINT32                HorizontalResolution,
  UINT32                VerticalResolution,
  UINT32                ColorDepth,
  UINT32                RefreshRate
  )
/*++

  Routine Description:
    Return the current video mode information.

  Arguments:
    This                  - Protocol instance pointer.
    HorizontalResolution  - Current video horizontal resolution in pixels
    VerticalResolution    - Current video Vertical resolution in pixels
    ColorDepth            - Current video color depth in bits per pixel
    RefreshRate           - Current video refresh rate in Hz.

  Returns:
    EFI_SUCCESS     - Mode information returned.
    EFI_NOT_STARTED - Video display is not initialized. Call SetMode ()
    EFI_INVALID_PARAMETER - One of the input args was NULL.

--*/
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    ADD IN/OUT description here
{
  EFI_STATUS        Status;
  UGA_PRIVATE_DATA  *Private;
  EFI_UGA_PIXEL     Fill;
  EFI_UGA_PIXEL     *NewFillLine;
  RECT              Rect;
  UINTN             Size;
  UINTN             Width;
  UINTN             Height;

  Private = UGA_DRAW_PRIVATE_DATA_FROM_THIS (This);

  if (Private->HardwareNeedsStarting) {
    Status = WinNtUgaStartWindow (
              Private,
              HorizontalResolution,
              VerticalResolution,
              ColorDepth,
              RefreshRate
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
    // See EFI spec chepter 10.5-EFI_UGA_DRAW_PROTOCOL.SetMode()
    //
    Private->WinNtThunk->HeapFree (Private->WinNtThunk->GetProcessHeap (), 0, Private->VirtualScreenInfo);

    //
    // Allocate DIB frame buffer directly from NT for performance enhancement
    // This buffer is the virtual screen/frame buffer. This buffer is not the
    // same a a frame buffer. The first row of this buffer will be the bottom
    // line of the image. This is an artifact of the way we draw to the screen.
    //
    Size = HorizontalResolution * VerticalResolution * sizeof (RGBQUAD) + sizeof (BITMAPV4HEADER);
    Private->VirtualScreenInfo = Private->WinNtThunk->HeapAlloc (
                                                        Private->WinNtThunk->GetProcessHeap (),
                                                        HEAP_ZERO_MEMORY,
                                                        Size
                                                        );

    //
    // Update the virtual screen info data structure
    //
    Private->VirtualScreenInfo->bV4Size           = sizeof (BITMAPV4HEADER);
    Private->VirtualScreenInfo->bV4Width          = HorizontalResolution;
    Private->VirtualScreenInfo->bV4Height         = VerticalResolution;
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
    Rect.right  = HorizontalResolution;
    Rect.bottom = VerticalResolution;

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
    Private->WinNtThunk->MoveWindow (Private->WindowHandle, Rect.left, Rect.top, Width, Height, TRUE);

  }

  NewFillLine = AllocatePool (sizeof (EFI_UGA_PIXEL) * HorizontalResolution);
  if (NewFillLine == NULL) {
    return EFI_DEVICE_ERROR;
  }

  if (Private->FillLine != NULL) {
    FreePool (Private->FillLine);
  }

  Private->FillLine             = NewFillLine;

  Private->HorizontalResolution = HorizontalResolution;
  Private->VerticalResolution   = VerticalResolution;
  Private->ColorDepth           = ColorDepth;
  Private->RefreshRate          = RefreshRate;

  Fill.Red                      = 0x00;
  Fill.Green                    = 0x00;
  Fill.Blue                     = 0x00;
  This->Blt (
          This,
          &Fill,
          EfiUgaVideoFill,
          0,
          0,
          0,
          0,
          HorizontalResolution,
          VerticalResolution,
          HorizontalResolution * sizeof (EFI_UGA_PIXEL)
          );
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtUgaBlt (
  IN  EFI_UGA_DRAW_PROTOCOL                   *This,
  IN  EFI_UGA_PIXEL                           *BltBuffer, OPTIONAL
  IN  EFI_UGA_BLT_OPERATION                   BltOperation,
  IN  UINTN                                   SourceX,
  IN  UINTN                                   SourceY,
  IN  UINTN                                   DestinationX,
  IN  UINTN                                   DestinationY,
  IN  UINTN                                   Width,
  IN  UINTN                                   Height,
  IN  UINTN                                   Delta         OPTIONAL
  )
/*++

  Routine Description:
    Blt pixels from the rectangle (Width X Height) formed by the BltBuffer
    onto the graphics screen starting a location (X, Y). (0, 0) is defined as
    the upper left hand side of the screen. (X, Y) can be outside of the
    current screen geometry and the BltBuffer will be cliped when it is
    displayed. X and Y can be negative or positive. If Width or Height is
    bigger than the current video screen the image will be clipped.

  Arguments:
    This          - Protocol instance pointer.
    X             - X location on graphics screen.
    Y             - Y location on the graphics screen.
    Width         - Width of BltBuffer.
    Height        - Hight of BltBuffer
    BltOperation  - Operation to perform on BltBuffer and video memory
    BltBuffer     - Buffer containing data to blt into video buffer. This
                    buffer has a size of Width*Height*sizeof(EFI_UGA_PIXEL)
    SourceX       - If the BltOperation is a EfiCopyBlt this is the source
                    of the copy. For other BLT operations this argument is not
                    used.
    SourceX       - If the BltOperation is a EfiCopyBlt this is the source
                    of the copy. For other BLT operations this argument is not
                    used.

  Returns:
    EFI_SUCCESS           - The palette is updated with PaletteArray.
    EFI_INVALID_PARAMETER - BltOperation is not valid.
    EFI_DEVICE_ERROR      - A hardware error occured writting to the video
                             buffer.

--*/
// TODO:    SourceY - add argument and description to function comment
// TODO:    DestinationX - add argument and description to function comment
// TODO:    DestinationY - add argument and description to function comment
// TODO:    Delta - add argument and description to function comment
{
  UGA_PRIVATE_DATA  *Private;
  EFI_TPL           OriginalTPL;
  UINTN             DstY;
  UINTN             SrcY;
  RGBQUAD           *VScreen;
  RGBQUAD           *VScreenSrc;
  EFI_UGA_PIXEL     *Blt;
  UINTN             Index;
  RECT              Rect;
  EFI_UGA_PIXEL     *FillPixel;

  Private = UGA_DRAW_PRIVATE_DATA_FROM_THIS (This);

  if ((BltOperation < 0) || (BltOperation >= EfiUgaBltMax)) {
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
    Delta = Width * sizeof (EFI_UGA_PIXEL);
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
    if (SourceY + Height > Private->VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (SourceX + Width > Private->HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
    // We would not want a timer based event (Cursor, ...) to come in while we are
    // doing this operation.
    //
    OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

    for (SrcY = SourceY, DstY = DestinationY; DstY < (Height + DestinationY); SrcY++, DstY++) {
      Blt = (EFI_UGA_PIXEL *) ((UINT8 *) BltBuffer + (DstY * Delta) + DestinationX * sizeof (EFI_UGA_PIXEL));
      VScreen = &Private->VirtualScreen[(Private->VerticalResolution - SrcY - 1) * Private->HorizontalResolution + SourceX];
      CopyMem (Blt, VScreen, sizeof (EFI_UGA_PIXEL) * Width);
    }
  } else {
    //
    // BltBuffer to Video: Source is BltBuffer, destination is Video
    //
    if (DestinationY + Height > Private->VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestinationX + Width > Private->HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
    // We would not want a timer based event (Cursor, ...) to come in while we are
    // doing this operation.
    //
    OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

    if (BltOperation == EfiUgaVideoFill) {
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

      VScreen = &Private->VirtualScreen[(Private->VerticalResolution - DstY - 1) * Private->HorizontalResolution + DestinationX];
      switch (BltOperation) {
      case EfiUgaBltBufferToVideo:
        Blt = (EFI_UGA_PIXEL *) ((UINT8 *) BltBuffer + (SrcY * Delta) + SourceX * sizeof (EFI_UGA_PIXEL));
        CopyMem (VScreen, Blt, Width * sizeof (EFI_UGA_PIXEL));
        break;

      case EfiUgaVideoToVideo:
        VScreenSrc = &Private->VirtualScreen[(Private->VerticalResolution - SrcY - 1) * Private->HorizontalResolution + SourceX];
        CopyMem (VScreen, VScreenSrc, Width * sizeof (EFI_UGA_PIXEL));
        break;

      case EfiUgaVideoFill:
        CopyMem (VScreen, Private->FillLine, Width * sizeof (EFI_UGA_PIXEL));
        break;
      }
    }
  }

  if (BltOperation != EfiUgaVideoToBltBuffer) {
    //
    // Mark the area we just blted as Invalid so WM_PAINT will update.
    //
    Rect.left   = DestinationX;
    Rect.top    = DestinationY;
    Rect.right  = DestinationX + Width;
    Rect.bottom = DestinationY + Height;
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

EFI_STATUS
WinNtUgaSupported (
  IN  EFI_WIN_NT_IO_PROTOCOL  *WinNtIo
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    WinNtIo - add argument and description to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  //
  // Check to see if the IO abstraction represents a device type we support.
  //
  // This would be replaced a check of PCI subsystem ID, etc.
  //
  if (!CompareGuid (WinNtIo->TypeGuid, &gEfiWinNtUgaGuid)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

LRESULT
CALLBACK
WinNtUgaThreadWindowProc (
  IN  HWND    hwnd,
  IN  UINT    iMsg,
  IN  WPARAM  wParam,
  IN  LPARAM  lParam
  )
/*++

Routine Description:
  Win32 Windows event handler.

Arguments:
  See Win32 Book

Returns:
  See Win32 Book

--*/
// TODO:    hwnd - add argument and description to function comment
// TODO:    iMsg - add argument and description to function comment
// TODO:    wParam - add argument and description to function comment
// TODO:    lParam - add argument and description to function comment
{
  UGA_PRIVATE_DATA  *Private;
  UINTN             Size;
  HDC               Handle;
  PAINTSTRUCT       PaintStruct;
  LPARAM            Index;
  EFI_INPUT_KEY     Key;

  //
  // BugBug - if there are two instances of this DLL in memory (such as is
  // the case for ERM), the correct instance of this function may not be called.
  // This also means that the address of the mTlsIndex value will be wrong, and
  // the value may be wrong too.
  //


  //
  // Use mTlsIndex global to get a Thread Local Storage version of Private.
  // This works since each Uga protocol has a unique Private data instance and
  // a unique thread.
  //
  Private = mWinNt->TlsGetValue (mTlsIndex);
  ASSERT (NULL != Private);

  switch (iMsg) {
  case WM_CREATE:
    Size = Private->HorizontalResolution * Private->VerticalResolution * sizeof (RGBQUAD);

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
    Private->VirtualScreenInfo->bV4Width          = Private->HorizontalResolution;
    Private->VirtualScreenInfo->bV4Height         = Private->VerticalResolution;
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
              Private->HorizontalResolution,              // Width
              Private->VerticalResolution,                // Height
              0,                                          // Source X
              0,                                          // Source Y
              0,                                          // DIB Start Scan Line
              Private->VerticalResolution,                // Number of scan lines
              Private->VirtualScreen,                     // Address of array of DIB bits
              (BITMAPINFO *) Private->VirtualScreenInfo,  // Address of structure with bitmap info
              DIB_RGB_COLORS                              // RGB or palette indexes
              );

    mWinNt->EndPaint (hwnd, &PaintStruct);
    return 0;

  //
  // F10 and the ALT key do not create a WM_KEYDOWN message, thus this special case
  //
  case WM_SYSKEYDOWN:
    Key.ScanCode = 0;
    switch (wParam) {
    case VK_F10:
      Key.ScanCode    = SCAN_F10;
      Key.UnicodeChar = 0;
      UgaPrivateAddQ (Private, Key);
      return 0;
    }
    break;

  case WM_KEYDOWN:
    Key.ScanCode = 0;
    switch (wParam) {
    case VK_HOME:       Key.ScanCode = SCAN_HOME;       break;
    case VK_END:        Key.ScanCode = SCAN_END;        break;
    case VK_LEFT:       Key.ScanCode = SCAN_LEFT;       break;
    case VK_RIGHT:      Key.ScanCode = SCAN_RIGHT;      break;
    case VK_UP:         Key.ScanCode = SCAN_UP;         break;
    case VK_DOWN:       Key.ScanCode = SCAN_DOWN;       break;
    case VK_DELETE:     Key.ScanCode = SCAN_DELETE;     break;
    case VK_INSERT:     Key.ScanCode = SCAN_INSERT;     break;
    case VK_PRIOR:      Key.ScanCode = SCAN_PAGE_UP;    break;
    case VK_NEXT:       Key.ScanCode = SCAN_PAGE_DOWN;  break;
    case VK_ESCAPE:     Key.ScanCode = SCAN_ESC;        break;

    case VK_F1:   Key.ScanCode = SCAN_F1;   break;
    case VK_F2:   Key.ScanCode = SCAN_F2;   break;
    case VK_F3:   Key.ScanCode = SCAN_F3;   break;
    case VK_F4:   Key.ScanCode = SCAN_F4;   break;
    case VK_F5:   Key.ScanCode = SCAN_F5;   break;
    case VK_F6:   Key.ScanCode = SCAN_F6;   break;
    case VK_F7:   Key.ScanCode = SCAN_F7;   break;
    case VK_F8:   Key.ScanCode = SCAN_F8;   break;
    case VK_F9:   Key.ScanCode = SCAN_F9;   break;
    case VK_F11:  Key.ScanCode = SCAN_F11;  break;
    case VK_F12:  Key.ScanCode = SCAN_F12;  break;
    }

    if (Key.ScanCode != 0) {
      Key.UnicodeChar = 0;
      UgaPrivateAddQ (Private, Key);
    }

    return 0;

  case WM_CHAR:
    //
    // The ESC key also generate WM_CHAR.
    //
    if (wParam == 0x1B) {
      return 0;
    }

    for (Index = 0; Index < (lParam & 0xffff); Index++) {
      if (wParam != 0) {
        Key.UnicodeChar = (CHAR16) wParam;
        Key.ScanCode    = 0;
        UgaPrivateAddQ (Private, Key);
      }
    }

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

DWORD
WINAPI
WinNtUgaThreadWinMain (
  LPVOID    lpParameter
  )
/*++

Routine Description:

  This thread simulates the end of WinMain () aplication. Each Winow nededs
  to process it's events. The messages are dispatched to
  WinNtUgaThreadWindowProc ().

  Be very careful sine WinNtUgaThreadWinMain () and WinNtUgaThreadWindowProc ()
  are running in a seperate thread. We have to do this to process the events.

Arguments:

  lpParameter - Handle of window to manage.

Returns:

  if a WM_QUIT message is returned exit.

--*/
{
  MSG               Message;
  UGA_PRIVATE_DATA  *Private;
  ATOM              Atom;
  RECT              Rect;

  Private = (UGA_PRIVATE_DATA *) lpParameter;
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
  Private->WindowsClass.lpfnWndProc   = WinNtUgaThreadWindowProc;
  Private->WindowsClass.cbClsExtra    = 0;
  Private->WindowsClass.cbWndExtra    = 0;
  Private->WindowsClass.hInstance     = NULL;
  Private->WindowsClass.hIcon         = Private->WinNtThunk->LoadIcon (NULL, IDI_APPLICATION);
  Private->WindowsClass.hCursor       = Private->WinNtThunk->LoadCursor (NULL, IDC_ARROW);
  Private->WindowsClass.hbrBackground = (HBRUSH) COLOR_WINDOW;
  Private->WindowsClass.lpszMenuName  = NULL;
  Private->WindowsClass.lpszClassName = WIN_NT_UGA_CLASS_NAME;
  Private->WindowsClass.hIconSm       = Private->WinNtThunk->LoadIcon (NULL, IDI_APPLICATION);

  //
  // This call will fail after the first time, but thats O.K. since we only need
  // WIN_NT_UGA_CLASS_NAME to exist to create the window.
  //
  // Note: Multiple instances of this DLL will use the same instance of this
  // Class, including the callback function, unless the Class is unregistered and
  // successfully registered again.
  //
  Atom = Private->WinNtThunk->RegisterClassEx (&Private->WindowsClass);

  //
  // Setting Rect values to allow for the AdjustWindowRect to provide
  // us the correct sizes for the client area when doing the CreateWindowEx
  //
  Rect.top    = 0;
  Rect.bottom = Private->VerticalResolution;
  Rect.left   = 0;
  Rect.right  = Private->HorizontalResolution;

  Private->WinNtThunk->AdjustWindowRect (&Rect, WS_OVERLAPPEDWINDOW, 0);

  Private->WindowHandle = Private->WinNtThunk->CreateWindowEx (
                                                0,
                                                WIN_NT_UGA_CLASS_NAME,
                                                Private->WindowName,
                                                WS_OVERLAPPEDWINDOW,
                                                CW_USEDEFAULT,
                                                CW_USEDEFAULT,
                                                Rect.right - Rect.left,
                                                Rect.bottom - Rect.top,
                                                NULL,
                                                NULL,
                                                NULL,
                                                &Private
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

  return Message.wParam;
}

EFI_STATUS
WinNtUgaStartWindow (
  IN  UGA_PRIVATE_DATA    *Private,
  IN  UINT32              HorizontalResolution,
  IN  UINT32              VerticalResolution,
  IN  UINT32              ColorDepth,
  IN  UINT32              RefreshRate
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private               - TODO: add argument description
  HorizontalResolution  - TODO: add argument description
  VerticalResolution    - TODO: add argument description
  ColorDepth            - TODO: add argument description
  RefreshRate           - TODO: add argument description

Returns:

  TODO: add return values

--*/
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

  Private->HorizontalResolution = HorizontalResolution;
  Private->VerticalResolution   = VerticalResolution;

  //
  // Register to be notified on exit boot services so we can destroy the window.
  //
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  KillNtUgaThread,
                  Private,
                  &mUgaScreenExitBootServicesEvent
                  );

  Private->ThreadInited = Private->WinNtThunk->CreateSemaphore (NULL, 0, 1, NULL);
  Private->ThreadHandle = Private->WinNtThunk->CreateThread (
                                                NULL,
                                                0,
                                                WinNtUgaThreadWinMain,
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

EFI_STATUS
WinNtUgaConstructor (
  UGA_PRIVATE_DATA    *Private
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    Private - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{

  Private->UgaDraw.GetMode        = WinNtUgaGetMode;
  Private->UgaDraw.SetMode        = WinNtUgaSetMode;
  Private->UgaDraw.Blt            = WinNtUgaBlt;

  Private->HardwareNeedsStarting  = TRUE;
  Private->FillLine               = NULL;

  WinNtUgaInitializeSimpleTextInForWindow (Private);

  return EFI_SUCCESS;
}

EFI_STATUS
WinNtUgaDestructor (
  UGA_PRIVATE_DATA     *Private
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    Private - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  UINT32  UnregisterReturn;

  if (!Private->HardwareNeedsStarting) {
    //
    // BugBug: Shutdown Uga Hardware and any child devices.
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

      UnregisterReturn = Private->WinNtThunk->UnregisterClass (
                                                Private->WindowsClass.lpszClassName,
                                                Private->WindowsClass.hInstance
                                                );
    }

    WinNtUgaDestroySimpleTextInForWindow (Private);
  }

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
KillNtUgaThread (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
/*++

Routine Description:

  This is the UGA screen's callback notification function for exit-boot-services.
  All we do here is call WinNtUgaDestructor().

Arguments:

  Event   - not used
  Context - pointer to the Private structure.

Returns:

  None.

--*/
{
  EFI_STATUS  Status;
  Status = WinNtUgaDestructor (Context);
}
