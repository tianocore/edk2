/** @file

  Copyright (c) 2014 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_SWITCH_STACK_LIB_H_
#define _FSP_SWITCH_STACK_LIB_H_

/**

  This function will switch the current stack to the previous saved stack.
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
EFI_STATUS
EFIAPI
Pei2LoaderSwitchStack (
  VOID
  );

/**

  This function is equivalent to Pei2LoaderSwitchStack () but just indicates
  the stack after switched is FSP stack.

  @return ReturnKey          After switching to the saved stack,
                             this value will be saved in eax before returning.


**/
EFI_STATUS
EFIAPI
Loader2PeiSwitchStack (
  VOID
  );

#endif
