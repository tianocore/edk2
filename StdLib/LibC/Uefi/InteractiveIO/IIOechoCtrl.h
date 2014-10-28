/** @file
  Constants and declarations for the Echo function.

  Copyright (c) 2012 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _IIO_ECHO_CTRL_H
#define _IIO_ECHO_CTRL_H
#include  <sys/termios.h>

__BEGIN_DECLS

/* These constants are assigned values within the Unicode Private Use range.
   The value of IIO_ECHO_MIN must be adjusted to ensure that IIO_ECHO_MAX
   never exceeds the value of (TtyFunKeyMin - 1).
*/
typedef enum {
  IIO_ECHO_MIN      = (TtySpecKeyMin),
  IIO_ECHO_DISCARD  = IIO_ECHO_MIN,       // Ignore this character completely
  IIO_ECHO_ERASE,                         // Erase previous character
  IIO_ECHO_KILL,                          // Kill the entire line
  IIO_ECHO_MAX
} IioEchoCtrl;

__END_DECLS

#endif  /* _IIO_ECHO_CTRL_H */
