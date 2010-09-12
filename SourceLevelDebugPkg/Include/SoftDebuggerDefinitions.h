/** @file
  Soft Debugger defintions. The definitions will also be used as part
  of debug transfer protocol. It is only intended to be used by Debug
  related module implementation.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SOFT_DEBUGGER_DEFINITIONS_H__
#define __SOFT_DEBUGGER_DEFINITIONS_H__

//
// Definition for processor mode (IA16, IA32, X64, ...)
//
#define SOFT_DEBUGGER_PROCESSOR_IA16        0
#define SOFT_DEBUGGER_PROCESSOR_IA32        1
#define SOFT_DEBUGGER_PROCESSOR_X64         2

//
// Break cause defintions
//
#define SOFT_DEBUGGER_BREAK_CAUSE_UNKNOWN        0
#define SOFT_DEBUGGER_BREAK_CAUSE_HW_BREAKPOINT  1
#define SOFT_DEBUGGER_BREAK_CAUSE_STEPPING       2
#define SOFT_DEBUGGER_BREAK_CAUSE_SW_BREAKPOINT  3
#define SOFT_DEBUGGER_BREAK_CAUSE_USER_HALT      4
#define SOFT_DEBUGGER_BREAK_CAUSE_IMAGE_LOAD     5
#define SOFT_DEBUGGER_BREAK_CAUSE_IMAGE_UNLOAD   6
#define SOFT_DEBUGGER_BREAK_CAUSE_SYSTEM_RESET   7
#define SOFT_DEBUGGER_BREAK_CAUSE_EXCEPTION      8

#define SOFT_DEBUGGER_SETTING_SMM_ENTRY_BREAK    1

#endif
