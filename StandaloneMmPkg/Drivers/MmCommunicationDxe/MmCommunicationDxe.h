/** @file

Copyright (c) 2016 - 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/MmCommunication.h>

#include <Guid/EventGroup.h>
#include <Guid/MemoryAttributesTable.h>
#include <Guid/PiSmmCommunicationRegionTable.h>
#include <Guid/MmCommBuffer.h>

#include <PiDxe.h>

#include <Protocol/SmmBase2.h>
#include <Protocol/MmCommunication2.h>
#include <Protocol/SmmAccess2.h>
#include <Protocol/SmmConfiguration.h>
#include <Protocol/SmmControl2.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/Cpu.h>

#include <Guid/EventGroup.h>

#include <Library/DxeServicesTableLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/ReportStatusCodeLib.h>

extern EFI_MEMORY_DESCRIPTOR *mUefiMemoryMap;
extern UINTN                 mUefiMemoryMapSize;
extern UINTN                 mUefiDescriptorSize;

extern EFI_GCD_MEMORY_SPACE_DESCRIPTOR   *mMemorySpaceMap;
extern UINTN                             mNumberOfDescriptors;

extern EFI_GCD_MEMORY_SPACE_DESCRIPTOR   *mGcdMemSpace;
extern UINTN                             mGcdMemNumberOfDesc;

extern EFI_MEMORY_ATTRIBUTES_TABLE  *mUefiMemoryAttributesTable;

extern EFI_MM_COMMUNICATION2_PROTOCOL  mMmCommunication2;

EFI_STATUS
EFIAPI
SmmCommunicationMmCommunicate2 (
  IN CONST EFI_MM_COMMUNICATION2_PROTOCOL  *This,
  IN OUT VOID                              *CommBufferPhysical,
  IN OUT VOID                              *CommBufferVirtual,
  IN OUT UINTN                             *CommSize OPTIONAL
  );
