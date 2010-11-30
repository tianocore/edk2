/*++

Copyright (c) 2004 - 2009, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

--*/

#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>

#include <PiPei.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/UgaDraw.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/XShm.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <Protocol/UnixThunk.h>
#include <Protocol/UnixUgaIo.h>

#include <Ppi/StatusCode.h>

#include <Library/PeCoffLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>

#include "Gasket.h"
#include "SecMain.h"


extern void msSleep (unsigned long Milliseconds);

/* XQueryPointer  */

struct uga_drv_shift_mask {
  unsigned char shift;
  unsigned char size;
  unsigned char csize;
};

#define NBR_KEYS 32
typedef struct {
  EFI_UNIX_UGA_IO_PROTOCOL UgaIo;

  Display *display;
  int screen;			/* values for window_size in main */
  Window win;
  GC gc;
  Visual *visual;

  int depth;
  unsigned int width;
  unsigned int height;
  unsigned int line_bytes;
  unsigned int pixel_shift;
  unsigned char *image_data;

  struct uga_drv_shift_mask r, g, b;

  int use_shm;
  XShmSegmentInfo xshm_info;
  XImage *image;

  unsigned int key_rd;
  unsigned int key_wr;
  unsigned int key_count;
  EFI_KEY_DATA keys[NBR_KEYS];

  EFI_KEY_STATE KeyState;
  
  UGA_REGISTER_KEY_NOTIFY_CALLBACK RegisterdKeyCallback;
  VOID                             *RegisterdKeyCallbackContext;
  
  int                        previous_x;
  int                        previous_y;
  EFI_SIMPLE_POINTER_STATE   pointer_state;
  int                        pointer_state_changed;
} UGA_IO_PRIVATE;

void
HandleEvents(UGA_IO_PRIVATE *drv);

void
fill_shift_mask (struct uga_drv_shift_mask *sm, unsigned long mask)
{
  sm->shift = 0;
  sm->size = 0;
  while ((mask & 1) == 0)
    {
      mask >>= 1;
      sm->shift++;
    }
  while (mask & 1)
    {
      sm->size++;
      mask >>= 1;
    }
  sm->csize = 8 - sm->size;
}

int
TryCreateShmImage(UGA_IO_PRIVATE *drv)
{
  drv->image = XShmCreateImage (drv->display, drv->visual,
                 drv->depth, ZPixmap, NULL, &drv->xshm_info,
                 drv->width, drv->height);
  if (drv->image == NULL)
    return 0;

  switch (drv->image->bitmap_unit) {
  case 32:
    drv->pixel_shift = 2;
    break;
  case 16:
    drv->pixel_shift = 1;
    break;
  case 8:
    drv->pixel_shift = 0;
    break;
  }

  drv->xshm_info.shmid = shmget
                          (IPC_PRIVATE, drv->image->bytes_per_line * drv->image->height,
                          IPC_CREAT | 0777);
  if (drv->xshm_info.shmid < 0) {
    XDestroyImage(drv->image);
    return 0;
  }
      
  drv->image_data = shmat (drv->xshm_info.shmid, NULL, 0);
  if(!drv->image_data) {
    shmctl (drv->xshm_info.shmid, IPC_RMID, NULL);
    XDestroyImage(drv->image);
    return 0;
  }
  
#ifndef __APPLE__  
  //
  // This closes shared memory in real time on OS X. Only closes after folks quit using
  // it on Linux. 
  //
  /* Can this fail ?  */
  shmctl (drv->xshm_info.shmid, IPC_RMID, NULL);
#endif

  drv->xshm_info.shmaddr = (char*)drv->image_data;
  drv->image->data = (char*)drv->image_data;

  if (!XShmAttach (drv->display, &drv->xshm_info)) {
    shmdt (drv->image_data);
    XDestroyImage(drv->image);
    return 0;
  }
  return 1;
}

EFI_STATUS
UgaClose (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo)
{
  UGA_IO_PRIVATE *drv = (UGA_IO_PRIVATE *)UgaIo;

  if (drv == NULL)
    return EFI_SUCCESS;
  if (drv->image != NULL)
    {
      XDestroyImage(drv->image);

      if (drv->use_shm)
        shmdt (drv->image_data);

      drv->image_data = NULL;
      drv->image = NULL;
    }
  XDestroyWindow(drv->display, drv->win);
  XCloseDisplay(drv->display);
  
#ifdef __APPLE__
  // Free up the shared memory
  shmctl (drv->xshm_info.shmid, IPC_RMID, NULL);
#endif
  
  free(drv);
  return EFI_SUCCESS;
}

EFI_STATUS
UgaSize(EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, UINT32 Width, UINT32 Height)
{
  UGA_IO_PRIVATE *drv = (UGA_IO_PRIVATE *)UgaIo;
  XSizeHints size_hints;

  /* Destroy current buffer if created.  */
  if (drv->image != NULL)
    {
      /* Before destroy buffer, need to make sure the buffer available for access. */
      XDestroyImage(drv->image);

      if (drv->use_shm)
        shmdt (drv->image_data);

      drv->image_data = NULL;
      drv->image = NULL;
    }

  drv->width = Width;
  drv->height = Height;
  XResizeWindow (drv->display, drv->win, Width, Height);

  /* Allocate image.  */
  if (XShmQueryExtension(drv->display) && TryCreateShmImage(drv)) {
    drv->use_shm = 1;
  } else {
    drv->use_shm = 0;
    if (drv->depth > 16)
      drv->pixel_shift = 2;
    else if (drv->depth > 8)
      drv->pixel_shift = 1;
    else
      drv->pixel_shift = 0;
      
      drv->image_data = malloc((drv->width * drv->height) << drv->pixel_shift);
      drv->image = XCreateImage (drv->display, drv->visual, drv->depth,
                                  ZPixmap, 0, (char *)drv->image_data,
                                  drv->width, drv->height,
                                  8 << drv->pixel_shift, 0);
    }
  drv->line_bytes = drv->image->bytes_per_line;
  fill_shift_mask (&drv->r, drv->image->red_mask);
  fill_shift_mask (&drv->g, drv->image->green_mask);
  fill_shift_mask (&drv->b, drv->image->blue_mask);

  /* Set WM hints.  */
  size_hints.flags = PSize | PMinSize | PMaxSize;
  size_hints.min_width = size_hints.max_width = size_hints.base_width = Width;
  size_hints.min_height = size_hints.max_height = size_hints.base_height = Height;
  XSetWMNormalHints (drv->display, drv->win, &size_hints);

  XMapWindow (drv->display, drv->win);
  HandleEvents(drv);
  return EFI_SUCCESS;
}

void
handleKeyEvent(UGA_IO_PRIVATE *drv, XEvent *ev)
{
  KeySym keysym;
  char str[4];
  EFI_KEY_DATA KeyData;
  int res;

  if (drv->key_count == NBR_KEYS)
    return;

  res = XLookupString(&ev->xkey, str, sizeof(str), &keysym, NULL);
  KeyData.Key.ScanCode = 0;
  KeyData.Key.UnicodeChar = 0;
  KeyData.KeyState.KeyShiftState = 0;

  //
  // KeyRelease is not supported (on Mac) so we can not easily implement Ex functions.
  // If a modifier key is hit by its self we get a keysym. If a modfifier and key is hit
  // we get the state bit set and keysym is the modified key. 
  //
  // We use lack of state bits being set to clear ToggleState and KeyShiftState. We can 
  // also use the stat bits to set ToggleState and KeyShiftState. 
  // Skipping EFI_SCROLL_LOCK_ACTIVE & EFI_NUM_LOCK_ACTIVE since they are not on Macs  
  //
  if ((ev->xkey.state & LockMask) == 0) {
    drv->KeyState.KeyToggleState &= ~EFI_CAPS_LOCK_ACTIVE;
  } else {
    drv->KeyState.KeyToggleState |= EFI_CAPS_LOCK_ACTIVE;
  }
  
  if ((ev->xkey.state & ControlMask) == 0) {
    drv->KeyState.KeyShiftState &= ~(EFI_RIGHT_CONTROL_PRESSED | EFI_LEFT_CONTROL_PRESSED);
  } else if ((drv->KeyState.KeyShiftState & EFI_RIGHT_CONTROL_PRESSED) == 0) {
    drv->KeyState.KeyShiftState |= EFI_LEFT_CONTROL_PRESSED;
  }
  
  if ((ev->xkey.state & ShiftMask) == 0) {
    drv->KeyState.KeyShiftState &= ~(EFI_RIGHT_SHIFT_PRESSED | EFI_LEFT_SHIFT_PRESSED);
  } else if ((drv->KeyState.KeyShiftState & EFI_RIGHT_SHIFT_PRESSED) == 0) {
    drv->KeyState.KeyShiftState |= EFI_LEFT_SHIFT_PRESSED;
  }
  
  if ((ev->xkey.state & Mod2Mask) == 0) {
    drv->KeyState.KeyShiftState &= ~(EFI_RIGHT_LOGO_PRESSED | EFI_LEFT_LOGO_PRESSED);
  } else if ((drv->KeyState.KeyShiftState & EFI_RIGHT_LOGO_PRESSED) == 0) {
    drv->KeyState.KeyShiftState |= EFI_LEFT_LOGO_PRESSED;
  }

  if ((ev->xkey.state & 0x2000) == 0) {
    drv->KeyState.KeyShiftState &= ~(EFI_LEFT_ALT_PRESSED);
  } else {
    drv->KeyState.KeyShiftState |= EFI_LEFT_ALT_PRESSED;
  }
  
  
  switch (keysym) {
  case XK_Control_R:
    drv->KeyState.KeyShiftState |= EFI_RIGHT_CONTROL_PRESSED;
    break;
  case XK_Control_L:
    drv->KeyState.KeyShiftState |= EFI_LEFT_CONTROL_PRESSED;
    break;

  case XK_Shift_R:
    drv->KeyState.KeyShiftState |= EFI_RIGHT_SHIFT_PRESSED;
    break;
  case XK_Shift_L:
    drv->KeyState.KeyShiftState |= EFI_LEFT_SHIFT_PRESSED;
    break;
  
  case XK_Mode_switch:
    drv->KeyState.KeyShiftState |= EFI_LEFT_ALT_PRESSED;
    break;

  case XK_Meta_R:
    drv->KeyState.KeyShiftState |= EFI_RIGHT_LOGO_PRESSED;
    break;
  case XK_Meta_L:
    drv->KeyState.KeyShiftState |= EFI_LEFT_LOGO_PRESSED;
    break;

  case XK_Home:       KeyData.Key.ScanCode = SCAN_HOME;       break;
  case XK_End:        KeyData.Key.ScanCode = SCAN_END;        break;
  case XK_Left:       KeyData.Key.ScanCode = SCAN_LEFT;       break;
  case XK_Right:      KeyData.Key.ScanCode = SCAN_RIGHT;      break;
  case XK_Up:         KeyData.Key.ScanCode = SCAN_UP;         break;
  case XK_Down:       KeyData.Key.ScanCode = SCAN_DOWN;       break;
  case XK_Delete:     KeyData.Key.ScanCode = SCAN_DELETE;     break;
  case XK_Insert:     KeyData.Key.ScanCode = SCAN_INSERT;     break;
  case XK_Page_Up:    KeyData.Key.ScanCode = SCAN_PAGE_UP;    break;
  case XK_Page_Down:  KeyData.Key.ScanCode = SCAN_PAGE_DOWN;  break;
  case XK_Escape:     KeyData.Key.ScanCode = SCAN_ESC;        break;

  case XK_F1:   KeyData.Key.ScanCode = SCAN_F1;   break;
  case XK_F2:   KeyData.Key.ScanCode = SCAN_F2;   break;
  case XK_F3:   KeyData.Key.ScanCode = SCAN_F3;   break;
  case XK_F4:   KeyData.Key.ScanCode = SCAN_F4;   break;
  case XK_F5:   KeyData.Key.ScanCode = SCAN_F5;   break;
  case XK_F6:   KeyData.Key.ScanCode = SCAN_F6;   break;
  case XK_F7:   KeyData.Key.ScanCode = SCAN_F7;   break;
  case XK_F8:   KeyData.Key.ScanCode = SCAN_F8;   break;
  case XK_F9:   KeyData.Key.ScanCode = SCAN_F9;   break;

  default:
    if (res == 1) {
      KeyData.Key.UnicodeChar = str[0];
    } else {
      return;
    }
  }

  // The global state is our state
  KeyData.KeyState.KeyShiftState = drv->KeyState.KeyShiftState;
  KeyData.KeyState.KeyToggleState = drv->KeyState.KeyToggleState;

  CopyMem (&drv->keys[drv->key_wr], &KeyData, sizeof (EFI_KEY_DATA));
  drv->key_wr = (drv->key_wr + 1) % NBR_KEYS;
  drv->key_count++;
  
  
#if defined(__APPLE__) || defined(MDE_CPU_X64)
  ReverseGasketUint64Uint64 (drv->RegisterdKeyCallback ,drv->RegisterdKeyCallbackContext, &KeyData);
#else
  drv->RegisterdKeyCallback (drv->RegisterdKeyCallbackContext, &KeyData);
#endif

  
}


void
handleMouseMoved(UGA_IO_PRIVATE *drv, XEvent *ev)
{
  if ( ev->xmotion.x != drv->previous_x )
  {
    drv->pointer_state.RelativeMovementX += ( ev->xmotion.x - drv->previous_x );
	drv->previous_x = ev->xmotion.x;
	drv->pointer_state_changed = 1;
  }

  if ( ev->xmotion.y != drv->previous_y )
  {
    drv->pointer_state.RelativeMovementY += ( ev->xmotion.y - drv->previous_y );
    drv->previous_y = ev->xmotion.y;
	drv->pointer_state_changed = 1;
  }

  drv->pointer_state.RelativeMovementZ = 0;
}

void
handleMouseDown(UGA_IO_PRIVATE *drv, XEvent *ev, BOOLEAN Pressed)
{
  if ( ev->xbutton.button == Button1 )
  {
    drv->pointer_state_changed = ( drv->pointer_state.LeftButton != Pressed );
	drv->pointer_state.LeftButton = Pressed;
  }
  if ( ev->xbutton.button == Button2 )
  {
    drv->pointer_state_changed = ( drv->pointer_state.RightButton != Pressed );
	drv->pointer_state.RightButton = Pressed;
  }
}

void
Redraw(UGA_IO_PRIVATE *drv, UINTN X, UINTN Y, UINTN Width, UINTN Height)
{
  if (drv->use_shm)
    XShmPutImage (drv->display, drv->win, drv->gc, drv->image,
                   X, Y, X, Y, Width, Height, False);
  else
    XPutImage (drv->display, drv->win, drv->gc, drv->image,
                X, Y, X, Y, Width, Height);
  XFlush(drv->display);
}

void
HandleEvent(UGA_IO_PRIVATE *drv, XEvent *ev)
{
  switch (ev->type)
    {
    case Expose:
      Redraw(drv, ev->xexpose.x, ev->xexpose.y,
        ev->xexpose.width, ev->xexpose.height);
      break;
    case GraphicsExpose:
      Redraw(drv, ev->xgraphicsexpose.x, ev->xgraphicsexpose.y,
        ev->xgraphicsexpose.width, ev->xgraphicsexpose.height);
      break;
    case KeyPress:
      handleKeyEvent(drv, ev);
      break;
    case KeyRelease:
      break;
    case MappingNotify:
      XRefreshKeyboardMapping(&ev->xmapping);
      break;
    case MotionNotify:
      handleMouseMoved(drv, ev);
      break;
    case ButtonPress:
      handleMouseDown(drv, ev, TRUE);
	  break;
    case ButtonRelease:
      handleMouseDown(drv, ev, FALSE);
	  break;
#if 0
    case DestroyNotify:
      XCloseDisplay (drv->display);
      exit (1);
      break;
#endif
    case NoExpose:
    default:
      break;
    }
}

void
HandleEvents(UGA_IO_PRIVATE *drv)
{
  while (XPending(drv->display) != 0)
    {
      XEvent ev;

      XNextEvent (drv->display, &ev);
      HandleEvent(drv, &ev);
    }
}

unsigned long
UgaPixelToColor (UGA_IO_PRIVATE *drv, EFI_UGA_PIXEL pixel)
{
  return ((pixel.Red >> drv->r.csize) << drv->r.shift)
    | ((pixel.Green >> drv->g.csize) << drv->g.shift)
    | ((pixel.Blue >> drv->b.csize) << drv->b.shift);
}

EFI_UGA_PIXEL
UgaColorToPixel (UGA_IO_PRIVATE *drv, unsigned long val)
{
  EFI_UGA_PIXEL res;

  memset (&res, 0, sizeof (EFI_UGA_PIXEL));
  /* FIXME: should round instead of truncate.  */
  res.Red = (val >> drv->r.shift) << drv->r.csize;
  res.Green = (val >> drv->g.shift) << drv->g.csize;
  res.Blue = (val >> drv->b.shift) << drv->b.csize;

  return res;
}

STATIC EFI_STATUS
CheckKeyInternal( UGA_IO_PRIVATE *drv, BOOLEAN delay )
{
  HandleEvents(drv);
  if (drv->key_count != 0)
    return EFI_SUCCESS;
  if ( delay )
    /* EFI is polling.  Be CPU-friendly.  */
    msSleep (20);
    return EFI_NOT_READY;
  }

EFI_STATUS
UgaCheckKey(EFI_UNIX_UGA_IO_PROTOCOL *UgaIo)
{
  UGA_IO_PRIVATE  *drv = (UGA_IO_PRIVATE *)UgaIo;
  return( CheckKeyInternal( drv, TRUE ) );
}

EFI_STATUS
EFIAPI
UgaGetKey (
  IN  EFI_UNIX_UGA_IO_PROTOCOL  *UgaIo, 
  IN  EFI_KEY_DATA              *KeyData
  )
{
  UGA_IO_PRIVATE *drv = (UGA_IO_PRIVATE *)UgaIo;
  EFI_STATUS status;

  status = CheckKeyInternal(drv, FALSE);
  if (status != EFI_SUCCESS)
    return status;

  CopyMem (KeyData, &drv->keys[drv->key_rd], sizeof (EFI_KEY_DATA));
  drv->key_rd = (drv->key_rd + 1) % NBR_KEYS;
  drv->key_count--;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
UgaKeySetState (
  IN EFI_UNIX_UGA_IO_PROTOCOL   *UgaIo, 
  IN EFI_KEY_TOGGLE_STATE       *KeyToggleState
  )
{
  UGA_IO_PRIVATE  *drv = (UGA_IO_PRIVATE *)UgaIo;
//  XKeyEvent       event;
  
  if (*KeyToggleState & EFI_CAPS_LOCK_ACTIVE) {
    if ((drv->KeyState.KeyToggleState & EFI_CAPS_LOCK_ACTIVE) == 0) {
      //
      // We could create an XKeyEvent and send a XK_Caps_Lock to
      // the UGA/GOP Window
      //
    }
  }
    
  drv->KeyState.KeyToggleState = *KeyToggleState;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
UgaRegisterKeyNotify (
  IN EFI_UNIX_UGA_IO_PROTOCOL           *UgaIo, 
  IN UGA_REGISTER_KEY_NOTIFY_CALLBACK   CallBack,
  IN VOID                               *Context
  )
{
  UGA_IO_PRIVATE  *drv = (UGA_IO_PRIVATE *)UgaIo;

  drv->RegisterdKeyCallback = CallBack;
  drv->RegisterdKeyCallbackContext = Context;

  return EFI_SUCCESS;
}


EFI_STATUS
UgaBlt(
  IN EFI_UNIX_UGA_IO_PROTOCOL                 *UgaIo,
  IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
  IN  EFI_UGA_BLT_OPERATION                   BltOperation,
  IN  UGA_BLT_ARGS                            *Args
  )
{
  UGA_IO_PRIVATE *Private = (UGA_IO_PRIVATE *)UgaIo;
  UINTN             DstY;
  UINTN             SrcY;
  UINTN             DstX;
  UINTN             SrcX;
  UINTN             Index;
  EFI_UGA_PIXEL     *Blt;
  UINT8             *Dst;
  UINT8             *Src;
  UINTN             Nbr;
  unsigned long     Color;

  //
  //  Check bounds
  //
  if (BltOperation == EfiUgaVideoToBltBuffer
      || BltOperation == EfiUgaVideoToVideo) {
    //
    // Source is Video.
    //
    if (Args->SourceY + Args->Height > Private->height) {
      return EFI_INVALID_PARAMETER;
    }

    if (Args->SourceX + Args->Width > Private->width) {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (BltOperation == EfiUgaBltBufferToVideo
      || BltOperation == EfiUgaVideoToVideo
      || BltOperation == EfiUgaVideoFill) {
    //
    // Destination is Video
    //
    if (Args->DestinationY + Args->Height > Private->height) {
      return EFI_INVALID_PARAMETER;
    }

    if (Args->DestinationX + Args->Width > Private->width) {
      return EFI_INVALID_PARAMETER;
    }
  }

  switch (BltOperation) {
  case EfiUgaVideoToBltBuffer:
    Blt = (EFI_UGA_PIXEL *)((UINT8 *)BltBuffer + (Args->DestinationY * Args->Delta) + Args->DestinationX * sizeof (EFI_UGA_PIXEL));
    Args->Delta -= Args->Width * sizeof (EFI_UGA_PIXEL);
    for (SrcY = Args->SourceY; SrcY < (Args->Height + Args->SourceY); SrcY++) {
      for (SrcX = Args->SourceX; SrcX < (Args->Width + Args->SourceX); SrcX++) {
        *Blt++ = UgaColorToPixel(Private,
                                  XGetPixel(Private->image, SrcX, SrcY));
      }
      Blt = (EFI_UGA_PIXEL *) ((UINT8 *) Blt + Args->Delta);
    }
    break;
  case EfiUgaBltBufferToVideo:
    Blt = (EFI_UGA_PIXEL *)((UINT8 *)BltBuffer + (Args->SourceY * Args->Delta) + Args->SourceX * sizeof (EFI_UGA_PIXEL));
    Args->Delta -= Args->Width * sizeof (EFI_UGA_PIXEL);
    for (DstY = Args->DestinationY; DstY < (Args->Height + Args->DestinationY); DstY++) {
      for (DstX = Args->DestinationX; DstX < (Args->Width + Args->DestinationX); DstX++) {
        XPutPixel(Private->image, DstX, DstY, UgaPixelToColor(Private, *Blt));
        Blt++;
      }
      Blt = (EFI_UGA_PIXEL *) ((UINT8 *) Blt + Args->Delta);
    }
    break;
  case EfiUgaVideoToVideo:
    Dst = Private->image_data + (Args->DestinationX << Private->pixel_shift)
      + Args->DestinationY * Private->line_bytes;
    Src = Private->image_data + (Args->SourceX << Private->pixel_shift)
      + Args->SourceY * Private->line_bytes;
    Nbr = Args->Width << Private->pixel_shift;
    if (Args->DestinationY < Args->SourceY) {
      for (Index = 0; Index < Args->Height; Index++) {
        memcpy (Dst, Src, Nbr);
        Dst += Private->line_bytes;
        Src += Private->line_bytes;
      }
    }
    else {
      Dst += (Args->Height - 1) * Private->line_bytes;
      Src += (Args->Height - 1) * Private->line_bytes;
      for (Index = 0; Index < Args->Height; Index++) {
      //
      // Source and Destination Y may be equal, therefore Dst and Src may
      // overlap.
      //
      memmove (Dst, Src, Nbr);
      Dst -= Private->line_bytes;
      Src -= Private->line_bytes;
      }
    }
    break;
  case EfiUgaVideoFill:
    Color = UgaPixelToColor(Private, *BltBuffer);
    for (DstY = Args->DestinationY; DstY < (Args->Height + Args->DestinationY); DstY++) {
      for (DstX = Args->DestinationX; DstX < (Args->Width + Args->DestinationX); DstX++) {
        XPutPixel(Private->image, DstX, DstY, Color);
      }
    }
    break;
  default:
      return EFI_INVALID_PARAMETER;
  }

  //
  //  Refresh screen.
  //
  switch (BltOperation) {
  case EfiUgaVideoToVideo:
    XCopyArea(Private->display, Private->win, Private->win, Private->gc,
               Args->SourceX, Args->SourceY, Args->Width, Args->Height, Args->DestinationX, Args->DestinationY);
    while (1) {
      XEvent ev;

      XNextEvent (Private->display, &ev);
      HandleEvent(Private, &ev);
      if (ev.type == NoExpose || ev.type == GraphicsExpose)
        break;
    }
    break;
  case EfiUgaVideoFill:
    Color = UgaPixelToColor(Private, *BltBuffer);
    XSetForeground(Private->display, Private->gc, Color);
    XFillRectangle(Private->display, Private->win, Private->gc,
                    Args->DestinationX, Args->DestinationY, Args->Width, Args->Height);
    XFlush(Private->display);
    break;
  case EfiUgaBltBufferToVideo:
    Redraw(Private, Args->DestinationX, Args->DestinationY, Args->Width, Args->Height);
    break;
  default:
    break;
  }
  return EFI_SUCCESS;
}

STATIC EFI_STATUS
CheckPointerInternal( UGA_IO_PRIVATE *drv, BOOLEAN delay )
{
  HandleEvents(drv);
  if (drv->pointer_state_changed != 0)
    return EFI_SUCCESS;
  if ( delay )
    /* EFI is polling.  Be CPU-friendly.  */
    msSleep (20);
  return EFI_NOT_READY;
}

EFI_STATUS
UgaCheckPointer(EFI_UNIX_UGA_IO_PROTOCOL *UgaIo)
{
  UGA_IO_PRIVATE  *drv = (UGA_IO_PRIVATE *)UgaIo;
  return( CheckPointerInternal( drv, TRUE ) );
}

EFI_STATUS
UgaGetPointerState (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, EFI_SIMPLE_POINTER_STATE *state)
{
  UGA_IO_PRIVATE *drv = (UGA_IO_PRIVATE *)UgaIo;
  EFI_STATUS status;

  status = CheckPointerInternal( drv, FALSE );
  if (status != EFI_SUCCESS)
    return status;
  
  memcpy( state, &drv->pointer_state, sizeof( EFI_SIMPLE_POINTER_STATE ) );

  drv->pointer_state.RelativeMovementX = 0;
  drv->pointer_state.RelativeMovementY = 0;
  drv->pointer_state.RelativeMovementZ = 0;
  drv->pointer_state_changed = 0;
  return EFI_SUCCESS;
}

EFI_STATUS
UgaCreate (EFI_UNIX_UGA_IO_PROTOCOL **Uga, CONST CHAR16 *Title)
{
  UGA_IO_PRIVATE *drv;
  unsigned int border_width = 0;
  char *display_name = NULL;
  int title_len;

  drv = (UGA_IO_PRIVATE *)calloc (1, sizeof (UGA_IO_PRIVATE));
  if (drv == NULL)
    return EFI_OUT_OF_RESOURCES;

#if defined(__APPLE__) || defined(MDE_CPU_X64)
//
//
//
  drv->UgaIo.UgaClose    = GasketUgaClose; 
  drv->UgaIo.UgaSize     = GasketUgaSize;
  drv->UgaIo.UgaCheckKey = GasketUgaCheckKey;
  drv->UgaIo.UgaGetKey   = GasketUgaGetKey;
  drv->UgaIo.UgaKeySetState         = GasketUgaKeySetState;
  drv->UgaIo.UgaRegisterKeyNotify   = GasketUgaRegisterKeyNotify;
  drv->UgaIo.UgaBlt      = GasketUgaBlt;
  drv->UgaIo.UgaCheckPointer        = GasketUgaCheckPointer;
  drv->UgaIo.UgaGetPointerState     = GasketUgaGetPointerState;
#else
  drv->UgaIo.UgaClose = UgaClose;
  drv->UgaIo.UgaSize = UgaSize;
  drv->UgaIo.UgaCheckKey = UgaCheckKey;
  drv->UgaIo.UgaGetKey = UgaGetKey;
  drv->UgaIo.UgaKeySetState         = UgaKeySetState;
  drv->UgaIo.UgaRegisterKeyNotify   = UgaRegisterKeyNotify;
  drv->UgaIo.UgaBlt = UgaBlt;
  drv->UgaIo.UgaCheckPointer        = UgaCheckPointer;
  drv->UgaIo.UgaGetPointerState     = UgaGetPointerState;
#endif
  
  

  drv->key_count = 0;
  drv->key_rd = 0;
  drv->key_wr = 0;
  drv->KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
  drv->KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;
  drv->RegisterdKeyCallback = NULL;
  drv->RegisterdKeyCallbackContext = NULL;
  
  
  drv->display = XOpenDisplay (display_name);
  if (drv->display == NULL)
    {
      fprintf (stderr, "uga: cannot connect to X server %s\n",
                XDisplayName (display_name));
      free (drv);
      return EFI_DEVICE_ERROR;
    }
  drv->screen = DefaultScreen (drv->display);
  drv->visual = DefaultVisual (drv->display, drv->screen);
  drv->win = XCreateSimpleWindow
               (drv->display, RootWindow (drv->display, drv->screen),
                0, 0, 4, 4, border_width,
                WhitePixel (drv->display, drv->screen),
                BlackPixel (drv->display, drv->screen));

  drv->depth = DefaultDepth (drv->display, drv->screen);
  XDefineCursor (drv->display, drv->win, XCreateFontCursor (drv->display, XC_pirate)); 

  /* Compute title len and convert to Ascii.  */
  for (title_len = 0; Title[title_len] != 0; title_len++)
    ;
  {
    char title[title_len + 1];
    int i;
    for (i = 0; i < title_len; i++)
      title[i] = Title[i];
    title[i] = 0;
    
    XStoreName (drv->display, drv->win, title);
  }

  XSelectInput (drv->display, drv->win,
                 ExposureMask | KeyPressMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask );
  drv->gc = DefaultGC (drv->display, drv->screen);

  *Uga = (EFI_UNIX_UGA_IO_PROTOCOL *)drv;
  return EFI_SUCCESS;
}
