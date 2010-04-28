/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  TianoDecompress.h
    
Abstract:

  The Tiano Decompress Protocol Interface

--*/

#ifndef _TIANO_DECOMPRESS_H_
#define _TIANO_DECOMPRESS_H_

#define EFI_TIANO_DECOMPRESS_PROTOCOL_GUID  \
  { 0xe84cf29c, 0x191f, 0x4eae, {0x96, 0xe1, 0xf4, 0x6a, 0xec, 0xea, 0xea, 0x0b} }

EFI_FORWARD_DECLARATION (EFI_TIANO_DECOMPRESS_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *EFI_TIANO_DECOMPRESS_GET_INFO) (
  IN EFI_TIANO_DECOMPRESS_PROTOCOL  *This,
  IN   VOID                                   *Source,
  IN   UINT32                                 SourceSize,
  OUT  UINT32                                 *DestinationSize,
  OUT  UINT32                                 *ScratchSize
  );
/*++

Routine Description:

  The GetInfo() function retrieves the size of the uncompressed buffer 
  and the temporary scratch buffer required to decompress the buffer 
  specified by Source and SourceSize.  If the size of the uncompressed
  buffer or the size of the scratch buffer cannot be determined from 
  the compressed data specified by Source and SourceData, then 
  EFI_INVALID_PARAMETER is returned.  Otherwise, the size of the uncompressed
  buffer is returned in DestinationSize, the size of the scratch buffer is 
  returned in ScratchSize, and EFI_SUCCESS is returned.
  
  The GetInfo() function does not have scratch buffer available to perform 
  a thorough checking of the validity of the source data. It just retrieves
  the 'Original Size' field from the beginning bytes of the source data and
  output it as DestinationSize.  And ScratchSize is specific to the decompression
  implementation.

Arguments:

  This            - The protocol instance pointer
  Source          - The source buffer containing the compressed data.
  SourceSize      - The size, in bytes, of source buffer.
  DestinationSize - A pointer to the size, in bytes, of the uncompressed buffer
                    that will be generated when the compressed buffer specified 
                    by Source and SourceSize is decompressed.
  ScratchSize     - A pointer to the size, in bytes, of the scratch buffer that 
                    is required to decompress the compressed buffer specified by
                    Source and SourceSize.

Returns:
  EFI_SUCCESS     - The size of the uncompressed data was returned in DestinationSize
                    and the size of the scratch buffer was returned in ScratchSize.
  EFI_INVALID_PARAMETER - The size of the uncompressed data or the size of the scratch
                  buffer cannot be determined from the compressed data specified by 
                  Source and SourceData.

--*/


typedef
EFI_STATUS
(EFIAPI *EFI_TIANO_DECOMPRESS_DECOMPRESS) (
  IN EFI_TIANO_DECOMPRESS_PROTOCOL  *This,
  IN     VOID*                                  Source,
  IN     UINT32                                 SourceSize,
  IN OUT VOID*                                  Destination,
  IN     UINT32                                 DestinationSize,
  IN OUT VOID*                                  Scratch,
  IN     UINT32                                 ScratchSize
  );
/*++

Routine Description:

  The Decompress() function extracts decompressed data to its original form.
  
  This protocol is designed so that the decompression algorithm can be 
  implemented without using any memory services.  As a result, the 
  Decompress() function is not allowed to call AllocatePool() or 
  AllocatePages() in its implementation.  It is the caller's responsibility 
  to allocate and free the Destination and Scratch buffers.
  
  If the compressed source data specified by Source and SourceSize is 
  sucessfully decompressed into Destination, then EFI_SUCCESS is returned.  
  If the compressed source data specified by Source and SourceSize is not in 
  a valid compressed data format, then EFI_INVALID_PARAMETER is returned.

Arguments:

  This            - The protocol instance pointer
  Source          - The source buffer containing the compressed data.
  SourceSize      - The size of source data.
  Destination     - On output, the destination buffer that contains 
                    the uncompressed data.
  DestinationSize - The size of destination buffer. The size of destination
                    buffer needed is obtained from GetInfo().
  Scratch         - A temporary scratch buffer that is used to perform the 
                    decompression.
  ScratchSize     - The size of scratch buffer. The size of scratch buffer needed
                    is obtained from GetInfo().

Returns:

  EFI_SUCCESS     - Decompression completed successfully, and the uncompressed 
                    buffer is returned in Destination.
  EFI_INVALID_PARAMETER 
                  - The source buffer specified by Source and SourceSize is 
                    corrupted (not in a valid compressed format).

--*/

struct _EFI_TIANO_DECOMPRESS_PROTOCOL {
  EFI_TIANO_DECOMPRESS_GET_INFO    GetInfo;
  EFI_TIANO_DECOMPRESS_DECOMPRESS  Decompress;
};

extern EFI_GUID gEfiTianoDecompressProtocolGuid;

#endif
