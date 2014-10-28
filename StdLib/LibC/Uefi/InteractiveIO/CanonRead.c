/** @file
  Canonical Interactive Input Function.

  The functions assume that isatty() is TRUE at the time they are called.

  Copyright (c) 2012 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>

#include  <LibConfig.h>

#include  <errno.h>
#include  <sys/syslimits.h>
#include  <sys/termios.h>
#include  <Device/IIO.h>
#include  <MainData.h>
#include  "IIOutilities.h"
#include  "IIOechoCtrl.h"

/** Read a line from the input file in canonical mode.
    Perform echoing and input processing as directed by the termios flags.

    @param[in]    filp      A pointer to a file descriptor structure.

    @return     The number of characters in the input buffer, or -1 if there
                was an error.
**/
ssize_t
IIO_CanonRead (
  struct __filedes *filp
  )
{
  cIIO             *This;
  cFIFO            *InBuf;
  struct termios   *Termio;
  struct __filedes *fpOut;
  size_t            NumRead;
  wint_t            InChar;
  tcflag_t          IFlag;
  tcflag_t          LFlag;
  BOOLEAN           EchoIsOK;
  BOOLEAN           Activate;
  BOOLEAN           FirstRead;
  int               OutMode;
  UINTN             MaxColumn;
  UINTN             MaxRow;

  NumRead   = MAX_INPUT;    // Workaround "potentially uninitialized" warning
  EchoIsOK  = FALSE;
  FirstRead = TRUE;
  This      = filp->devdata;
  Termio    = &This->Termio;
  InBuf     = This->InBuf;

  // Get a copy of the flags we are going to use
  IFlag = Termio->c_iflag;
  LFlag = Termio->c_lflag;

  /* Determine what the current screen size is. Also validates the output device. */
  OutMode = IIO_GetOutputSize(STDOUT_FILENO, &MaxColumn, &MaxRow);
  if(OutMode >= 0) {
    /*  Set the maximum screen dimensions. */
    This->MaxColumn = MaxColumn;
    This->MaxRow    = MaxRow;

    /*  Record where the cursor is at the beginning of this Input operation.
        The currently set stdout device is used to determine this.  If there is
        no stdout, or stdout is not an interactive device, nothing is recorded.
    */
    if (IIO_GetCursorPosition(STDOUT_FILENO, &This->InitialXY.Column, &This->InitialXY.Row) >= 0) {
      This->CurrentXY.Column  = This->InitialXY.Column;
      This->CurrentXY.Row     = This->InitialXY.Row;
      EchoIsOK  = TRUE;   // Can only echo to stdout
    }
  }

  // For now, we only echo to stdout.
  fpOut = &gMD->fdarray[STDOUT_FILENO];

  //  Input and process characters until BufferSize is exhausted.
  do {
    InChar = IIO_GetInChar(filp, FirstRead);
    if (InChar == WEOF) {
      NumRead = 0;
      break;
    }
    FirstRead = FALSE;
    Activate  = TRUE;
    if(InChar == CHAR_CARRIAGE_RETURN) {
      if((IFlag & IGNCR) != 0) {
        continue;   // Restart the do loop, discarding the CR
      }
      else if((IFlag & ICRNL) != 0) {
        InChar = L'\n';
      }
    }
    else if(InChar == CHAR_LINEFEED) {
      if((IFlag & INLCR) != 0) {
        InChar = L'\r';
      }
    }
    else if(CCEQ(Termio->c_cc[VINTR], InChar)) {
      if((LFlag & ISIG) != 0) {
        // Raise Signal
        // Flush Input Buffer
        // Return to caller
        InChar = IIO_ECHO_DISCARD;
        errno = EINTR;
      }
      else {
        Activate = FALSE;
      }
    }
    else if(CCEQ(Termio->c_cc[VQUIT], InChar)) {
      if((LFlag & ISIG) != 0) {
        // Raise Signal
        // Flush Input Buffer
        // Return to caller
        InChar = IIO_ECHO_DISCARD;
        errno = EINTR;
      }
      else {
        Activate = FALSE;
      }
    }
    else if(CCEQ(Termio->c_cc[VEOF], InChar)) {
      InChar = WEOF;
      NumRead = 0;
      EchoIsOK = FALSE;   // Buffer, but don't echo this character
    }
    else if(CCEQ(Termio->c_cc[VEOL], InChar)) {
      EchoIsOK = FALSE;   // Buffer, but don't echo this character
    }
    else if(CCEQ(Termio->c_cc[VERASE], InChar)) {
      InChar = IIO_ECHO_ERASE;
      Activate = FALSE;
    }
    else if(CCEQ(Termio->c_cc[VKILL], InChar)) {
      InChar = IIO_ECHO_KILL;
      Activate = FALSE;
    }
    else {
      if((InChar < TtySpecKeyMin) || (InChar >= TtyFunKeyMax)) {
        Activate = FALSE;
      }
    }
    /** The Echo function is responsible for:
          * Adding the character to the input buffer, if appropriate.
          * Removing characters from the input buffer for ERASE and KILL processing.
          * Visually removing characters from the screen if ECHOE is set.
          * Ensuring one can not backspace beyond the beginning of the input text.
          * Sending final echo strings to output.
    **/
    (void)This->Echo(fpOut, (wchar_t)InChar, EchoIsOK);
    NumRead = InBuf->Count(InBuf, AsElements);
  } while((NumRead < MAX_INPUT) &&
          (Activate == FALSE));

  return (ssize_t)NumRead;
}
