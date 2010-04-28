/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    Bmp.c
 .c
    
Abstract:

  GUID used as a filename for the BMP logo 

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (Bmp)

EFI_GUID gEfiDefaultBmpLogoGuid  = EFI_DEFAULT_BMP_LOGO_GUID;

EFI_GUID_STRING (&gEfiDefaultBmpLogoGuid, "Guid for BMP logo", "Guid for BMP logo");

