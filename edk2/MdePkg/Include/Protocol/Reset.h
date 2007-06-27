/** @file
  Reset Architectural Protocol as defined in the DXE CIS

  Used to provide ResetSystem runtime services

  The ResetSystem () EFI 1.0 service is added to the EFI system table and the 
  EFI_RESET_ARCH_PROTOCOL_GUID protocol is registered with a NULL pointer.

  No CRC of the EFI system table is required, as it is done in the DXE core.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  Version 0.91B.

**/

#ifndef __ARCH_PROTOCOL_RESET_H__
#define __ARCH_PROTOCOL_RESET_H__

//
// Global ID for the Reset Architectural Protocol
//
#define EFI_RESET_ARCH_PROTOCOL_GUID  \
  { 0x27CFAC88, 0x46CC, 0x11d4, {0x9A, 0x38, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D } }

extern EFI_GUID gEfiResetArchProtocolGuid;

#endif
