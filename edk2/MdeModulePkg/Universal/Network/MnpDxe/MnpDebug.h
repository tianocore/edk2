/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  MnpDebug.h

Abstract:


**/

#ifndef _MNP_DEBUG_H_
#define _MNP_DEBUG_H_

#define MNP_DEBUG_TRACE(PrintArg) NET_DEBUG_TRACE ("Mnp", PrintArg)
#define MNP_DEBUG_WARN(PrintArg)  NET_DEBUG_WARNING ("Mnp", PrintArg)
#define MNP_DEBUG_ERROR(PrintArg) NET_DEBUG_ERROR ("Mnp", PrintArg)

#endif
