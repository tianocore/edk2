/** @file
  Provides a service to retrieve a pointer to the Standalone MM Services Table.
  Only available to MM_STANDALONE, SMM/DXE Combined and SMM module types.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __MM_SERVICES_TABLE_LIB_H__
#define __MM_SERVICES_TABLE_LIB_H__

#include <PiMm.h>

extern EFI_MM_SYSTEM_TABLE         *gMmst;

#endif
