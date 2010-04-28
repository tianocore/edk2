/*++

Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  TcgService.c

Abstract:

  TcgService Protocol GUID as defined in TCG_EFI_Protocol_1_20_Final

  See http://trustedcomputinggroup.org for the latest specification

--*/

#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION(TcgService)

EFI_GUID gEfiTcgProtocolGuid         = EFI_TCG_PROTOCOL_GUID;
EFI_GUID gEfiTcgPlatformProtocolGuid = EFI_TCG_PLATFORM_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiTcgServiceProtocolGuid, "TcgService", "TCG Services Protocol");
