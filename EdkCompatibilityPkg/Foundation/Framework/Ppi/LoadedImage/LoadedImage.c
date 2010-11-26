/*++

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

 LoadedImage.c

Abstract:

  The file describes the PPI which notifies other drivers 
  of the PEIM being initialized by the PEI Dispatcher.

--*/

#include "Tiano.h"
#include "PeiBind.h"
#include "PeiApi.h"
#include EFI_PPI_DEFINITION (LoadedImage)

EFI_GUID  gEfiPeiLoadedImagePpiGuid = EFI_PEI_LOADED_IMAGE_PPI_GUID;

EFI_GUID_STRING(&gEfiPeiLoadedImagePpiGuid, "LoadedImagePpi", "LoadedImage PPI");
