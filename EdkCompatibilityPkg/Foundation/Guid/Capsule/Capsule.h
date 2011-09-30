/*++

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Capsule.h
    
Abstract:

  Capsule update Guid definitions

--*/

#ifndef _EFI_CAPSULE_VENDOR_GUID_H_
#define _EFI_CAPSULE_VENDOR_GUID_H_

//
// Note -- This guid is used as a vendor GUID (depending on implementation)
// for the capsule variable if the capsule pointer is passes through reset
// via a variable.
//
#define EFI_CAPSULE_VENDOR_GUID  \
  { 0x711C703F, 0xC285, 0x4B10, {0xA3, 0xB0, 0x36, 0xEC, 0xBD, 0x3C, 0x8B, 0xE2} }
  
//
// If capsule data is passed via a variable, then this name should be used.
//
#define EFI_CAPSULE_VARIABLE_NAME L"CapsuleUpdateData"

//
// The variable describes the long mode buffer used by IA32 Capsule PEIM
// to call X64 CapsuleCoalesce code to handle >4GB capsule blocks.
//
#define EFI_CAPSULE_LONG_MODE_BUFFER_NAME L"CapsuleLongModeBuffer"

typedef struct {
  EFI_PHYSICAL_ADDRESS   Base;
  EFI_PHYSICAL_ADDRESS   Length;
  UINT64                 StackSize;
} EFI_CAPSULE_LONG_MODE_BUFFER;

extern EFI_GUID gEfiCapsuleVendorGuid;

#endif // #ifndef _EFI_CAPSULE_VENDOR_GUID_H_
