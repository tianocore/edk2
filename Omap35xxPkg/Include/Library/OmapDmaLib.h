/** @file

  Abstractions for simple OMAP DMA.
  OMAP_DMA4 structure elements are described in the OMAP35xx TRM.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __OMAP_DMA_LIB_H__
#define __OMAP_DMA_LIB_H__


// Example from DMA chapter of the OMAP35xx spec
typedef struct {
  UINT8     DataType;                      // DMA4_CSDPi[1:0]
  UINT8     ReadPortAccessType;            // DMA4_CSDPi[8:7]
  UINT8     WritePortAccessType;           // DMA4_CSDPi[15:14]
  UINT8     SourceEndiansim;               // DMA4_CSDPi[21]
  UINT8     DestinationEndianism;          // DMA4_CSDPi[19]
  UINT8     WriteMode;                     // DMA4_CSDPi[17:16]
  UINT8     SourcePacked;                  // DMA4_CSDPi[6]
  UINT8     DestinationPacked;             // DMA4_CSDPi[13]
  UINT32    NumberOfElementPerFrame;       // DMA4_CENi
  UINT32    NumberOfFramePerTransferBlock; // DMA4_CFNi
  UINT32    SourceStartAddress;            // DMA4_CSSAi
  UINT32    DestinationStartAddress;       // DMA4_CDSAi
  UINT32    SourceElementIndex;            // DMA4_CSEi
  UINT32    SourceFrameIndex;              // DMA4_CSFi
  UINT32    DestinationElementIndex;       // DMA4_CDEi
  UINT32    DestinationFrameIndex;         // DMA4_CDFi
  UINT8     ReadPortAccessMode;            // DMA4_CCRi[13:12]
  UINT8     WritePortAccessMode;           // DMA4_CCRi[15:14]
  UINT8     ReadPriority;                  // DMA4_CCRi[6]
  UINT8     WritePriority;                 // DMA4_CCRi[23]
  UINT8     ReadRequestNumber;             // DMA4_CCRi[4:0]
  UINT8     WriteRequestNumber;            // DMA4_CCRi[20:19]
} OMAP_DMA4;


/**
  Configure OMAP DMA Channel

  @param  Channel               DMA Channel to configure
  @param  Dma4                  Pointer to structure used to initialize DMA registers for the Channel

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_INVALID_PARAMETER Channel is not valid
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested information.

**/
EFI_STATUS
EFIAPI
EnableDmaChannel (
  IN  UINTN       Channel,
  IN  OMAP_DMA4   *Dma4
  );

/**
  Turn of DMA channel configured by EnableDma().

  @param  Channel               DMA Channel to configure
  @param  SuccesMask            Bits in DMA4_CSR register indicate EFI_SUCCESS
  @param  ErrorMask             Bits in DMA4_CSR register indicate EFI_DEVICE_ERROR

  @retval EFI_SUCCESS           DMA hardware disabled
  @retval EFI_INVALID_PARAMETER Channel is not valid
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested information.

**/
EFI_STATUS
EFIAPI
DisableDmaChannel (
  IN  UINTN       Channel,
  IN  UINT32      SuccessMask,
  IN  UINT32      ErrorMask
  );



#endif

