/** @file
  This is an implementation of the ACPI S3 Save protocol.  This is defined in
  S3 boot path specification 0.9.

Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Protocol/AcpiS3Save.h>

#include "AcpiS3Save.h"

/**
  Hook point for AcpiVariableThunkPlatform for InstallAcpiS3Save.
**/
VOID
InstallAcpiS3SaveThunk (
  VOID
  );

/**
  Hook point for AcpiVariableThunkPlatform for S3Ready.

**/
VOID
S3ReadyThunkPlatform (
  VOID
  );

UINTN     mLegacyRegionSize;

EFI_ACPI_S3_SAVE_PROTOCOL mS3Save = {
  LegacyGetS3MemorySize,
  S3Ready,
};

/**
  Allocate memory below 4G memory address.

  This function allocates memory below 4G memory address.

  @param  MemoryType   Memory type of memory to allocate.
  @param  Size         Size of memory to allocate.
  
  @return Allocated address for output.

**/
VOID*
AllocateMemoryBelow4G (
  IN EFI_MEMORY_TYPE    MemoryType,
  IN UINTN              Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer;

  Pages = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   MemoryType,
                   Pages,
                   &Address
                   );
  ASSERT_EFI_ERROR (Status);

  Buffer = (VOID *) (UINTN) Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}

/**
  Gets the buffer of legacy memory below 1 MB 
  This function is to get the buffer in legacy memory below 1MB that is required during S3 resume.

  @param This           A pointer to the EFI_ACPI_S3_SAVE_PROTOCOL instance.
  @param Size           The returned size of legacy memory below 1 MB.

  @retval EFI_SUCCESS           Size is successfully returned.
  @retval EFI_INVALID_PARAMETER The pointer Size is NULL.

**/
EFI_STATUS
EFIAPI
LegacyGetS3MemorySize (
  IN  EFI_ACPI_S3_SAVE_PROTOCOL   *This,
  OUT UINTN                       *Size
  )
{
  if (Size == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Size = mLegacyRegionSize;
  return EFI_SUCCESS;
}

/**
  Prepares all information that is needed in the S3 resume boot path.
  
  Allocate the resources or prepare informations and save in ACPI variable set for S3 resume boot path  
  
  @param This                 A pointer to the EFI_ACPI_S3_SAVE_PROTOCOL instance.
  @param LegacyMemoryAddress  The base address of legacy memory.

  @retval EFI_NOT_FOUND         Some necessary information cannot be found.
  @retval EFI_SUCCESS           All information was saved successfully.
  @retval EFI_OUT_OF_RESOURCES  Resources were insufficient to save all the information.
  @retval EFI_INVALID_PARAMETER The memory range is not located below 1 MB.

**/
EFI_STATUS
EFIAPI
S3Ready (
  IN EFI_ACPI_S3_SAVE_PROTOCOL    *This,
  IN VOID                         *LegacyMemoryAddress
  )
{
  STATIC BOOLEAN                  AlreadyEntered;

  DEBUG ((EFI_D_INFO, "S3Ready!\n"));

  //
  // Platform may invoke AcpiS3Save->S3Save() before ExitPmAuth, because we need save S3 information there, while BDS ReadyToBoot may invoke it again.
  // So if 2nd S3Save() is triggered later, we need ignore it.
  //
  if (AlreadyEntered) {
    return EFI_SUCCESS;
  }
  AlreadyEntered = TRUE;

  if (FeaturePcdGet(PcdFrameworkCompatibilitySupport)) {
    S3ReadyThunkPlatform ();
  }

  return EFI_SUCCESS;
}

/**
  The Driver Entry Point.
  
  The function is the driver Entry point which will produce AcpiS3SaveProtocol.
  
  @param ImageHandle   A handle for the image that is initializing this driver
  @param SystemTable   A pointer to the EFI system table

  @retval EFI_SUCCESS               Driver initialized successfully
  @retval EFI_UNSUPPORTED           Do not support ACPI S3
  @retval EFI_OUT_OF_RESOURCES      Could not allocate needed resources

**/
EFI_STATUS
EFIAPI
InstallAcpiS3Save (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS        Status;

  if (!PcdGetBool (PcdAcpiS3Enable)) {
    return EFI_UNSUPPORTED;
  }

  if (!FeaturePcdGet(PcdPlatformCsmSupport)) {
    //
    // More memory for no CSM tip, because GDT need relocation
    //
    mLegacyRegionSize = 0x250;
  } else {
    mLegacyRegionSize = 0x100;
  }

  if (FeaturePcdGet(PcdFrameworkCompatibilitySupport)) {
    InstallAcpiS3SaveThunk ();
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiAcpiS3SaveProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mS3Save
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
