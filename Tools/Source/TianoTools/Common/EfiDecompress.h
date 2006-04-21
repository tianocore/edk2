/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  EfiDecompress.h

Abstract:

  Header file for compression routine
  
--*/
#include <Base.h>
#include <UefiBaseTypes.h>

#ifndef _EFI_DECOMPRESS_H
#define _EFI_DECOMPRESS_H
EFI_STATUS
GetInfo (
  IN      VOID    *Source,
  IN      UINT32  SrcSize,
  OUT     UINT32  *DstSize,
  OUT     UINT32  *ScratchSize
  );

/*++

Routine Description:

  The implementation of EFI_DECOMPRESS_PROTOCOL.GetInfo().

Arguments:

  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  DstSize     - The size of destination buffer.
  ScratchSize - The size of scratch buffer.

Returns:

  EFI_SUCCESS           - The size of destination buffer and the size of scratch buffer are successull retrieved.
  EFI_INVALID_PARAMETER - The source data is corrupted

--*/
EFI_STATUS
Decompress (
  IN      VOID    *Source,
  IN      UINT32  SrcSize,
  IN OUT  VOID    *Destination,
  IN      UINT32  DstSize,
  IN OUT  VOID    *Scratch,
  IN      UINT32  ScratchSize
  )
;

/*++

Routine Description:

  The implementation of EFI_DECOMPRESS_PROTOCOL.Decompress().

Arguments:

  This        - The protocol instance pointer
  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  Destination - The destination buffer to store the decompressed data
  DstSize     - The size of destination buffer.
  Scratch     - The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.
  ScratchSize - The size of scratch buffer.

Returns:

  EFI_SUCCESS           - Decompression is successfull
  EFI_INVALID_PARAMETER - The source data is corrupted

--*/
typedef
EFI_STATUS
(*GETINFO_FUNCTION) (
  IN      VOID    *Source,
  IN      UINT32  SrcSize,
  OUT     UINT32  *DstSize,
  OUT     UINT32  *ScratchSize
  );

typedef
EFI_STATUS
(*DECOMPRESS_FUNCTION) (
  IN      VOID    *Source,
  IN      UINT32  SrcSize,
  IN OUT  VOID    *Destination,
  IN      UINT32  DstSize,
  IN OUT  VOID    *Scratch,
  IN      UINT32  ScratchSize
  );
#endif
