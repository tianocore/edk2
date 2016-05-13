/** @file

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FSP_SWITCH_STACK_LIB_H_
#define _FSP_SWITCH_STACK_LIB_H_

/**

  This funciton will switch the current stack to the previous saved stack.
  Before calling the previous stack has to be set in  FSP_GLOBAL_DATA.CoreStack.
                    EIP
                    FLAGS  16 bit  FLAGS  16 bit
                    EDI
                    ESI
                    EBP
                    ESP
                    EBX
                    EDX
                    ECX
                    EAX
                    DWORD     IDT base1
  StackPointer:     DWORD     IDT base2

  @return ReturnKey          After switching to the saved stack,
                             this value will be saved in eax before returning.


**/
UINT32
EFIAPI
Pei2LoaderSwitchStack (
  VOID
  );

#endif
