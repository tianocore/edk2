/** @file
  Abstract device driver for the UEFI Console.

  Manipulates abstractions for stdin, stdout, stderr.

  This device is a WIDE device and this driver returns WIDE
  characters.  It this the responsibility of the caller to convert between
  narrow and wide characters in order to perform the desired operations.

  The devices status as a wide device is indicatd by _S_IWTTY being set in
  f_iflags.

  Copyright (c) 2016, Daryl McDaniel. All rights reserved.<BR>
  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include  <Uefi.h>
#include  <Library/BaseLib.h>
#include  <Library/MemoryAllocationLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/DebugLib.h>
#include  <Protocol/SimpleTextIn.h>
#include  <Protocol/SimpleTextOut.h>

#include  <LibConfig.h>

#include  <errno.h>
#include  <wctype.h>
#include  <wchar.h>
#include  <stdarg.h>
#include  <sys/fcntl.h>
#include  <unistd.h>
#include  <sys/termios.h>
#include  <Efi/SysEfi.h>
#include  <kfile.h>
#include  <Device/Device.h>
#include  <Device/IIO.h>
#include  <MainData.h>

static const CHAR16* const
stdioNames[NUM_SPECIAL]   = {
  L"stdin:", L"stdout:", L"stderr:"
};

static const int stdioFlags[NUM_SPECIAL] = {
  O_RDONLY,             // stdin
  O_WRONLY,             // stdout
  O_WRONLY              // stderr
};

static DeviceNode    *ConNode[NUM_SPECIAL];
static ConInstance   *ConInstanceList;

static cIIO          *IIO;

/* Flags settable by Ioctl */
static BOOLEAN        TtyCooked;
static BOOLEAN        TtyEcho;

/** Convert string from MBCS to WCS and translate \n to \r\n.

    It is the caller's responsibility to ensure that dest is
    large enough to hold the converted results.  It is guaranteed
    that there will be fewer than n characters placed in dest.

    @param[out]     dest    WCS buffer to receive the converted string.
    @param[in]      buf     MBCS string to convert to WCS.
    @param[in]      n       Number of BYTES contained in buf.
    @param[in,out]  Cs      Pointer to the character state object for this stream

    @return   The number of BYTES consumed from buf.
**/
ssize_t
WideTtyCvt( CHAR16 *dest, const char *buf, ssize_t n, mbstate_t *Cs)
{
  ssize_t i     = 0;
  int     numB  = 0;
  wchar_t wc[2];

  while(n > 0) {
    numB = (int)mbrtowc(wc, buf, MIN(MB_LEN_MAX,n), Cs);
    if( numB == 0) {
      break;
    };
    if(numB < 0) {    // If an unconvertable character, replace it.
      wc[0] = BLOCKELEMENT_LIGHT_SHADE;
      numB = 1;
    }
    if(wc[0] == L'\n') {
      *dest++ = L'\r';
      ++i;
    }
    *dest++ = (CHAR16)wc[0];
    i += numB;
    n -= numB;
    buf += numB;
  }
  *dest = 0;
  return i;
}

/** Position the console cursor to the coordinates specified by Position.

    @param[in]  filp      Pointer to the file descriptor structure for this file.
    @param[in]  Position  A value containing the target X and Y coordinates.
    @param[in]  whence    Ignored by the Console device.

    @retval   Position    Success.  Returns a copy of the Position argument.
    @retval   -1          filp is not associated with a valid console stream.
    @retval   -1          This console stream is attached to stdin.
    @retval   -1          The SetCursorPosition operation failed.
**/
static
off_t
EFIAPI
da_ConSeek(
  struct __filedes   *filp,
  off_t               Position,
  int                 whence      ///< Ignored by Console
)
{
  ConInstance                       *Stream;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *Proto;
  XY_OFFSET                          CursorPos;

  Stream = BASE_CR(filp->f_ops, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if(Stream->Cookie != CON_COOKIE) {    // Cookie == 'IoAb'
    EFIerrno = RETURN_INVALID_PARAMETER;
    return -1;    // Looks like a bad This pointer
  }
  if(Stream->InstanceNum == STDIN_FILENO) {
    // Seek is not valid for stdin
    EFIerrno = RETURN_UNSUPPORTED;
    return -1;
  }
  // Everything is OK to do the final verification and "seek".
  Proto = (EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)Stream->Dev;
  CursorPos.Offset = Position;

  EFIerrno = Proto->SetCursorPosition(Proto,
                                      (INTN)CursorPos.XYpos.Column,
                                      (INTN)CursorPos.XYpos.Row);

  if(RETURN_ERROR(EFIerrno)) {
    return -1;
  }
  else {
    return Position;
  }
}

/* Write a NULL terminated WCS to the EFI console.

  NOTE: The UEFI Console is a wide device, _S_IWTTY, so characters received
        by da_ConWrite are WIDE characters.  It is the responsibility of the
        higher-level function(s) to perform any necessary conversions.

  @param[in,out]  BufferSize  Number of characters in Buffer.
  @param[in]      Buffer      The WCS string to be displayed

  @return   The number of Characters written.
*/
static
ssize_t
EFIAPI
da_ConWrite(
  IN  struct __filedes     *filp,
  IN  off_t                *Position,
  IN  size_t                BufferSize,
  IN  const void           *Buffer
  )
{
  EFI_STATUS                          Status;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *Proto;
  ConInstance                        *Stream;
  ssize_t                             NumChar;
  XY_OFFSET                          CursorPos;

  NumChar = -1;
  Stream = BASE_CR(filp->f_ops, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if(Stream->Cookie != CON_COOKIE) {    // Cookie == 'IoAb'
    EFIerrno = RETURN_INVALID_PARAMETER;
    return -1;    // Looks like a bad This pointer
  }
  if(Stream->InstanceNum == STDIN_FILENO) {
    // Write is not valid for stdin
    EFIerrno = RETURN_UNSUPPORTED;
    return -1;
  }
  // Everything is OK to do the write.
  Proto = (EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)Stream->Dev;

  Status = EFI_SUCCESS;
  if(Position != NULL) {
    CursorPos.Offset = *Position;

    Status = Proto->SetCursorPosition(Proto,
                                      (INTN)CursorPos.XYpos.Column,
                                      (INTN)CursorPos.XYpos.Row);

  }
  if(!RETURN_ERROR(Status)) {
  // Send the Unicode buffer to the console
    Status = Proto->OutputString( Proto, (CHAR16 *)Buffer);
  }

  // Depending on status, update BufferSize and return
  if(!RETURN_ERROR(Status)) {
    NumChar = BufferSize;
    Stream->NumWritten += NumChar;
  }
  EFIerrno = Status;      // Make error reason available to caller
  return NumChar;
}

/** Read a wide character from the console input device.

    Returns NUL or a translated input character.

    @param[in]      filp          Pointer to file descriptor for this file.
    @param[out]     Buffer        Buffer in which to place the read character.

    @retval    EFI_DEVICE_ERROR   A hardware error has occurred.
    @retval    EFI_NOT_READY      No data is available.  Try again later.
    @retval    EFI_SUCCESS        One wide character has been placed in Character
                                    - 0x0000  NUL, ignore this
                                    - Otherwise, should be a good wide character in Character
**/
static
EFI_STATUS
da_ConRawRead (
  IN OUT  struct __filedes   *filp,
     OUT  wchar_t            *Character
)
{
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *Proto;
  ConInstance                      *Stream;
  cIIO                             *Self;
  EFI_STATUS                        Status;
  EFI_INPUT_KEY                     Key = {0,0};
  wchar_t                           RetChar;

  Self    = (cIIO *)filp->devdata;
  Stream  = BASE_CR(filp->f_ops, ConInstance, Abstraction);
  Proto   = (EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)Stream->Dev;

  if(Stream->UnGetKey == CHAR_NULL) {
    Status = Proto->ReadKeyStroke(Proto, &Key);
  }
  else {
    Status  = EFI_SUCCESS;
    // Use the data in the Un-get buffer
    // Guaranteed that ScanCode and UnicodeChar are not both NUL
    Key.ScanCode        = SCAN_NULL;
    Key.UnicodeChar     = Stream->UnGetKey;
    Stream->UnGetKey    = CHAR_NULL;
  }
  if(Status == EFI_SUCCESS) {
    // Translate the Escape Scan Code to an ESC character
    if (Key.ScanCode != 0) {
      if (Key.ScanCode == SCAN_ESC) {
        RetChar = CHAR_ESC;
      }
      else if((Self->Termio.c_iflag & IGNSPEC) != 0) {
        // If we are ignoring special characters, return a NUL
        RetChar = 0;
      }
      else {
        // Must be a control, function, or other non-printable key.
        // Map it into the Platform portion of the Unicode private use area
        RetChar = TtyFunKeyMax - Key.ScanCode;
      }
    }
    else {
      RetChar = Key.UnicodeChar;
    }
    *Character = RetChar;
  }
  else {
    *Character = 0;
  }
  return Status;
}

/** Read a wide character from the console input device.

  NOTE: The UEFI Console is a wide device, _S_IWTTY, so characters returned
        by da_ConRead are WIDE characters.  It is the responsibility of the
        higher-level function(s) to perform any necessary conversions.

    A NUL character, 0x0000, is never returned.  In the event that such a character
    is encountered, the read is either retried or -1 is returned with errno set
    to EAGAIN.

    @param[in]      filp          Pointer to file descriptor for this file.
    @param[in]      offset        Ignored.
    @param[in]      BufferSize    Buffer size, in bytes.
    @param[out]     Buffer        Buffer in which to place the read characters.

    @retval    -1   An error has occurred.  Reason in errno and EFIerrno.
    @retval    -1   No data is available.  errno is set to EAGAIN
    @retval     1   One wide character has been placed in Buffer
**/
static
ssize_t
EFIAPI
da_ConRead(
  IN OUT  struct __filedes   *filp,
  IN OUT  off_t              *offset,         // Console ignores this
  IN      size_t              BufferSize,
     OUT  VOID               *Buffer
)
{
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *Proto;
  ConInstance                      *Stream;
  //cIIO                              *Self;
  EFI_STATUS                        Status;
  UINTN                             Edex;
  ssize_t                           NumRead;
  BOOLEAN                           BlockingMode;
  wchar_t                           RetChar;

  NumRead = -1;
  if(BufferSize < sizeof(wchar_t)) {
    errno = EINVAL;     // Buffer is too small to hold one character
  }
  else {
    Stream = BASE_CR(filp->f_ops, ConInstance, Abstraction);
    Proto = (EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)Stream->Dev;
    BlockingMode = (BOOLEAN)((filp->Oflags & O_NONBLOCK) == 0);

    do {
      Status = EFI_SUCCESS;
      if(BlockingMode) {
        // Read a byte in Blocking mode
        Status = gBS->WaitForEvent( 1, &Proto->WaitForKey, &Edex);
      }

      /*  WaitForEvent should not be able to fail since
            NumberOfEvents is set to constant 1 so is never 0
            Event is set by the Simple Text Input protocol so should never be EVT_NOTIFY_SIGNAL
            Current TPL should be TPL_APPLICATION.
          ASSERT so that we catch any problems during development.
      */
      ASSERT(Status == EFI_SUCCESS);

      Status = da_ConRawRead (filp, &RetChar);
    } while ( BlockingMode &&
             (RetChar == 0) &&
             (Status != EFI_DEVICE_ERROR));

    EFIerrno = Status;
    if(Status == EFI_SUCCESS) {
      // Got a keystroke.
      NumRead = 1;   // Indicate that Key holds the data
    }
    else if(Status == EFI_NOT_READY) {
      // Keystroke data is not available
      errno = EAGAIN;
    }
    else {
      // Hardware error
      errno = EIO;
    }
    if (RetChar == 0) {
      NumRead = -1;
      errno = EAGAIN;
    }
    else {
      *((wchar_t *)Buffer) = RetChar;
    }
  }
  return NumRead;
}

/** Console-specific helper function for the fstat() function.

    st_size       Set to number of characters read for stdin and number written for stdout and stderr.
    st_physsize   1 for stdin, 0 if QueryMode error, else max X and Y coordinates for the current mode.
    st_curpos     0 for stdin, current X & Y coordinates for stdout and stderr
    st_blksize    Set to 1 since this is a character device

    All other members of the stat structure are left unchanged.

    @param[in]      filp          Pointer to file descriptor for this file.
    @param[out]     Buffer        Pointer to a stat structure to receive the information.
    @param[in,out]  Something     Ignored.

    @retval   0   Successful completion.
    @retval   -1  Either filp is not associated with a console stream, or
                  Buffer is NULL.  errno is set to EINVAL.
**/
static
int
EFIAPI
da_ConStat(
  struct __filedes   *filp,
  struct stat        *Buffer,
  void               *Something
  )
{
  ConInstance                        *Stream;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *Proto;
  XY_OFFSET                           CursorPos;
  INT32                               OutMode;
  UINTN                               ModeCol;
  UINTN                               ModeRow;

// ConGetInfo
  Stream = BASE_CR(filp->f_ops, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if ((Stream->Cookie != CON_COOKIE) ||    // Cookie == 'IoAb'
      (Buffer == NULL))
  {
    errno     = EINVAL;
    EFIerrno = RETURN_INVALID_PARAMETER;
    return -1;
  }
  // All of our parameters are correct, so fill in the information.
  Buffer->st_blksize  = 0;   // Character device, not a block device
  Buffer->st_mode     = filp->f_iflags;

// ConGetPosition
  if(Stream->InstanceNum == STDIN_FILENO) {
    // This is stdin
    Buffer->st_curpos    = 0;
    Buffer->st_size      = (off_t)Stream->NumRead;
    Buffer->st_physsize  = 1;
  }
  else {
    Proto = (EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)Stream->Dev;
    CursorPos.XYpos.Column  = (UINT32)Proto->Mode->CursorColumn;
    CursorPos.XYpos.Row     = (UINT32)Proto->Mode->CursorRow;
    Buffer->st_curpos       = (off_t)CursorPos.Offset;
    Buffer->st_size         = (off_t)Stream->NumWritten;

    OutMode  = Proto->Mode->Mode;
    EFIerrno = Proto->QueryMode(Proto, (UINTN)OutMode, &ModeCol, &ModeRow);
    if(RETURN_ERROR(EFIerrno)) {
      Buffer->st_physsize = 0;
    }
    else {
      CursorPos.XYpos.Column  = (UINT32)ModeCol;
      CursorPos.XYpos.Row     = (UINT32)ModeRow;
      Buffer->st_physsize     = (off_t)CursorPos.Offset;
    }
  }
  return 0;
}

/** Console-specific helper for the ioctl system call.

    The console device does not directly participate in ioctl operations.
    This function completes the device abstraction and returns an error value
    to indicate that the function is not supported for this device.

    @retval   -1    Function is not supported for this device.
**/
static
int
EFIAPI
da_ConIoctl(
  struct __filedes   *filp,
  ULONGN              cmd,
  va_list             argp
  )
{
  errno   = ENODEV;
  return  -1;
}

/** Open an abstract Console Device.

    @param[in]    DevNode       Pointer to the Device control structure for this stream.
    @param[in]    filp          Pointer to the new file control structure for this stream.
    @param[in]    DevInstance   Not used for the console device.
    @param[in]    Path          Not used for the console device.
    @param[in]    MPath         Not used for the console device.

    @retval   0   This console stream has been successfully opened.
    @retval   -1  The DevNode or filp pointer is NULL.
    @retval   -1  DevNode does not point to a valid console stream device.
**/
int
EFIAPI
da_ConOpen(
  DeviceNode         *DevNode,
  struct __filedes   *filp,
  int                 DevInstance,    // Not used for console devices
  wchar_t            *Path,           // Not used for console devices
  wchar_t            *MPath           // Not used for console devices
  )
{
  ConInstance    *Stream;
  UINT32          Instance;
  int             RetVal = -1;

  if((filp    != NULL)    &&
      (DevNode != NULL))
  {
  Stream = (ConInstance *)DevNode->InstanceList;
  // Quick check to see if Stream looks reasonable
    if(Stream->Cookie == CON_COOKIE)
    {
      Instance = Stream->InstanceNum;
      if(Instance < NUM_SPECIAL) {
        gMD->StdIo[Instance] = Stream;
        filp->f_iflags |= (_S_IFCHR | _S_ITTY | _S_IWTTY | _S_ICONSOLE);
        filp->f_offset = 0;
        filp->f_ops = &Stream->Abstraction;
        filp->devdata = (void *)IIO;
        RetVal = 0;
      }
    }
  }
  if (RetVal < 0) {
    EFIerrno = RETURN_INVALID_PARAMETER;
    errno = EINVAL;
  }
  return RetVal;

}

/** Flush a console device's IIO buffers.

    Flush the IIO Input or Output buffers associated with the specified file.

    If the console is open for output, write any unwritten data in the associated
    output buffer (stdout or stderr) to the console.

    If the console is open for input, discard any remaining data
    in the input buffer.

    @param[in]    filp    Pointer to the target file's descriptor structure.

    @retval     0     Always succeeds
**/
static
int
EFIAPI
da_ConFlush(
  struct __filedes *filp
)
{
  cFIFO      *OutBuf;
  ssize_t     NumProc;
  int         Flags;


    if(filp->MyFD == STDERR_FILENO) {
      OutBuf = IIO->ErrBuf;
    }
    else {
      OutBuf = IIO->OutBuf;
    }

    Flags = filp->Oflags & O_ACCMODE;   // Get the device's open mode
    if (Flags != O_WRONLY)  {   // (Flags == O_RDONLY) || (Flags == O_RDWR)
      // Readable so discard the contents of the input buffer
      IIO->InBuf->Flush(IIO->InBuf, UNICODE_STRING_MAX);
    }
    if (Flags != O_RDONLY)  {   // (Flags == O_WRONLY) || (Flags == O_RDWR)
      // Writable so flush the output buffer
      // At this point, the characters to write are in OutBuf
      // First, linearize and consume the buffer
      NumProc = OutBuf->Read(OutBuf, gMD->UString, UNICODE_STRING_MAX-1);
      if (NumProc > 0) {  // Optimization -- Nothing to do if no characters
        gMD->UString[NumProc] = 0;   // Ensure that the buffer is terminated

        /*  OutBuf always contains wide characters.
            The UEFI Console (this device) always expects wide characters.
            There is no need to handle devices that expect narrow characters
            like the device-independent functions do.
        */
        // Do the actual write of the data to the console
        (void) da_ConWrite(filp, NULL, NumProc, gMD->UString);
        // Paranoia -- Make absolutely sure that OutBuf is empty in case fo_write
        // wasn't able to consume everything.
        OutBuf->Flush(OutBuf, UNICODE_STRING_MAX);
      }
    }
  return 0;
}

/** Close an open file.

    @param[in]  filp    Pointer to the file descriptor structure for this file.

    @retval   0     The file has been successfully closed.
    @retval   -1    filp does not point to a valid console descriptor.
**/
static
int
EFIAPI
da_ConClose(
  IN      struct __filedes   *filp
)
{
  ConInstance    *Stream;

  Stream = BASE_CR(filp->f_ops, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if(Stream->Cookie != CON_COOKIE) {    // Cookie == 'IoAb'
    errno     = EINVAL;
    EFIerrno = RETURN_INVALID_PARAMETER;
    return -1;    // Looks like a bad File Descriptor pointer
  }
  // Stream and filp look OK, so continue.
  // Flush the I/O buffers
  (void) da_ConFlush(filp);

  // Break the connection to IIO
  filp->devdata = NULL;

  gMD->StdIo[Stream->InstanceNum] = NULL;   // Mark the stream as closed
  return 0;
}

#include  <sys/poll.h>
/*  Returns a bit mask describing which operations could be completed immediately.

    Testable Events for this device are:
    (POLLIN | POLLRDNORM)   A Unicode character is available to read
    (POLLIN)                A ScanCode is ready.
    (POLLOUT)               The device is ready for output - always set on stdout and stderr.

    Non-testable Events which are only valid in return values are:
      POLLERR                 The specified device is not one of stdin, stdout, or stderr.
      POLLHUP                 The specified stream has been disconnected
      POLLNVAL                da_ConPoll was called with an invalid parameter.

  NOTE: The "Events" handled by this function are not UEFI events.

    @param[in]  filp      Pointer to the file control structure for this stream.
    @param[in]  events    A bit mask identifying the events to be examined
                          for this device.

    @return   Returns a bit mask comprised of both testable and non-testable
              event codes indicating both the state of the operation and the
              status of the device.
*/
static
short
EFIAPI
da_ConPoll(
  struct __filedes   *filp,
  short              events
  )
{
  ConInstance                      *Stream;
  EFI_STATUS                        Status = RETURN_SUCCESS;
  short                             RdyMask = 0;

  Stream = BASE_CR(filp->f_ops, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if(Stream->Cookie != CON_COOKIE) {    // Cookie == 'IoAb'
    errno     = EINVAL;
    EFIerrno = RETURN_INVALID_PARAMETER;
    return POLLNVAL;    // Looks like a bad filp pointer
  }
  if(Stream->InstanceNum == 0) {
    // STDIN: Only input is supported for this device
    Status = da_ConRawRead (filp, &Stream->UnGetKey);
    if(Status == RETURN_SUCCESS) {
      RdyMask = POLLIN;
      if ((Stream->UnGetKey <  TtyFunKeyMin)   ||
          (Stream->UnGetKey >= TtyFunKeyMax))
      {
        RdyMask |= POLLRDNORM;
      }
    }
    else {
      Stream->UnGetKey  = CHAR_NULL;
    }
  }
  else if(Stream->InstanceNum < NUM_SPECIAL) {  // Not 0, is it 1 or 2?
    // (STDOUT || STDERR): Only output is supported for this device
    RdyMask = POLLOUT;
  }
  else {
    RdyMask = POLLERR;    // Not one of the standard streams
  }
  EFIerrno = Status;

  return (RdyMask & (events | POLL_RETONLY));
}

/** Construct the Console stream devices: stdin, stdout, stderr.

    Allocate the instance structure and populate it with the information for
    each stream device.
**/
RETURN_STATUS
EFIAPI
__Cons_construct(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
  ConInstance    *Stream;
  RETURN_STATUS   Status;
  int             i;

  Status = RETURN_OUT_OF_RESOURCES;
  ConInstanceList = (ConInstance *)AllocateZeroPool(NUM_SPECIAL * sizeof(ConInstance));
  if(ConInstanceList != NULL) {
    IIO = New_cIIO();
    if(IIO == NULL) {
      FreePool(ConInstanceList);
    }
    else {
      Status = RETURN_SUCCESS;
      for( i = 0; i < NUM_SPECIAL; ++i) {
        // Get pointer to instance.
        Stream = &ConInstanceList[i];

        Stream->Cookie      = CON_COOKIE;
        Stream->InstanceNum = i;
        Stream->CharState.A = 0;    // Start in the initial state

        switch(i) {
          case STDIN_FILENO:
            Stream->Dev = SystemTable->ConIn;
            break;
          case STDOUT_FILENO:
            Stream->Dev = SystemTable->ConOut;
            break;
          case STDERR_FILENO:
            if(SystemTable->StdErr == NULL) {
              Stream->Dev = SystemTable->ConOut;
            }
            else {
              Stream->Dev = SystemTable->StdErr;
            }
            break;
          default:
            return RETURN_VOLUME_CORRUPTED;     // This is a "should never happen" case.
        }

        Stream->Abstraction.fo_close    = &da_ConClose;
        Stream->Abstraction.fo_read     = &da_ConRead;
        Stream->Abstraction.fo_write    = &da_ConWrite;
        Stream->Abstraction.fo_stat     = &da_ConStat;
        Stream->Abstraction.fo_lseek    = &da_ConSeek;
        Stream->Abstraction.fo_fcntl    = &fnullop_fcntl;
        Stream->Abstraction.fo_ioctl    = &da_ConIoctl;
        Stream->Abstraction.fo_poll     = &da_ConPoll;
        Stream->Abstraction.fo_flush    = &da_ConFlush;
        Stream->Abstraction.fo_delete   = &fbadop_delete;
        Stream->Abstraction.fo_mkdir    = &fbadop_mkdir;
        Stream->Abstraction.fo_rmdir    = &fbadop_rmdir;
        Stream->Abstraction.fo_rename   = &fbadop_rename;

        Stream->NumRead     = 0;
        Stream->NumWritten  = 0;
        Stream->UnGetKey    = CHAR_NULL;

        if(Stream->Dev == NULL) {
          continue;                 // No device for this stream.
        }
            ConNode[i] = __DevRegister(stdioNames[i], NULL, &da_ConOpen, Stream,
                                       1, sizeof(ConInstance), stdioFlags[i]);
        if(ConNode[i] == NULL) {
              Status = EFIerrno;    // Grab error code that DevRegister produced.
          break;
        }
        Stream->Parent = ConNode[i];
      }
      /* Initialize Ioctl flags until Ioctl is really implemented. */
      TtyCooked = TRUE;
      TtyEcho   = TRUE;
    }
  }
  return  Status;
}

RETURN_STATUS
EFIAPI
__Cons_deconstruct(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
  int   i;

  for(i = 0; i < NUM_SPECIAL; ++i) {
    if(ConNode[i] != NULL) {
      FreePool(ConNode[i]);
    }
  }
  if(ConInstanceList != NULL) {
    FreePool(ConInstanceList);
  }
  if(IIO != NULL) {
    IIO->Delete(IIO);
    IIO = NULL;
  }

  return RETURN_SUCCESS;
}

/* ######################################################################### */
#if 0 /* Not implemented (yet?) for Console */

static
int
EFIAPI
da_ConCntl(
  struct __filedes *filp,
  UINT32,
  void *,
  void *
  )
{
}
#endif  /* Not implemented for Console */
