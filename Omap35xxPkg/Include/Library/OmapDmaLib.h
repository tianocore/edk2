/** @file

  Abstractions for simple OMAP DMA. The DMA functions are modeled on 
  the PCI IO protocol and follow the same rules as outlined in the UEFI specification.
  OMAP_DMA4 structure elements are described in the OMAP35xx TRM. Currently 
  there is no PCI'less DMA protocol, if one existed it could be used to
  replace this library.

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

typedef enum {
  ///
  /// A read operation from system memory by a bus master.
  ///
  MapOperationBusMasterRead,
  ///
  /// A write operation from system memory by a bus master.
  ///
  MapOperationBusMasterWrite,
  ///
  /// Provides both read and write access to system memory by both the processor and a
  /// bus master. The buffer is coherent from both the processor's and the bus master's point of view.
  ///
  MapOperationBusMasterCommonBuffer,
  MapOperationMaximum
} DMA_MAP_OPERATION;


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
  );




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
  );


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
                                   
**/
EFI_STATUS
EFIAPI
DmaAllocateBuffer (
  IN  EFI_MEMORY_TYPE              MemoryType,
  IN  UINTN                        Pages,
  OUT VOID                         **HostAddress
  );


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
  );


#endif 

