/** @file
  Declarations local to the Uefi SysCalls module of the Standard C Library.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _SYSEFI_H
#define _SYSEFI_H
#include  <Protocol/SimpleFileSystem.h>

#define EFI_FILE_MODE_MASK    ( EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE )
#define OMODE_MASK            0xFFFF00UL
#define OMODE_SHIFT           8

#define S_ACC_READ            ( S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH )
#define S_ACC_WRITE           ( S_IWUSR | S_IWGRP | S_IWOTH )
#define S_ACC_MASK            ( S_IRWXU | S_IRWXG | S_IRWXO )

UINT64
Oflags2EFI( int oflags);

UINT64
Omode2EFI( int mode);

/* Converts the first several EFI status values into the appropriate errno value.
*/
int
EFI2errno( RETURN_STATUS Status);

#endif  /* _SYSEFI_H */
