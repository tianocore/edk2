/** @file
  Arm generic timer parser.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/timer/arm,arch_timer.yaml
**/

#ifndef ARM_GENERIC_TIMER_PARSER_H_
#define ARM_GENERIC_TIMER_PARSER_H_

/** An enum listing the FDT interrupt items.
*/
typedef enum FdtTimerInterruptItems {
  FdtSecureTimerIrq,      ///< Secure timer IRQ
  FdtNonSecureTimerIrq,   ///< Non-secure timer IRQ
  FdtVirtualTimerIrq,     ///< Virtual timer IRQ
  FdtHypervisorTimerIrq,  ///< Hypervisor timer IRQ
  FdtMaxTimerItem         ///< Max timer item
} FDT_TIMER_INTERRUPT_ITEMS;

/** CM_ARM_BOOT_ARCH_INFO parser function.

  The following structure is populated:
  typedef struct CmArmGenericTimerInfo {
    UINT64  CounterControlBaseAddress;        // {default}
    UINT64  CounterReadBaseAddress;           // {default}
    UINT32  SecurePL1TimerGSIV;               // {Populated}
    UINT32  SecurePL1TimerFlags;              // {Populated}
    UINT32  NonSecurePL1TimerGSIV;            // {Populated}
    UINT32  NonSecurePL1TimerFlags;           // {Populated}
    UINT32  VirtualTimerGSIV;                 // {Populated}
    UINT32  VirtualTimerFlags;                // {Populated}
    UINT32  NonSecurePL2TimerGSIV;            // {Populated}
    UINT32  NonSecurePL2TimerFlags;           // {Populated}
    UINT32  VirtualPL2TimerGSIV;              // {default}
    UINT32  VirtualPL2TimerFlags;             // {default}
  } CM_ARM_GENERIC_TIMER_INFO;

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.

  @param [in]  FdtParserHandle A handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
ArmGenericTimerInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  );

#endif // ARM_GENERIC_TIMER_PARSER_H_
