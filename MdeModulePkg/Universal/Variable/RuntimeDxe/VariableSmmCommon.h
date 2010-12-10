/** @file

  The internal header file includes the common header files shared 
  by VariableSmm module and VariableSmmRuntimeDxe module.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

**/

#ifndef _VARIABLE_SMM_COMMON_H_
#define _VARIABLE_SMM_COMMON_H_

#include <PiDxe.h>

#include <Protocol/SmmVariable.h>
#include <Protocol/SmmFirmwareVolumeBlock.h>
#include <Guid/VariableFormat.h>

#define EFI_SMM_VARIABLE_WRITE_GUID \
  { 0x93ba1826, 0xdffb, 0x45dd, { 0x82, 0xa7, 0xe7, 0xdc, 0xaa, 0x3b, 0xbd, 0xf3 } }

///
/// Size of SMM communicate header, without including the payload.
///
#define SMM_COMMUNICATE_HEADER_SIZE  (OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data))

///
/// Size of SMM variable communicate header, without including the payload.
///
#define SMM_VARIABLE_COMMUNICATE_HEADER_SIZE  (OFFSET_OF (SMM_VARIABLE_COMMUNICATE_HEADER, Data))


#endif
