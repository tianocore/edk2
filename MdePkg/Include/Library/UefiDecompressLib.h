/** @file
	Return UEFI Decompress Protocol 

	Copyright (c) 2006, Intel Corporation                                                         
	All rights reserved. This program and the accompanying materials                          
	are licensed and made available under the terms and conditions of the BSD License         
	which accompanies this distribution.  The full text of the license may be found at        
	http://opensource.org/licenses/bsd-license.php                                            

	THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
	WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

	Module Name:	UefiDecompressLib.h

**/

#ifndef __UEFI_DECPOMPRESS_LIB_H__
#define __UEFI_DECPOMPRESS_LIB_H__

RETURN_STATUS
EFIAPI
UefiDecompressGetInfo (
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  );

RETURN_STATUS
EFIAPI
UefiDecompress (
  IN CONST VOID  *Source,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch
  );

#endif
