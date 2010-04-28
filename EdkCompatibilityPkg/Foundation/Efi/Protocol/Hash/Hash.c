/*++

  Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    Hash.c
    
Abstract: 
  EFI_HASH_SERVICE_BINDING_PROTOCOL as defined in UEFI 2.0.
  EFI_HASH_PROTOCOL as defined in UEFI 2.0.
  The EFI Hash Service Binding Protocol is used to locate hashing services support 
  provided by a driver and create and destroy instances of the EFI Hash Protocol 
  so that a multiple drivers can use the underlying hashing services.
  The EFI Service Binding Protocol defines the generic Service Binding Protocol functions.

Revision History
--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (Hash)

EFI_GUID  gEfiHashServiceBindingProtocolGuid = EFI_HASH_SERVICE_BINDING_PROTOCOL;
EFI_GUID  gEfiHashProtocolGuid               = EFI_HASH_PROTOCOL_GUID;
EFI_GUID  gEfiHashAlgorithmSha1Guid          = EFI_HASH_ALGORITHM_SHA1_GUID;
EFI_GUID  gEfiHashAlgorithmSha224Guid        = EFI_HASH_ALGORITHM_SHA224_GUID;
EFI_GUID  gEfiHashAlgorithmSha256Guid        = EFI_HASH_ALGORITHM_SHA256_GUID;
EFI_GUID  gEfiHashAlgorithmSha384Guid        = EFI_HASH_ALGORITHM_SHA384_GUID;
EFI_GUID  gEfiHashAlgorithmSha512Guid        = EFI_HASH_ALGORITHM_SHA512_GUID;
EFI_GUID  gEfiHashAlgorithmMD5Guid           = EFI_HASH_ALGORTIHM_MD5_GUID;

EFI_GUID_STRING(&gEfiHashProtocolGuid, "Hash protoco", "UEFI 2.0 Hash protocol");
EFI_GUID_STRING(&gEfiHashServiceBindingProtocolGuid, "Hash service binding protoco", "UEFI 2.0 Hash service binding protocol");
