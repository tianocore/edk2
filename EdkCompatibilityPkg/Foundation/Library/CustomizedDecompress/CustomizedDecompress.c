/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  CustomizedDecompress.c

Abstract:

  Implementation file for Customized decompression routine
  
--*/
#include "TianoCommon.h"
#include "CustomizedDecompress.h"

EFI_CUSTOMIZED_DECOMPRESS_PROTOCOL  mCustomizedDecompress = {
  CustomizedGetInfo,
  CustomizedDecompress
};

EFI_STATUS
InstallCustomizedDecompress (
  EFI_CUSTOMIZED_DECOMPRESS_PROTOCOL  **This
  )
/*++

Routine Description:

  Install customeized decompress protocol.

Arguments:

  This                  - The protocol that needs to be installed.

Returns:

  EFI_SUCCESS           - Always success

--*/
{
  *This = &mCustomizedDecompress;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CustomizedGetInfo (
  IN EFI_CUSTOMIZED_DECOMPRESS_PROTOCOL     *This,
  IN      VOID                              *Source,
  IN      UINT32                            SrcSize,
  OUT     UINT32                            *DstSize,
  OUT     UINT32                            *ScratchSize
  )
/*++

Routine Description:

  The implementation of Customized GetInfo().

Arguments:
  This        - The EFI customized decompress protocol
  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  DstSize     - The size of destination buffer.
  ScratchSize - The size of scratch buffer.

Returns:

  EFI_SUCCESS           - The size of destination buffer and the size of scratch buffer are successull retrieved.
  EFI_INVALID_PARAMETER - The source data is corrupted
  EFI_UNSUPPORTED       - Not supported

--*/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
CustomizedDecompress (
  IN EFI_CUSTOMIZED_DECOMPRESS_PROTOCOL     *This,
  IN      VOID                              *Source,
  IN      UINT32                            SrcSize,
  IN OUT  VOID                              *Destination,
  IN      UINT32                            DstSize,
  IN OUT  VOID                              *Scratch,
  IN      UINT32                            ScratchSize
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
  EFI_UNSUPPORTED       - Not supported

--*/
{
  return EFI_UNSUPPORTED;
}
