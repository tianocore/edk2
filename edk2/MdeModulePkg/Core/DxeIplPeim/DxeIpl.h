/**@file

Copyright (c) 2006 - 2007, Intel Corporation
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
#include <Protocol/EdkDecompress.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Protocol/CustomizedDecompress.h>
#include <Protocol/Decompress.h>
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

EFI_STATUS
PeiLoadFile (
  IN  EFI_PEI_FILE_HANDLE                       FileHandle,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  )
;

EFI_STATUS
DxeIplFindDxeCore (
  OUT EFI_PEI_FILE_HANDLE   *FileHandle
  )
;

EFI_STATUS
GetImageReadFunction (
  IN      PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
;

EFI_STATUS
EFIAPI
DxeLoadCore (
  IN EFI_DXE_IPL_PPI       *This,
  IN EFI_PEI_SERVICES      **PeiServices,
  IN EFI_PEI_HOB_POINTERS  HobList
  );

VOID
HandOffToDxeCore (
  IN EFI_PHYSICAL_ADDRESS   DxeCoreEntryPoint,
  IN EFI_PEI_HOB_POINTERS   HobList,
  IN EFI_PEI_PPI_DESCRIPTOR *EndOfPeiSignal
  );

VOID
UpdateStackHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  );

EFI_STATUS
EFIAPI
PeimInitializeDxeIpl (
  IN EFI_PEI_FILE_HANDLE       FfsHandle,
  IN EFI_PEI_SERVICES          **PeiServices
  )
;


#endif
