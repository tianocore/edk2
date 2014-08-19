/** @file
  Include file for basic command line parser for EBL (Embedded Boot Loader)

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EBL_LIB_H__
#define __EBL_LIB_H__

#include <PiDxe.h>
#include <Protocol/EblAddCommand.h>


VOID
EblAddCommand (
  IN const EBL_COMMAND_TABLE   *Entry
  );

VOID
EblAddCommands (
  IN const EBL_COMMAND_TABLE   *EntryArray,
  IN UINTN                     ArrayCount
  );


//
// LIbrary constructor called directly from Ebl Code.
// This module calls EblAddCommand () or EblAddCommands () to register new commands
//
VOID
EblInitializeExternalCmd (
  VOID
  );



#endif

