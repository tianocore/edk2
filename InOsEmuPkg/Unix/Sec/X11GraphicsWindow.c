/*++ @file

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SecMain.h"

#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/XShm.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#define KEYSYM_LOWER  0
#define KEYSYM_UPPER  1

/* XQueryPointer  */

struct uga_drv_shift_mask {
  unsigned char shift;
  unsigned char size;
  unsigned char csize;
};

#define NBR_KEYS 32
typedef struct {
  EMU_GRAPHICS_WINDOW_PROTOCOL GraphicsIo;

  Display *display;
  int screen;      /* values for window_size in main */
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
  
  EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK    MakeRegisterdKeyCallback;
  EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK    BreakRegisterdKeyCallback;
  VOID                                                *RegisterdKeyCallbackContext;
  
  int                        previous_x;
  int                        previous_y;
  EFI_SIMPLE_POINTER_STATE   pointer_state;
  int                        pointer_state_changed;
} GRAPHICS_IO_PRIVATE;

void
HandleEvents(GRAPHICS_IO_PRIVATE *drv);

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
TryCreateShmImage (
  IN  GRAPHICS_IO_PRIVATE *drv
  )
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
X11Size(
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsIo, 
  IN  UINT32                        Width, 
  IN  UINT32                        Height
  )
{
  GRAPHICS_IO_PRIVATE *drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;
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
handleKeyEvent(GRAPHICS_IO_PRIVATE *drv, XEvent *ev, BOOLEAN Make)
{
  KeySym        *KeySym;
  EFI_KEY_DATA  KeyData;
  int           KeySymArraySize;
 
  if (Make) {
    if (drv->key_count == NBR_KEYS) {
      return;
    }
  }

  // keycode is a physical key on the keyboard
  // KeySym is a mapping of a physical key
  // KeyboardMapping is the array of KeySym for a given keycode. key, shifted key, option key, command key, ...
  //
  // Returns an array of KeySymArraySize of KeySym for the keycode. [0] is lower case, [1] is upper case,
  // [2] and [3] are based on option and command modifiers. The problem we have is command V
  // could be mapped to a crazy Unicode character so the old scheme of returning a string. 
  //
  KeySym = XGetKeyboardMapping (drv->display, ev->xkey.keycode, 1, &KeySymArraySize);
   
  KeyData.Key.ScanCode = 0;
  KeyData.Key.UnicodeChar = 0;
  KeyData.KeyState.KeyShiftState = 0;

  //
  // Skipping EFI_SCROLL_LOCK_ACTIVE & EFI_NUM_LOCK_ACTIVE since they are not on Macs  
  //
  if ((ev->xkey.state & LockMask) == 0) {
    drv->KeyState.KeyToggleState &= ~EFI_CAPS_LOCK_ACTIVE;
  } else {
    if (Make) {
      drv->KeyState.KeyToggleState |= EFI_CAPS_LOCK_ACTIVE;
    }
  }
  
  // Skipping EFI_MENU_KEY_PRESSED and EFI_SYS_REQ_PRESSED
  
  switch (*KeySym) {
  case XK_Control_R:
    if (Make) {
      drv->KeyState.KeyShiftState |=  EFI_RIGHT_CONTROL_PRESSED;
    } else {
      drv->KeyState.KeyShiftState &= ~EFI_RIGHT_CONTROL_PRESSED;
    }
   break;
  case XK_Control_L:
    if (Make) {    
      drv->KeyState.KeyShiftState |=  EFI_LEFT_CONTROL_PRESSED;
    } else {
      drv->KeyState.KeyShiftState &= ~EFI_LEFT_CONTROL_PRESSED;
    }
    break;

  case XK_Shift_R:
    if (Make) {
      drv->KeyState.KeyShiftState |=  EFI_RIGHT_SHIFT_PRESSED;
    } else {
      drv->KeyState.KeyShiftState &= ~EFI_RIGHT_SHIFT_PRESSED;
    }
    break;
  case XK_Shift_L:
    if (Make) {
      drv->KeyState.KeyShiftState |=  EFI_LEFT_SHIFT_PRESSED;
    } else {
      drv->KeyState.KeyShiftState &= ~EFI_LEFT_SHIFT_PRESSED;
    }
    break;
  
  case XK_Mode_switch:
    if (Make) {
      drv->KeyState.KeyShiftState |=  EFI_LEFT_ALT_PRESSED;
    } else {
      drv->KeyState.KeyShiftState &= ~EFI_LEFT_ALT_PRESSED;
    }
    break;

  case XK_Meta_R:
    if (Make) {
      drv->KeyState.KeyShiftState |=  EFI_RIGHT_LOGO_PRESSED;
    } else {
      drv->KeyState.KeyShiftState &= ~EFI_RIGHT_LOGO_PRESSED;
    }
    break;
  case XK_Meta_L:
    if (Make) {
      drv->KeyState.KeyShiftState |=  EFI_LEFT_LOGO_PRESSED;
    } else {
      drv->KeyState.KeyShiftState &= ~EFI_LEFT_LOGO_PRESSED;
    }
    break;
  
  case XK_KP_Home:
  case XK_Home:       KeyData.Key.ScanCode = SCAN_HOME;       break;
  
  case XK_KP_End:
  case XK_End:        KeyData.Key.ScanCode = SCAN_END;        break;
  
  case XK_KP_Left: 
  case XK_Left:       KeyData.Key.ScanCode = SCAN_LEFT;       break;
  
  case XK_KP_Right:
  case XK_Right:      KeyData.Key.ScanCode = SCAN_RIGHT;      break;
  
  case XK_KP_Up:
  case XK_Up:         KeyData.Key.ScanCode = SCAN_UP;         break;
  
  case XK_KP_Down:
  case XK_Down:       KeyData.Key.ScanCode = SCAN_DOWN;       break;
  
  case XK_KP_Delete:
  case XK_Delete:       KeyData.Key.ScanCode = SCAN_DELETE;     break;
  
  case XK_KP_Insert:  
  case XK_Insert:     KeyData.Key.ScanCode = SCAN_INSERT;     break;
  
  case XK_KP_Page_Up:
  case XK_Page_Up:    KeyData.Key.ScanCode = SCAN_PAGE_UP;    break;
  
  case XK_KP_Page_Down:
  case XK_Page_Down:  KeyData.Key.ScanCode = SCAN_PAGE_DOWN;  break;
  
  case XK_Escape:     KeyData.Key.ScanCode = SCAN_ESC;        break;

  case XK_Pause:      KeyData.Key.ScanCode = SCAN_PAUSE;      break;

  case XK_KP_F1:
  case XK_F1:   KeyData.Key.ScanCode = SCAN_F1;   break;
  
  case XK_KP_F2:
  case XK_F2:   KeyData.Key.ScanCode = SCAN_F2;   break;

  case XK_KP_F3:
  case XK_F3:   KeyData.Key.ScanCode = SCAN_F3;   break;

  case XK_KP_F4:
  case XK_F4:   KeyData.Key.ScanCode = SCAN_F4;   break;

  case XK_F5:   KeyData.Key.ScanCode = SCAN_F5;   break;
  case XK_F6:   KeyData.Key.ScanCode = SCAN_F6;   break;
  case XK_F7:   KeyData.Key.ScanCode = SCAN_F7;   break;
  
  // Don't map into X11 by default on a Mac
  // System Preferences->Keyboard->Keyboard Shortcuts can be configured 
  // to not use higher function keys as shortcuts and the will show up
  // in X11. 
  case XK_F8:   KeyData.Key.ScanCode = SCAN_F8;   break;
  case XK_F9:   KeyData.Key.ScanCode = SCAN_F9;   break;
  case XK_F10:  KeyData.Key.ScanCode = SCAN_F10;  break;
  
  case XK_F11:  KeyData.Key.ScanCode = SCAN_F11;  break;
  case XK_F12:  KeyData.Key.ScanCode = SCAN_F12;  break;
  
  case XK_F13:  KeyData.Key.ScanCode = SCAN_F13;  break;
  case XK_F14:  KeyData.Key.ScanCode = SCAN_F14;  break;
  case XK_F15:  KeyData.Key.ScanCode = SCAN_F15;  break;
  case XK_F16:  KeyData.Key.ScanCode = SCAN_F16;  break;
  case XK_F17:  KeyData.Key.ScanCode = SCAN_F17;  break;
  case XK_F18:  KeyData.Key.ScanCode = SCAN_F18;  break;
  case XK_F19:  KeyData.Key.ScanCode = SCAN_F19;  break;
  case XK_F20:  KeyData.Key.ScanCode = SCAN_F20;  break;
  case XK_F21:  KeyData.Key.ScanCode = SCAN_F21;  break;
  case XK_F22:  KeyData.Key.ScanCode = SCAN_F22;  break;
  case XK_F23:  KeyData.Key.ScanCode = SCAN_F23;  break;
  case XK_F24:  KeyData.Key.ScanCode = SCAN_F24;  break;

  // No mapping in X11
  //case XK_:   KeyData.Key.ScanCode = SCAN_MUTE;            break;             
  //case XK_:   KeyData.Key.ScanCode = SCAN_VOLUME_UP;       break;            
  //case XK_:   KeyData.Key.ScanCode = SCAN_VOLUME_DOWN;     break;       
  //case XK_:   KeyData.Key.ScanCode = SCAN_BRIGHTNESS_UP;   break;      
  //case XK_:   KeyData.Key.ScanCode = SCAN_BRIGHTNESS_DOWN; break;    
  //case XK_:   KeyData.Key.ScanCode = SCAN_SUSPEND;         break; 
  //case XK_:   KeyData.Key.ScanCode = SCAN_HIBERNATE;       break; 
  //case XK_:   KeyData.Key.ScanCode = SCAN_TOGGLE_DISPLAY;  break;       
  //case XK_:   KeyData.Key.ScanCode = SCAN_RECOVERY;        break;      
  //case XK_:   KeyData.Key.ScanCode = SCAN_EJECT;           break;     

  case XK_BackSpace:  KeyData.Key.UnicodeChar = 0x0008; break;

  case XK_KP_Tab:
  case XK_Tab:        KeyData.Key.UnicodeChar = 0x0009; break;

  case XK_Linefeed:   KeyData.Key.UnicodeChar = 0x000a; break;
  
  case XK_KP_Enter:
  case XK_Return:     KeyData.Key.UnicodeChar = 0x000d; break;

  case XK_KP_Equal      : KeyData.Key.UnicodeChar = L'='; break;                     
  case XK_KP_Multiply   : KeyData.Key.UnicodeChar = L'*'; break;                  
  case XK_KP_Add        : KeyData.Key.UnicodeChar = L'+'; break;                       
  case XK_KP_Separator  : KeyData.Key.UnicodeChar = L'~'; break;                  
  case XK_KP_Subtract   : KeyData.Key.UnicodeChar = L'-'; break;                  
  case XK_KP_Decimal    : KeyData.Key.UnicodeChar = L'.'; break;                   
  case XK_KP_Divide     : KeyData.Key.UnicodeChar = L'/'; break;                    

  case XK_KP_0    : KeyData.Key.UnicodeChar = L'0'; break;                         
  case XK_KP_1    : KeyData.Key.UnicodeChar = L'1'; break;                         
  case XK_KP_2    : KeyData.Key.UnicodeChar = L'2'; break;                         
  case XK_KP_3    : KeyData.Key.UnicodeChar = L'3'; break;                         
  case XK_KP_4    : KeyData.Key.UnicodeChar = L'4'; break;                        
  case XK_KP_5    : KeyData.Key.UnicodeChar = L'5'; break;                         
  case XK_KP_6    : KeyData.Key.UnicodeChar = L'6'; break;                         
  case XK_KP_7    : KeyData.Key.UnicodeChar = L'7'; break;                         
  case XK_KP_8    : KeyData.Key.UnicodeChar = L'8'; break;                        
  case XK_KP_9    : KeyData.Key.UnicodeChar = L'9'; break;                         

  default:
    ;
  }

  // The global state is our state
  KeyData.KeyState.KeyShiftState = drv->KeyState.KeyShiftState;
  KeyData.KeyState.KeyToggleState = drv->KeyState.KeyToggleState;

  if (*KeySym < XK_BackSpace) {
    if (((drv->KeyState.KeyShiftState & (EFI_LEFT_SHIFT_PRESSED | EFI_RIGHT_SHIFT_PRESSED)) != 0) ||
        ((drv->KeyState.KeyToggleState & EFI_CAPS_LOCK_ACTIVE) != 0) ) {
      
      KeyData.Key.UnicodeChar = (CHAR16)KeySym[KEYSYM_UPPER];

      // Per UEFI spec since we converted the Unicode clear the shift bits we pass up 
      KeyData.KeyState.KeyShiftState &= ~(EFI_LEFT_SHIFT_PRESSED | EFI_RIGHT_SHIFT_PRESSED);
    } else {
      KeyData.Key.UnicodeChar = (CHAR16)KeySym[KEYSYM_LOWER];
    }
  } else {
    // XK_BackSpace is the start of XK_MISCELLANY. These are the XK_? keys we process in this file 
    ; 
  }
  
  if (Make) {
    memcpy (&drv->keys[drv->key_wr], &KeyData, sizeof (EFI_KEY_DATA));
    drv->key_wr = (drv->key_wr + 1) % NBR_KEYS;
    drv->key_count++; 
    if (drv->MakeRegisterdKeyCallback != NULL) {
      ReverseGasketUint64Uint64 (drv->MakeRegisterdKeyCallback ,drv->RegisterdKeyCallbackContext, &KeyData);
    }
  } else {
    if (drv->BreakRegisterdKeyCallback != NULL) {
      ReverseGasketUint64Uint64 (drv->BreakRegisterdKeyCallback ,drv->RegisterdKeyCallbackContext, &KeyData);
    }
  }
}


void
handleMouseMoved(GRAPHICS_IO_PRIVATE *drv, XEvent *ev)
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
handleMouseDown(GRAPHICS_IO_PRIVATE *drv, XEvent *ev, BOOLEAN Pressed)
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
Redraw(GRAPHICS_IO_PRIVATE *drv, UINTN X, UINTN Y, UINTN Width, UINTN Height)
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
HandleEvent(GRAPHICS_IO_PRIVATE *drv, XEvent *ev)
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
      handleKeyEvent(drv, ev, TRUE);
      break;
    case KeyRelease:
      handleKeyEvent(drv, ev, FALSE);
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
HandleEvents(GRAPHICS_IO_PRIVATE *drv)
{
  while (XPending(drv->display) != 0)
    {
      XEvent ev;

      XNextEvent (drv->display, &ev);
      HandleEvent(drv, &ev);
    }
}

unsigned long
X11PixelToColor (GRAPHICS_IO_PRIVATE *drv, EFI_UGA_PIXEL pixel)
{
  return ((pixel.Red >> drv->r.csize) << drv->r.shift)
    | ((pixel.Green >> drv->g.csize) << drv->g.shift)
    | ((pixel.Blue >> drv->b.csize) << drv->b.shift);
}

EFI_UGA_PIXEL
X11ColorToPixel (GRAPHICS_IO_PRIVATE *drv, unsigned long val)
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
CheckKeyInternal( GRAPHICS_IO_PRIVATE *drv, BOOLEAN delay )
{
  HandleEvents(drv);
  if (drv->key_count != 0)
    return EFI_SUCCESS;
  if ( delay )
    /* EFI is polling.  Be CPU-friendly.  */
    SecSleep (20);
    return EFI_NOT_READY;
  }

EFI_STATUS
X11CheckKey(EMU_GRAPHICS_WINDOW_PROTOCOL *GraphicsIo)
{
  GRAPHICS_IO_PRIVATE  *drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;
  return CheckKeyInternal(drv, TRUE);
}

EFI_STATUS
EFIAPI
X11GetKey (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsIo, 
  IN  EFI_KEY_DATA                   *KeyData
  )
{
  GRAPHICS_IO_PRIVATE *drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;
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
X11KeySetState (
  IN EMU_GRAPHICS_WINDOW_PROTOCOL   *GraphicsIo, 
  IN EFI_KEY_TOGGLE_STATE       *KeyToggleState
  )
{
  GRAPHICS_IO_PRIVATE  *drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;
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
X11RegisterKeyNotify (
  IN EMU_GRAPHICS_WINDOW_PROTOCOL                       *GraphicsIo, 
  IN EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK    MakeCallBack,
  IN EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK    BreakCallBack,
  IN VOID                                                *Context
  )
{
  GRAPHICS_IO_PRIVATE  *drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;

  drv->MakeRegisterdKeyCallback         = MakeCallBack;
  drv->BreakRegisterdKeyCallback        = BreakCallBack;
  drv->RegisterdKeyCallbackContext = Context;

  return EFI_SUCCESS;
}


EFI_STATUS
X11Blt (
  IN EMU_GRAPHICS_WINDOW_PROTOCOL             *GraphicsIo,
  IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
  IN  EFI_UGA_BLT_OPERATION                   BltOperation,
  IN  EMU_GRAPHICS_WINDOWS__BLT_ARGS          *Args
  )
{
  GRAPHICS_IO_PRIVATE *Private = (GRAPHICS_IO_PRIVATE *)GraphicsIo;
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
        *Blt++ = X11ColorToPixel(Private,
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
        XPutPixel(Private->image, DstX, DstY, X11PixelToColor(Private, *Blt));
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
    Color = X11PixelToColor(Private, *BltBuffer);
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
    Color = X11PixelToColor(Private, *BltBuffer);
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
CheckPointerInternal( GRAPHICS_IO_PRIVATE *drv, BOOLEAN delay )
{
  HandleEvents(drv);
  if (drv->pointer_state_changed != 0)
    return EFI_SUCCESS;
  if ( delay )
    /* EFI is polling.  Be CPU-friendly.  */
    SecSleep (20);
  return EFI_NOT_READY;
}

EFI_STATUS
X11CheckPointer(EMU_GRAPHICS_WINDOW_PROTOCOL *GraphicsIo)
{
  GRAPHICS_IO_PRIVATE  *drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;
  return( CheckPointerInternal( drv, TRUE ) );
}

EFI_STATUS
X11GetPointerState (EMU_GRAPHICS_WINDOW_PROTOCOL *GraphicsIo, EFI_SIMPLE_POINTER_STATE *state)
{
  GRAPHICS_IO_PRIVATE *drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;
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
X11GraphicsWindowOpen (
  IN  EMU_IO_THUNK_PROTOCOL   *This
  )
{
  GRAPHICS_IO_PRIVATE *drv;
  unsigned int border_width = 0;
  char *display_name = NULL;
  int title_len;

  drv = (GRAPHICS_IO_PRIVATE *)calloc (1, sizeof (GRAPHICS_IO_PRIVATE));
  if (drv == NULL)
    return EFI_OUT_OF_RESOURCES;

  drv->GraphicsIo.Size                = GasketX11Size;
  drv->GraphicsIo.CheckKey            = GasketX11CheckKey;
  drv->GraphicsIo.GetKey              = GasketX11GetKey;
  drv->GraphicsIo.KeySetState         = GasketX11KeySetState;
  drv->GraphicsIo.RegisterKeyNotify   = GasketX11RegisterKeyNotify;
  drv->GraphicsIo.Blt                 = GasketX11Blt;
  drv->GraphicsIo.CheckPointer        = GasketX11CheckPointer;
  drv->GraphicsIo.GetPointerState     = GasketX11GetPointerState;
  

  drv->key_count = 0;
  drv->key_rd = 0;
  drv->key_wr = 0;
  drv->KeyState.KeyShiftState      = EFI_SHIFT_STATE_VALID;
  drv->KeyState.KeyToggleState     = EFI_TOGGLE_STATE_VALID;
  drv->MakeRegisterdKeyCallback    = NULL;
  drv->BreakRegisterdKeyCallback   = NULL;
  drv->RegisterdKeyCallbackContext = NULL;
  
  
  drv->display = XOpenDisplay (display_name);
  if (drv->display == NULL) {
    fprintf (stderr, "uga: cannot connect to X server %s\n", XDisplayName (display_name));
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
  for (title_len = 0; This->ConfigString[title_len] != 0; title_len++)
    ;
  {
    char title[title_len + 1];
    int i;
    for (i = 0; i < title_len; i++)
      title[i] = This->ConfigString[i];
    title[i] = 0;
    
    XStoreName (drv->display, drv->win, title);
  }

//  XAutoRepeatOff (drv->display);
  XSelectInput (drv->display, drv->win,
                 ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask );
  drv->gc = DefaultGC (drv->display, drv->screen);

  This->Private   = (VOID *)drv;
  This->Interface = (VOID *)drv;
  return EFI_SUCCESS;
}


EFI_STATUS
X11GraphicsWindowClose (
  IN  EMU_IO_THUNK_PROTOCOL   *This
  )
{
  GRAPHICS_IO_PRIVATE *drv = (GRAPHICS_IO_PRIVATE *)This->Private;

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


EMU_IO_THUNK_PROTOCOL gX11ThunkIo = {
  &gEmuGraphicsWindowProtocolGuid,
  NULL,
  NULL,
  0,
  GasketX11GraphicsWindowOpen,
  GasketX11GraphicsWindowClose,
  NULL
};


