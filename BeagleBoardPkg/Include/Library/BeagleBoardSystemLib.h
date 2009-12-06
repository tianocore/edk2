/** @file

  Copyright (c) 2008-2009 Apple Inc. All rights reserved.<BR>

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __BEAGLEBOARD_YSTEMLIB_H__
#define __BEAGLEBOARD_YSTEMLIB_H__

VOID
EFIAPI
GoLittleEndian (
  UINTN   ImageAddress
  );

VOID
EFIAPI
ResetSystem (
  IN EFI_RESET_TYPE   ResetType
  );

VOID
EFIAPI
ShutdownEfi (
  VOID
  );

#endif // __BEAGLEBOARD_YSTEMLIB_H__
