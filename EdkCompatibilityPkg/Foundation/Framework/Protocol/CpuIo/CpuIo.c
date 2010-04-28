/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  CpuIO.c

Abstract:

  CPU IO Protocol GUID as defined in Tiano


--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (CpuIo)

EFI_GUID  gEfiCpuIoProtocolGuid = EFI_CPU_IO_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiCpuIoProtocolGuid, "CPU IO", "CPU IO Protocol");
