/** @file
  Provides a service to retrieve a pointer to the DXE Services Table.
  Only available to DXE module types.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DXE_SERVICES_TABLE_LIB_H__
#define __DXE_SERVICES_TABLE_LIB_H__

///
/// Cache copy of the DXE Services Table
///
extern EFI_DXE_SERVICES  *gDS;

#endif

