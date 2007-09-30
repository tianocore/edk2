/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  PxeArch.h

Abstract:
  Defines PXE Arch type


**/

#ifndef _EFI_PXE_ARCH_H_
#define _EFI_PXE_ARCH_H_

//
// warning #175: subscript out of range
//
#pragma warning (disable: 175)

#define SYS_ARCH GetSysArch()

UINT16
GetSysArch (
  VOID
  );

#endif
