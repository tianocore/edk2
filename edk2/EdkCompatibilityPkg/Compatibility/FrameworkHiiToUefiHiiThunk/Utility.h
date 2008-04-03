/**@file

  This file contains utility functions by HII Thunk Modules.
  
Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_THUNK_UTILITY_H
#define _HII_THUNK_UTILITY_H

EFI_GUID *
GetGuidOfFirstFormset (
  CONST EFI_HII_FORM_PACKAGE * FormPackage
);

#endif
