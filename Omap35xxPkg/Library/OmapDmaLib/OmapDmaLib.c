/** @file
  Abstractions for simple OMAP DMA channel.


  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/OmapDmaLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Omap3530/Omap3530.h>


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
  IN  OMAP_DMA4   *DMA4
  )
{
  UINT32  RegVal;


  if (Channel > DMA4_MAX_CHANNEL) {
    return EFI_INVALID_PARAMETER;
  }

  /* 1) Configure the transfer parameters in the logical DMA registers */
  /*-------------------------------------------------------------------*/

  /* a) Set the data type CSDP[1:0], the Read/Write Port access type
        CSDP[8:7]/[15:14], the Source/dest endianism CSDP[21]/CSDP[19],
        write mode CSDP[17:16], source/dest packed or nonpacked CSDP[6]/CSDP[13] */

  // Read CSDP
  RegVal = MmioRead32 (DMA4_CSDP (Channel));

  // Build reg
  RegVal = ((RegVal & ~ 0x3) | DMA4->DataType );
  RegVal = ((RegVal & ~(0x3 <<  7)) | (DMA4->ReadPortAccessType << 7));
  RegVal = ((RegVal & ~(0x3 << 14)) | (DMA4->WritePortAccessType << 14));
  RegVal = ((RegVal & ~(0x1 << 21)) | (DMA4->SourceEndiansim << 21));
  RegVal = ((RegVal & ~(0x1 << 19)) | (DMA4->DestinationEndianism << 19));
  RegVal = ((RegVal & ~(0x3 << 16)) | (DMA4->WriteMode << 16));
  RegVal = ((RegVal & ~(0x1 <<  6)) | (DMA4->SourcePacked << 6));
  RegVal = ((RegVal & ~(0x1 << 13)) | (DMA4->DestinationPacked << 13));
  // Write CSDP
  MmioWrite32 (DMA4_CSDP (Channel), RegVal);

  /* b) Set the number of element per frame CEN[23:0]*/
  MmioWrite32 (DMA4_CEN (Channel), DMA4->NumberOfElementPerFrame);

  /* c) Set the number of frame per block CFN[15:0]*/
  MmioWrite32 (DMA4_CFN (Channel), DMA4->NumberOfFramePerTransferBlock);

  /* d) Set the Source/dest start address index CSSA[31:0]/CDSA[31:0]*/
  MmioWrite32 (DMA4_CSSA (Channel), DMA4->SourceStartAddress);
  MmioWrite32 (DMA4_CDSA (Channel), DMA4->DestinationStartAddress);

  /* e) Set the Read Port addressing mode CCR[13:12], the Write Port addressing mode CCR[15:14],
        read/write priority CCR[6]/CCR[26]
        I changed LCH CCR[20:19]=00 and CCR[4:0]=00000 to
        LCH CCR[20:19]= DMA4->WriteRequestNumber and CCR[4:0]=DMA4->ReadRequestNumber
  */

  // Read CCR
  RegVal = MmioRead32 (DMA4_CCR (Channel));

  // Build reg
  RegVal  = ((RegVal &  ~0x1f)            | DMA4->ReadRequestNumber);
  RegVal  = ((RegVal &  ~(BIT20 | BIT19)) | DMA4->WriteRequestNumber << 19);
  RegVal  = ((RegVal & ~(0x3 << 12)) | (DMA4->ReadPortAccessMode << 12));
  RegVal  = ((RegVal & ~(0x3 << 14)) | (DMA4->WritePortAccessMode << 14));
  RegVal  = ((RegVal & ~(0x1 <<  6)) | (DMA4->ReadPriority << 6));
  RegVal  = ((RegVal & ~(0x1 << 26)) | (DMA4->WritePriority << 26));

  // Write CCR
  MmioWrite32 (DMA4_CCR (Channel), RegVal);

  /* f)- Set the source element index CSEI[15:0]*/
  MmioWrite32 (DMA4_CSEI (Channel), DMA4->SourceElementIndex);

  /* - Set the source frame index CSFI[15:0]*/
  MmioWrite32 (DMA4_CSFI (Channel), DMA4->SourceFrameIndex);


  /* - Set the destination element index CDEI[15:0]*/
  MmioWrite32 (DMA4_CDEI (Channel), DMA4->DestinationElementIndex);

  /* - Set the destination frame index CDFI[31:0]*/
  MmioWrite32 (DMA4_CDFI (Channel), DMA4->DestinationFrameIndex);

  MmioWrite32 (DMA4_CDFI (Channel), DMA4->DestinationFrameIndex);

  // Enable all the status bits since we are polling
  MmioWrite32 (DMA4_CICR (Channel), DMA4_CICR_ENABLE_ALL);
  MmioWrite32 (DMA4_CSR (Channel),  DMA4_CSR_RESET);

  /* 2) Start the DMA transfer by Setting the enable bit CCR[7]=1 */
  /*--------------------------------------------------------------*/
  //write enable bit
  MmioOr32 (DMA4_CCR(Channel), DMA4_CCR_ENABLE); //Launch transfer

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINT32      Reg;


  if (Channel > DMA4_MAX_CHANNEL) {
    return EFI_INVALID_PARAMETER;
  }

  do {
    Reg = MmioRead32 (DMA4_CSR(Channel));
    if ((Reg & ErrorMask) != 0) {
      Status = EFI_DEVICE_ERROR;
      DEBUG ((EFI_D_ERROR, "DMA Error (%d) %x\n", Channel, Reg));
      break;
    }
  } while ((Reg & SuccessMask) != SuccessMask);


  // Disable all status bits and clear them
  MmioWrite32 (DMA4_CICR (Channel), 0);
  MmioWrite32 (DMA4_CSR (Channel),  DMA4_CSR_RESET);

  MmioAnd32 (DMA4_CCR(0), ~(DMA4_CCR_ENABLE | DMA4_CCR_RD_ACTIVE | DMA4_CCR_WR_ACTIVE));
  return Status;
}



