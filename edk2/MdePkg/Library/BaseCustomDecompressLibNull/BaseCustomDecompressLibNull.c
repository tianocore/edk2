/** @file
  Null implementation of the custom decompress library

  Copyright (c) 2006 - 2007, Intel Corporation.<BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <Base.h>
#include <Library/CustomDecompressLib.h>


/**
  The internal implementation of *_DECOMPRESS_PROTOCOL.GetInfo().

  @param[in]     Source           The source buffer containing the compressed data.
  @param[in]     SourceSize       The size of source buffer
  @param[out]    DestinationSize  The size of destination buffer.
  @param[out]    ScratchSize      The size of scratch buffer.

  @retval  RETURN_SUCCESS           The size of destination buffer and the size of scratch buffer are successull retrieved.
  @retval  RETURN_INVALID_PARAMETER The source data is corrupted

**/
RETURN_STATUS
EFIAPI
CustomDecompressGetInfo (
  IN  CONST GUID  *DecompressGuid,
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  )
{
  return RETURN_UNSUPPORTED;
}


/**
  The internal implementation of *_DECOMPRESS_PROTOCOL.Decompress().

  @param[in]     Source           The source buffer containing the compressed data.
  @param[in]     Destination      The destination buffer to store the decompressed data
  @param[out]    Scratch          The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.


  @retval  RETURN_SUCCESS            Decompression is successfull
  @retval  RETURN_INVALID_PARAMETER The source data is corrupted

**/
RETURN_STATUS
EFIAPI
CustomDecompress (
  IN const GUID  *DecompressGuid,
  IN CONST VOID  *Source,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  Get decompress method guid list.

  @param[in, out]  AlgorithmGuidTable   The decompress method guid list.
  @param[in, out]  NumberOfAlgorithms   The number of decompress methods.

  @retval  RETURN_SUCCESS            Get all algorithmes list successfully..
**/
RETURN_STATUS
EFIAPI
CustomDecompressGetAlgorithms (
   IN OUT  GUID    **AlgorithmGuidTable,
   IN OUT  UINT32  *NumberOfAlgorithms
  )
{
  *NumberOfAlgorithms = 0;
  return RETURN_SUCCESS; 
}
