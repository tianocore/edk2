/** @file
  Definitions for the Interactive IO library.

  The functions assume that isatty() is TRUE at the time they are called.

  Copyright (c) 2016, Daryl McDaniel. All rights reserved.<BR>
  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <Library/MemoryAllocationLib.h>

#include  <LibConfig.h>

#include  <assert.h>
#include  <errno.h>
#include  <sys/syslimits.h>
#include  <sys/termios.h>
#include  <Device/IIO.h>
#include  <MainData.h>
#include  "IIOutilities.h"
#include  "IIOechoCtrl.h"

// Instrumentation used for debugging
#define   IIO_C_DEBUG   0       ///< Set to 1 to enable instrumentation, 0 to disable

#if IIO_C_DEBUG
  static volatile size_t  IIO_C_WRemainder = 0;   ///< Characters in Out buffer after IIO_Write
  static volatile size_t  IIO_C_RRemainder = 0;   ///< Characters in In buffer after IIO_Read

  #define W_INSTRUMENT  IIO_C_WRemainder =
  #define R_INSTRUMENT  IIO_C_RRemainder =
#else // ! IIO_C_DEBUG -- don't instrument code
  #define W_INSTRUMENT  (void)
  #define R_INSTRUMENT  (void)
#endif  // IIO_C_DEBUG

/** Read from an Interactive IO device.

  NOTE: If _S_IWTTY is set, the internal buffer contains WIDE characters.
        They will need to be converted to MBCS when returned.

    Input is line buffered if ICANON is set,
    otherwise MIN determines how many characters to input.
    Currently MIN is always zero, meaning 0 or 1 character is input in
    noncanonical mode.

    @param[in]    filp        Pointer to the descriptor of the device (file) to be read.
    @param[in]    BufferSize  Maximum number of bytes to be returned to the caller.
    @param[out]   Buffer      Pointer to the buffer where the input is to be stored.

    @retval   -1    An error occurred.  No data is available.
    @retval    0    No data was available.  Try again later.
    @retval   >0    The number of bytes consumed by the returned data.
**/
static
ssize_t
EFIAPI
IIO_Read(
  struct __filedes *filp,
  size_t BufferSize,
  VOID *Buffer
  )
{
  cIIO     *This;
  ssize_t   NumRead;
  tcflag_t  Flags;
  size_t    XlateSz;
  size_t    Needed;

  NumRead = -1;
  This = filp->devdata;
  if(This != NULL) {
    Flags = This->Termio.c_lflag;
    if(Flags & ICANON) {
      NumRead = IIO_CanonRead(filp);
    }
    else {
      NumRead = IIO_NonCanonRead(filp);
    }
    // At this point, the input has been accumulated in the input buffer.
    if(filp->f_iflags & _S_IWTTY) {
      // Data in InBuf is wide characters.  Convert to MBCS
      // First, convert into a linear buffer
      NumRead = This->InBuf->Copy(This->InBuf, gMD->UString2, (INT32)UNICODE_STRING_MAX-1);
      gMD->UString2[NumRead] = 0;   // Ensure that the buffer is terminated
      // Determine the needed space
      XlateSz = EstimateWtoM((const wchar_t *)gMD->UString2, BufferSize, &Needed);

      // Now translate this into MBCS in Buffer
      NumRead = wcstombs((char *)Buffer, (const wchar_t *)gMD->UString2, XlateSz);

      // Consume the translated characters
      (void) This->InBuf->Flush(This->InBuf, Needed);
    }
    else {
      // Data in InBuf is narrow characters.  Use verbatim.
      NumRead = This->InBuf->Read(This->InBuf, Buffer, (INT32)BufferSize);
    }
#if IIO_C_DEBUG
    IIO_C_RRemainder = This->InBuf->Count(This->InBuf, AsElements);
#endif // IIO_C_DEBUG
  }
  return NumRead;
}

/** Handle write to a Terminal (Interactive) device.

    Processes characters from buffer buf and writes them to the Terminal device
    specified by filp.

    The parameter buf points to a MBCS string to be output. This is processed
    and buffered one character at a time by IIO_WriteOne() which handles TAB
    expansion, NEWLINE to CARRIAGE_RETURN + NEWLINE expansion, as well as
    basic line editing functions. The number of characters actually written to
    the output device will seldom equal the number of characters consumed from
    buf.

    In this implementation, all of the special characters processed by
    IIO_WriteOne() are single-byte characters with values less than 128.
    (7-bit ASCII or the single-byte UTF-8 characters)

    Every byte that is not one of the recognized special characters is passed,
    unchanged, to the Terminal device.

    @param[in]      filp      Pointer to a file descriptor structure.
    @param[in]      buf       Pointer to the MBCS string to be output.
    @param[in]      N         Number of bytes in buf.

    @retval   >=0     Number of bytes consumed from buf and sent to the
                      Terminal device.
**/
static
ssize_t
EFIAPI
IIO_Write(
  struct __filedes *filp,
  const char *buf,
  ssize_t N
  )
{
  cIIO       *This;
  cFIFO      *OutBuf;
  mbstate_t  *OutState;
  char       *MbcsPtr;
  ssize_t     NumConsumed;
  ssize_t     NumProc;
  size_t      CharLen;
  UINTN       MaxColumn;
  UINTN       MaxRow;
  wchar_t     OutChar[2];     // Just in case we run into a 4-byte MBCS character
  int         OutMode;

  NumConsumed = -1;

  /* Determine what the current screen size is. Also validates the output device. */
  OutMode = IIO_GetOutputSize(filp->MyFD, &MaxColumn, &MaxRow);

  This = filp->devdata;
  if((This != NULL) && (OutMode >= 0)) {
    if(filp->MyFD == STDERR_FILENO) {
      OutBuf = This->ErrBuf;
      OutState  = &This->ErrState;
    }
    else {
      OutBuf = This->OutBuf;
      OutState  = &This->OutState;
    }

    /*  Set the maximum screen dimensions. */
    This->MaxColumn = MaxColumn;
    This->MaxRow    = MaxRow;

    /*  Record where the cursor is at the beginning of the Output operation. */
    (void)IIO_GetCursorPosition(filp->MyFD, &This->InitialXY.Column, &This->InitialXY.Row);
    This->CurrentXY.Column  = This->InitialXY.Column;
    This->CurrentXY.Row     = This->InitialXY.Row;

    NumConsumed = 0;
    OutChar[0]  = (wchar_t)buf[0];
    while((OutChar[0] != 0) && (NumConsumed < N)) {
      CharLen = mbrtowc(OutChar, (const char *)&buf[NumConsumed], MB_CUR_MAX, OutState);
      if (CharLen < 0) {  // Encoding Error
        OutChar[0] = BLOCKELEMENT_LIGHT_SHADE;
        CharLen = 1;  // Consume a byte
        (void)mbrtowc(NULL, NULL, 1, OutState);  // Re-Initialize the conversion state
      }
      NumProc = IIO_WriteOne(filp, OutBuf, OutChar[0]);
      if(NumProc >= 0) {
        // Successfully processed and buffered one character
        NumConsumed += CharLen;   // Index of start of next character
      }
      else {
        if (errno == ENOSPC) {
          // Not enough room in OutBuf to hold a potentially expanded character
          break;
        }
        return -1;    // Something corrupted and filp->devdata is now NULL
      }
    }
    // At this point, the characters to write are in OutBuf
    // First, linearize the buffer
    NumProc = OutBuf->Copy(OutBuf, gMD->UString, UNICODE_STRING_MAX-1);
    gMD->UString[NumProc] = 0;   // Ensure that the buffer is terminated

    if(filp->f_iflags & _S_IWTTY) {
      // Output device expects wide characters, Output what we have
      NumProc = filp->f_ops->fo_write(filp, NULL, NumProc, gMD->UString);

      // Consume the output characters
      W_INSTRUMENT OutBuf->Flush(OutBuf, NumProc);
    }
    else {
      // Output device expects narrow characters, convert to MBCS
      MbcsPtr = (char *)gMD->UString2;
      // Determine the needed space. NumProc is the number of bytes needed.
      NumProc = (ssize_t)EstimateWtoM((const wchar_t *)gMD->UString, UNICODE_STRING_MAX * sizeof(wchar_t), &CharLen);

      // Now translate this into MBCS in the buffer pointed to by MbcsPtr.
      // The returned value, NumProc, is the resulting number of bytes.
      NumProc = wcstombs(MbcsPtr, (const wchar_t *)gMD->UString, NumProc);
      MbcsPtr[NumProc] = 0;   // Ensure the buffer is terminated

      // Send the MBCS buffer to Output
      NumProc = filp->f_ops->fo_write(filp, NULL, NumProc, MbcsPtr);
      // Mark the Mbcs buffer after the last byte actually written
      MbcsPtr[NumProc] = 0;
      // Count the CHARACTERS actually sent
      CharLen = CountMbcsChars(MbcsPtr);

      // Consume the number of output characters actually sent
      W_INSTRUMENT OutBuf->Flush(OutBuf, CharLen);
    }
  }
  else {
    if(This == NULL) {
      errno = EINVAL;
    }
    // Otherwise, errno is already set.
  }
  return NumConsumed;
}

/** Echo a character to an output device.
    Performs translation and edit processing depending upon termios flags.

    @param[in]    filp      A pointer to a file descriptor structure.
    @param[in]    EChar     The character to echo.
    @param[in]    EchoIsOK  TRUE if the caller has determined that characters
                            should be echoed.  Otherwise, just buffer.

    @return   Returns the number of characters actually output.
**/
static
ssize_t
EFIAPI
IIO_Echo(
  struct __filedes *filp,
  wchar_t           EChar,
  BOOLEAN           EchoIsOK
  )
{
  cIIO     *This;
  ssize_t   NumWritten;
  cFIFO    *OutBuf;
  char     *MbcsPtr;
  ssize_t   NumProc;
  tcflag_t  LFlags;

  NumWritten = -1;
  This = filp->devdata;
  if(This != NULL) {
    OutBuf = This->OutBuf;
    LFlags = This->Termio.c_lflag & (ECHOK | ECHOE);

    if((EChar >= TtyFunKeyMin) && (EChar < TtyFunKeyMax)) {
      // A special function key was pressed, buffer it, don't echo, and activate.
      // Process and buffer the character.  May produce multiple characters.
      NumProc = IIO_EchoOne(filp, EChar, FALSE);    // Don't echo this character
      EChar   = CHAR_LINEFEED;                      // Every line must end with '\n' (legacy)
    }
    // Process and buffer the character.  May produce multiple characters.
    NumProc = IIO_EchoOne(filp, EChar, EchoIsOK);

    // At this point, the character(s) to write are in OutBuf
    // First, linearize the buffer
    NumWritten = OutBuf->Copy(OutBuf, gMD->UString, UNICODE_STRING_MAX-1);
    gMD->UString[NumWritten] = 0;   // Ensure that the buffer is terminated

    if((EChar == IIO_ECHO_KILL) && (LFlags & ECHOE) && EchoIsOK) {
      // Position the cursor to the start of input.
      (void)IIO_SetCursorPosition(filp, &This->InitialXY);
    }
    // Output the buffer
    if(filp->f_iflags & _S_IWTTY) {
      // Output device expects wide characters, Output what we have
      NumWritten = filp->f_ops->fo_write(filp, NULL, NumWritten, gMD->UString);
    }
    else {
      // Output device expects narrow characters, convert to MBCS
      MbcsPtr = (char *)gMD->UString2;
      // Determine the needed space
      NumProc = (ssize_t)EstimateWtoM((const wchar_t *)gMD->UString, UNICODE_STRING_MAX * sizeof(wchar_t), NULL);

      // Now translate this into MBCS in Buffer
      NumWritten = wcstombs(MbcsPtr, (const wchar_t *)gMD->UString, NumProc);
      MbcsPtr[NumWritten] = 0;   // Ensure the buffer is terminated

      // Send the MBCS buffer to Output
      NumWritten = filp->f_ops->fo_write(filp, NULL, NumWritten, MbcsPtr);
    }
    // Consume the echoed characters
    (void)OutBuf->Flush(OutBuf, NumWritten);

    if(EChar == IIO_ECHO_KILL) {
      if(LFlags == ECHOK) {
        NumWritten = IIO_WriteOne(filp, OutBuf, CHAR_LINEFEED);
      }
      else if((LFlags & ECHOE) && EchoIsOK) {
        // Position the cursor to the start of input.
        (void)IIO_SetCursorPosition(filp, &This->InitialXY);
      }
      NumWritten = 0;
    }
  }
  else {
    errno = EINVAL;
  }

  return NumWritten;
}

static
void
FifoDelete(cFIFO *Member)
{
  if(Member != NULL) {
    Member->Delete(Member);
  }
}

/** Destructor for an IIO instance.

    Releases all resources used by a particular IIO instance.
**/
static
void
EFIAPI
IIO_Delete(
  cIIO *Self
  )
{
  if(Self != NULL) {
    FifoDelete(Self->ErrBuf);
    FifoDelete(Self->OutBuf);
    FifoDelete(Self->InBuf);
    if(Self->AttrBuf != NULL) {
      FreePool(Self->AttrBuf);
    }
    FreePool(Self);
  }
}

/** Constructor for new IIO instances.

    @return   Returns NULL or a pointer to a new IIO instance.
**/
cIIO *
EFIAPI
New_cIIO(void)
{
  cIIO     *IIO;
  cc_t     *TempBuf;
  int       i;

  IIO = (cIIO *)AllocateZeroPool(sizeof(cIIO));
  if(IIO != NULL) {
    IIO->InBuf    = New_cFIFO(MAX_INPUT, sizeof(wchar_t));
    IIO->OutBuf   = New_cFIFO(MAX_OUTPUT, sizeof(wchar_t));
    IIO->ErrBuf   = New_cFIFO(MAX_OUTPUT, sizeof(wchar_t));
    IIO->AttrBuf  = (UINT8 *)AllocateZeroPool(MAX_OUTPUT);

    if((IIO->InBuf   == NULL) || (IIO->OutBuf   == NULL) ||
       (IIO->ErrBuf  == NULL) || (IIO->AttrBuf  == NULL))
    {
      IIO_Delete(IIO);
      IIO = NULL;
    }
    else {
      IIO->Delete = IIO_Delete;
      IIO->Read   = IIO_Read;
      IIO->Write  = IIO_Write;
      IIO->Echo   = IIO_Echo;
    }
    // Initialize Termio member
    TempBuf = &IIO->Termio.c_cc[0];
    TempBuf[0] = 8;                 // Default length for TABs
    for(i=1; i < NCCS; ++i) {
      TempBuf[i] = _POSIX_VDISABLE;
    }
    TempBuf[VMIN]         = 0;
    TempBuf[VTIME]        = 0;
    IIO->Termio.c_ispeed  = B115200;
    IIO->Termio.c_ospeed  = B115200;
    IIO->Termio.c_iflag   = ICRNL;
    IIO->Termio.c_oflag   = OPOST | ONLCR | ONOCR | ONLRET;
    IIO->Termio.c_cflag   = 0;
    IIO->Termio.c_lflag   = ECHO | ECHONL;
  }
  return IIO;
}
