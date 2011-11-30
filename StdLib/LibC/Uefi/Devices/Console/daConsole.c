/** @file
  Abstract device driver for the UEFI Console.

  Manipulates abstractions for stdin, stdout, stderr.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
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
#include  <Protocol/SimpleTextIn.h>
#include  <Protocol/SimpleTextOut.h>

#include  <LibConfig.h>

#include  <errno.h>
#include  <wctype.h>
#include  <wchar.h>
#include  <stdarg.h>
#include  <sys/fcntl.h>
#include  <unistd.h>
#include  <kfile.h>
#include  <Device/Device.h>
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

static wchar_t       *ConReadBuf;

/* Flags settable by Ioctl */
static BOOLEAN        TtyCooked;
static BOOLEAN        TtyEcho;

ssize_t
WideTtyCvt( CHAR16 *dest, const char *buf, size_t n)
{
  UINTN   i;
  wint_t  wc;

  for(i = 0; i < n; ++i) {
    wc = btowc(*buf++);
    if( wc == 0) {
      break;
    };
    if(wc < 0) {
      wc = BLOCKELEMENT_LIGHT_SHADE;
    }
    if(wc == L'\n') {
      *dest++ = L'\r';
    }
    *dest++ = (CHAR16)wc;
  }
  *dest = 0;
  return (ssize_t)i;
}

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
    EFIerrno = RETURN_INVALID_PARAMETER;
    return -1;    // Looks like a bad File Descriptor pointer
  }
  gMD->StdIo[Stream->InstanceNum] = NULL;   // Mark the stream as closed
  return RETURN_SUCCESS;
}

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
  XYoffset                           CursorPos;

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

  @param[in,out]  BufferSize  Number of bytes in Buffer.  Set to zero if
                              the string couldn't be displayed.
  @param[in]      Buffer      The WCS string to be displayed

  @return   The number of characters written.
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
  //XYoffset                            CursorPos;

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

  // Convert string from MBCS to WCS and translate \n to \r\n.
  NumChar = WideTtyCvt(gMD->UString, (const char *)Buffer, BufferSize);
  //if(NumChar > 0) {
  //  BufferSize = (size_t)(NumChar * sizeof(CHAR16));
  //}
  BufferSize = NumChar;

  //if( Position != NULL) {
  //  CursorPos.Offset = (UINT64)*Position;

  //  Status = Proto->SetCursorPosition(Proto,
  //                                    (INTN)CursorPos.XYpos.Column,
  //                                    (INTN)CursorPos.XYpos.Row);
  //  if(RETURN_ERROR(Status)) {
  //    return -1;
  //  }
  //}

  // Send the Unicode buffer to the console
  Status = Proto->OutputString( Proto, gMD->UString);
  // Depending on status, update BufferSize and return
  if(RETURN_ERROR(Status)) {
    BufferSize = 0;    // We don't really know how many characters made it out
  }
  else {
    //BufferSize = NumChar;
    Stream->NumWritten += NumChar;
  }
  EFIerrno = Status;
  return BufferSize;
}

/** Read characters from the console input device.

    @param[in,out]  filp          Pointer to file descriptor for this file.
    @param[in,out]  offset        Ignored.
    @param[in]      BufferSize    Buffer size, in bytes.
    @param[out]     Buffer        Buffer in which to place the read characters.

    @return     Number of bytes actually placed into Buffer.

    @todo       Handle encodings other than ASCII-7 and UEFI.
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
  wchar_t                          *OutPtr;
  EFI_INPUT_KEY                     Key;
  UINTN                             NumChar;
  UINTN                             Edex;
  EFI_STATUS                        Status = RETURN_SUCCESS;
  UINTN                             i;
  char                              EchoBuff[MB_CUR_MAX + 1];
  int                               NumEcho;

  Stream = BASE_CR(filp->f_ops, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if(Stream->Cookie != CON_COOKIE) {    // Cookie == 'IoAb'
    EFIerrno = RETURN_INVALID_PARAMETER;
    return -1;    // Looks like a bad This pointer
  }
  if(Stream->InstanceNum != STDIN_FILENO) {
    // Read only valid for stdin
    EFIerrno = RETURN_UNSUPPORTED;
    return -1;
  }
  // It looks like things are OK for trying to read
  // We will accumulate *BufferSize characters or until we encounter
  // an "activation" character.  Currently any control character.
  Proto = (EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)Stream->Dev;
  OutPtr = ConReadBuf;
  NumChar = (BufferSize > MAX_INPUT)? MAX_INPUT : BufferSize;
  i = 0;
  do {
    if((Stream->UnGetKey.UnicodeChar == CHAR_NULL) && (Stream->UnGetKey.ScanCode == SCAN_NULL)) {
      Status = gBS->WaitForEvent( 1, &Proto->WaitForKey, &Edex);
      if(Status != RETURN_SUCCESS) {
        break;
      }
      Status = Proto->ReadKeyStroke(Proto, &Key);
      if(Status != RETURN_SUCCESS) {
        break;
      }
    }
    else {
      Key.ScanCode          = Stream->UnGetKey.ScanCode;
      Key.UnicodeChar       = Stream->UnGetKey.UnicodeChar;
      Stream->UnGetKey.ScanCode     = SCAN_NULL;
      Stream->UnGetKey.UnicodeChar  = CHAR_NULL;
    }
    if(Key.ScanCode == SCAN_NULL) {
      NumEcho = 0;
      if(TtyCooked && (Key.UnicodeChar == CHAR_CARRIAGE_RETURN)) {
        *OutPtr++ = CHAR_LINEFEED;
        NumEcho = wctomb(EchoBuff, CHAR_LINEFEED);
      }
      else {
        *OutPtr++ = Key.UnicodeChar;
        NumEcho = wctomb(EchoBuff, Key.UnicodeChar);
      }
      ++i;
      EchoBuff[NumEcho] = 0;  /* Terminate the Echo buffer */
      if(TtyEcho) {
        /* Echo the character just input */
        da_ConWrite(&gMD->fdarray[STDOUT_FILENO], NULL, 2, EchoBuff);
      }
    }
    if(iswcntrl(Key.UnicodeChar)) {    // If a control character, or a scan code
      break;
    }
  } while(i < NumChar);

  *OutPtr = L'\0';    // Terminate the input buffer

  /*  Convert the input buffer and place in Buffer.
      If the fully converted input buffer won't fit, write what will and
      leave the rest in ConReadBuf with ConReadLeft indicating how many
      unconverted characters remain in ConReadBuf.
  */
  NumEcho = (int)wcstombs(Buffer, ConReadBuf, BufferSize);   /* Re-use NumEcho to hold number of bytes in Buffer */
  /* More work needs to be done before locales other than C can be supported. */

  EFIerrno = Status;
  return (ssize_t)NumEcho;  // Will be 0 if we didn't get a key
}

/** Console-specific helper function for the fstat() function.

    st_size       Set to number of characters read for stdin and number written for stdout and stderr.
    st_physsize   1 for stdin, 0 if QueryMode error, else max X and Y coordinates for the current mode.
    st_curpos     0 for stdin, current X & Y coordinates for stdout and stderr
    st_blksize    Set to 1 since this is a character device

    All other members of the stat structure are left unchanged.
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
  XYoffset                            CursorPos;
  INT32                               OutMode;
  UINTN                               ModeCol;
  UINTN                               ModeRow;

// ConGetInfo
  Stream = BASE_CR(filp->f_ops, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if ((Stream->Cookie != CON_COOKIE) ||    // Cookie == 'IoAb'
      (Buffer == NULL))
  {
    EFIerrno = RETURN_INVALID_PARAMETER;
    return -1;
  }
  // All of our parameters are correct, so fill in the information.
  Buffer->st_blksize = 1;

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

static
int
EFIAPI
da_ConIoctl(
  struct __filedes   *filp,
  ULONGN              cmd,
  va_list             argp
  )
{
  return -EPERM;
}

/** Open an abstract Console Device.
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
  ConInstance                      *Stream;

  if((filp == NULL)           ||
     (DevNode  == NULL))
  {
    EFIerrno = RETURN_INVALID_PARAMETER;
    errno = EINVAL;
    return -1;
  }
  Stream = (ConInstance *)DevNode->InstanceList;
  // Quick check to see if Stream looks reasonable
  if(Stream->Cookie != CON_COOKIE) {    // Cookie == 'IoAb'
    EFIerrno = RETURN_INVALID_PARAMETER;
    errno = EINVAL;
    return -1;    // Looks like a bad This pointer
  }
  gMD->StdIo[Stream->InstanceNum] = Stream;
  filp->f_iflags |= (S_IFREG | _S_IFCHR | _S_ICONSOLE);
  filp->f_offset = 0;
  filp->f_ops = &Stream->Abstraction;

  return 0;
}

#include  <sys/poll.h>
/*  Returns a bit mask describing which operations could be completed immediately.

    (POLLIN | POLLRDNORM)   A Unicode character is available to read
    (POLLIN)                A ScanCode is ready.
    (POLLOUT)               The device is ready for output - always set on stdout and stderr.

*/
static
short
EFIAPI
da_ConPoll(
  struct __filedes   *filp,
  short              events
  )
{
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *Proto;
  ConInstance                      *Stream;
  EFI_STATUS                        Status = RETURN_SUCCESS;
  short                             RdyMask = 0;

  Stream = BASE_CR(filp->f_ops, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if(Stream->Cookie != CON_COOKIE) {    // Cookie == 'IoAb'
    EFIerrno = RETURN_INVALID_PARAMETER;
    return POLLNVAL;    // Looks like a bad filp pointer
  }
  if(Stream->InstanceNum == 0) {
    // Only input is supported for this device
    Proto = (EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)Stream->Dev;
    if((Stream->UnGetKey.UnicodeChar == CHAR_NULL) && (Stream->UnGetKey.ScanCode == SCAN_NULL)) {
      Status = Proto->ReadKeyStroke(Proto, &Stream->UnGetKey);
      if(Status == RETURN_SUCCESS) {
        RdyMask = POLLIN;
        if(Stream->UnGetKey.UnicodeChar != CHAR_NULL) {
          RdyMask |= POLLRDNORM;
        }
      }
      else {
        Stream->UnGetKey.ScanCode     = SCAN_NULL;
        Stream->UnGetKey.UnicodeChar  = CHAR_NULL;
      }
    }
  }
  else if(Stream->InstanceNum < NUM_SPECIAL) {  // Not 0, is it 1 or 2?
    // Only output is supported for this device
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
  RETURN_STATUS   Status = RETURN_SUCCESS;
  int             i;

  ConInstanceList = (ConInstance *)AllocateZeroPool(NUM_SPECIAL * sizeof(ConInstance));
  ConReadBuf      = (wchar_t *)AllocateZeroPool((MAX_INPUT + 1) * sizeof(wchar_t));
  if((ConInstanceList == NULL) || (ConReadBuf == NULL))  {
    return RETURN_OUT_OF_RESOURCES;
  }

  for( i = 0; i < NUM_SPECIAL; ++i) {
    // Get pointer to instance.
    Stream = &ConInstanceList[i];

    Stream->Cookie      = CON_COOKIE;
    Stream->InstanceNum = i;

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
    Stream->Abstraction.fo_flush    = &fnullop_flush;
    Stream->Abstraction.fo_delete   = &fbadop_delete;
    Stream->Abstraction.fo_mkdir    = &fbadop_mkdir;
    Stream->Abstraction.fo_rmdir    = &fbadop_rmdir;
    Stream->Abstraction.fo_rename   = &fbadop_rename;

    Stream->NumRead     = 0;
    Stream->NumWritten  = 0;
    Stream->UnGetKey.ScanCode     = SCAN_NULL;
    Stream->UnGetKey.UnicodeChar  = CHAR_NULL;

    if(Stream->Dev == NULL) {
      continue;                 // No device for this stream.
    }
    ConNode[i] = __DevRegister(stdioNames[i], NULL, &da_ConOpen, Stream, 1, sizeof(ConInstance), stdioFlags[i]);
    if(ConNode[i] == NULL) {
      Status = EFIerrno;
      break;
    }
    Stream->Parent = ConNode[i];
  }
  /* Initialize Ioctl flags until Ioctl is really implemented. */
  TtyCooked = TRUE;
  TtyEcho   = TRUE;

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
  if(ConReadBuf != NULL) {
    FreePool(ConReadBuf);
  }

  return RETURN_SUCCESS;
}

/* ######################################################################### */
#if 0 /* Not implemented for Console */

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

static
int
EFIAPI
da_ConFlush(
  struct __filedes *filp
  )
{
  return 0;
}
#endif  /* Not implemented for Console */
