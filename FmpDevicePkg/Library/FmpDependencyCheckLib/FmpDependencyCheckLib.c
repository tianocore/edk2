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

  if (!FeaturePcdGet (PcdFmpDependencyCheckEnable)) {
    return TRUE;
  }

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

/**
  Generates a Null-terminated Unicode string UEFI Variable name from a base name
  and a hardware instance.  If the hardware instance value is 0, then the base
  name is returned.  If the hardware instance value is non-zero, then the 64-bit
  hardware instance value is converted to a 16 character hex string and appended
  to base name.  The UEFI Variable name returned is allocated using the UEFI
  Boot Service AllocatePool().

  @return  Pointer to the allocated UEFI Variable name.  Returns NULL if the
           UEFI Variable can not be allocated.
**/
static
CHAR16 *
GenerateFmpDepexVariableName (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  CHAR16      *VariableName;
  UINT64      HardwareInstance;

  //
  // Allocate Unicode string with room for BaseVariableName and a 16 digit
  // hexadecimal value for the HardwareInstance value.
  //
  Size = StrSize (FMP_DEPENDENCY_VARIABLE_NAME) + 16 * sizeof (CHAR16);
  VariableName = AllocateCopyPool (Size, FMP_DEPENDENCY_VARIABLE_NAME);
  if (VariableName == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDependencyCheckLib: Failed to generate variable name %s.\n", FMP_DEPENDENCY_VARIABLE_NAME));
    return VariableName;
  }

  //
  // Get the hardware instance from FmpDeviceLib
  //
  Status = FmpDeviceGetHardwareInstance (&HardwareInstance);
  if (EFI_ERROR (Status)) {
    return VariableName;
  }

  UnicodeValueToStringS (
    &VariableName[StrLen(FMP_DEPENDENCY_VARIABLE_NAME)],
    Size,
    PREFIX_ZERO | RADIX_HEX,
    HardwareInstance,
    16
    );
  return VariableName;
}

/**
  Save dependency to Fmp device.

  @param[in]  Depex       Fmp dependency.
  @param[in]  DepexSize   Size, in bytes, of the Fmp dependency.

  @retval  EFI_SUCCESS       Save Fmp dependency succeeds.
  @retval  EFI_UNSUPPORTED   Save Fmp dependency is not supported.
  @retval  Others            Save Fmp dependency fails.

**/
EFI_STATUS
EFIAPI
SaveFmpDependency (
  IN EFI_FIRMWARE_IMAGE_DEP  *Depex,
  IN UINT32                  DepexSize
  )
{
  EFI_STATUS Status;
  CHAR16     *VariableName;

  if (!FeaturePcdGet (PcdFmpDependencyCheckEnable)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Generate dependency variable name by HardwareInstance.
  //
  VariableName = GenerateFmpDepexVariableName ();
  if (VariableName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Save dependency to variable.
  //
  Status = gRT->SetVariable (
             VariableName,
             &gEfiCallerIdGuid,
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
             DepexSize,
             (VOID *) Depex
             );

  if (VariableName != NULL) {
    FreePool (VariableName);
  }

  return Status;
}

/**
  Get dependency from the Fmp device.

  @param[out]  DepexSize   Size, in bytes, of the dependency.

  @retval  The pointer to dependency.
  @retval  NULL

**/
EFI_FIRMWARE_IMAGE_DEP*
EFIAPI
GetFmpDependency (
  OUT UINT32  *DepexSize  OPTIONAL
  )
{
  EFI_STATUS             Status;
  EFI_FIRMWARE_IMAGE_DEP *Depex;
  UINTN                  Size;
  CHAR16                 *VariableName;

  if (DepexSize != NULL) {
    *DepexSize = 0;
  }

  if (!FeaturePcdGet (PcdFmpDependencyCheckEnable)) {
    return NULL;
  }

  Depex = NULL;
  Size  = 0;

  //
  // Generate dependency variable name by HardwareInstance.
  //
  VariableName = GenerateFmpDepexVariableName ();
  if (VariableName == NULL) {
    return NULL;
  }

  //
  // Get dependency from variable.
  //
  Status = GetVariable2 (
             VariableName,
             &gEfiCallerIdGuid,
             (VOID **) &Depex,
             &Size
             );
  if (EFI_ERROR (Status)) {
    if (Status != EFI_NOT_FOUND) {
      //
      // Treat EFI_NOT_FOUND as no dependency.
      // Other errors are treated as dependency evaluates to FALSE.
      //
      Size = 2;
      Depex = AllocatePool (Size * sizeof(UINT8));
      if (Depex != NULL) {
        Depex->Dependencies[0] = EFI_FMP_DEP_FALSE;
        Depex->Dependencies[1] = EFI_FMP_DEP_END;
      }
    }
  }

  if (DepexSize != NULL) {
    *DepexSize = (UINT32) Size;
  }

  if (VariableName != NULL) {
    FreePool (VariableName);
  }

  return Depex;
}

/**
  The constructor function to lock FmpDepex variable.

  @param[in]   ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]   SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor succeeds.

**/
EFI_STATUS
EFIAPI
FmpDependencyCheckLibConstructor (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
)
{
  EFI_STATUS                    Status;
  EDKII_VARIABLE_LOCK_PROTOCOL  *VariableLock;
  CHAR16                        *VariableName;

  VariableLock = NULL;
  Status = gBS->LocateProtocol (
                  &gEdkiiVariableLockProtocolGuid,
                  NULL,
                  (VOID **)&VariableLock
                  );
  if (EFI_ERROR (Status) || VariableLock == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDependencyCheckLib: Failed to locate Variable Lock Protocol (%r).\n", Status));
    return EFI_SUCCESS;
  }

  //
  // Generate dependency variable name by HardwareInstance.
  //
  VariableName = GenerateFmpDepexVariableName ();
  if (VariableName == NULL) {
    return EFI_SUCCESS;
  }

  Status = VariableLock->RequestToLock (
                           VariableLock,
                           VariableName,
                           &gEfiCallerIdGuid
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDependencyCheckLib: Failed to lock variable %s (%r)\n", VariableName, Status));
  }

  if (VariableName != NULL) {
    FreePool (VariableName);
  }

  return EFI_SUCCESS;
}
