/*++ @file

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Host.h"

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

struct uga_drv_shift_mask {
  unsigned char    shift;
  unsigned char    size;
  unsigned char    csize;
};

#define NBR_KEYS  32
typedef struct {
  EMU_GRAPHICS_WINDOW_PROTOCOL                        GraphicsIo;

  Display                                             *display;
  int                                                 screen; // values for window_size in main
  Window                                              win;
  GC                                                  gc;
  Visual                                              *visual;

  int                                                 depth;
  unsigned int                                        width;
  unsigned int                                        height;
  unsigned int                                        line_bytes;
  unsigned int                                        pixel_shift;
  unsigned char                                       *image_data;

  struct uga_drv_shift_mask                           r, g, b;

  int                                                 use_shm;
  XShmSegmentInfo                                     xshm_info;
  XImage                                              *image;
  char                                                *Title;

  unsigned int                                        key_rd;
  unsigned int                                        key_wr;
  unsigned int                                        key_count;
  EFI_KEY_DATA                                        keys[NBR_KEYS];

  EFI_KEY_STATE                                       KeyState;

  EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK    MakeRegisterdKeyCallback;
  EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK    BreakRegisterdKeyCallback;
  VOID                                                *RegisterdKeyCallbackContext;

  int                                                 previous_x;
  int                                                 previous_y;
  EFI_SIMPLE_POINTER_STATE                            pointer_state;
  int                                                 pointer_state_changed;
} GRAPHICS_IO_PRIVATE;

void
HandleEvents (
  IN GRAPHICS_IO_PRIVATE  *Drv
  );

void
fill_shift_mask (
  IN  struct uga_drv_shift_mask  *sm,
  IN  unsigned long              mask
  )
{
  sm->shift = 0;
  sm->size  = 0;
  while ((mask & 1) == 0) {
    mask >>= 1;
    sm->shift++;
  }

  while (mask & 1) {
    sm->size++;
    mask >>= 1;
  }

  sm->csize = 8 - sm->size;
}

int
TryCreateShmImage (
  IN  GRAPHICS_IO_PRIVATE  *Drv
  )
{
  Drv->image = XShmCreateImage (
                 Drv->display,
                 Drv->visual,
                 Drv->depth,
                 ZPixmap,
                 NULL,
                 &Drv->xshm_info,
                 Drv->width,
                 Drv->height
                 );
  if (Drv->image == NULL) {
    return 0;
  }

  switch (Drv->image->bitmap_unit) {
    case 32:
      Drv->pixel_shift = 2;
      break;
    case 16:
      Drv->pixel_shift = 1;
      break;
    case 8:
      Drv->pixel_shift = 0;
      break;
  }

  Drv->xshm_info.shmid = shmget (
                           IPC_PRIVATE,
                           Drv->image->bytes_per_line * Drv->image->height,
                           IPC_CREAT | 0777
                           );
  if (Drv->xshm_info.shmid < 0) {
    XDestroyImage (Drv->image);
    return 0;
  }

  Drv->image_data = shmat (Drv->xshm_info.shmid, NULL, 0);
  if (!Drv->image_data) {
    shmctl (Drv->xshm_info.shmid, IPC_RMID, NULL);
    XDestroyImage (Drv->image);
    return 0;
  }

 #ifndef __APPLE__
  //
  // This closes shared memory in real time on OS X. Only closes after folks quit using
  // it on Linux.
  //
  shmctl (Drv->xshm_info.shmid, IPC_RMID, NULL);
 #endif

  Drv->xshm_info.shmaddr = (char *)Drv->image_data;
  Drv->image->data       = (char *)Drv->image_data;

  if (!XShmAttach (Drv->display, &Drv->xshm_info)) {
    shmdt (Drv->image_data);
    XDestroyImage (Drv->image);
    return 0;
  }

  return 1;
}

EFI_STATUS
X11Size (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsIo,
  IN  UINT32                        Width,
  IN  UINT32                        Height
  )
{
  GRAPHICS_IO_PRIVATE  *Drv;
  XSizeHints           size_hints;

  // Destroy current buffer if created.
  Drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;
  if (Drv->image != NULL) {
    // Before destroy buffer, need to make sure the buffer available for access.
    XDestroyImage (Drv->image);

    if (Drv->use_shm) {
      shmdt (Drv->image_data);
    }

    Drv->image_data = NULL;
    Drv->image      = NULL;
  }

  Drv->width  = Width;
  Drv->height = Height;
  XResizeWindow (Drv->display, Drv->win, Width, Height);

  // Allocate image.
  if (XShmQueryExtension (Drv->display) && TryCreateShmImage (Drv)) {
    Drv->use_shm = 1;
  } else {
    Drv->use_shm = 0;
    if (Drv->depth > 16) {
      Drv->pixel_shift = 2;
    } else if (Drv->depth > 8) {
      Drv->pixel_shift = 1;
    } else {
      Drv->pixel_shift = 0;
    }

    Drv->image_data = malloc ((Drv->width * Drv->height) << Drv->pixel_shift);
    Drv->image      = XCreateImage (
                        Drv->display,
                        Drv->visual,
                        Drv->depth,
                        ZPixmap,
                        0,
                        (char *)Drv->image_data,
                        Drv->width,
                        Drv->height,
                        8 << Drv->pixel_shift,
                        0
                        );
  }

  Drv->line_bytes = Drv->image->bytes_per_line;

  fill_shift_mask (&Drv->r, Drv->image->red_mask);
  fill_shift_mask (&Drv->g, Drv->image->green_mask);
  fill_shift_mask (&Drv->b, Drv->image->blue_mask);

  // Set WM hints.
  size_hints.flags      = PSize | PMinSize | PMaxSize;
  size_hints.min_width  = size_hints.max_width = size_hints.base_width = Width;
  size_hints.min_height = size_hints.max_height = size_hints.base_height = Height;
  XSetWMNormalHints (Drv->display, Drv->win, &size_hints);

  XMapWindow (Drv->display, Drv->win);
  HandleEvents (Drv);
  return EFI_SUCCESS;
}

void
handleKeyEvent (
  IN  GRAPHICS_IO_PRIVATE  *Drv,
  IN  XEvent               *ev,
  IN  BOOLEAN              Make
  )
{
  KeySym        *KeySym;
  EFI_KEY_DATA  KeyData;
  int           KeySymArraySize;

  if (Make) {
    if (Drv->key_count == NBR_KEYS) {
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
  KeySym = XGetKeyboardMapping (Drv->display, ev->xkey.keycode, 1, &KeySymArraySize);

  KeyData.Key.ScanCode           = 0;
  KeyData.Key.UnicodeChar        = 0;
  KeyData.KeyState.KeyShiftState = 0;

  //
  // Skipping EFI_SCROLL_LOCK_ACTIVE & EFI_NUM_LOCK_ACTIVE since they are not on Macs
  //
  if ((ev->xkey.state & LockMask) == 0) {
    Drv->KeyState.KeyToggleState &= ~EFI_CAPS_LOCK_ACTIVE;
  } else {
    if (Make) {
      Drv->KeyState.KeyToggleState |= EFI_CAPS_LOCK_ACTIVE;
    }
  }

  // Skipping EFI_MENU_KEY_PRESSED and EFI_SYS_REQ_PRESSED

  switch (*KeySym) {
    case XK_Control_R:
      if (Make) {
        Drv->KeyState.KeyShiftState |=  EFI_RIGHT_CONTROL_PRESSED;
      } else {
        Drv->KeyState.KeyShiftState &= ~EFI_RIGHT_CONTROL_PRESSED;
      }

      break;
    case XK_Control_L:
      if (Make) {
        Drv->KeyState.KeyShiftState |=  EFI_LEFT_CONTROL_PRESSED;
      } else {
        Drv->KeyState.KeyShiftState &= ~EFI_LEFT_CONTROL_PRESSED;
      }

      break;

    case XK_Shift_R:
      if (Make) {
        Drv->KeyState.KeyShiftState |=  EFI_RIGHT_SHIFT_PRESSED;
      } else {
        Drv->KeyState.KeyShiftState &= ~EFI_RIGHT_SHIFT_PRESSED;
      }

      break;
    case XK_Shift_L:
      if (Make) {
        Drv->KeyState.KeyShiftState |=  EFI_LEFT_SHIFT_PRESSED;
      } else {
        Drv->KeyState.KeyShiftState &= ~EFI_LEFT_SHIFT_PRESSED;
      }

      break;

    case XK_Mode_switch:
      if (Make) {
        Drv->KeyState.KeyShiftState |=  EFI_LEFT_ALT_PRESSED;
      } else {
        Drv->KeyState.KeyShiftState &= ~EFI_LEFT_ALT_PRESSED;
      }

      break;

    case XK_Meta_R:
      if (Make) {
        Drv->KeyState.KeyShiftState |=  EFI_RIGHT_LOGO_PRESSED;
      } else {
        Drv->KeyState.KeyShiftState &= ~EFI_RIGHT_LOGO_PRESSED;
      }

      break;
    case XK_Meta_L:
      if (Make) {
        Drv->KeyState.KeyShiftState |=  EFI_LEFT_LOGO_PRESSED;
      } else {
        Drv->KeyState.KeyShiftState &= ~EFI_LEFT_LOGO_PRESSED;
      }

      break;

    case XK_KP_Home:
    case XK_Home:       KeyData.Key.ScanCode = SCAN_HOME;
      break;

    case XK_KP_End:
    case XK_End:        KeyData.Key.ScanCode = SCAN_END;
      break;

    case XK_KP_Left:
    case XK_Left:       KeyData.Key.ScanCode = SCAN_LEFT;
      break;

    case XK_KP_Right:
    case XK_Right:      KeyData.Key.ScanCode = SCAN_RIGHT;
      break;

    case XK_KP_Up:
    case XK_Up:         KeyData.Key.ScanCode = SCAN_UP;
      break;

    case XK_KP_Down:
    case XK_Down:       KeyData.Key.ScanCode = SCAN_DOWN;
      break;

    case XK_KP_Delete:
    case XK_Delete:       KeyData.Key.ScanCode = SCAN_DELETE;
      break;

    case XK_KP_Insert:
    case XK_Insert:     KeyData.Key.ScanCode = SCAN_INSERT;
      break;

    case XK_KP_Page_Up:
    case XK_Page_Up:    KeyData.Key.ScanCode = SCAN_PAGE_UP;
      break;

    case XK_KP_Page_Down:
    case XK_Page_Down:  KeyData.Key.ScanCode = SCAN_PAGE_DOWN;
      break;

    case XK_Escape:     KeyData.Key.ScanCode = SCAN_ESC;
      break;

    case XK_Pause:      KeyData.Key.ScanCode = SCAN_PAUSE;
      break;

    case XK_KP_F1:
    case XK_F1:   KeyData.Key.ScanCode = SCAN_F1;
      break;

    case XK_KP_F2:
    case XK_F2:   KeyData.Key.ScanCode = SCAN_F2;
      break;

    case XK_KP_F3:
    case XK_F3:   KeyData.Key.ScanCode = SCAN_F3;
      break;

    case XK_KP_F4:
    case XK_F4:   KeyData.Key.ScanCode = SCAN_F4;
      break;

    case XK_F5:   KeyData.Key.ScanCode = SCAN_F5;
      break;
    case XK_F6:   KeyData.Key.ScanCode = SCAN_F6;
      break;
    case XK_F7:   KeyData.Key.ScanCode = SCAN_F7;
      break;

    // Don't map into X11 by default on a Mac
    // System Preferences->Keyboard->Keyboard Shortcuts can be configured
    // to not use higher function keys as shortcuts and the will show up
    // in X11.
    case XK_F8:   KeyData.Key.ScanCode = SCAN_F8;
      break;
    case XK_F9:   KeyData.Key.ScanCode = SCAN_F9;
      break;
    case XK_F10:  KeyData.Key.ScanCode = SCAN_F10;
      break;

    case XK_F11:  KeyData.Key.ScanCode = SCAN_F11;
      break;
    case XK_F12:  KeyData.Key.ScanCode = SCAN_F12;
      break;

    case XK_F13:  KeyData.Key.ScanCode = SCAN_F13;
      break;
    case XK_F14:  KeyData.Key.ScanCode = SCAN_F14;
      break;
    case XK_F15:  KeyData.Key.ScanCode = SCAN_F15;
      break;
    case XK_F16:  KeyData.Key.ScanCode = SCAN_F16;
      break;
    case XK_F17:  KeyData.Key.ScanCode = SCAN_F17;
      break;
    case XK_F18:  KeyData.Key.ScanCode = SCAN_F18;
      break;
    case XK_F19:  KeyData.Key.ScanCode = SCAN_F19;
      break;
    case XK_F20:  KeyData.Key.ScanCode = SCAN_F20;
      break;
    case XK_F21:  KeyData.Key.ScanCode = SCAN_F21;
      break;
    case XK_F22:  KeyData.Key.ScanCode = SCAN_F22;
      break;
    case XK_F23:  KeyData.Key.ScanCode = SCAN_F23;
      break;
    case XK_F24:  KeyData.Key.ScanCode = SCAN_F24;
      break;

    // No mapping in X11
    // case XK_:   KeyData.Key.ScanCode = SCAN_MUTE;            break;
    // case XK_:   KeyData.Key.ScanCode = SCAN_VOLUME_UP;       break;
    // case XK_:   KeyData.Key.ScanCode = SCAN_VOLUME_DOWN;     break;
    // case XK_:   KeyData.Key.ScanCode = SCAN_BRIGHTNESS_UP;   break;
    // case XK_:   KeyData.Key.ScanCode = SCAN_BRIGHTNESS_DOWN; break;
    // case XK_:   KeyData.Key.ScanCode = SCAN_SUSPEND;         break;
    // case XK_:   KeyData.Key.ScanCode = SCAN_HIBERNATE;       break;
    // case XK_:   KeyData.Key.ScanCode = SCAN_TOGGLE_DISPLAY;  break;
    // case XK_:   KeyData.Key.ScanCode = SCAN_RECOVERY;        break;
    // case XK_:   KeyData.Key.ScanCode = SCAN_EJECT;           break;

    case XK_BackSpace:  KeyData.Key.UnicodeChar = 0x0008;
      break;

    case XK_KP_Tab:
    case XK_Tab:        KeyData.Key.UnicodeChar = 0x0009;
      break;

    case XK_Linefeed:   KeyData.Key.UnicodeChar = 0x000a;
      break;

    case XK_KP_Enter:
    case XK_Return:     KeyData.Key.UnicodeChar = 0x000d;
      break;

    case XK_KP_Equal: KeyData.Key.UnicodeChar = L'=';
      break;
    case XK_KP_Multiply: KeyData.Key.UnicodeChar = L'*';
      break;
    case XK_KP_Add: KeyData.Key.UnicodeChar = L'+';
      break;
    case XK_KP_Separator: KeyData.Key.UnicodeChar = L'~';
      break;
    case XK_KP_Subtract: KeyData.Key.UnicodeChar = L'-';
      break;
    case XK_KP_Decimal: KeyData.Key.UnicodeChar = L'.';
      break;
    case XK_KP_Divide: KeyData.Key.UnicodeChar = L'/';
      break;

    case XK_KP_0: KeyData.Key.UnicodeChar = L'0';
      break;
    case XK_KP_1: KeyData.Key.UnicodeChar = L'1';
      break;
    case XK_KP_2: KeyData.Key.UnicodeChar = L'2';
      break;
    case XK_KP_3: KeyData.Key.UnicodeChar = L'3';
      break;
    case XK_KP_4: KeyData.Key.UnicodeChar = L'4';
      break;
    case XK_KP_5: KeyData.Key.UnicodeChar = L'5';
      break;
    case XK_KP_6: KeyData.Key.UnicodeChar = L'6';
      break;
    case XK_KP_7: KeyData.Key.UnicodeChar = L'7';
      break;
    case XK_KP_8: KeyData.Key.UnicodeChar = L'8';
      break;
    case XK_KP_9: KeyData.Key.UnicodeChar = L'9';
      break;

    default:
      ;
  }

  // The global state is our state
  KeyData.KeyState.KeyShiftState  = Drv->KeyState.KeyShiftState;
  KeyData.KeyState.KeyToggleState = Drv->KeyState.KeyToggleState;

  if (*KeySym < XK_BackSpace) {
    if (((Drv->KeyState.KeyShiftState & (EFI_LEFT_SHIFT_PRESSED | EFI_RIGHT_SHIFT_PRESSED)) != 0) ||
        ((Drv->KeyState.KeyToggleState & EFI_CAPS_LOCK_ACTIVE) != 0))
    {
      KeyData.Key.UnicodeChar = (CHAR16)KeySym[KEYSYM_UPPER];

      // Per UEFI spec since we converted the Unicode clear the shift bits we pass up
      KeyData.KeyState.KeyShiftState &= ~(EFI_LEFT_SHIFT_PRESSED | EFI_RIGHT_SHIFT_PRESSED);
    } else {
      KeyData.Key.UnicodeChar = (CHAR16)KeySym[KEYSYM_LOWER];
    }
  } else {
    // XK_BackSpace is the start of XK_MISCELLANY. These are the XK_? keys we process in this file
  }

  if (Make) {
    memcpy (&Drv->keys[Drv->key_wr], &KeyData, sizeof (EFI_KEY_DATA));
    Drv->key_wr = (Drv->key_wr + 1) % NBR_KEYS;
    Drv->key_count++;
    if (Drv->MakeRegisterdKeyCallback != NULL) {
      ReverseGasketUint64Uint64 (Drv->MakeRegisterdKeyCallback, Drv->RegisterdKeyCallbackContext, &KeyData);
    }
  } else {
    if (Drv->BreakRegisterdKeyCallback != NULL) {
      ReverseGasketUint64Uint64 (Drv->BreakRegisterdKeyCallback, Drv->RegisterdKeyCallbackContext, &KeyData);
    }
  }
}

void
handleMouseMoved (
  IN  GRAPHICS_IO_PRIVATE  *Drv,
  IN  XEvent               *ev
  )
{
  if (ev->xmotion.x != Drv->previous_x) {
    Drv->pointer_state.RelativeMovementX += (ev->xmotion.x - Drv->previous_x);
    Drv->previous_x                       = ev->xmotion.x;
    Drv->pointer_state_changed            = 1;
  }

  if (ev->xmotion.y != Drv->previous_y) {
    Drv->pointer_state.RelativeMovementY += (ev->xmotion.y - Drv->previous_y);
    Drv->previous_y                       = ev->xmotion.y;
    Drv->pointer_state_changed            = 1;
  }

  Drv->pointer_state.RelativeMovementZ = 0;
}

void
handleMouseDown (
  IN  GRAPHICS_IO_PRIVATE  *Drv,
  IN  XEvent               *ev,
  IN  BOOLEAN              Pressed
  )
{
  if (ev->xbutton.button == Button1) {
    Drv->pointer_state_changed    = (Drv->pointer_state.LeftButton != Pressed);
    Drv->pointer_state.LeftButton = Pressed;
  }

  if ( ev->xbutton.button == Button2 ) {
    Drv->pointer_state_changed     = (Drv->pointer_state.RightButton != Pressed);
    Drv->pointer_state.RightButton = Pressed;
  }
}

void
Redraw (
  IN  GRAPHICS_IO_PRIVATE  *Drv,
  IN  UINTN                X,
  IN  UINTN                Y,
  IN  UINTN                Width,
  IN  UINTN                Height
  )
{
  if (Drv->use_shm) {
    XShmPutImage (
      Drv->display,
      Drv->win,
      Drv->gc,
      Drv->image,
      X,
      Y,
      X,
      Y,
      Width,
      Height,
      False
      );
  } else {
    XPutImage (
      Drv->display,
      Drv->win,
      Drv->gc,
      Drv->image,
      X,
      Y,
      X,
      Y,
      Width,
      Height
      );
  }

  XFlush (Drv->display);
}

void
HandleEvent (
  GRAPHICS_IO_PRIVATE  *Drv,
  XEvent               *ev
  )
{
  switch (ev->type) {
    case Expose:
      Redraw (
        Drv,
        ev->xexpose.x,
        ev->xexpose.y,
        ev->xexpose.width,
        ev->xexpose.height
        );
      break;
    case GraphicsExpose:
      Redraw (
        Drv,
        ev->xgraphicsexpose.x,
        ev->xgraphicsexpose.y,
        ev->xgraphicsexpose.width,
        ev->xgraphicsexpose.height
        );
      break;
    case KeyPress:
      handleKeyEvent (Drv, ev, TRUE);
      break;
    case KeyRelease:
      handleKeyEvent (Drv, ev, FALSE);
      break;
    case MappingNotify:
      XRefreshKeyboardMapping (&ev->xmapping);
      break;
    case MotionNotify:
      handleMouseMoved (Drv, ev);
      break;
    case ButtonPress:
      handleMouseDown (Drv, ev, TRUE);
      break;
    case ButtonRelease:
      handleMouseDown (Drv, ev, FALSE);
      break;
 #if 0
    case DestroyNotify:
      XCloseDisplay (Drv->display);
      exit (1);
      break;
 #endif
    case NoExpose:
    default:
      break;
  }
}

void
HandleEvents (
  IN  GRAPHICS_IO_PRIVATE  *Drv
  )
{
  XEvent  ev;

  while (XPending (Drv->display) != 0) {
    XNextEvent (Drv->display, &ev);
    HandleEvent (Drv, &ev);
  }
}

unsigned long
X11PixelToColor (
  IN  GRAPHICS_IO_PRIVATE  *Drv,
  IN  EFI_UGA_PIXEL        pixel
  )
{
  return ((pixel.Red   >> Drv->r.csize) << Drv->r.shift)
         | ((pixel.Green >> Drv->g.csize) << Drv->g.shift)
         | ((pixel.Blue  >> Drv->b.csize) << Drv->b.shift);
}

EFI_UGA_PIXEL
X11ColorToPixel (
  IN  GRAPHICS_IO_PRIVATE  *Drv,
  IN  unsigned long        val
  )
{
  EFI_UGA_PIXEL  Pixel;

  memset (&Pixel, 0, sizeof (EFI_UGA_PIXEL));

  // Truncation not an issue since X11 and EFI are both using 8 bits per color
  Pixel.Red   =   (val >> Drv->r.shift) << Drv->r.csize;
  Pixel.Green = (val >> Drv->g.shift) << Drv->g.csize;
  Pixel.Blue  =  (val >> Drv->b.shift) << Drv->b.csize;

  return Pixel;
}

EFI_STATUS
X11CheckKey (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsIo
  )
{
  GRAPHICS_IO_PRIVATE  *Drv;

  Drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;

  HandleEvents (Drv);

  if (Drv->key_count != 0) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_READY;
}

EFI_STATUS
X11GetKey (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsIo,
  IN  EFI_KEY_DATA                  *KeyData
  )
{
  EFI_STATUS           EfiStatus;
  GRAPHICS_IO_PRIVATE  *Drv;

  Drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;

  EfiStatus = X11CheckKey (GraphicsIo);
  if (EFI_ERROR (EfiStatus)) {
    return EfiStatus;
  }

  CopyMem (KeyData, &Drv->keys[Drv->key_rd], sizeof (EFI_KEY_DATA));
  Drv->key_rd = (Drv->key_rd + 1) % NBR_KEYS;
  Drv->key_count--;

  return EFI_SUCCESS;
}

EFI_STATUS
X11KeySetState (
  IN EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsIo,
  IN EFI_KEY_TOGGLE_STATE          *KeyToggleState
  )
{
  GRAPHICS_IO_PRIVATE  *Drv;

  Drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;

  if (*KeyToggleState & EFI_CAPS_LOCK_ACTIVE) {
    if ((Drv->KeyState.KeyToggleState & EFI_CAPS_LOCK_ACTIVE) == 0) {
      //
      // We could create an XKeyEvent and send a XK_Caps_Lock to
      // the UGA/GOP Window
      //
    }
  }

  Drv->KeyState.KeyToggleState = *KeyToggleState;
  return EFI_SUCCESS;
}

EFI_STATUS
X11RegisterKeyNotify (
  IN EMU_GRAPHICS_WINDOW_PROTOCOL                      *GraphicsIo,
  IN EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK  MakeCallBack,
  IN EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK  BreakCallBack,
  IN VOID                                              *Context
  )
{
  GRAPHICS_IO_PRIVATE  *Drv;

  Drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;

  Drv->MakeRegisterdKeyCallback    = MakeCallBack;
  Drv->BreakRegisterdKeyCallback   = BreakCallBack;
  Drv->RegisterdKeyCallbackContext = Context;

  return EFI_SUCCESS;
}

EFI_STATUS
X11Blt (
  IN EMU_GRAPHICS_WINDOW_PROTOCOL     *GraphicsIo,
  IN  EFI_UGA_PIXEL                   *BltBuffer OPTIONAL,
  IN  EFI_UGA_BLT_OPERATION           BltOperation,
  IN  EMU_GRAPHICS_WINDOWS__BLT_ARGS  *Args
  )
{
  GRAPHICS_IO_PRIVATE  *Private;
  UINTN                DstY;
  UINTN                SrcY;
  UINTN                DstX;
  UINTN                SrcX;
  UINTN                Index;
  EFI_UGA_PIXEL        *Blt;
  UINT8                *Dst;
  UINT8                *Src;
  UINTN                Nbr;
  unsigned long        Color;
  XEvent               ev;

  Private = (GRAPHICS_IO_PRIVATE *)GraphicsIo;

  //
  //  Check bounds
  //
  if (  (BltOperation == EfiUgaVideoToBltBuffer)
     || (BltOperation == EfiUgaVideoToVideo))
  {
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

  if (  (BltOperation == EfiUgaBltBufferToVideo)
     || (BltOperation == EfiUgaVideoToVideo)
     || (BltOperation == EfiUgaVideoFill))
  {
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
      Blt          = (EFI_UGA_PIXEL *)((UINT8 *)BltBuffer + (Args->DestinationY * Args->Delta) + Args->DestinationX * sizeof (EFI_UGA_PIXEL));
      Args->Delta -= Args->Width * sizeof (EFI_UGA_PIXEL);
      for (SrcY = Args->SourceY; SrcY < (Args->Height + Args->SourceY); SrcY++) {
        for (SrcX = Args->SourceX; SrcX < (Args->Width + Args->SourceX); SrcX++) {
          *Blt++ = X11ColorToPixel (Private, XGetPixel (Private->image, SrcX, SrcY));
        }

        Blt = (EFI_UGA_PIXEL *)((UINT8 *)Blt + Args->Delta);
      }

      break;
    case EfiUgaBltBufferToVideo:
      Blt          = (EFI_UGA_PIXEL *)((UINT8 *)BltBuffer + (Args->SourceY * Args->Delta) + Args->SourceX * sizeof (EFI_UGA_PIXEL));
      Args->Delta -= Args->Width * sizeof (EFI_UGA_PIXEL);
      for (DstY = Args->DestinationY; DstY < (Args->Height + Args->DestinationY); DstY++) {
        for (DstX = Args->DestinationX; DstX < (Args->Width + Args->DestinationX); DstX++) {
          XPutPixel (Private->image, DstX, DstY, X11PixelToColor (Private, *Blt));
          Blt++;
        }

        Blt = (EFI_UGA_PIXEL *)((UINT8 *)Blt + Args->Delta);
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
      } else {
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
      Color = X11PixelToColor (Private, *BltBuffer);
      for (DstY = Args->DestinationY; DstY < (Args->Height + Args->DestinationY); DstY++) {
        for (DstX = Args->DestinationX; DstX < (Args->Width + Args->DestinationX); DstX++) {
          XPutPixel (Private->image, DstX, DstY, Color);
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
      XCopyArea (
        Private->display,
        Private->win,
        Private->win,
        Private->gc,
        Args->SourceX,
        Args->SourceY,
        Args->Width,
        Args->Height,
        Args->DestinationX,
        Args->DestinationY
        );

      while (1) {
        XNextEvent (Private->display, &ev);
        HandleEvent (Private, &ev);
        if ((ev.type == NoExpose) || (ev.type == GraphicsExpose)) {
          break;
        }
      }

      break;
    case EfiUgaVideoFill:
      Color = X11PixelToColor (Private, *BltBuffer);
      XSetForeground (Private->display, Private->gc, Color);
      XFillRectangle (
        Private->display,
        Private->win,
        Private->gc,
        Args->DestinationX,
        Args->DestinationY,
        Args->Width,
        Args->Height
        );
      XFlush (Private->display);
      break;
    case EfiUgaBltBufferToVideo:
      Redraw (Private, Args->DestinationX, Args->DestinationY, Args->Width, Args->Height);
      break;
    default:
      break;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
X11CheckPointer (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsIo
  )
{
  GRAPHICS_IO_PRIVATE  *Drv;

  Drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;

  HandleEvents (Drv);
  if (Drv->pointer_state_changed != 0) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_READY;
}

EFI_STATUS
X11GetPointerState (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsIo,
  IN  EFI_SIMPLE_POINTER_STATE      *State
  )
{
  EFI_STATUS           EfiStatus;
  GRAPHICS_IO_PRIVATE  *Drv;

  Drv = (GRAPHICS_IO_PRIVATE *)GraphicsIo;

  EfiStatus = X11CheckPointer (GraphicsIo);
  if (EfiStatus != EFI_SUCCESS) {
    return EfiStatus;
  }

  memcpy (State, &Drv->pointer_state, sizeof (EFI_SIMPLE_POINTER_STATE));

  Drv->pointer_state.RelativeMovementX = 0;
  Drv->pointer_state.RelativeMovementY = 0;
  Drv->pointer_state.RelativeMovementZ = 0;
  Drv->pointer_state_changed           = 0;
  return EFI_SUCCESS;
}

EFI_STATUS
X11GraphicsWindowOpen (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  )
{
  GRAPHICS_IO_PRIVATE  *Drv;
  unsigned int         border_width  = 0;
  char                 *display_name = NULL;

  Drv = (GRAPHICS_IO_PRIVATE *)calloc (1, sizeof (GRAPHICS_IO_PRIVATE));
  if (Drv == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Drv->GraphicsIo.Size              = GasketX11Size;
  Drv->GraphicsIo.CheckKey          = GasketX11CheckKey;
  Drv->GraphicsIo.GetKey            = GasketX11GetKey;
  Drv->GraphicsIo.KeySetState       = GasketX11KeySetState;
  Drv->GraphicsIo.RegisterKeyNotify = GasketX11RegisterKeyNotify;
  Drv->GraphicsIo.Blt               = GasketX11Blt;
  Drv->GraphicsIo.CheckPointer      = GasketX11CheckPointer;
  Drv->GraphicsIo.GetPointerState   = GasketX11GetPointerState;

  Drv->key_count                   = 0;
  Drv->key_rd                      = 0;
  Drv->key_wr                      = 0;
  Drv->KeyState.KeyShiftState      = EFI_SHIFT_STATE_VALID;
  Drv->KeyState.KeyToggleState     = EFI_TOGGLE_STATE_VALID;
  Drv->MakeRegisterdKeyCallback    = NULL;
  Drv->BreakRegisterdKeyCallback   = NULL;
  Drv->RegisterdKeyCallbackContext = NULL;

  Drv->display = XOpenDisplay (display_name);
  if (Drv->display == NULL) {
    fprintf (stderr, "uga: cannot connect to X server %s\n", XDisplayName (display_name));
    free (Drv);
    return EFI_DEVICE_ERROR;
  }

  Drv->screen = DefaultScreen (Drv->display);
  Drv->visual = DefaultVisual (Drv->display, Drv->screen);
  Drv->win    = XCreateSimpleWindow (
                  Drv->display,
                  RootWindow (Drv->display, Drv->screen),
                  0,
                  0,
                  4,
                  4,
                  border_width,
                  WhitePixel (Drv->display, Drv->screen),
                  BlackPixel (Drv->display, Drv->screen)
                  );

  Drv->depth = DefaultDepth (Drv->display, Drv->screen);
  XDefineCursor (Drv->display, Drv->win, XCreateFontCursor (Drv->display, XC_pirate));

  Drv->Title = malloc (StrSize (This->ConfigString));
  UnicodeStrToAsciiStrS (This->ConfigString, Drv->Title, StrSize (This->ConfigString));
  XStoreName (Drv->display, Drv->win, Drv->Title);

  //  XAutoRepeatOff (Drv->display);
  XSelectInput (
    Drv->display,
    Drv->win,
    ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask
    );
  Drv->gc = DefaultGC (Drv->display, Drv->screen);

  This->Private   = (VOID *)Drv;
  This->Interface = (VOID *)Drv;
  return EFI_SUCCESS;
}

EFI_STATUS
X11GraphicsWindowClose (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  )
{
  GRAPHICS_IO_PRIVATE  *Drv;

  Drv = (GRAPHICS_IO_PRIVATE *)This->Private;

  if (Drv == NULL) {
    return EFI_SUCCESS;
  }

  if (Drv->image != NULL) {
    XDestroyImage (Drv->image);

    if (Drv->use_shm) {
      shmdt (Drv->image_data);
    }

    Drv->image_data = NULL;
    Drv->image      = NULL;
  }

  XDestroyWindow (Drv->display, Drv->win);
  XCloseDisplay (Drv->display);

 #ifdef __APPLE__
  // Free up the shared memory
  shmctl (Drv->xshm_info.shmid, IPC_RMID, NULL);
 #endif

  free (Drv);
  return EFI_SUCCESS;
}

EMU_IO_THUNK_PROTOCOL  gX11ThunkIo = {
  &gEmuGraphicsWindowProtocolGuid,
  NULL,
  NULL,
  0,
  GasketX11GraphicsWindowOpen,
  GasketX11GraphicsWindowClose,
  NULL
};
