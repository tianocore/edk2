/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DecompressLibraryHob.h

Abstract:

  Declaration of HOB that is used to pass decompressor library functions from PEI to DXE

--*/

#ifndef __DECOMPRESS_LIBRARY_HOB_H__
#define __DECOMPRESS_LIBRARY_HOB_H__

typedef
RETURN_STATUS
(EFIAPI *DECOMPRESS_LIBRARY_GET_INFO) (
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  );

typedef
RETURN_STATUS
(EFIAPI *DECOMPRESS_LIBRARY_DECOMPRESS) (
  IN CONST VOID  *Source,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch
  );

typedef struct {
  DECOMPRESS_LIBRARY_GET_INFO    GetInfo;
  DECOMPRESS_LIBRARY_DECOMPRESS  Decompress;
} DECOMPRESS_LIBRARY;

#endif
