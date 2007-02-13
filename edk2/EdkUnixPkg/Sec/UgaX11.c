#include "Uefi/UefiSpec.h"
#include "Protocol/UnixThunk.h"
#include "Protocol/SimpleTextIn.h"
#include "Protocol/UgaDraw.h"
#include "Protocol/UnixUgaIo.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/XShm.h>
#include <X11/keysym.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>

extern void msSleep (unsigned long Milliseconds);

/* XQueryPointer  */

struct uga_drv_shift_mask
{
  unsigned char shift;
  unsigned char size;
  unsigned char csize;
};

#define NBR_KEYS 32
typedef struct
{
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
  EFI_INPUT_KEY keys[NBR_KEYS];
} UGA_IO_PRIVATE;

static void
HandleEvents(UGA_IO_PRIVATE *drv);

static void
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

static int
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
  if (drv->xshm_info.shmid < 0)
    {
      XDestroyImage(drv->image);
      return 0;
    }
      
  drv->image_data = shmat (drv->xshm_info.shmid, NULL, 0);
  if(!drv->image_data)
    {
      shmctl (drv->xshm_info.shmid, IPC_RMID, NULL);
      XDestroyImage(drv->image);
      return 0;
    }
  /* Can this fail ?  */
  shmctl (drv->xshm_info.shmid, IPC_RMID, NULL);

  drv->xshm_info.shmaddr = (char*)drv->image_data;
  drv->image->data = (char*)drv->image_data;
	  
  if (!XShmAttach (drv->display, &drv->xshm_info))
    {
      shmdt (drv->image_data);
      XDestroyImage(drv->image);
      return 0;
    }
  return 1;
}

static
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
  free(drv);
  return EFI_SUCCESS;
}

static
EFI_STATUS
UgaSize(EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, UINT32 Width, UINT32 Height)
{
  UGA_IO_PRIVATE *drv = (UGA_IO_PRIVATE *)UgaIo;
  XSizeHints size_hints;

  /* Destroy current buffer if created.  */
  if (drv->image != NULL)
    {
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
  if (XShmQueryExtension(drv->display) && TryCreateShmImage(drv))
    {
      drv->use_shm = 1;
    }	
  else	
    {
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

static void
handleKeyEvent(UGA_IO_PRIVATE *drv, XEvent *ev)
{
  KeySym keysym;
  char str[4];
  EFI_INPUT_KEY Key;
  int res;

  if (drv->key_count == NBR_KEYS)
    return;

  res = XLookupString(&ev->xkey, str, sizeof(str), &keysym, NULL);
  Key.ScanCode = 0;
  Key.UnicodeChar = 0;
  switch (keysym) {
  case XK_Home:       Key.ScanCode = SCAN_HOME;       break;
  case XK_End:        Key.ScanCode = SCAN_END;        break;
  case XK_Left:       Key.ScanCode = SCAN_LEFT;       break;
  case XK_Right:      Key.ScanCode = SCAN_RIGHT;      break;
  case XK_Up:         Key.ScanCode = SCAN_UP;         break;
  case XK_Down:       Key.ScanCode = SCAN_DOWN;       break;
  case XK_Delete:     Key.ScanCode = SCAN_DELETE;     break;
  case XK_Insert:     Key.ScanCode = SCAN_INSERT;     break;
  case XK_Page_Up:    Key.ScanCode = SCAN_PAGE_UP;    break;
  case XK_Page_Down:  Key.ScanCode = SCAN_PAGE_DOWN;  break;
  case XK_Escape:     Key.ScanCode = SCAN_ESC;        break;

  case XK_F1:   Key.ScanCode = SCAN_F1;   break;
  case XK_F2:   Key.ScanCode = SCAN_F2;   break;
  case XK_F3:   Key.ScanCode = SCAN_F3;   break;
  case XK_F4:   Key.ScanCode = SCAN_F4;   break;
  case XK_F5:   Key.ScanCode = SCAN_F5;   break;
  case XK_F6:   Key.ScanCode = SCAN_F6;   break;
  case XK_F7:   Key.ScanCode = SCAN_F7;   break;
  case XK_F8:   Key.ScanCode = SCAN_F8;   break;
  case XK_F9:   Key.ScanCode = SCAN_F9;   break;

  default:
    if (res == 1) {
      Key.UnicodeChar = str[0];
    } else {
      return;
    }
  }

  drv->keys[drv->key_wr] = Key;
  drv->key_wr = (drv->key_wr + 1) % NBR_KEYS;
  drv->key_count++;
}

static void
Redraw(UGA_IO_PRIVATE *drv, UINTN X, UINTN Y, UINTN Width, UINTN Height)
{
  if (drv->use_shm)
    XShmPutImage (drv->display, drv->win, drv->gc, drv->image,
		  X, Y, X, Y, Width, Height, False);
  else
    XPutImage (drv->display, drv->win, drv->gc, drv->image,
		  X, Y, X, Y, Width, Height);
}

static void
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
    case MappingNotify:
      XRefreshKeyboardMapping(&ev->xmapping);
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

static void
HandleEvents(UGA_IO_PRIVATE *drv)
{
  while (XPending(drv->display) != 0)
    {
      XEvent ev;
	  
      XNextEvent (drv->display, &ev);
      HandleEvent(drv, &ev);
    }
}

static
unsigned long
UgaPixelToColor (UGA_IO_PRIVATE *drv, EFI_UGA_PIXEL pixel)
{
  return ((pixel.Red >> drv->r.csize) << drv->r.shift)
    | ((pixel.Green >> drv->g.csize) << drv->g.shift)
    | ((pixel.Blue >> drv->b.csize) << drv->b.shift);
}

static
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

static
EFI_STATUS
UgaCheckKey(EFI_UNIX_UGA_IO_PROTOCOL *UgaIo)
{
  UGA_IO_PRIVATE *drv = (UGA_IO_PRIVATE *)UgaIo;
  HandleEvents(drv);
  if (drv->key_count != 0)
    return EFI_SUCCESS;
  else {
    /* EFI is certainly polling.  Be CPU-friendly.  */
    msSleep (20);
    return EFI_NOT_READY;
  }
}

static
EFI_STATUS
UgaGetKey(EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, EFI_INPUT_KEY *key)
{
  UGA_IO_PRIVATE *drv = (UGA_IO_PRIVATE *)UgaIo;
  EFI_STATUS status;

  status = UgaCheckKey(UgaIo);
  if (status != EFI_SUCCESS)
    return status;

  *key = drv->keys[drv->key_rd];
  drv->key_rd = (drv->key_rd + 1) % NBR_KEYS;
  drv->key_count--;
  return EFI_SUCCESS;
}

EFI_STATUS
UgaBlt(EFI_UNIX_UGA_IO_PROTOCOL *UgaIo,
       IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
       IN  EFI_UGA_BLT_OPERATION                   BltOperation,
       IN  UINTN                                   SourceX,
       IN  UINTN                                   SourceY,
       IN  UINTN                                   DestinationX,
       IN  UINTN                                   DestinationY,
       IN  UINTN                                   Width,
       IN  UINTN                                   Height,
       IN  UINTN                                   Delta OPTIONAL
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
    if (SourceY + Height > Private->height) {
      return EFI_INVALID_PARAMETER;
    }

    if (SourceX + Width > Private->width) {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (BltOperation == EfiUgaBltBufferToVideo
      || BltOperation == EfiUgaVideoToVideo
      || BltOperation == EfiUgaVideoFill) {
    //
    // Destination is Video
    //
    if (DestinationY + Height > Private->height) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestinationX + Width > Private->width) {
      return EFI_INVALID_PARAMETER;
    }
  }

  switch (BltOperation) {
  case EfiUgaVideoToBltBuffer:
    Blt = BltBuffer;
    Delta -= Width * sizeof (EFI_UGA_PIXEL);
    for (SrcY = SourceY; SrcY < (Height + SourceY); SrcY++) {
      for (SrcX = SourceX; SrcX < (Width + SourceX); SrcX++) {
	*Blt++ = UgaColorToPixel(Private,
				 XGetPixel(Private->image, SrcX, SrcY));
      }
      Blt = (EFI_UGA_PIXEL *) ((UINT8 *) Blt + Delta);
    }
    break;
  case EfiUgaBltBufferToVideo:
    Blt = BltBuffer;
    Delta -= Width * sizeof (EFI_UGA_PIXEL);
    for (DstY = DestinationY; DstY < (Height + DestinationY); DstY++) {
      for (DstX = DestinationX; DstX < (Width + DestinationX); DstX++) {
	XPutPixel(Private->image, DstX, DstY, UgaPixelToColor(Private, *Blt));
	Blt++;
      }
      Blt = (EFI_UGA_PIXEL *) ((UINT8 *) Blt + Delta);
    }
    break;
  case EfiUgaVideoToVideo:
    Dst = Private->image_data + (DestinationX << Private->pixel_shift)
      + DestinationY * Private->line_bytes;
    Src = Private->image_data + (SourceX << Private->pixel_shift)
      + SourceY * Private->line_bytes;
    Nbr = Width << Private->pixel_shift;
    if (DestinationY < SourceY) {
      for (Index = 0; Index < Height; Index++) {
	memcpy (Dst, Src, Nbr);
	Dst += Private->line_bytes;
	Src += Private->line_bytes;
      }
    }
    else {
      Dst += (Height - 1) * Private->line_bytes;
      Src += (Height - 1) * Private->line_bytes;
      for (Index = 0; Index < Height; Index++) {
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
    for (DstY = DestinationY; DstY < (Height + DestinationY); DstY++) {
      for (DstX = DestinationX; DstX < (Width + DestinationX); DstX++) {
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
	      SourceX, SourceY, Width, Height, DestinationX, DestinationY);
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
		   DestinationX, DestinationY, Width, Height);
    break;
  case EfiUgaBltBufferToVideo:
    Redraw(Private, DestinationX, DestinationY, Width, Height);
    break;
  default:
    break;
  }
  return EFI_SUCCESS;
}

EFI_STATUS
UgaCreate (EFI_UNIX_UGA_IO_PROTOCOL **Uga, CONST CHAR16 *Title)
{
  UGA_IO_PRIVATE *drv;
  unsigned int border_width = 0;
  char *display_name = NULL;
  int title_len;

  drv = (UGA_IO_PRIVATE *)
    calloc (1, sizeof (UGA_IO_PRIVATE));
  if (drv == NULL)
    return EFI_OUT_OF_RESOURCES;

  drv->UgaIo.UgaClose = UgaClose;
  drv->UgaIo.UgaSize = UgaSize;
  drv->UgaIo.UgaCheckKey = UgaCheckKey;
  drv->UgaIo.UgaGetKey = UgaGetKey;
  drv->UgaIo.UgaBlt = UgaBlt;

  drv->key_count = 0;
  drv->key_rd = 0;
  drv->key_wr = 0;
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
	 BlackPixel (drv->display, drv->screen),
	 WhitePixel (drv->display, drv->screen));

  drv->depth = DefaultDepth (drv->display, drv->screen);

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
		ExposureMask | KeyPressMask);
  drv->gc = DefaultGC (drv->display, drv->screen);

  *Uga = (EFI_UNIX_UGA_IO_PROTOCOL *)drv;
  return EFI_SUCCESS;
}
