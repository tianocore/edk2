/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  LegacyBiosThunk.h
    
Abstract:

  The EFI Legacy BIOS Thunk Protocol is used to abstract Thunk16 call.

  Note: The names for EFI_IA32_REGISTER_SET elements were picked to follow 
  well known naming conventions.

  Thunk - A thunk is a transition from one processor mode to another. A Thunk
          is a transition from native EFI mode to 16-bit mode. A reverse thunk
          would be a transition from 16-bit mode to native EFI mode.


  Note: Note: Note: Note: Note: Note: Note:

  You most likely should not use this protocol! Find the EFI way to solve the
  problem to make your code portable

  Note: Note: Note: Note: Note: Note: Note:

Revision History

--*/

#ifndef _EFI_LEGACY_BIOS_THUNK_H_
#define _EFI_LEGACY_BIOS_THUNK_H_

#include EFI_PROTOCOL_DEFINITION (LegacyBios)

#define EFI_LEGACY_BIOS_THUNK_PROTOCOL_GUID \
  { \
    0x4c51a7ba, 0x7195, 0x442d, {0x87, 0x92, 0xbe, 0xea, 0x6e, 0x2f, 0xf6, 0xec} \
  }

EFI_FORWARD_DECLARATION (EFI_LEGACY_BIOS_THUNK_PROTOCOL);

typedef
BOOLEAN
(EFIAPI *EFI_LEGACY_BIOS_THUNK_INT86) (
  IN EFI_LEGACY_BIOS_THUNK_PROTOCOL   * This,
  IN  UINT8                           BiosInt,
  IN OUT  EFI_IA32_REGISTER_SET       * Regs
  )
/*++

  Routine Description:
    Thunk to 16-bit real mode and execute a software interrupt with a vector 
    of BiosInt. Regs will contain the 16-bit register context on entry and 
    exit.

  Arguments:
    This    - Protocol instance pointer.
    BiosInt - Processor interrupt vector to invoke
    Reg     - Register contexted passed into (and returned) from thunk to 
              16-bit mode

  Returns:
    FALSE   - Thunk completed, and there were no BIOS errors in the target code.
              See Regs for status.
    TRUE    - There was a BIOS erro in the target code.

--*/
;

typedef
BOOLEAN
(EFIAPI *EFI_LEGACY_BIOS_THUNK_FARCALL86) (
  IN EFI_LEGACY_BIOS_THUNK_PROTOCOL   * This,
  IN  UINT16                          Segment,
  IN  UINT16                          Offset,
  IN  EFI_IA32_REGISTER_SET           * Regs,
  IN  VOID                            *Stack,
  IN  UINTN                           StackSize
  )
/*++

  Routine Description:
    Thunk to 16-bit real mode and call Segment:Offset. Regs will contain the 
    16-bit register context on entry and exit. Arguments can be passed on 
    the Stack argument

  Arguments:
    This      - Protocol instance pointer.
    Segment   - Segemnt of 16-bit mode call
    Offset    - Offset of 16-bit mdoe call
    Reg       - Register contexted passed into (and returned) from thunk to 
                16-bit mode
    Stack     - Caller allocated stack used to pass arguments
    StackSize - Size of Stack in bytes

  Returns:
    FALSE     - Thunk completed, and there were no BIOS errors in the target code.
                See Regs for status.
    TRUE      - There was a BIOS erro in the target code.

--*/
;

struct _EFI_LEGACY_BIOS_THUNK_PROTOCOL {
  EFI_LEGACY_BIOS_THUNK_INT86                 Int86;
  EFI_LEGACY_BIOS_THUNK_FARCALL86             FarCall86;
};

extern EFI_GUID gEfiLegacyBiosThunkProtocolGuid;

#endif
