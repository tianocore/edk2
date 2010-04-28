/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Apriori.c
    
Abstract:

  GUID used as an FV filename for A Priori file. The A Priori file contains a
  list of FV filenames that the DXE dispatcher will schedule reguardless of
  the dependency grammer.

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (Apriori)

EFI_GUID  gAprioriGuid = EFI_APRIORI_GUID;

EFI_GUID_STRING(&gAprioriGuid, "Apriori File Name", "Apriori File containing FV GUIDs");
