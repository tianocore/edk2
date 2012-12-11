/** @file
  Constants and declarations for the Interactive IO library.

  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _INTERACTIVE_IO_H
#define _INTERACTIVE_IO_H

#include  <sys/EfiSysCall.h>
#include  <sys/termios.h>
#include  <Containers/Fifo.h>
#include  <kfile.h>

__BEGIN_DECLS

typedef struct _IIO_Instance  cIIO;

cIIO * EFIAPI New_cIIO(void);   // Creates a new cIIO structure

/*  Types of Member functions of the TTY I/O "class". */
typedef void    (EFIAPI *cIIO_Delete)    (cIIO *This);

typedef ssize_t (EFIAPI *cIIO_Read)      (struct __filedes *filp, size_t BufferSize, VOID *Buffer);

typedef ssize_t (EFIAPI *cIIO_Write)     (struct __filedes *filp, const char *buf, ssize_t n);

typedef ssize_t (EFIAPI *cIIO_Echo)      (struct __filedes *filp, wchar_t EChar, BOOLEAN EchoIsOK);

/** Structure defining an instance of the Interactive I/O "class".  **/
struct _IIO_Instance {
  /* ######## Public Functions ######## */
  cIIO_Delete      Delete;
  cIIO_Read        Read;
  cIIO_Write       Write;
  cIIO_Echo        Echo;

  /* ######## PRIVATE Data ######## */

  // Wide input buffer -- stdin
  cFIFO          *InBuf;

  // Wide output buffer -- stdout
  cFIFO          *OutBuf;

  // Attributes for characters in the output buffer
  UINT8          *AttrBuf;

  // Wide output buffer -- stderr
  cFIFO          *ErrBuf;

  // Character conversion states for the buffers
  mbstate_t       OutState;
  mbstate_t       ErrState;

  //  Cursor position at beginning of operation
  //  and at each character thereafter.
  CURSOR_XY       InitialXY;
  CURSOR_XY       CurrentXY;

  UINTN           MaxColumn;    // Width of the output device
  UINTN           MaxRow;       // Height of the output device

  // termios structure
  struct termios  Termio;
};

// Helper Functions
ssize_t IIO_CanonRead     (struct __filedes *filp);
ssize_t IIO_NonCanonRead  (struct __filedes *filp);
ssize_t IIO_WriteOne      (struct __filedes *filp, cFIFO *Buf, wchar_t InCh);
ssize_t IIO_EchoOne       (struct __filedes *filp, wchar_t InCh, BOOLEAN EchoIsOK);

__END_DECLS
#endif  /* _INTERACTIVE_IO_H */
