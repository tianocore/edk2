/** @file
  This file defines the debug agent GUID for HOB and configuration table.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_DEBUG_AGENT_GUID_H__
#define __EFI_DEBUG_AGENT_GUID_H__

///
/// This guid is used as a variable GUID for the capsule variable
/// if the capsule pointer is passed through reset via a variable.
///
/// This guid is also used as a hob GUID for the capsule data
/// when the capsule pointer is passed from PEI phase to DXE phase.
///
#define EFI_DEBUG_AGENT_GUID  \
  { \
    0x865a5a9b, 0xb85d, 0x474c, { 0x84, 0x55, 0x65, 0xd1, 0xbe, 0x84, 0x4b, 0xe2 } \
  }

extern EFI_GUID gEfiDebugAgentGuid;

#endif
