/** @file

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

    WinGopScreen.c

Abstract:

  This file produces the graphics abstration of GOP. It is called by
  WinNtGopDriver.c file which deals with the UEFI 2.0 driver model.
  This file just does graphics.


**/

#include "WinGop.h"

DWORD  mTlsIndex         = TLS_OUT_OF_INDEXES;
DWORD  mTlsIndexUseCount = 0;                     // lets us know when we can free mTlsIndex.

BOOLEAN
WinNtGopConvertParamToEfiKeyShiftState (
  IN  GRAPHICS_PRIVATE_DATA  *Private,
  IN  WPARAM                 *wParam,
  IN  LPARAM                 *lParam,
  IN  BOOLEAN                Flag
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
        Private->RightCtrl = Flag;
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
      Private->LeftLogo = Flag;
      return TRUE;

    case VK_RWIN:
      Private->RightLogo = Flag;
      return TRUE;

    case VK_APPS:
      Private->Menu = Flag;
      return TRUE;
    //
    // BUGBUG: PrintScreen/SysRq can not trigger WM_KEYDOWN message,
    // so SySReq shift state is not supported here.
    //
    case VK_PRINT:
      Private->SysReq = Flag;
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
  IN  GRAPHICS_PRIVATE_DATA  *Private,
  IN  WPARAM                 *wParam,
  IN  LPARAM                 *lParam,
  IN  EFI_INPUT_KEY          *Key
  )
{
  BOOLEAN  Flag;

  Flag = FALSE;
  switch (*wParam) {
    case VK_HOME:       Key->ScanCode = SCAN_HOME;
      Flag                            = TRUE;
      break;
    case VK_END:        Key->ScanCode = SCAN_END;
      Flag                            = TRUE;
      break;
    case VK_LEFT:       Key->ScanCode = SCAN_LEFT;
      Flag                            = TRUE;
      break;
    case VK_RIGHT:      Key->ScanCode = SCAN_RIGHT;
      Flag                            = TRUE;
      break;
    case VK_UP:         Key->ScanCode = SCAN_UP;
      Flag                            = TRUE;
      break;
    case VK_DOWN:       Key->ScanCode = SCAN_DOWN;
      Flag                            = TRUE;
      break;
    case VK_DELETE:     Key->ScanCode = SCAN_DELETE;
      Flag                            = TRUE;
      break;
    case VK_INSERT:     Key->ScanCode = SCAN_INSERT;
      Flag                            = TRUE;
      break;
    case VK_PRIOR:      Key->ScanCode = SCAN_PAGE_UP;
      Flag                            = TRUE;
      break;
    case VK_NEXT:       Key->ScanCode = SCAN_PAGE_DOWN;
      Flag                            = TRUE;
      break;
    case VK_ESCAPE:     Key->ScanCode = SCAN_ESC;
      Flag                            = TRUE;
      break;

    case VK_F1:         Key->ScanCode = SCAN_F1;
      Flag                            = TRUE;
      break;
    case VK_F2:         Key->ScanCode = SCAN_F2;
      Flag                            = TRUE;
      break;
    case VK_F3:         Key->ScanCode = SCAN_F3;
      Flag                            = TRUE;
      break;
    case VK_F4:         Key->ScanCode = SCAN_F4;
      Flag                            = TRUE;
      break;
    case VK_F5:         Key->ScanCode = SCAN_F5;
      Flag                            = TRUE;
      break;
    case VK_F6:         Key->ScanCode = SCAN_F6;
      Flag                            = TRUE;
      break;
    case VK_F7:         Key->ScanCode = SCAN_F7;
      Flag                            = TRUE;
      break;
    case VK_F8:         Key->ScanCode = SCAN_F8;
      Flag                            = TRUE;
      break;
    case VK_F9:         Key->ScanCode = SCAN_F9;
      Flag                            = TRUE;
      break;
    case VK_F11:        Key->ScanCode = SCAN_F11;
      Flag                            = TRUE;
      break;
    case VK_F12:        Key->ScanCode = SCAN_F12;
      Flag                            = TRUE;
      break;

    case VK_F13:        Key->ScanCode = SCAN_F13;
      Flag                            = TRUE;
      break;
    case VK_F14:        Key->ScanCode = SCAN_F14;
      Flag                            = TRUE;
      break;
    case VK_F15:        Key->ScanCode = SCAN_F15;
      Flag                            = TRUE;
      break;
    case VK_F16:        Key->ScanCode = SCAN_F16;
      Flag                            = TRUE;
      break;
    case VK_F17:        Key->ScanCode = SCAN_F17;
      Flag                            = TRUE;
      break;
    case VK_F18:        Key->ScanCode = SCAN_F18;
      Flag                            = TRUE;
      break;
    case VK_F19:        Key->ScanCode = SCAN_F19;
      Flag                            = TRUE;
      break;
    case VK_F20:        Key->ScanCode = SCAN_F20;
      Flag                            = TRUE;
      break;
    case VK_F21:        Key->ScanCode = SCAN_F21;
      Flag                            = TRUE;
      break;
    case VK_F22:        Key->ScanCode = SCAN_F22;
      Flag                            = TRUE;
      break;
    case VK_F23:        Key->ScanCode = SCAN_F23;
      Flag                            = TRUE;
      break;
    case VK_F24:        Key->ScanCode = SCAN_F24;
      Flag                            = TRUE;
      break;
    case VK_PAUSE:      Key->ScanCode = SCAN_PAUSE;
      Flag                            = TRUE;
      break;

    //
    // Set toggle state
    //
    case VK_NUMLOCK:
      Private->NumLock = (BOOLEAN)(!Private->NumLock);
      Flag             = TRUE;
      break;
    case VK_SCROLL:
      Private->ScrollLock = (BOOLEAN)(!Private->ScrollLock);
      Flag                = TRUE;
      break;
    case VK_CAPITAL:
      Private->CapsLock = (BOOLEAN)(!Private->CapsLock);
      Flag              = TRUE;
      break;
  }

  return (WinNtGopConvertParamToEfiKeyShiftState (Private, wParam, lParam, TRUE)) == TRUE ? TRUE : Flag;
}

//
// GOP Protocol Member Functions
//

/**
  Change the resolution and resize of the window
**/
EFI_STATUS
EFIAPI
WinNtWndSize (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsIo,
  IN  UINT32                        Width,
  IN  UINT32                        Height
  )
{
  RETURN_STATUS                         RStatus;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  Info;
  GRAPHICS_PRIVATE_DATA                 *Private;
  RECT                                  Rect;
  BITMAPV4HEADER                        *VirtualScreenInfo;
  FRAME_BUFFER_CONFIGURE                *FrameBufferConfigure;
  UINTN                                 FrameBufferConfigureSize;

  Private = GRAPHICS_PRIVATE_DATA_FROM_THIS (GraphicsIo);

  //
  // Allocate DIB frame buffer directly from NT for performance enhancement
  // This buffer is the virtual screen/frame buffer.
  //
  VirtualScreenInfo = HeapAlloc (
                        GetProcessHeap (),
                        HEAP_ZERO_MEMORY,
                        Width * Height * sizeof (RGBQUAD) + sizeof (BITMAPV4HEADER)
                        );
  if (VirtualScreenInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Update the virtual screen info data structure
  // Use negative Height to make sure screen/buffer are using the same coordinate.
  //
  VirtualScreenInfo->bV4Size     = sizeof (BITMAPV4HEADER);
  VirtualScreenInfo->bV4Width    = Width;
  VirtualScreenInfo->bV4Height   = -(LONG)Height;
  VirtualScreenInfo->bV4Planes   = 1;
  VirtualScreenInfo->bV4BitCount = 32;
  //
  // uncompressed
  //
  VirtualScreenInfo->bV4V4Compression = BI_RGB;

  Info.HorizontalResolution = Width;
  Info.VerticalResolution   = Height;
  Info.PixelFormat          = PixelBlueGreenRedReserved8BitPerColor;
  Info.PixelsPerScanLine    = Width;
  FrameBufferConfigureSize  = 0;
  RStatus                   = FrameBufferBltConfigure (VirtualScreenInfo + 1, &Info, NULL, &FrameBufferConfigureSize);
  ASSERT (RStatus == EFI_BUFFER_TOO_SMALL);
  FrameBufferConfigure = AllocatePool (FrameBufferConfigureSize);
  if (FrameBufferConfigure == NULL) {
    HeapFree (GetProcessHeap (), 0, VirtualScreenInfo);
    return EFI_OUT_OF_RESOURCES;
  }

  RStatus = FrameBufferBltConfigure (VirtualScreenInfo + 1, &Info, FrameBufferConfigure, &FrameBufferConfigureSize);
  ASSERT_RETURN_ERROR (RStatus);

  if (Private->FrameBufferConfigure != NULL) {
    FreePool (Private->FrameBufferConfigure);
  }

  Private->FrameBufferConfigure = FrameBufferConfigure;

  //
  // Free the old buffer. We do not save the content of the old buffer since the
  // screen is to be cleared anyway. Clearing the screen is required by the EFI spec.
  // See UEFI spec -EFI_GRAPHICS_OUTPUT_PROTOCOL.SetMode()
  //
  if (Private->VirtualScreenInfo != NULL) {
    HeapFree (GetProcessHeap (), 0, Private->VirtualScreenInfo);
  }

  Private->VirtualScreenInfo = VirtualScreenInfo;

  Private->Width  = Width;
  Private->Height = Height;

  //
  // Use the AdjuctWindowRect fuction to calculate the real width and height
  // of the new window including the border and caption
  //
  Rect.left   = 0;
  Rect.top    = 0;
  Rect.right  = Width;
  Rect.bottom = Height;

  AdjustWindowRect (&Rect, WS_OVERLAPPEDWINDOW, 0);

  Width  = Rect.right - Rect.left;
  Height = Rect.bottom - Rect.top;

  //
  // Retrieve the original window position information
  //
  GetWindowRect (Private->WindowHandle, &Rect);

  //
  // Adjust the window size
  //
  MoveWindow (Private->WindowHandle, Rect.left, Rect.top, (INT32)Width, (INT32)Height, TRUE);

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
  @param  Height                 Height of BltBuffer
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
  @retval EFI_DEVICE_ERROR       A hardware error occurred writing to the video
                                 buffer.

**/
// TODO:    SourceY - add argument and description to function comment
// TODO:    DestinationX - add argument and description to function comment
// TODO:    DestinationY - add argument and description to function comment
// TODO:    Delta - add argument and description to function comment
EFI_STATUS
WinNtWndBlt (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL    *GraphicsIo,
  IN  EFI_UGA_PIXEL                   *BltBuffer OPTIONAL,
  IN  EFI_UGA_BLT_OPERATION           BltOperation,
  IN  EMU_GRAPHICS_WINDOWS__BLT_ARGS  *Args
  )
{
  RETURN_STATUS          RStatus;
  GRAPHICS_PRIVATE_DATA  *Private;
  RECT                   Rect;

  Private = GRAPHICS_PRIVATE_DATA_FROM_THIS (GraphicsIo);
  RStatus = FrameBufferBlt (
              Private->FrameBufferConfigure,
              BltBuffer,
              BltOperation,
              Args->SourceX,
              Args->SourceY,
              Args->DestinationX,
              Args->DestinationY,
              Args->Width,
              Args->Height,
              Args->Delta
              );
  if (RETURN_ERROR (RStatus)) {
    return (EFI_STATUS)RStatus;
  }

  if (BltOperation != EfiBltVideoToBltBuffer) {
    //
    // Mark the area we just blted as Invalid so WM_PAINT will update.
    //
    Rect.left   = (LONG)Args->DestinationX;
    Rect.top    = (LONG)Args->DestinationY;
    Rect.right  = (LONG)(Args->DestinationX + Args->Width);
    Rect.bottom = (LONG)(Args->DestinationY + Args->Height);
    InvalidateRect (Private->WindowHandle, &Rect, FALSE);

    //
    // Send the WM_PAINT message to the thread that is drawing the window. We
    // are in the main thread and the window drawing is in a child thread.
    // There is a child thread per window. We have no CriticalSection or Mutex
    // since we write the data and the other thread displays the data. While
    // we may miss some data for a short period of time this is no different than
    // a write combining on writes to a frame buffer.
    //

    UpdateWindow (Private->WindowHandle);
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
  GRAPHICS_PRIVATE_DATA  *Private;
  HDC                    Handle;
  PAINTSTRUCT            PaintStruct;
  LPARAM                 Index;
  EFI_INPUT_KEY          Key;
  BOOLEAN                AltIsPress;
  INT32                  PosX;
  INT32                  PosY;

  //
  // Use mTlsIndex global to get a Thread Local Storage version of Private.
  // This works since each Gop protocol has a unique Private data instance and
  // a unique thread.
  //
  AltIsPress = FALSE;
  Private    = TlsGetValue (mTlsIndex);
  ASSERT (NULL != Private);

  switch (iMsg) {
    case WM_PAINT:
      Handle = BeginPaint (hwnd, &PaintStruct);

      SetDIBitsToDevice (
        Handle,                                   // Destination Device Context
        0,                                        // Destination X - 0
        0,                                        // Destination Y - 0
        Private->Width,                           // Width
        Private->Height,                          // Height
        0,                                        // Source X
        0,                                        // Source Y
        0,                                        // DIB Start Scan Line
        Private->Height,                          // Number of scan lines
        Private->VirtualScreenInfo + 1,           // Address of array of DIB bits
        (BITMAPINFO *)Private->VirtualScreenInfo, // Address of structure with bitmap info
        DIB_RGB_COLORS                            // RGB or palette indexes
        );

      EndPaint (hwnd, &PaintStruct);
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
        if (Key.ScanCode != 0) {
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
          Key.UnicodeChar = (CHAR16)wParam;
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

    case WM_MOUSEMOVE:
      PosX = GET_X_LPARAM (lParam);
      PosY = GET_Y_LPARAM (lParam);

      if (Private->PointerPreviousX != PosX) {
        Private->PointerState.RelativeMovementX += (PosX - Private->PointerPreviousX);
        Private->PointerPreviousX                = PosX;
        Private->PointerStateChanged             = TRUE;
      }

      if (Private->PointerPreviousY != PosY) {
        Private->PointerState.RelativeMovementY += (PosY - Private->PointerPreviousY);
        Private->PointerPreviousY                = PosY;
        Private->PointerStateChanged             = TRUE;
      }

      Private->PointerState.RelativeMovementZ = 0;
      return 0;

    case WM_LBUTTONDOWN:
      Private->PointerState.LeftButton = TRUE;
      Private->PointerStateChanged     = TRUE;
      return 0;

    case WM_LBUTTONUP:
      Private->PointerState.LeftButton = FALSE;
      Private->PointerStateChanged     = TRUE;
      return 0;

    case WM_RBUTTONDOWN:
      Private->PointerState.RightButton = TRUE;
      Private->PointerStateChanged      = TRUE;
      return 0;

    case WM_RBUTTONUP:
      Private->PointerState.RightButton = FALSE;
      Private->PointerStateChanged      = TRUE;
      return 0;

    case WM_CLOSE:
      //
      // This close message is issued by user, core is not aware of this,
      // so don't release the window display resource, just hide the window.
      //
      ShowWindow (Private->WindowHandle, SW_HIDE);
      return 0;

    case WM_DESTROY:
      DestroyWindow (hwnd);
      PostQuitMessage (0);

      HeapFree (GetProcessHeap (), 0, Private->VirtualScreenInfo);

      ExitThread (0);

    default:
      break;
  }

  return DefWindowProc (hwnd, iMsg, wParam, lParam);
}

/**
  This thread simulates the end of WinMain () application. Each Window needs
  to process its events. The messages are dispatched to
  WinNtGopThreadWindowProc ().
  Be very careful since WinNtGopThreadWinMain () and WinNtGopThreadWindowProc ()
  are running in a separate thread. We have to do this to process the events.

  @param  lpParameter            Handle of window to manage.

  @return if a WM_QUIT message is returned exit.

**/
DWORD
WINAPI
WinNtGopThreadWinMain (
  LPVOID  lpParameter
  )
{
  MSG                    Message;
  GRAPHICS_PRIVATE_DATA  *Private;
  RECT                   Rect;

  Private = (GRAPHICS_PRIVATE_DATA *)lpParameter;
  ASSERT (NULL != Private);

  //
  // Since each thread has unique private data, save the private data in Thread
  // Local Storage slot. Then the shared global mTlsIndex can be used to get
  // thread specific context.
  //
  TlsSetValue (mTlsIndex, Private);

  Private->ThreadId = GetCurrentThreadId ();

  Private->WindowsClass.cbSize        = sizeof (WNDCLASSEX);
  Private->WindowsClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  Private->WindowsClass.lpfnWndProc   = WinNtGopThreadWindowProc;
  Private->WindowsClass.cbClsExtra    = 0;
  Private->WindowsClass.cbWndExtra    = 0;
  Private->WindowsClass.hInstance     = NULL;
  Private->WindowsClass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
  Private->WindowsClass.hCursor       = LoadCursor (NULL, IDC_ARROW);
  Private->WindowsClass.hbrBackground = (HBRUSH)(UINTN)COLOR_WINDOW;
  Private->WindowsClass.lpszMenuName  = NULL;
  Private->WindowsClass.lpszClassName = WIN_NT_GOP_CLASS_NAME;
  Private->WindowsClass.hIconSm       = LoadIcon (NULL, IDI_APPLICATION);

  //
  // Use 100 x 100 as initial Window size.
  //
  Private->Width  = 100;
  Private->Height = 100;

  //
  // This call will fail after the first time, but thats O.K. since we only need
  // WIN_NT_GOP_CLASS_NAME to exist to create the window.
  //
  RegisterClassEx (&Private->WindowsClass);

  //
  // Setting Rect values to allow for the AdjustWindowRect to provide
  // us the correct sizes for the client area when doing the CreateWindowEx
  //
  Rect.top    = 0;
  Rect.bottom = Private->Height;
  Rect.left   = 0;
  Rect.right  = Private->Width;

  AdjustWindowRect (&Rect, WS_OVERLAPPEDWINDOW, 0);

  Private->WindowHandle = CreateWindowEx (
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
  // The reset of this thread is the standard windows program. We need a separate
  // thread since we must process the message loop to make windows act like
  // windows.
  //

  ShowWindow (Private->WindowHandle, SW_SHOW);
  UpdateWindow (Private->WindowHandle);

  //
  // Let the main thread get some work done
  //
  ReleaseSemaphore (Private->ThreadInited, 1, NULL);

  //
  // This is the message loop that all Windows programs need.
  //
  while (GetMessage (&Message, Private->WindowHandle, 0, 0)) {
    TranslateMessage (&Message);
    DispatchMessage (&Message);
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
EFIAPI
WinNtGraphicsWindowOpen (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  )
{
  DWORD                  NewThreadId;
  GRAPHICS_PRIVATE_DATA  *Private;

  Private = AllocateZeroPool (sizeof (*Private));

  GopPrivateCreateQ (Private, &Private->QueueForRead);

  Private->GraphicsWindowIo.Size              = WinNtWndSize;
  Private->GraphicsWindowIo.CheckKey          = WinNtWndCheckKey;
  Private->GraphicsWindowIo.GetKey            = WinNtWndGetKey;
  Private->GraphicsWindowIo.KeySetState       = WinNtWndKeySetState;
  Private->GraphicsWindowIo.RegisterKeyNotify = WinNtWndRegisterKeyNotify;
  Private->GraphicsWindowIo.Blt               = WinNtWndBlt;
  Private->GraphicsWindowIo.CheckPointer      = WinNtWndCheckPointer;
  Private->GraphicsWindowIo.GetPointerState   = WinNtWndGetPointerState;

  Private->WindowName = This->ConfigString;
  //
  // Initialize a Thread Local Storge variable slot. We use TLS to get the
  // correct Private data instance into the windows thread.
  //
  if (mTlsIndex == TLS_OUT_OF_INDEXES) {
    ASSERT (0 == mTlsIndexUseCount);
    mTlsIndex = TlsAlloc ();
  }

  //
  // always increase the use count!
  //
  mTlsIndexUseCount++;

  Private->ThreadInited = CreateSemaphore (NULL, 0, 1, NULL);
  Private->ThreadHandle = CreateThread (
                            NULL,
                            0,
                            WinNtGopThreadWinMain,
                            (VOID *)Private,
                            0,
                            &NewThreadId
                            );

  //
  // The other thread has entered the windows message loop so we can
  // continue our initialization.
  //
  WaitForSingleObject (Private->ThreadInited, INFINITE);
  CloseHandle (Private->ThreadInited);

  This->Private   = Private;
  This->Interface = &Private->GraphicsWindowIo;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtGraphicsWindowClose (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  )
{
  GRAPHICS_PRIVATE_DATA  *Private;

  Private = (GRAPHICS_PRIVATE_DATA *)This->Private;

  //
  // BugBug: Shutdown GOP Hardware and any child devices.
  //
  SendMessage (Private->WindowHandle, WM_DESTROY, 0, 0);
  CloseHandle (Private->ThreadHandle);

  mTlsIndexUseCount--;

  //
  // The callback function for another window could still be called,
  // so we need to make sure there are no more users of mTlsIndex.
  //
  if (0 == mTlsIndexUseCount) {
    ASSERT (TLS_OUT_OF_INDEXES != mTlsIndex);

    TlsFree (mTlsIndex);
    mTlsIndex = TLS_OUT_OF_INDEXES;

    UnregisterClass (
      Private->WindowsClass.lpszClassName,
      Private->WindowsClass.hInstance
      );
  }

  GopPrivateDestroyQ (Private, &Private->QueueForRead);
  return EFI_SUCCESS;
}

EMU_IO_THUNK_PROTOCOL  mWinNtWndThunkIo = {
  &gEmuGraphicsWindowProtocolGuid,
  NULL,
  NULL,
  0,
  WinNtGraphicsWindowOpen,
  WinNtGraphicsWindowClose,
  NULL
};
