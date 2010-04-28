/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  CustomizedCompress.c

Abstract:

  Header file for Customized compression routine
  
--*/

#include "TianoCommon.h"

EFI_STATUS
SetCustomizedCompressionType (
  IN  CHAR8   *Type
  )
/*++

Routine Description:

The implementation of Customized SetCompressionType().

Arguments:
  Type        - The type if compression.
    
Returns:
    
  EFI_SUCCESS           - The type has been set.
  EFI_UNSUPPORTED       - This type is unsupported.

    
--*/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
CustomizedGetInfo (
  IN      VOID    *Source,
  IN      UINT32  SrcSize,
  OUT     UINT32  *DstSize,
  OUT     UINT32  *ScratchSize
  )
/*++

Routine Description:

The implementation of Customized GetInfo().

Arguments:
  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  DstSize     - The size of destination buffer.
  ScratchSize - The size of scratch buffer.
    
Returns:
    
  EFI_SUCCESS           - The size of destination buffer and the size of scratch buffer are successull retrieved.
  EFI_INVALID_PARAMETER - The source data is corrupted
  EFI_UNSUPPORTED       - The operation is unsupported.

    
--*/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
CustomizedDecompress (
  IN      VOID    *Source,
  IN      UINT32  SrcSize,
  IN OUT  VOID    *Destination,
  IN      UINT32  DstSize,
  IN OUT  VOID    *Scratch,
  IN      UINT32  ScratchSize
  )
/*++

Routine Description:

  The implementation of Customized Decompress().

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
  EFI_UNSUPPORTED       - The operation is unsupported.

--*/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
CustomizedCompress (
  IN      UINT8   *SrcBuffer,
  IN      UINT32  SrcSize,
  IN      UINT8   *DstBuffer,
  IN OUT  UINT32  *DstSize
  )
/*++

Routine Description:

  The Customized compression routine.

Arguments:

  SrcBuffer   - The buffer storing the source data
  SrcSize     - The size of source data
  DstBuffer   - The buffer to store the compressed data
  DstSize     - On input, the size of DstBuffer; On output,
                the size of the actual compressed data.

Returns:

  EFI_BUFFER_TOO_SMALL  - The DstBuffer is too small. In this case,
                DstSize contains the size needed.
  EFI_SUCCESS           - Compression is successful.

  EFI_UNSUPPORTED       - The operation is unsupported.
--*/
{
  return EFI_UNSUPPORTED;
}
