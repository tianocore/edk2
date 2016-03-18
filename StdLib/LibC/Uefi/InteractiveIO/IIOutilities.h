/** @file
  Utilities for Interactive I/O Functions.

  The functions assume that isatty() is TRUE at the time they are called.

  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _IIO_UTILITIES_H
#define _IIO_UTILITIES_H

#include  <sys/EfiSysCall.h>

__BEGIN_DECLS

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
  struct __filedes  **filp    // Optional - filp == NULL if unused
  );

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
  );

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
  );

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
  CURSOR_XY        *StartXY
  );

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
);

/** Calculate the number of character positions between two X/Y coordinate pairs.

    Using the current output device characteristics, calculate the number of
    characters between two coordinates.

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
  );

__END_DECLS
#endif  /* _IIO_UTILITIES_H */
