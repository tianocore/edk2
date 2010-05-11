/** @file
  OMAP35xx DMA abstractions modeled on PCI IO protocol. EnableDma()/DisableDma()
  are from OMAP35xx TRM. 

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
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UncachedMemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Omap3530/Omap3530.h>

#include <Protocol/Cpu.h>

typedef struct {
  EFI_PHYSICAL_ADDRESS      HostAddress;
  EFI_PHYSICAL_ADDRESS      DeviceAddress;
  UINTN                     NumberOfBytes;
  DMA_MAP_OPERATION         Operation;
} MAP_INFO_INSTANCE;



EFI_CPU_ARCH_PROTOCOL      *gCpu;

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



/**                                                                 
  Provides the DMA controller-specific addresses needed to access system memory.
  
  Operation is relative to the DMA bus master.
            
  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the DMA controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.                                                 
  @param  DeviceAddress         The resulting map address for the bus master controller to use to
                                access the hosts HostAddress.                                        
  @param  Mapping               A resulting value to pass to Unmap().
                                  
  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.                                
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.
                                   
**/
EFI_STATUS
EFIAPI
DmaMap (
  IN     DMA_MAP_OPERATION              Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    PHYSICAL_ADDRESS               *DeviceAddress,
  OUT    VOID                           **Mapping
  )
{
  MAP_INFO_INSTANCE     *Map;

  if ( HostAddress == NULL || NumberOfBytes == NULL || 
       DeviceAddress == NULL || Mapping == NULL ) {
    return EFI_INVALID_PARAMETER;
  }
  

  if (Operation >= MapOperationMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  *DeviceAddress = ConvertToPhysicalAddress (HostAddress);

  // Remember range so we can flush on the other side
  Map = AllocatePool (sizeof (MAP_INFO_INSTANCE));
  if (Map == NULL) {
    return  EFI_OUT_OF_RESOURCES;
  }
  
  *Mapping = Map;

  Map->HostAddress   = (UINTN)HostAddress;
  Map->DeviceAddress = *DeviceAddress;
  Map->NumberOfBytes = *NumberOfBytes;
  Map->Operation     = Operation;

  // EfiCpuFlushTypeWriteBack, EfiCpuFlushTypeInvalidate
  gCpu->FlushDataCache (gCpu, (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress, *NumberOfBytes, EfiCpuFlushTypeWriteBackInvalidate);
  
  return EFI_SUCCESS;
}


/**                                                                 
  Completes the DmaMapBusMasterRead(), DmaMapBusMasterWrite(), or DmaMapBusMasterCommonBuffer()
  operation and releases any corresponding resources.
            
  @param  Mapping               The mapping value returned from DmaMap*().
                                  
  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.
                                   
**/
EFI_STATUS
EFIAPI
DmaUnmap (
  IN  VOID                         *Mapping
  )
{
  MAP_INFO_INSTANCE *Map;
  
  if (Mapping == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }
  
  Map = (MAP_INFO_INSTANCE *)Mapping;
  if (Map->Operation == MapOperationBusMasterWrite) {
    //
    // Make sure we read buffer from uncached memory and not the cache
    //
    gCpu->FlushDataCache (gCpu, Map->HostAddress, Map->NumberOfBytes, EfiCpuFlushTypeInvalidate);
  } 
  
  FreePool (Map);

  return EFI_SUCCESS;
}

/**                                                                 
  Allocates pages that are suitable for an DmaMap() of type MapOperationBusMasterCommonBuffer.
  mapping.                                                                       
            
  @param  MemoryType            The type of memory to allocate, EfiBootServicesData or
                                EfiRuntimeServicesData.                               
  @param  Pages                 The number of pages to allocate.                                
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.                                        

                                @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.                     
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.  
                                   
**/EFI_STATUS
EFIAPI
DmaAllocateBuffer (
  IN  EFI_MEMORY_TYPE              MemoryType,
  IN  UINTN                        Pages,
  OUT VOID                         **HostAddress
  )
{
  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The only valid memory types are EfiBootServicesData and EfiRuntimeServicesData
  //
  // We used uncached memory to keep coherency
  //
  if (MemoryType == EfiBootServicesData) {
    *HostAddress = UncachedAllocatePages (Pages);
  } else if (MemoryType != EfiRuntimeServicesData) {
    *HostAddress = UncachedAllocateRuntimePages (Pages);
  } else {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}


/**                                                                 
  Frees memory that was allocated with DmaAllocateBuffer().
            
  @param  Pages                 The number of pages to free.                                
  @param  HostAddress           The base system memory address of the allocated range.                                    
                                  
  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with DmaAllocateBuffer().
                                     
**/
EFI_STATUS
EFIAPI
DmaFreeBuffer (
  IN  UINTN                        Pages,
  IN  VOID                         *HostAddress
  )
{
  if (HostAddress == NULL) {
     return EFI_INVALID_PARAMETER;
  } 
  
  UncachedFreePages (HostAddress, Pages);
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
OmapDmaLibConstructor (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS              Status;

  // Get the Cpu protocol for later use
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&gCpu);
  ASSERT_EFI_ERROR(Status);

  return EFI_SUCCESS;
}

