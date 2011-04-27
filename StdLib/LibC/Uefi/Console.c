/** @file
  File abstractions of the console.

  Manipulates EFI_FILE_PROTOCOL abstractions for stdin, stdout, stderr.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include  <Uefi.h>
#include  <Library/BaseLib.h>
#include  <Library/BaseMemoryLib.h>
#include  <Library/MemoryAllocationLib.h>
#include  <Library/UefiBootServicesTableLib.h>

#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  <stdio.h>
#include  <sys/fcntl.h>
#include  <MainData.h>
#include  <Efi/Console.h>

static const CHAR16 * stdioNames[NUM_SPECIAL]   = {
  L"stdin:", L"stdout:", L"stderr:"
};
static const int stdioFlags[NUM_SPECIAL] = {
  O_RDONLY,             // stdin
  O_WRONLY,             // stdout
  O_WRONLY              // stderr
};

static const int  stdioMode[NUM_SPECIAL] = {
  0444,   // stdin
  0222,   // stdout
  0222    // stderr
};

static
EFI_STATUS
EFIAPI
ConClose(
  IN EFI_FILE_PROTOCOL         *This
  )
{
  ConInstance    *Stream;

  Stream = BASE_CR(This, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if(Stream->Cookie != 0x62416F49) {    // Cookie == 'IoAb'
    return RETURN_INVALID_PARAMETER;    // Looks like a bad This pointer
  }
  // Nothing to Flush.
  // Mark the Stream as closed.
  Stream->Dev = NULL;

  return RETURN_SUCCESS;
}

static
EFI_STATUS
EFIAPI
ConDelete(
  IN EFI_FILE_PROTOCOL         *This
  )
{
  return RETURN_UNSUPPORTED;
}

static
EFI_STATUS
EFIAPI
ConRead(
  IN EFI_FILE_PROTOCOL         *This,
  IN OUT UINTN                 *BufferSize,
  OUT VOID                     *Buffer
  )
{
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *Proto;
  ConInstance    *Stream;
  CHAR16         *OutPtr;
  EFI_INPUT_KEY   Key;
  UINTN           NumChar;
  UINTN           Edex;
  EFI_STATUS      Status;
  UINTN           i;

  Stream = BASE_CR(This, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if(Stream->Cookie != 0x62416F49) {    // Cookie == 'IoAb'
    return RETURN_INVALID_PARAMETER;    // Looks like a bad This pointer
  }
  if(Stream->Dev == NULL) {
    // Can't read from an unopened Stream
    return RETURN_DEVICE_ERROR;
  }
  if(Stream != &gMD->StdIo[0]) {
    // Read only valid for stdin
    return RETURN_UNSUPPORTED;
  }
  // It looks like things are OK for trying to read
  // We will accumulate *BufferSize characters or until we encounter
  // an "activation" character.  Currently any control character.
  Proto = (EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)Stream->Dev;
  OutPtr = Buffer;
  NumChar = (*BufferSize - 1) / sizeof(CHAR16);
  i = 0;
  do {
    Status = gBS->WaitForEvent( 1, &Proto->WaitForKey, &Edex);
    if(Status != EFI_SUCCESS) {
      break;
    }
    Status = Proto->ReadKeyStroke(Proto, &Key);
    if(Status != EFI_SUCCESS) {
      break;
    }
    if(Key.ScanCode == SCAN_NULL) {
      *OutPtr++ = Key.UnicodeChar;
      ++i;
    }
    if(Key.UnicodeChar < 0x0020) {    // If a control character, or a scan code
      break;
    }
  } while(i < NumChar);
  *BufferSize = i * sizeof(CHAR16);
  return Status;
}

/* Write a NULL terminated WCS to the EFI console.

  @param[in,out]  BufferSize  Number of bytes in Buffer.  Set to zero if
                              the string couldn't be displayed.
  @parem[in]      Buffer      The WCS string to be displayed

*/
static
EFI_STATUS
EFIAPI
ConWrite(
  IN EFI_FILE_PROTOCOL         *This,
  IN OUT UINTN                 *BufferSize,
  IN VOID                      *Buffer
  )
{
  EFI_STATUS      Status;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *Proto;
  ConInstance    *Stream;
  CHAR16         *MyBuf;
  UINTN           NumChar;

  Stream = BASE_CR(This, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if(Stream->Cookie != 0x62416F49) {    // Cookie == 'IoAb'
    return RETURN_INVALID_PARAMETER;    // Looks like a bad This pointer
  }
  if(Stream->Dev == NULL) {
    // Can't write to an unopened Stream
    return RETURN_DEVICE_ERROR;
  }
  if(Stream == &gMD->StdIo[0]) {
    // Write is not valid for stdin
    return RETURN_UNSUPPORTED;
  }
  // Everything is OK to do the write.
  Proto = (EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)Stream->Dev;
  MyBuf = (CHAR16 *)Buffer;
  NumChar = *BufferSize;

  // Send MyBuf to the console
  Status = Proto->OutputString( Proto, MyBuf);
  // Depending on status, update BufferSize and return
  if(Status != EFI_SUCCESS) {
    *BufferSize = 0;    // We don't really know how many characters made it out
  }
  else {
    *BufferSize = NumChar;
    Stream->NumWritten += NumChar;
  }
  return Status;
}

static
EFI_STATUS
EFIAPI
ConGetPosition(
  IN EFI_FILE_PROTOCOL         *This,
  OUT UINT64                   *Position
  )
{
  ConInstance                       *Stream;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *Proto;
  XYoffset                           CursorPos;

  Stream = BASE_CR(This, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if(Stream->Cookie != 0x62416F49) {    // Cookie == 'IoAb'
    return RETURN_INVALID_PARAMETER;    // Looks like a bad This pointer
  }
  if(Stream == &gMD->StdIo[0]) {
    // This is stdin
    *Position = Stream->NumRead;
  }
  else {
    Proto = (EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)Stream->Dev;
    CursorPos.XYpos.Column = (UINT32)Proto->Mode->CursorColumn;
    CursorPos.XYpos.Row    = (UINT32)Proto->Mode->CursorRow;
    *Position = CursorPos.Offset;
  }
  return RETURN_SUCCESS;
}

static
EFI_STATUS
EFIAPI
ConSetPosition(
  IN EFI_FILE_PROTOCOL         *This,
  IN UINT64                     Position
  )
{
  ConInstance                       *Stream;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *Proto;
  XYoffset                           CursorPos;

  Stream = BASE_CR(This, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if(Stream->Cookie != 0x62416F49) {    // Cookie == 'IoAb'
    return RETURN_INVALID_PARAMETER;    // Looks like a bad This pointer
  }
  if(Stream->Dev == NULL) {
    // Can't write to an unopened Stream
    return RETURN_DEVICE_ERROR;
  }
  if(Stream == &gMD->StdIo[0]) {
    // Seek is not valid for stdin
    return RETURN_UNSUPPORTED;
  }
  // Everything is OK to do the final verification and "seek".
  Proto = (EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)Stream->Dev;
  CursorPos.Offset = Position;

  return Proto->SetCursorPosition(Proto,
                                  (INTN)CursorPos.XYpos.Column,
                                  (INTN)CursorPos.XYpos.Row);
}

static
EFI_STATUS
EFIAPI
ConGetInfo(
  IN EFI_FILE_PROTOCOL         *This,
  IN EFI_GUID                  *InformationType,
  IN OUT UINTN                 *BufferSize,
  OUT VOID                     *Buffer
  )
{
  EFI_FILE_INFO       *InfoBuf;
  ConInstance         *Stream;

  Stream = BASE_CR(This, ConInstance, Abstraction);
  // Quick check to see if Stream looks reasonable
  if((Stream->Cookie != 0x62416F49) ||    // Cookie == 'IoAb'
     (Buffer == NULL))
  {
    return RETURN_INVALID_PARAMETER;
  }
  if(*BufferSize < sizeof(EFI_FILE_INFO)) {
    *BufferSize = sizeof(EFI_FILE_INFO);
    return RETURN_BUFFER_TOO_SMALL;
  }
  // All of our parameters are correct, so fill in the information.
  (void) ZeroMem(Buffer, sizeof(EFI_FILE_INFO));
  InfoBuf = (EFI_FILE_INFO *)Buffer;
  InfoBuf->Size = sizeof(EFI_FILE_INFO);
  InfoBuf->FileSize = 1;
  InfoBuf->PhysicalSize = 1;
  *BufferSize = sizeof(EFI_FILE_INFO);

  return RETURN_SUCCESS;
}

static
EFI_STATUS
EFIAPI
ConSetInfo(
  IN EFI_FILE_PROTOCOL         *This,
  IN EFI_GUID                  *InformationType,
  IN UINTN                      BufferSize,
  IN VOID                      *Buffer
  )
{
  return RETURN_UNSUPPORTED;
}

static
EFI_STATUS
EFIAPI
ConFlush(
  IN EFI_FILE_PROTOCOL         *This
  )
{
  return RETURN_SUCCESS;
}

EFI_STATUS
EFIAPI
ConOpen(
  IN  EFI_FILE_PROTOCOL        *This,
  OUT EFI_FILE_PROTOCOL       **NewHandle,
  IN  CHAR16                   *FileName,
  IN  UINT64                    OpenMode,   // Ignored
  IN  UINT64                    Attributes  // Ignored
  )
{
  ConInstance                      *Stream;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Proto;
  UINTN                             Columns;
  UINTN                             Rows;
  int                               i;

  if((NewHandle == NULL)  ||
     (FileName  == NULL))
  {
    return RETURN_INVALID_PARAMETER;
  }
  // Process FileName
  for(i = 0; i < NUM_SPECIAL; ++i) {
    if(StrCmp( stdioNames[i], FileName) == 0) {
      break;
    }
  }
  if(i >= NUM_SPECIAL) {
    return RETURN_NO_MAPPING;
  }
  // Get pointer to instance.
  Stream = &gMD->StdIo[i];
  if(Stream->Dev == NULL) {
    // If this stream has been closed, then
    // Initialize instance.
    Stream->Cookie      = 0x62416F49;
    Stream->NumRead     = 0;
    Stream->NumWritten  = 0;
    switch(i) {
      case 0:
        Stream->Dev = gST->ConIn;
        break;
      case 1:
        Stream->Dev = gST->ConOut;
        break;
      case 2:
        if(gST->StdErr == NULL) {
          Stream->Dev = gST->ConOut;
        }
        else {
          Stream->Dev = gST->StdErr;
        }
        break;
      default:
        return RETURN_VOLUME_CORRUPTED;     // This is a "should never happen" case.
    }
    Stream->Abstraction.Revision      = 0x00010000;
    Stream->Abstraction.Open          = &ConOpen;
    Stream->Abstraction.Close         = &ConClose;
    Stream->Abstraction.Delete        = &ConDelete;
    Stream->Abstraction.Read          = &ConRead;
    Stream->Abstraction.Write         = &ConWrite;
    Stream->Abstraction.GetPosition   = &ConGetPosition;
    Stream->Abstraction.SetPosition   = &ConSetPosition;
    Stream->Abstraction.GetInfo       = &ConGetInfo;
    Stream->Abstraction.SetInfo       = &ConSetInfo;
    Stream->Abstraction.Flush         = &ConFlush;
    // Get additional information if this is an Output stream
    if(i != 0) {
      Proto = Stream->Dev;
      Stream->ConOutMode = Proto->Mode->Mode;
      if( Proto->QueryMode(Proto, Stream->ConOutMode, &Columns, &Rows) != RETURN_SUCCESS) {
        Stream->Dev = NULL;   // Mark this stream as closed
        return RETURN_INVALID_PARAMETER;
      }
      Stream->MaxConXY.XYpos.Column = (UINT32)Columns;
      Stream->MaxConXY.XYpos.Row    = (UINT32)Rows;
    }
  }
  // Save NewHandle and return.
  *NewHandle = &Stream->Abstraction;

  return RETURN_SUCCESS;
}
