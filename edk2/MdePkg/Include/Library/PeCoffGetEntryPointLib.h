/** @file
  Memory Only PE COFF loader

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  PeCoffGetEntryPointLib.h

**/

#ifndef __PE_COFF_GET_ENTRY_POINT_LIB_H__
#define __PE_COFF_GET_ENTRY_POINT_LIB_H__

/**
  Retrieves and returns a pointer to the entry point to a PE/COFF image that has been loaded
  into system memory with the PE/COFF Loader Library functions.

  Retrieves the entry point to the PE/COFF image specified by Pe32Data and returns this entry
  point in EntryPoint.  If the entry point could not be retrieved from the PE/COFF image, then
  return RETURN_INVALID_PARAMETER.  Otherwise return RETURN_SUCCESS.
  If Pe32Data is NULL, then ASSERT().
  If EntryPoint is NULL, then ASSERT().

  @param  Pe32Data                  Pointer to the PE/COFF image that is loaded in system memory.
  @param  EntryPoint                Pointer to entry point to the PE/COFF image to return.

  @retval RETURN_SUCCESS            EntryPoint was returned.
  @retval RETURN_INVALID_PARAMETER  The entry point could not be found in the PE/COFF image.

**/
RETURN_STATUS
EFIAPI
PeCoffLoaderGetEntryPoint (
  IN  VOID  *Pe32Data,
  OUT VOID  **EntryPoint
  );

/**
  Returns the machine type of PE/COFF image. 

  @param  Image   Pointer to a PE/COFF header

  @return         Machine type or zero if not a valid iamge

**/
UINT16
EFIAPI
PeCoffLoaderGetMachineType (
  IN  VOID    *Image
  );


#endif
