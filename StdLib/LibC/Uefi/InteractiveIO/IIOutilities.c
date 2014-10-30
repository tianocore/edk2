/** @file
  Utilities for Interactive I/O Functions.

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
#include  <Protocol/SimpleTextOut.h>

#include  <LibConfig.h>

#include  <assert.h>
#include  <errno.h>
#include  <sys/syslimits.h>
#include  <sys/termios.h>
#include  <Device/IIO.h>
#include  <MainData.h>
#include  "IIOutilities.h"

/** Get the low-level UEFI protocol associated with an open file.

    @param[in]    fd    File descriptor for an open file.
    @param[out]   filp  NULL, or a pointer to where a pointer to the file's
                        file descriptor structure is to be stored.

    @return   Returns NULL if fd is not a valid file descriptor, otherwise
              a pointer to the file's associated UEFI protocol is returned.
**/
void *
EFIAPI
IIO_GetDeviceProto (
  int                 fd,
  struct __filedes  **filp
  )
{
  void               *Proto;
  ConInstance        *Stream;
  struct __filedes   *pfil;

  Proto = NULL;
  if(ValidateFD( fd, VALID_OPEN)) {
    pfil = &gMD->fdarray[fd];
    Stream = BASE_CR(pfil->f_ops, ConInstance, Abstraction);
    Proto = (void *)Stream->Dev;
    if(filp != NULL) {
      *filp = pfil;
    }
  }
  return Proto;
}

/** Get a character either from the input buffer or from hardware.

    @param[in]    filp      Pointer to a file descriptor structure.
    @param[in]    First     Set to TRUE to identify the initial read.

    @return   Returns a character read from either the input buffer
              or from the open file (device) identified by filp.
              A return value of WEOF indicates an error has occurred.
**/
wint_t
EFIAPI
IIO_GetInChar (
  struct __filedes *filp,
  BOOLEAN           First
)
{
  cIIO             *This;
  cFIFO            *InBuf;
  size_t            Status;
  ssize_t           NumRead;
  wint_t            RetVal;
  wchar_t           InChar;

  static size_t     BufCnt;

  This      = filp->devdata;
  InBuf     = This->InBuf;

  NumRead = -1;
  InChar  =  0;
  if(First) {
    BufCnt = InBuf->Count(InBuf, AsElements);
  }
  if(BufCnt > 0) {
    Status = InBuf->Read(InBuf, &InChar, 1);
    if (Status > 0) {
      --BufCnt;
      NumRead = 1;
    }
  }
  else {
    NumRead = filp->f_ops->fo_read(filp, &filp->f_offset, sizeof(wchar_t), &InChar);
  }
  if(NumRead <= 0) {
    RetVal = WEOF;
  }
  else {
    RetVal = (wint_t)InChar;
  }
  return RetVal;
}

/** Get the current cursor position.

    @param[in]      fd      File descriptor for an open file.
    @param[out]     Column  Pointer to where the current cursor column is to be stored.
    @param[out]     Row     Pointer to where the current cursor row is to be stored.

    @retval   -1    fd is not an IIO output device.
    @retval    0    Cursor position retrieved, Cursor is Not Visible.
    @retval    1    Cursor position retrieved, Cursor is Visible.
**/
int
EFIAPI
IIO_GetCursorPosition (
  int       fd,
  UINT32   *Column,
  UINT32   *Row
  )
{
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *Proto;
  struct __filedes                   *pStdOut;
  int                                 RetVal;

  RetVal    = -1;

  Proto = (EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)IIO_GetDeviceProto(fd, &pStdOut);
  if(Proto != NULL) {
    if(((pStdOut->f_iflags & _S_ITTY) != 0) &&    // file is a TTY
       ((pStdOut->Oflags & O_ACCMODE) != 0))      // and it is open for output
    {
      // fd is for a TTY or "Interactive IO" device
      *Column  = Proto->Mode->CursorColumn;
      *Row     = Proto->Mode->CursorRow;
      if(Proto->Mode->CursorVisible) {
        RetVal = 1;
      }
      else {
        RetVal = 0;
      }
    }
  }
  return RetVal;
}

/** Set the cursor position.

    @param[in]    filp    Pointer to the output device's file descriptor structure.
    @param[in]    StartXY Pointer to a cursor coordinate (XY) structure indicating
                          the desired coordinate to move the cursor to.

    @retval   -1    fd is not an IIO output device
    @retval    0    Cursor position set successfully.
**/
int
EFIAPI
IIO_SetCursorPosition (
  struct __filedes *filp,
  CURSOR_XY        *CursorXY
  )
{
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *Proto;
  cIIO                               *This;
  EFI_STATUS                          Status;
  int                                 RetVal;

  RetVal    = -1;

  This = filp->devdata;
  Proto = (EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)IIO_GetDeviceProto(filp->MyFD, NULL);
  if(Proto != NULL) {
    if(((filp->f_iflags & _S_ITTY) != 0) &&    // file is a TTY
       ((filp->Oflags & O_ACCMODE) != 0))      // and it is open for output
    {
      // fd is for a TTY or "Interactive IO" device
      Status = Proto->SetCursorPosition(Proto, CursorXY->Column, CursorXY->Row);
      if(Status == EFI_SUCCESS) {
        This->CurrentXY.Column  = CursorXY->Column;
        This->CurrentXY.Row     = CursorXY->Row;
        RetVal = 0;
      }
    }
  }
  return RetVal;
}

/** Get Output screen size and mode.

    @param[in]    fd    File descriptor of the output device.
    @param[out]   Col   Pointer to where to store the MAX Column, or NULL.
    @param[out]   Row   Pointer to where to store the MAX Row, or NULL.

    @retval   <0    An error occurred.  The reason is in errno and EFIerrno.
                      * EIO     UEFI QueryMode failed
                      * ENOTTY  fd does not refer to an interactive output device
    @retval   >=0   Current output mode
**/
int
EFIAPI
IIO_GetOutputSize (
  int       fd,
  UINTN    *Col,
  UINTN    *Row
)
{
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *Proto;
  struct __filedes                   *pStdOut;
  EFI_STATUS                          Status;
  UINTN                               TempCol;
  UINTN                               TempRow;
  UINTN                               TempMode;
  int                                 RetVal;

  RetVal    = -1;

  Proto = (EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)IIO_GetDeviceProto(fd, &pStdOut);
  if(Proto != NULL) {
    if(((pStdOut->f_iflags & _S_ITTY) != 0)   &&    // file is a TTY
        ((pStdOut->Oflags & O_ACCMODE) != 0))       // and it is open for output
    {
      // fd is for a TTY or "Interactive IO" device
      TempMode = Proto->Mode->Mode;
      Status = Proto->QueryMode(Proto, TempMode, &TempCol, &TempRow);
      if(EFI_ERROR(Status)) {
        EFIerrno  = Status;
        errno     = EIO;
      }
      else {
        *Col    = TempCol;
        *Row    = TempRow;
        RetVal  = (int)TempMode;
      }
    }
    else {
      errno = ENOTTY;
    }
  }
  return RetVal;
}

/** Calculate the number of character positions between two X/Y coordinate pairs.

    Using the current output device characteristics, calculate the number of
    characters between two coordinates.  It is assumed that EndXY points to
    an output location that occurs after StartXY.

    RowDelta is the computed difference between the ending and starting rows.
    If RowDelta < 0, then EndXY is NOT after StartXY, so assert.

    ColumnDelta is the computed number of character positions (columns) between
    the starting position and the ending position.  If ColumnDelta is < 0,
    then EndXY is NOT after StartXY, so assert.

    @param[in]      This      Pointer to the IIO instance to be examined.
    @param[in]      StartXY   Pointer to the starting coordinate pair.
    @param[in]      EndXY     Pointer to the ending coordinate pair.

    @return   Returns the difference between the starting and ending coordinates.
              The return value is positive if the coordinates contained in EndXY
              are larger than StartXY, otherwise the return value is negative.
**/
int
EFIAPI
IIO_CursorDelta (
  cIIO         *This,
  CURSOR_XY    *StartXY,
  CURSOR_XY    *EndXY
)
{
  int    ColumnDelta;
  int    RowDelta;

  RowDelta = (int)EndXY->Row - (int)StartXY->Row;

  assert(RowDelta >= 0);    // assert if EndXY is NOT after StartXY

  ColumnDelta = (int)((This->MaxColumn * RowDelta) + EndXY->Column);
  ColumnDelta -= (int)StartXY->Column;

  return ColumnDelta;
}
