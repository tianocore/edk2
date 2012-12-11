/** @file
  Echo characters to an Interactive I/O Output device.

  The functions assume that isatty() is TRUE at the time they are called.
  Since the UEFI console is a WIDE character device, these functions do all
  processing using wide characters.

  It is the responsibility of the caller, or higher level function, to perform
  any necessary translation between wide and narrow characters.

  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>

#include  <LibConfig.h>

#include  <assert.h>
#include  <errno.h>
#include  <sys/termios.h>
#include  <Device/IIO.h>
#include  "IIOutilities.h"
#include  "IIOechoCtrl.h"

/** Echo one character to an IIO file.

    If character InCh is a special "echo control" character, process it and output
    the resultant character(s), if any.  Otherwise pass the character on to the
    IIO_WriteOne() function which performs generic output processing, if needed.

    @param[in]    filp        Pointer to an open IIO file's file descriptor structure.
    @param[in]    InCh        The wide character to be echoed.
    @param[in]    EchoIsOK    A flag indicating whether echoing is appropriate for this
                              device or not.

    @retval   -1    The filp argument does not refer to an IIO device.
                    Global value errno is set to EINVAL.
    @retval   >=0   The number of characters actually output.

    @sa   IIO_WriteOne
**/
ssize_t
IIO_EchoOne (
  struct __filedes     *filp,
  wchar_t               InCh,
  BOOLEAN               EchoIsOK
  )
{
  cIIO       *This;
  cFIFO      *OutBuf;
  cFIFO      *InBuf;
  UINT8      *AttrBuf;
  ssize_t     NumEcho;
  tcflag_t    LFlags;
  UINT32      AttrDex;
  int         i;

  NumEcho = -1;
  This    = filp->devdata;

  if(This != NULL) {
    LFlags  = This->Termio.c_lflag;
    OutBuf  = This->OutBuf;
    InBuf   = This->InBuf;
    AttrBuf = This->AttrBuf;
    AttrDex = InBuf->GetWDex(InBuf);

    switch(InCh) {
      case IIO_ECHO_DISCARD:
        // Do not buffer or otherwise process
        NumEcho = 0;
        break;

      case IIO_ECHO_ERASE:
        // Delete last character from InBuf
        if(!InBuf->IsEmpty(InBuf)) {
          (void)InBuf->Truncate(InBuf);

          // Erase screen character(s) based on Attrib value
          if(LFlags & ECHO) {
            AttrDex = (UINT32)ModuloDecrement(AttrDex, InBuf->NumElements);
            NumEcho = AttrBuf[AttrDex];
            for(i = 0; i < NumEcho; ++i) {
              (void)IIO_WriteOne(filp, OutBuf, CHAR_BACKSPACE);
            }
            if(LFlags & ECHOE) {
              for(i = 0; i < NumEcho; ++i) {
                (void)IIO_WriteOne(filp, OutBuf, L' ');
              }
              for(i = 0; i < NumEcho; ++i) {
                (void)IIO_WriteOne(filp, OutBuf, CHAR_BACKSPACE);
              }
            }
          }
          else {
            NumEcho = 0;
          }
        }
        break;

      case IIO_ECHO_KILL:
        // Flush contents of InBuf and OutBuf
        InBuf->Flush(InBuf, (size_t)-1);
        OutBuf->Flush(OutBuf, (size_t)-1);

        // Erase characters from screen.
        if(LFlags & ECHOE) {
          NumEcho = IIO_CursorDelta(This, &This->InitialXY, &This->CurrentXY);
          for(i = 0; i < NumEcho; ++i) {
            (void)IIO_WriteOne(filp, OutBuf, L' ');
          }
        }
        break;

      default:
        // Add character to input buffer
        (void)InBuf->Write(InBuf, &InCh, 1);

        NumEcho = 0;  // In case echoing is not enabled or OK
        // If echoing is OK and enabled, "echo" character using IIO_WriteOne
        if( EchoIsOK                &&
            ( (LFlags & ECHO)       ||
              ((LFlags & ECHONL) && (InCh == CHAR_LINEFEED))))
        {
          NumEcho = IIO_WriteOne(filp, OutBuf, InCh);
        }
        AttrBuf[AttrDex] = (UINT8)NumEcho;
        break;
    }
  }
  else {
    errno = EINVAL;
  }
  return NumEcho;
}
