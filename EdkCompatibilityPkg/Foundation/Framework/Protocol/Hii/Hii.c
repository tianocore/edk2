/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Hii.c

Abstract:

  This file defines the Human Interface Infrastructure protocol which will 
  be used by resources which want to publish IFR/Font/String data and have it 
  collected by the Configuration engine.

 
--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (Hii)

EFI_GUID  gEfiHiiProtocolGuid = EFI_HII_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiHiiProtocolGuid, "Human Interface Infrastructure Protocol", "HII 1.0 protocol");
