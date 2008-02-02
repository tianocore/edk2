/** @file
  HII Library implementation that uses DXE protocols and services.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  HiiLib.c

**/


#include <FrameworkDxe.h>


#include <Protocol/FrameworkHii.h>

#include <Library/HiiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

EFI_HII_PROTOCOL      *gHiiProtocol = NULL;


EFI_STATUS
EFIAPI
HiiLibFrameworkConstructor (
  IN      EFI_HANDLE                ImageHandle,
  IN      EFI_SYSTEM_TABLE          *SystemTable
  )
{
  EFI_STATUS Status;

  Status = gBS->LocateProtocol (
                  &gEfiHiiProtocolGuid,
                  NULL,
                  (VOID **) &gHiiProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
  
}

/**
  This function allocates pool for an EFI_HII_PACKAGES structure
  with enough space for the variable argument list of package pointers.
  The allocated structure is initialized using NumberOfPackages, Guid,
  and the variable length argument list of package pointers.

  @param  NumberOfPackages The number of HII packages to prepare.
  @param  Guid Package GUID.

  @return The allocated and initialized packages.

**/
EFI_HII_PACKAGES *
InternalPreparePackages (
  IN UINTN           NumberOfPackages,
  IN CONST EFI_GUID  *Guid OPTIONAL,
  IN VA_LIST         Args
  )
{
  EFI_HII_PACKAGES  *HiiPackages;
  VOID              **Package;
  UINTN             Index;

  ASSERT (NumberOfPackages > 0);

  HiiPackages                   = AllocateZeroPool (sizeof (EFI_HII_PACKAGES) + NumberOfPackages * sizeof (VOID *));
  ASSERT (HiiPackages != NULL);

  HiiPackages->GuidId           = (EFI_GUID *) Guid;
  HiiPackages->NumberOfPackages = NumberOfPackages;
  Package                       = (VOID **) (((UINT8 *) HiiPackages) + sizeof (EFI_HII_PACKAGES));

  for (Index = 0; Index < NumberOfPackages; Index++) {
    *Package = VA_ARG (Args, VOID *);
    Package++;
  }

  return HiiPackages;

}

EFI_STATUS
EFIAPI
PrepareAndCreateNewPackages (
  IN            UINTN      NumberOfPackages,
  IN CONST      EFI_GUID   *GuidId,
  OUT           VOID      **HiiHandle,         //Framework is FRAMEWORK_HII_HANDLE; UEFI is EFI_HII_HANDLE; 
                                     // C:\D\Work\Tiano\Tiano_Main_Trunk\TIANO\Platform\IntelEpg\SR870BN4\MemorySubClassDriver\DualChannelDdr\MemorySubClass.c make use of this output value
  ...
  )
{
  EFI_STATUS                Status;
  EFI_HII_PACKAGES          *PackageList;
  VA_LIST                   Args;
  FRAMEWORK_EFI_HII_HANDLE  FrameworkHiiHandle;

  
  VA_START (Args, HiiHandle);
  PackageList = InternalPreparePackages (NumberOfPackages, GuidId, Args);
  VA_END (Args);
  
  Status      = gHiiProtocol->NewPack (gHiiProtocol, PackageList, &FrameworkHiiHandle);
  *HiiHandle  = (VOID *) (UINTN) FrameworkHiiHandle;

  return Status;
}


