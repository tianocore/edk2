/** @file
  HII Library implementation that uses DXE protocols and services.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <FrameworkDxe.h>

#include <Protocol/FrameworkHii.h>

#include <Library/FrameworkHiiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_HII_PROTOCOL *mHii = NULL;

/**
  Library constustor function for HiiLib library instance locate the
  gEfiHiiProtocolGuid firstly, the other interface in this library
  instance will dependent on the protocol of gEfiHiiProtocolGuid.
  So the depex of gEfiHiiProtocolGuid is required for this library 
  instance.
  If protocol of gEfiHiiProtocolGuid is not installed, then ASSERT().
  
  @param  ImageHandle  The image handle of driver module who use this library instance.
  @param  SystemTable  Pointer to the EFI System Table.
  
  @retval EFI_SUCCESS  library constuctor always success.
**/
EFI_STATUS
EFIAPI
FrameworkHiiLibConstructor (
  IN     EFI_HANDLE                 ImageHandle,
  IN     EFI_SYSTEM_TABLE           *SystemTable
  )
{
  EFI_STATUS                        Status;
  
  Status = gBS->LocateProtocol (
                 &gEfiHiiProtocolGuid,
                 NULL,
                 (VOID **) &mHii
                 );
  ASSERT_EFI_ERROR (Status);
  ASSERT (mHii != NULL);

  return EFI_SUCCESS;
}

/**
  This function is internal function that prepare and create
  HII packages with given number and package's guid.
  It is invoked by HiiAddPackages() and PreparePackages() interface.
  If the parameter of package's number is 0, then ASSERT().
  
  @param NumberOfPackages  Given number of package item in a HII package list.
  @param Guid              Given GUID of a HII package list.
  @param Marker            Package's content list.
  
  @return                  pointer to new created HII package list.
**/
EFI_HII_PACKAGES *
InternalPreparePackages (
  IN     UINTN                      NumberOfPackages,
  IN     CONST EFI_GUID             *Guid OPTIONAL,
  IN     VA_LIST                    Marker
  )
{
  EFI_HII_PACKAGES                  *HiiPackages;
  VOID                              **Package;
  UINTN                             Index;

  ASSERT (NumberOfPackages > 0);

  HiiPackages = AllocateZeroPool (sizeof (EFI_HII_PACKAGES) + NumberOfPackages * sizeof (VOID *));
  ASSERT (HiiPackages != NULL);

  HiiPackages->GuidId           = (EFI_GUID *) Guid;
  HiiPackages->NumberOfPackages = NumberOfPackages;
  Package                       = (VOID **) (((UINT8 *) HiiPackages) + sizeof (EFI_HII_PACKAGES));

  for (Index = 0; Index < NumberOfPackages; Index++) {
    *Package = VA_ARG (Marker, VOID *);
    Package++;
  }

  return HiiPackages;
}

/**
  This function allocates pool for an EFI_HII_PACKAGES structure
  with enough space for the variable argument list of package pointers.
  The allocated structure is initialized using NumberOfPackages, Guid,
  and the variable length argument list of package pointers.

  @param  NumberOfPackages  The number of HII packages to prepare.
  @param  Guid              Package GUID.
  @param  ...               The variable argument list of package pointers.

  @return                   The allocated and initialized packages.
**/
EFI_HII_PACKAGES *
EFIAPI
PreparePackages (
  IN     UINTN                      NumberOfPackages,
  IN     CONST EFI_GUID             *Guid OPTIONAL,
  ...
  )
{
  VA_LIST                           Args;

  VA_START (Args, Guid);
  return InternalPreparePackages (NumberOfPackages, Guid, Args);
}


/**
  This function allocates pool for an EFI_HII_PACKAGE_LIST structure
  with additional space that is big enough to host all packages described by the variable 
  argument list of package pointers.  The allocated structure is initialized using NumberOfPackages, 
  GuidId,  and the variable length argument list of package pointers.

  Then, EFI_HII_PACKAGE_LIST will be register to the default System HII Database. The
  Handle to the newly registered Package List is returned throught HiiHandle.

  @param  NumberOfPackages    The number of HII packages to register.
  @param  GuidId              Package List GUID ID.
  @param  DriverHandle        The pointer of driver handle
  @param  HiiHandle           The ID used to retrieve the Package List later.
  @param  ...                 The variable argument list describing all HII Package.

  @return                     The allocated and initialized packages.
**/
EFI_STATUS
EFIAPI
HiiLibAddPackages (
  IN     UINTN                      NumberOfPackages,
  IN     CONST EFI_GUID             *GuidId,
  IN     EFI_HANDLE                 DriverHandle, OPTIONAL
     OUT EFI_HII_HANDLE             *HiiHandle, 
  ...
  )
{
  VA_LIST                           Args;
  EFI_HII_PACKAGES                  *FrameworkHiiPacages;
  FRAMEWORK_EFI_HII_HANDLE          FrameworkHiiHandle;
  EFI_STATUS                        Status;

  VA_START (Args, HiiHandle);

  FrameworkHiiPacages = InternalPreparePackages (NumberOfPackages, GuidId, Args);
  Status = mHii->NewPack (mHii, FrameworkHiiPacages, &FrameworkHiiHandle);
  if (HiiHandle != NULL) {
    if (EFI_ERROR (Status)) {
      *HiiHandle = NULL;
    } else {
      *HiiHandle = (EFI_HII_HANDLE) (UINTN) FrameworkHiiHandle;
    }
  }

  FreePool (FrameworkHiiPacages);
  
  return Status;
}

/**
  Removes a package list from the default HII database.

  If HiiHandle is NULL, then ASSERT.
  If HiiHandle is not a valid EFI_HII_HANDLE in the default HII database, then ASSERT.

  @param  HiiHandle      The handle that was previously registered to the data base that is requested for removal.

  @return VOID
**/
VOID
EFIAPI
HiiLibRemovePackages (
  IN     EFI_HII_HANDLE             HiiHandle
  )
{
  EFI_STATUS                        Status;
  
  Status = mHii->RemovePack (mHii, (FRAMEWORK_EFI_HII_HANDLE) (UINTN) HiiHandle);
  ASSERT_EFI_ERROR (Status);
}

/**
  This function adds the string into String Package of each language.

  @param  PackageList            Handle of the package list where this string will
                                 be added.
  @param  StringId               On return, contains the new strings id, which is
                                 unique within PackageList.
  @param  String                 Points to the new null-terminated string.

  @retval EFI_SUCCESS            The new string was added successfully.
  @retval EFI_NOT_FOUND          The specified PackageList could not be found in
                                 database.
  @retval EFI_OUT_OF_RESOURCES   Could not add the string due to lack of resources.
  @retval EFI_INVALID_PARAMETER  String is NULL or StringId is NULL is NULL.

**/
EFI_STATUS
EFIAPI
HiiLibNewString (
  IN     EFI_HII_HANDLE             PackageList,
     OUT EFI_STRING_ID              *StringId,
  IN     CONST EFI_STRING           String
  )
{
  FRAMEWORK_EFI_HII_HANDLE          FrameworkHiiHandle;
  EFI_STATUS                        Status;

  FrameworkHiiHandle = (FRAMEWORK_EFI_HII_HANDLE) (UINTN) PackageList;
  Status = mHii->NewString (
                   mHii,
                   NULL,
                   FrameworkHiiHandle,
                   StringId,
                   String
                   );

  return Status;
}

/**
  Get the string given the StringId and String package Producer's Guid. The caller
  is responsible to free the *String.

  If PackageList with the matching ProducerGuid is not found, then ASSERT.
  If PackageList with the matching ProducerGuid is found but no String is
  specified by StringId is found, then ASSERT.

  @param  ProducerGuid           The Guid of String package list.
  @param  StringId               The String ID.
  @param  String                 The output string.

  @retval EFI_SUCCESS            Operation is successful.
  @retval EFI_OUT_OF_RESOURCES   There is not enought memory in the system.
**/
EFI_STATUS
EFIAPI
HiiLibGetStringFromToken (
  IN     EFI_GUID                   *ProducerGuid,
  IN     EFI_STRING_ID              StringId,
     OUT EFI_STRING                 *String
  )
{
  return EFI_SUCCESS;  
}

/**
  Get string specified by StringId form the HiiHandle. The caller
  is responsible to free the *String.

  If String is NULL, then ASSERT.
  If HiiHandle could not be found in the default HII database, then ASSERT.
  If StringId is not found in PackageList, then ASSERT.

  @param  PackageList            The HII handle of package list.
  @param  StringId               The String ID.
  @param  String                 The output string.

  @retval EFI_NOT_FOUND          String is not found.
  @retval EFI_SUCCESS            Operation is successful.
  @retval EFI_OUT_OF_RESOURCES   There is not enought memory in the system.

**/
EFI_STATUS
EFIAPI
HiiLibGetStringFromHandle (
  IN     EFI_HII_HANDLE             PackageList,
  IN     EFI_STRING_ID              StringId,
     OUT EFI_STRING                 *String
  )
{
  return EFI_SUCCESS;
}

/**
  Create the driver handle for HII driver. The protocol and 
  Package list of this driver wili be installed into this 
  driver handle. 
  The implement set DriverHandle to NULL simpliy to let 
  handle manager create a default new handle.
  
  @param  DriverHandle   The pointer of driver handle
  
  @return                Always success.
**/
EFI_STATUS
EFIAPI
HiiLibCreateHiiDriverHandle (
  OUT EFI_HANDLE                    *DriverHandle
  )
{
  //
  // Driver
  // This implementation does nothing as DriverHandle concept only
  // applies to UEFI HII specification.
  //
  
  *DriverHandle = NULL;
  
  return EFI_SUCCESS;
}

