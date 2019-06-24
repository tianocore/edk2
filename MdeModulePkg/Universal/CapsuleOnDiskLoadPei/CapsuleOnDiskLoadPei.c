/** @file
  Recovery module.

  Caution: This module requires additional review when modified.
  This module will have external input - Capsule-on-Disk Temp Relocation image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  RetrieveRelocatedCapsule() will receive untrusted input and do basic validation.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <Uefi.h>
#include <PiPei.h>

//
// The protocols, PPI and GUID defintions for this module
//
#include <Ppi/MasterBootMode.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/Capsule.h>
#include <Ppi/CapsuleOnDisk.h>
#include <Ppi/DeviceRecoveryModule.h>

#include <Guid/FirmwareFileSystem2.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/CapsuleLib.h>
#include <Library/ReportStatusCodeLib.h>

/**
  Loads a DXE capsule from some media into memory and updates the HOB table
  with the DXE firmware volume information.

  @param[in]  PeiServices   General-purpose services that are available to every PEIM.
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_MODULE_PPI instance.

  @retval EFI_SUCCESS        The capsule was loaded correctly.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A recovery DXE capsule cannot be found.

**/
EFI_STATUS
EFIAPI
LoadCapsuleOnDisk (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EDKII_PEI_CAPSULE_ON_DISK_PPI   *This
  );

static EDKII_PEI_CAPSULE_ON_DISK_PPI mCapsuleOnDiskPpi = {
  LoadCapsuleOnDisk
};

static EFI_PEI_PPI_DESCRIPTOR mCapsuleOnDiskPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiPeiCapsuleOnDiskPpiGuid,
  &mCapsuleOnDiskPpi
};

/**
  Determine if capsule comes from memory by checking Capsule PPI.

  @param[in]  PeiServices General purpose services available to every PEIM.

  @retval TRUE   Capsule comes from memory.
  @retval FALSE  No capsule comes from memory.

**/
static
BOOLEAN
CheckCapsuleFromRam (
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{
  EFI_STATUS              Status;
  PEI_CAPSULE_PPI         *Capsule;

  Status = PeiServicesLocatePpi (
             &gEfiPeiCapsulePpiGuid,
             0,
             NULL,
             (VOID **) &Capsule
             );
  if (!EFI_ERROR(Status)) {
    Status = Capsule->CheckCapsuleUpdate ((EFI_PEI_SERVICES **)PeiServices);
    if (!EFI_ERROR(Status)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Determine if it is a Capsule On Disk mode.

  @retval TRUE         Capsule On Disk mode.
  @retval FALSE        Not capsule On Disk mode.

**/
static
BOOLEAN
IsCapsuleOnDiskMode (
  VOID
  )
{
  EFI_STATUS                      Status;
  UINTN                           Size;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI *PPIVariableServices;
  BOOLEAN                         CodRelocInfo;

  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **) &PPIVariableServices
             );
  ASSERT_EFI_ERROR (Status);

  Size = sizeof (BOOLEAN);
  Status = PPIVariableServices->GetVariable (
                                  PPIVariableServices,
                                  COD_RELOCATION_INFO_VAR_NAME,
                                  &gEfiCapsuleVendorGuid,
                                  NULL,
                                  &Size,
                                  &CodRelocInfo
                                  );

  if (EFI_ERROR (Status) || Size != sizeof(BOOLEAN) || !CodRelocInfo) {
    DEBUG (( DEBUG_ERROR, "Error Get CodRelocationInfo variable %r!\n", Status));
    return FALSE;
  }

  return TRUE;
}

/**
  Gets capsule images from relocated capsule buffer.
  Create Capsule hob for each Capsule.

  Caution: This function may receive untrusted input.
  Capsule-on-Disk Temp Relocation image is external input, so this function
  will validate Capsule-on-Disk Temp Relocation image to make sure the content
  is read within the buffer.

  @param[in]  RelocCapsuleBuf        Buffer pointer to the relocated capsule.
  @param[in]  RelocCapsuleTotalSize  Total size of the relocated capsule.

  @retval EFI_SUCCESS     Succeed to get capsules and create hob.
  @retval Others          Fail to get capsules and create hob.

**/
static
EFI_STATUS
RetrieveRelocatedCapsule (
  IN UINT8                *RelocCapsuleBuf,
  IN UINTN                RelocCapsuleTotalSize
  )
{
  UINTN                    Index;
  UINT8                    *CapsuleDataBufEnd;
  UINT8                    *CapsulePtr;
  UINT32                   CapsuleSize;
  UINT64                   TotalImageSize;
  UINTN                    CapsuleNum;

  //
  // Temp file contains at least 2 capsule (including 1 capsule name capsule) & 1 UINT64
  //
  if (RelocCapsuleTotalSize < sizeof(UINT64) + sizeof(EFI_CAPSULE_HEADER) * 2) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem(&TotalImageSize, RelocCapsuleBuf, sizeof(UINT64));

  DEBUG ((DEBUG_INFO, "ProcessRelocatedCapsule CapsuleBuf %x TotalCapSize %lx\n",
                      RelocCapsuleBuf, TotalImageSize));

  RelocCapsuleBuf += sizeof(UINT64);

  //
  // TempCaspule file length check
  //
  if (MAX_ADDRESS - TotalImageSize <= sizeof(UINT64) ||
      (UINT64)RelocCapsuleTotalSize != TotalImageSize + sizeof(UINT64) ||
      (UINTN)(MAX_ADDRESS - (PHYSICAL_ADDRESS)(UINTN)RelocCapsuleBuf) <= TotalImageSize) {
    return EFI_INVALID_PARAMETER;
  }

  CapsuleDataBufEnd = RelocCapsuleBuf + TotalImageSize;

  //
  // TempCapsule file integrity check over Capsule Header to ensure no data corruption in NV Var & Relocation storage
  //
  CapsulePtr = RelocCapsuleBuf;
  CapsuleNum = 0;

  while (CapsulePtr < CapsuleDataBufEnd) {
    if ((CapsuleDataBufEnd - CapsulePtr) < sizeof(EFI_CAPSULE_HEADER) ||
        ((EFI_CAPSULE_HEADER *)CapsulePtr)->CapsuleImageSize < sizeof(EFI_CAPSULE_HEADER) ||
        (UINTN)(MAX_ADDRESS - (PHYSICAL_ADDRESS)(UINTN)CapsulePtr) < ((EFI_CAPSULE_HEADER *)CapsulePtr)->CapsuleImageSize
        ) {
      break;
    }
    CapsulePtr += ((EFI_CAPSULE_HEADER *)CapsulePtr)->CapsuleImageSize;
    CapsuleNum ++;
  }

  if (CapsulePtr != CapsuleDataBufEnd) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Capsule count must be less than PcdCapsuleMax, avoid building too many CvHobs to occupy all the free space in HobList.
  //
  if (CapsuleNum > PcdGet16 (PcdCapsuleMax)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Re-iterate the capsule buffer to create Capsule hob & Capsule Name Str Hob for each Capsule saved in relocated capsule file
  //
  CapsulePtr = RelocCapsuleBuf;
  Index      = 0;
  while (CapsulePtr < CapsuleDataBufEnd) {
    CapsuleSize = ((EFI_CAPSULE_HEADER *)CapsulePtr)->CapsuleImageSize;
    BuildCvHob ((EFI_PHYSICAL_ADDRESS)(UINTN)CapsulePtr, CapsuleSize);

    DEBUG((DEBUG_INFO, "Capsule saved in address %x size %x\n", CapsulePtr, CapsuleSize));

    CapsulePtr += CapsuleSize;
    Index++;
  }

  return EFI_SUCCESS;
}

/**
  Recovery module entrypoint

  @param[in] FileHandle   Handle of the file being invoked.
  @param[in] PeiServices  Describes the list of possible PEI Services.

  @return EFI_SUCCESS Recovery module is initialized.
**/
EFI_STATUS
EFIAPI
InitializeCapsuleOnDiskLoad (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;
  UINTN       BootMode;
  UINTN       FileNameSize;

  BootMode = GetBootModeHob();
  ASSERT(BootMode == BOOT_ON_FLASH_UPDATE);

  //
  // If there are capsules provisioned in memory, quit.
  // Only one capsule resource is accept, CapsuleOnRam's priority is higher than CapsuleOnDisk.
  //
  if (CheckCapsuleFromRam(PeiServices)) {
    DEBUG((DEBUG_ERROR, "Capsule On Memory Detected! Quit.\n"));
    return EFI_ABORTED;
  }

  DEBUG_CODE (
   VOID *CapsuleOnDiskModePpi;

  if (!IsCapsuleOnDiskMode()){
    return EFI_NOT_FOUND;
  }

  //
  // Check Capsule On Disk Relocation flag. If exists, load capsule & create Capsule Hob
  //
  Status = PeiServicesLocatePpi (
             &gEdkiiPeiBootInCapsuleOnDiskModePpiGuid,
             0,
             NULL,
             (VOID **)&CapsuleOnDiskModePpi
             );
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "Locate CapsuleOnDiskModePpi error %x\n", Status));
      return Status;
    }
  );

  Status = PeiServicesInstallPpi (&mCapsuleOnDiskPpiList);
  ASSERT_EFI_ERROR (Status);

  FileNameSize = PcdGetSize (PcdCoDRelocationFileName);
  Status = PcdSetPtrS (PcdRecoveryFileName, &FileNameSize, (VOID *) PcdGetPtr(PcdCoDRelocationFileName));
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Loads a DXE capsule from some media into memory and updates the HOB table
  with the DXE firmware volume information.

  @param[in]  PeiServices   General-purpose services that are available to every PEIM.
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_MODULE_PPI instance.

  @retval EFI_SUCCESS        The capsule was loaded correctly.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A recovery DXE capsule cannot be found.

**/
EFI_STATUS
EFIAPI
LoadCapsuleOnDisk (
  IN EFI_PEI_SERVICES                     **PeiServices,
  IN EDKII_PEI_CAPSULE_ON_DISK_PPI          *This
  )
{
  EFI_STATUS                          Status;
  EFI_PEI_DEVICE_RECOVERY_MODULE_PPI  *DeviceRecoveryPpi;
  UINTN                               NumberRecoveryCapsules;
  UINTN                               Instance;
  UINTN                               CapsuleInstance;
  UINTN                               CapsuleSize;
  EFI_GUID                            CapsuleType;
  VOID                                *CapsuleBuffer;

  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Load Capsule On Disk Entry\n"));

  for (Instance = 0; ; Instance++) {
    Status = PeiServicesLocatePpi (
               &gEfiPeiDeviceRecoveryModulePpiGuid,
               Instance,
               NULL,
               (VOID **)&DeviceRecoveryPpi
               );
    DEBUG ((DEBUG_INFO, "LoadCapsuleOnDisk - LocateRecoveryPpi (%d) - %r\n", Instance, Status));
    if (EFI_ERROR (Status)) {
      if (Instance == 0) {
        REPORT_STATUS_CODE (
          EFI_ERROR_CODE | EFI_ERROR_MAJOR,
          (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_RECOVERY_PPI_NOT_FOUND)
          );
      }
      break;
    }
    NumberRecoveryCapsules = 0;
    Status = DeviceRecoveryPpi->GetNumberRecoveryCapsules (
                                  (EFI_PEI_SERVICES **)PeiServices,
                                  DeviceRecoveryPpi,
                                  &NumberRecoveryCapsules
                                  );
    DEBUG ((DEBUG_INFO, "LoadCapsuleOnDisk - GetNumberRecoveryCapsules (%d) - %r\n", NumberRecoveryCapsules, Status));
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (CapsuleInstance = 1; CapsuleInstance <= NumberRecoveryCapsules; CapsuleInstance++) {
      CapsuleSize = 0;
      Status = DeviceRecoveryPpi->GetRecoveryCapsuleInfo (
                                    (EFI_PEI_SERVICES **)PeiServices,
                                    DeviceRecoveryPpi,
                                    CapsuleInstance,
                                    &CapsuleSize,
                                    &CapsuleType
                                    );
      DEBUG ((DEBUG_INFO, "LoadCapsuleOnDisk - GetRecoveryCapsuleInfo (%d - %x) - %r\n", CapsuleInstance, CapsuleSize, Status));
      if (EFI_ERROR (Status)) {
        break;
      }

      //
      // Allocate the memory so that it gets preserved into DXE.
      // Capsule is special because it may need to populate to system table
      //
      CapsuleBuffer = AllocateRuntimePages (EFI_SIZE_TO_PAGES (CapsuleSize));

      if (CapsuleBuffer == NULL) {
        DEBUG ((DEBUG_ERROR, "LoadCapsuleOnDisk - AllocateRuntimePages fail\n"));
        continue;
      }

      Status = DeviceRecoveryPpi->LoadRecoveryCapsule (
                                    (EFI_PEI_SERVICES **)PeiServices,
                                    DeviceRecoveryPpi,
                                    CapsuleInstance,
                                    CapsuleBuffer
                                    );
      DEBUG ((DEBUG_INFO, "LoadCapsuleOnDisk - LoadRecoveryCapsule (%d) - %r\n", CapsuleInstance, Status));
      if (EFI_ERROR (Status)) {
        FreePages (CapsuleBuffer, EFI_SIZE_TO_PAGES(CapsuleSize));
        break;
      }

      //
      // Capsule Update Mode, Split relocated Capsule buffer into different capsule vehical hobs.
      //
      Status = RetrieveRelocatedCapsule(CapsuleBuffer, CapsuleSize);

      break;
    }

    if (EFI_ERROR (Status)) {
      REPORT_STATUS_CODE (
        EFI_ERROR_CODE | EFI_ERROR_MAJOR,
        (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_NO_RECOVERY_CAPSULE)
        );
    }

    return Status;
  }

  //
  // Any attack against GPT, Relocation Info Variable or temp relocation file will result in no Capsule HOB and return EFI_NOT_FOUND.
  // After flow to DXE phase. since no capsule hob is detected. Platform will clear Info flag and force restart.
  // No volunerability will be exposed
  //

  return EFI_NOT_FOUND;
}
