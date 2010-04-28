/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PrimaryConsoleInDevice.c
    
Abstract:


--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION(PrimaryConsoleInDevice)


EFI_GUID  gEfiPrimaryConsoleInDeviceGuid = EFI_PRIMARY_CONSOLE_IN_DEVICE_GUID;

EFI_GUID_STRING(&gEfiPrimaryConsoleInDeviceGuid, "Primary Console In Device Guid", 
                "EFI Primary Conosle In Device Guid");
