/*++

Copyright (c) 2004, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  TianoApi.h

Abstract:

  Tiano intrinsic definitions.


--*/

#ifndef _TIANO_API_H_
#define _TIANO_API_H_

//
// Pointer to internal runtime function
//
#define EFI_INTERNAL_FUNCTION 0x00000002

//
// Pointer to internal runtime pointer
//
#define EFI_INTERNAL_POINTER  0x00000004

//
// Pointer to internal runtime pointer
//
#define EFI_IPF_GP_POINTER  0x00000008

#define EFI_TPL_DRIVER      6

//
// EFI Event Types
//
#define EFI_EVENT_TIMER                         0x80000000
#define EFI_EVENT_RUNTIME                       0x40000000
#define EFI_EVENT_RUNTIME_CONTEXT               0x20000000

#define EFI_EVENT_NOTIFY_WAIT                   0x00000100
#define EFI_EVENT_NOTIFY_SIGNAL                 0x00000200

#define EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES     0x00000201
#define EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE 0x60000202

#define EFI_EVENT_EFI_SIGNAL_MASK               0x000000FF
#define EFI_EVENT_EFI_SIGNAL_MAX                4

//
// Task priority level
//
#define EFI_TPL_APPLICATION 4
#define EFI_TPL_CALLBACK    8
#define EFI_TPL_NOTIFY      16
#define EFI_TPL_HIGH_LEVEL  31

#endif
