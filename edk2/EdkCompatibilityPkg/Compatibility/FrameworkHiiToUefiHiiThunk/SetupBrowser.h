/**@file
  This file contains macros to be included by SetupBrowser.c.
  
Copyright (c) 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HIITHUNK_SETUPBROWSER_H_
#define _HIITHUNK_SETUPBROWSER_H_

//
// In order to follow UEFI spec to do auto booting after a time-out, the GUID of Formset of Frontpage must match this value.
//
#define FRAMEWORK_BDS_FRONTPAGE_FORMSET_GUID  { 0x9e0c30bc, 0x3f06, 0x4ba6, {0x82, 0x88, 0x9, 0x17, 0x9b, 0x85, 0x5d, 0xbe }}

#define ONE_SECOND  10000000

#endif
