/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiPeOptionalHeader.h

Abstract:
  Defines the optional header in the PE image per the PE specification.  This
  file must be included only from within EfiImage.h since 
  EFI_IMAGE_DATA_DIRECTORY and EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES are defined
  there.

--*/

#ifndef _EFI_PE_OPTIONAL_HEADER_H_
#define _EFI_PE_OPTIONAL_HEADER_H_

#define EFI_IMAGE_MACHINE_TYPE (EFI_IMAGE_MACHINE_ARMTHUMB_MIXED)

#define EFI_IMAGE_MACHINE_TYPE_SUPPORTED(Machine) \
  (((Machine) == EFI_IMAGE_MACHINE_ARMTHUMB_MIXED) || ((Machine) == EFI_IMAGE_MACHINE_EBC))

#define EFI_IMAGE_MACHINE_CROSS_TYPE_SUPPORTED(Machine) (FALSE) 

#define EFI_IMAGE_NT_OPTIONAL_HDR_MAGIC EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC
typedef EFI_IMAGE_OPTIONAL_HEADER32 EFI_IMAGE_OPTIONAL_HEADER;
typedef EFI_IMAGE_NT_HEADERS32      EFI_IMAGE_NT_HEADERS;

#endif
