/** @file
  UEFI Component Name(2) protocol implementation for iSCSI.

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IScsiImpl.h"

//
// EFI Component Name Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL     gIScsiComponentName = {
  IScsiComponentNameGetDriverName,
  IScsiComponentNameGetControllerName,
  "eng"
};

//
// EFI Component Name 2 Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL    gIScsiComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME) IScsiComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) IScsiComponentNameGetControllerName,
  "en"
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE mIScsiDriverNameTable[] = {
  {"eng;en", L"iSCSI Driver"}, 
  {NULL, NULL}
};

/**
  Retrieves a Unicode string that is the user readable name of the EFI Driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.
  
  @param[in]  This        A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
  @param[in]  Language    A pointer to a three character ISO 639-2 language identifier.
                          This is the language of the driver name that that the caller
                          is requesting, and it must match one of the languages specified
                          in SupportedLanguages.  The number of languages supported by a
                          driver is up to the driver writer.
  @param[out]  DriverName A pointer to the Unicode string to return.  This Unicode string
                          is the name of the driver specified by This in the language
                          specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by This
                                and the language specified by Language was returned
                                in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support the
                                language specified by Language.
**/
EFI_STATUS
EFIAPI
IScsiComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL   *This,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **DriverName
  )
{
  return LookupUnicodeString2 (
          Language,
          This->SupportedLanguages,
          mIScsiDriverNameTable,
          DriverName,
          (BOOLEAN)(This == &gIScsiComponentName)
          );
}

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by an EFI Driver.Currently not implemented.

  @param[in]  This             A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
  @param[in]  ControllerHandle The handle of a controller that the driver specified by
                               This is managing.  This handle specifies the controller
                               whose name is to be returned.
  @param[in]  ChildHandle      The handle of the child controller to retrieve the name
                               of.  This is an optional parameter that may be NULL.  It
                               will be NULL for device drivers.  It will also be NULL
                               for a bus drivers that wish to retrieve the name of the
                               bus controller.  It will not be NULL for a bus driver
                               that wishes to retrieve the name of a child controller.
  @param[in]  Language         A pointer to a three character ISO 639-2 language
                               identifier.  This is the language of the controller name
                               that that the caller is requesting, and it must match one
                               of the languages specified in SupportedLanguages.  The
                               number of languages supported by a driver is up to the
                               driver writer.
  @param[out]  ControllerName  A pointer to the Unicode string to return.  This Unicode
                               string is the name of the controller specified by
                               ControllerHandle and ChildHandle in the language specified
                               by Language from the point of view of the driver specified
                               by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in the
                                language specified by Language for the driver
                                specified by This was returned in DriverName.                                
  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently managing
                                the controller specified by ControllerHandle and
                                ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support the
                                language specified by Language.
**/
EFI_STATUS
EFIAPI
IScsiComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL   *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    ChildHandle        OPTIONAL,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}
