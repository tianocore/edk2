/** @file
  Provides FMP capsule dependency check services when updating the firmware
  image of a FMP device.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FmpDependencyLib.h>
#include <Library/FmpDependencyCheckLib.h>
#include <Library/FmpDeviceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Protocol/VariableLock.h>

#define FMP_DEPENDENCY_VARIABLE_NAME  L"FmpDepex"

/**
  Check if there is dependency in EFI_FIRMWARE_IMAGE_DESCRIPTOR.

  @param[in]  FmpImageInfoBuf   Pointer to EFI Firmware Image Descriptor.
  @param[in]  DescriptorVer     Version of the descriptor.
  @param[in]  DescriptorSize    Size of the Descriptor.

  @retval  TRUE    There is dependency in descriptor.
  @retval  FALSE   There is no dependency in descriptor.

**/
BOOLEAN
DoDependencyExist (
  IN EFI_FIRMWARE_IMAGE_DESCRIPTOR  *FmpImageInfoBuf,
  IN UINT32                         DescriptorVer,
  IN UINTN                          DescriptorSize
  )
{
  //
  // Descriptor version must be greater than or equal to 4.
  //
  if (DescriptorVer < 4) {
    return FALSE;
  }

  //
  // Dependency must be enabled.
  //
  if ((FmpImageInfoBuf->AttributesSetting & IMAGE_ATTRIBUTE_DEPENDENCY) == 0) {
    return FALSE;
  }

  //
  // Descriptor size must be large enough to reference the 'Dependencies' field.
  //
  if (DescriptorSize < OFFSET_OF(EFI_FIRMWARE_IMAGE_DESCRIPTOR, Dependencies)) {
    return FALSE;
  }

  //
  // Dependencies must not be NULL.
  //
  if (FmpImageInfoBuf->Dependencies == NULL) {
    return FALSE;
  }

  return TRUE;
}

/**
  Get the size of dependencies. Assume the dependencies is validated before
  calling this function.

  @param[in]  Dependencies   Pointer to the EFI_FIRMWARE_IMAGE_DEP.

  @retval  The size of dependencies.

**/
UINTN
GetDependencySize (
  IN CONST EFI_FIRMWARE_IMAGE_DEP  *Dependencies
  )
{
  UINTN Index;

  if (Dependencies == NULL) {
    return 0;
  }

  Index = 0;
  while (Dependencies->Dependencies[Index] != EFI_FMP_DEP_END) {
    Index ++;
  }

  return Index + 1;
}


/**
  Check dependency for firmware update.

  @param[in]  ImageTypeId        Image Type Id.
  @param[in]  Version            New version.
  @param[in]  Dependencies       Fmp dependency.
  @param[in]  DependenciesSize   Size, in bytes, of the Fmp dependency.

  @retval  TRUE    Dependencies are satisfied.
  @retval  FALSE   Dependencies are unsatisfied or dependency check fails.

**/
BOOLEAN
EFIAPI
CheckFmpDependency (
  IN  CONST EFI_GUID                ImageTypeId,
  IN  CONST UINT32                  Version,
  IN  CONST EFI_FIRMWARE_IMAGE_DEP  *Dependencies,    OPTIONAL
  IN  CONST UINT32                  DependenciesSize  OPTIONAL
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             Index;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *Fmp;
  UINTN                             ImageInfoSize;
  UINT32                            *DescriptorVer;
  UINT8                             FmpImageInfoCount;
  UINTN                             *DescriptorSize;
  UINT32                            PackageVersion;
  CHAR16                            *PackageVersionName;
  UINTN                             DepexSize;
  UINTN                             NumberOfFmpInstance;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR     **FmpImageInfoBuf;
  FMP_DEPEX_CHECK_VERSION_DATA      *FmpVersions;
  UINTN                             FmpVersionsCount;
  BOOLEAN                           IsSatisfied;

  FmpImageInfoBuf     = NULL;
  DescriptorVer       = NULL;
  DescriptorSize      = NULL;
  NumberOfFmpInstance = 0;
  FmpVersions         = NULL;
  FmpVersionsCount    = 0;
  IsSatisfied         = TRUE;
  PackageVersionName  = NULL;

  //
  // Get ImageDescriptors of all FMP instances, and archive them for dependency evaluation.
  //
  Status = gBS->LocateHandleBuffer (
                ByProtocol,
                &gEfiFirmwareManagementProtocolGuid,
                NULL,
                &NumberOfFmpInstance,
                &HandleBuffer
                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CheckFmpDependency: Get Firmware Management Protocol failed. (%r)", Status));
    goto cleanup;
  }

  FmpImageInfoBuf = AllocateZeroPool (sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR *) * NumberOfFmpInstance);
  if (FmpImageInfoBuf == NULL) {
    IsSatisfied = FALSE;
    goto cleanup;
  }

  DescriptorVer = AllocateZeroPool (sizeof(UINT32) * NumberOfFmpInstance);
  if (DescriptorVer == NULL ) {
    IsSatisfied = FALSE;
    goto cleanup;
  }

  DescriptorSize = AllocateZeroPool (sizeof(UINTN) * NumberOfFmpInstance);
  if (DescriptorSize == NULL ) {
    IsSatisfied = FALSE;
    goto cleanup;
  }

  FmpVersions = AllocateZeroPool (sizeof(FMP_DEPEX_CHECK_VERSION_DATA) * NumberOfFmpInstance);
  if (FmpVersions == NULL) {
    IsSatisfied = FALSE;
    goto cleanup;
  }

  for (Index = 0; Index < NumberOfFmpInstance; Index ++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareManagementProtocolGuid,
                    (VOID **) &Fmp
                    );
    if (EFI_ERROR(Status)) {
      continue;
    }

    ImageInfoSize = 0;
    Status = Fmp->GetImageInfo (
                    Fmp,
                    &ImageInfoSize,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );
    if (Status != EFI_BUFFER_TOO_SMALL) {
      continue;
    }

    FmpImageInfoBuf[Index] = AllocateZeroPool (ImageInfoSize);
    if (FmpImageInfoBuf[Index] == NULL) {
      continue;
    }

    Status = Fmp->GetImageInfo (
                    Fmp,
                    &ImageInfoSize,               // ImageInfoSize
                    FmpImageInfoBuf[Index],       // ImageInfo
                    &DescriptorVer[Index],        // DescriptorVersion
                    &FmpImageInfoCount,           // DescriptorCount
                    &DescriptorSize[Index],       // DescriptorSize
                    &PackageVersion,              // PackageVersion
                    &PackageVersionName           // PackageVersionName
                    );
    if (EFI_ERROR(Status)) {
      FreePool (FmpImageInfoBuf[Index]);
      FmpImageInfoBuf[Index] = NULL;
      continue;
    }

    if (PackageVersionName != NULL) {
      FreePool (PackageVersionName);
      PackageVersionName = NULL;
    }

    CopyGuid (&FmpVersions[FmpVersionsCount].ImageTypeId, &FmpImageInfoBuf[Index]->ImageTypeId);
    FmpVersions[FmpVersionsCount].Version = FmpImageInfoBuf[Index]->Version;
    FmpVersionsCount ++;
  }

  //
  // Step 1 - Evaluate firmware image's depex, against the version of other Fmp instances.
  //
  if (Dependencies != NULL) {
    IsSatisfied = EvaluateDependency (Dependencies, DependenciesSize, FmpVersions, FmpVersionsCount);
  }

  if (!IsSatisfied) {
    DEBUG ((DEBUG_ERROR, "CheckFmpDependency: %g\'s dependency is not satisfied!\n", ImageTypeId));
    goto cleanup;
  }

  //
  // Step 2 - Evaluate the depex of all other Fmp instances, against the new version in
  // the firmware image.
  //

  //
  // Update the new version to FmpVersions.
  //
  for (Index = 0; Index < FmpVersionsCount; Index ++) {
    if (CompareGuid (&ImageTypeId, &FmpVersions[Index].ImageTypeId)) {
      FmpVersions[Index].Version = Version;
      break;
    }
  }

  //
  // Evaluate the Dependencies one by one.
  //
  for (Index = 0; Index < NumberOfFmpInstance; Index ++) {
    if (FmpImageInfoBuf[Index] != NULL) {
      //
      // Skip the Fmp instance to be "SetImage".
      //
      if (CompareGuid (&ImageTypeId, &FmpImageInfoBuf[Index]->ImageTypeId)) {
        continue;
      }
      //
      // Check if dependency exists in the EFI_FIRMWARE_IMAGE_DESCRIPTOR.
      //
      if (DoDependencyExist (FmpImageInfoBuf[Index], DescriptorVer[Index], DescriptorSize[Index])) {
        //
        // Get the size of dependency.
        // Assume that the dependencies in EFI_FIRMWARE_IMAGE_DESCRIPTOR is validated when PopulateDescriptor().
        //
        DepexSize = GetDependencySize (FmpImageInfoBuf[Index]->Dependencies);
        if (DepexSize > 0) {
          IsSatisfied = EvaluateDependency (FmpImageInfoBuf[Index]->Dependencies, DepexSize, FmpVersions, FmpVersionsCount);
          if (!IsSatisfied) {
            DEBUG ((DEBUG_ERROR, "CheckFmpDependency: %g\'s dependency is not satisfied!\n", FmpImageInfoBuf[Index]->ImageTypeId));
            break;
          }
        }
      }
    }
  }

cleanup:
  if (FmpImageInfoBuf != NULL) {
    for (Index = 0; Index < NumberOfFmpInstance; Index ++) {
      if (FmpImageInfoBuf[Index] != NULL) {
        FreePool (FmpImageInfoBuf[Index]);
      }
    }
    FreePool (FmpImageInfoBuf);
  }

  if (DescriptorVer != NULL) {
    FreePool (DescriptorVer);
  }

  if (DescriptorSize != NULL) {
    FreePool (DescriptorSize);
  }

  if (FmpVersions != NULL) {
    FreePool (FmpVersions);
  }

  return IsSatisfied;
}
