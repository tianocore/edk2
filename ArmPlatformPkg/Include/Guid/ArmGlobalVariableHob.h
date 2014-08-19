/** @file
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __ARM_GLOBAL_VARIABLE_GUID_H__
#define __ARM_GLOBAL_VARIABLE_GUID_H__

#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>

#define ARM_HOB_GLOBAL_VARIABLE_GUID  \
  { 0xc3253c90, 0xa24f, 0x4599, { 0xa6, 0x64, 0x1f, 0x88, 0x13, 0x77, 0x8f, 0xc9} };

extern EFI_GUID gArmGlobalVariableGuid;

///
/// Describes all memory ranges used during the HOB producer
/// phase that exist outside the HOB list. This HOB type
/// describes how memory is used, not the physical attributes of memory.
///
typedef struct {
  ///
  /// The Guid HOB header. Header.HobType = EFI_HOB_TYPE_GUID_EXTENSION
  ///                 and  Header.Name    = gArmGlobalVariableGuid
  ///
  EFI_HOB_GUID_TYPE            Header;

  ///
  /// The base address of memory allocated by this HOB. Type
  /// EFI_PHYSICAL_ADDRESS is defined in AllocatePages() in the UEFI 2.0
  /// specification.
  ///
  EFI_PHYSICAL_ADDRESS        GlobalVariableBase;

  ///
  /// The length in bytes of memory allocated by this HOB.
  ///
  UINT32                      GlobalVariableSize;
} ARM_HOB_GLOBAL_VARIABLE;

#endif
