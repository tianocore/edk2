/** @file
  Master header file for DxeIpl PEIM. All source files in this module should
  include this file for common defininitions.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PEI_DXEIPL_H__
#define __PEI_DXEIPL_H__

#include <PiPei.h>
#include <Ppi/DxeIpl.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Guid/MemoryTypeInformation.h>
#include <Ppi/Decompress.h>
#include <Ppi/FirmwareVolumeInfo.h>

#include <Guid/MemoryAllocationHob.h>
#include <Guid/FirmwareFileSystem2.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/UefiDecompressLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffLib.h>
#include <Library/S3Lib.h>
#include <Library/RecoveryLib.h>

#define STACK_SIZE      0x20000
#define BSP_STORE_SIZE  0x4000

#define GET_OCCUPIED_SIZE(ActualSize, Alignment) ((ActualSize + (Alignment - 1)) & ~(Alignment - 1))

extern BOOLEAN gInMemory;




/**
   Loads and relocates a PE/COFF image into memory.

   @param FileHandle        The image file handle
   @param ImageAddress      The base address of the relocated PE/COFF image
   @param ImageSize         The size of the relocated PE/COFF image
   @param EntryPoint        The entry point of the relocated PE/COFF image
   
   @return EFI_SUCCESS           The file was loaded and relocated
   @return EFI_OUT_OF_RESOURCES  There was not enough memory to load and relocate the PE/COFF file

**/
EFI_STATUS
PeiLoadFile (
  IN  EFI_PEI_FILE_HANDLE                       FileHandle,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  );



/**
   Find DxeCore driver from all First Volumes.

   @param FileHandle    Pointer to FFS file to search.
   
   @return EFI_SUCESS   Success to find the FFS in specificed FV
   @return others       Fail to find the FFS in specificed FV

**/
EFI_STATUS
DxeIplFindDxeCore (
  OUT EFI_PEI_FILE_HANDLE   *FileHandle
  );



/**
   This function simply retrieves the function pointer of ImageRead in
   ImageContext structure.
    
   @param ImageContext       A pointer to the structure of 
                             PE_COFF_LOADER_IMAGE_CONTEXT
   
   @retval EFI_SUCCESS       This function always return EFI_SUCCESS.

**/
EFI_STATUS
GetImageReadFunction (
  IN      PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );



/**
   Main entry point to last PEIM 
    
   @param This          Entry point for DXE IPL PPI
   @param PeiServices   General purpose services available to every PEIM.
   @param HobList       Address to the Pei HOB list
   
   @return EFI_SUCCESS              DXE core was successfully loaded. 
   @return EFI_OUT_OF_RESOURCES     There are not enough resources to load DXE core.

**/
EFI_STATUS
EFIAPI
DxeLoadCore (
  IN EFI_DXE_IPL_PPI       *This,
  IN EFI_PEI_SERVICES      **PeiServices,
  IN EFI_PEI_HOB_POINTERS  HobList
  );



/**
   Transfers control to DxeCore.

   This function performs a CPU architecture specific operations to execute
   the entry point of DxeCore with the parameters of HobList.
   It also intalls EFI_END_OF_PEI_PPI to signal the end of PEI phase.

   @param DxeCoreEntryPoint         The entrypoint of DxeCore.
   @param HobList                   The start of HobList passed to DxeCore.
   @param EndOfPeiSignal            The PPI descriptor for EFI_END_OF_PEI_PPI.

**/
VOID
HandOffToDxeCore (
  IN EFI_PHYSICAL_ADDRESS   DxeCoreEntryPoint,
  IN EFI_PEI_HOB_POINTERS   HobList,
  IN EFI_PEI_PPI_DESCRIPTOR *EndOfPeiSignal
  );



/**
   Updates the Stack HOB passed to DXE phase.

   This function traverses the whole HOB list and update the stack HOB to
   reflect the real stack that is used by DXE core.

   @param BaseAddress           The lower address of stack used by DxeCore.
   @param Length                The length of stack used by DxeCore.

**/
VOID
UpdateStackHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  );



/**
  Initializes the Dxe Ipl PPI

  @param  FfsHandle   The handle of FFS file.
  @param  PeiServices General purpose services available to
                      every PEIM.
  @return EFI_SUCESS

**/
EFI_STATUS
EFIAPI
PeimInitializeDxeIpl (
  IN EFI_PEI_FILE_HANDLE       FfsHandle,
  IN EFI_PEI_SERVICES          **PeiServices
  );


#endif
