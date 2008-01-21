/** @file
  Public include file for the HII Library

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __HII_LIB_H__
#define __HII_LIB_H__

//#include <PiDxe.h>

//
// Limited buffer size recommended by RFC4646 (4.3.  Length Considerations)
// (42 characters plus a NULL terminator)
//
#define RFC_3066_ENTRY_SIZE             (42 + 1)

/**
  This function allocates pool for an EFI_HII_PACKAGE_LIST structure
  with additional space that is big enough to host all packages described by the variable 
  argument list of package pointers.  The allocated structure is initialized using NumberOfPackages, 
  GuidId,  and the variable length argument list of package pointers.

  Then, EFI_HII_PACKAGE_LIST will be register to the default System HII Database. The
  Handle to the newly registered Package List is returned throught HiiHandle.

  @param  NumberOfPackages  The number of HII packages to register.
  @param  GuidId                    Package List GUID ID.
  @param  HiiHandle                The ID used to retrieve the Package List later.
  @param  ...                          The variable argument list describing all HII Package.

  @return
  The allocated and initialized packages.

**/

EFI_STATUS
EFIAPI
HiiLibAddPackagesToHiiDatabase (
  IN       UINTN               NumberOfPackages,
  IN CONST EFI_GUID            *GuidId,
  IN       EFI_HANDLE          DriverHandle, OPTIONAL
  OUT      EFI_HII_HANDLE      *HiiHandle, OPTIONAL
  ...
  )
;

EFI_STATUS
EFIAPI
HiiLibAddFontPackageToHiiDatabase (
  IN       UINTN               FontSize,
  IN CONST UINT8               *FontBinary,
  IN CONST EFI_GUID            *GuidId,
  OUT      EFI_HII_HANDLE      *HiiHandle OPTIONAL
  )
;

EFI_STATUS
EFIAPI
HiiLibRemovePackagesFromHiiDatabase (
  IN      EFI_HII_HANDLE      HiiHandle
  )
;

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
HiiLibCreateString (
  IN  EFI_HII_HANDLE                  PackageList,
  OUT EFI_STRING_ID                   *StringId,
  IN  CONST EFI_STRING                String
  )
;

/**
  This function update the specified string in String Package of each language.

  @param  PackageList            Handle of the package list where this string will
                                 be added.
  @param  StringId               Ths String Id to be updated.
  @param  String                 Points to the new null-terminated string.

  @retval EFI_SUCCESS            The new string was added successfully.
  @retval EFI_NOT_FOUND          The specified PackageList could not be found in
                                 database.
  @retval EFI_OUT_OF_RESOURCES   Could not add the string due to lack of resources.
  @retval EFI_INVALID_PARAMETER  String is NULL or StringId is NULL is NULL.

**/
EFI_STATUS
EFIAPI
HiiLibUpdateString (
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  IN  CONST EFI_STRING                String
  )
;



//
// Just use the UEFI prototype
//
EFI_STATUS
EFIAPI
HiiLibGetStringFromGuidId (
  IN  EFI_GUID                        *ProducerGuid,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      *String
  )
;

//
// This function is Implementation Specifc. HII_VENDOR_DEVICE_PATH
// This should be moved to MdeModulepkg.
//
EFI_STATUS
EFIAPI
HiiLibCreateHiiDriverHandle (
  OUT EFI_HANDLE               *DriverHandle
  )
;

EFI_STATUS
EFIAPI
HiiLibDestroyHiiDriverHandle (
  IN EFI_HANDLE                 DriverHandle
  )
;


EFI_STATUS
HiiLibExtractClassFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle,
  OUT     UINT16              *Class,
  OUT     EFI_STRING_ID       *FormSetTitle,
  OUT     EFI_STRING_ID       *FormSetHelp
  )
/*++

Routine Description:
  Extract formset class for given HII handle.

Arguments:
  HiiHandle       - Hii handle
  Class           - Class of the formset
  FormSetTitle    - Formset title string
  FormSetHelp     - Formset help string

Returns:
  EFI_SUCCESS     - Successfully extract Class for specified Hii handle.

--*/
;

#endif
