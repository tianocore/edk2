/** @file
	Memory Only PE COFF loader

	Copyright (c) 2006, Intel Corporation                                                         
	All rights reserved. This program and the accompanying materials                          
	are licensed and made available under the terms and conditions of the BSD License         
	which accompanies this distribution.  The full text of the license may be found at        
	http://opensource.org/licenses/bsd-license.php                                            

	THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
	WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

	Module Name:	PeCoffGetEntryPointLib.h

**/

#ifndef __PE_COFF_GET_ENTRY_POINT_LIB_H__
#define __PE_COFF_GET_ENTRY_POINT_LIB_H__

/**
	Loads a PE/COFF image into memory

	@param	Pe32Data Pointer to a PE/COFF Image
	
	@param	EntryPoint Pointer to the entry point of the PE/COFF image

	@retval EFI_SUCCESS            if the EntryPoint was returned
	@retval EFI_INVALID_PARAMETER  if the EntryPoint could not be found from Pe32Data

**/
RETURN_STATUS
EFIAPI
PeCoffLoaderGetEntryPoint (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  )
;

#endif
