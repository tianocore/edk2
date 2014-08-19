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

#ifndef __EBL_ADD_EXTERNAL_COMMAND_LIB_H__
#define __EBL_ADD_EXTERNAL_COMMAND_LIB_H__

#include <PiDxe.h>
#include <Protocol/EblAddCommand.h>


EFI_STATUS
EFIAPI
EblAddExternalCommands (
  IN const EBL_COMMAND_TABLE   *EntryArray,
  IN UINTN                     ArrayCount
  );

/**

  Return a keypress or optionally timeout if a timeout value was passed in.

  An optional callback function is called every second when waiting for a

  timeout.



  @param  Key           EFI Key information returned

  @param  TimeoutInSec  Number of seconds to wait to timeout

  @param  CallBack      Callback called every second during the timeout wait



  @return EFI_SUCCESS  Key was returned

  @return EFI_TIMEOUT  If the TimoutInSec expired



**/

EFI_STATUS

EFIAPI

EblGetCharKey (

  IN OUT EFI_INPUT_KEY            *Key,

  IN     UINTN                    TimeoutInSec,

  IN     EBL_GET_CHAR_CALL_BACK   CallBack   OPTIONAL

  );





/**

  This routine is used prevent command output data from scrolling off the end

  of the screen. The global gPageBreak is used to turn on or off this feature.

  If the CurrentRow is near the end of the screen pause and print out a prompt

  If the use hits Q to quit return TRUE else for any other key return FALSE.

  PrefixNewline is used to figure out if a newline is needed before the prompt

  string. This depends on the last print done before calling this function.

  CurrentRow is updated by one on a call or set back to zero if a prompt is

  needed.



  @param  CurrentRow  Used to figure out if its the end of the page and updated

  @param  PrefixNewline  Did previous print issue a newline



  @return TRUE if Q was hit to quit, FALSE in all other cases.



**/

BOOLEAN

EFIAPI

EblAnyKeyToContinueQtoQuit (

  IN  UINTN   *CurrentRow,

  IN  BOOLEAN PrefixNewline

  );



#endif

