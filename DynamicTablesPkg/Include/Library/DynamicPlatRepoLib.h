/** @file
  Dynamic Platform Info Repository

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef DYNAMIC_PLAT_REPO_H_
#define DYNAMIC_PLAT_REPO_H_

#include <Protocol/ConfigurationManagerProtocol.h>

/** A structure describing the platform configuration
    manager repository information
*/
typedef VOID *DYNAMIC_PLATFORM_REPOSITORY_INFO;

/** Add an object to the dynamic platform repository.

  @param [in]  This       This dynamic platform repository.
  @param [in]  CmObjDesc  CmObj to add. The data is copied.
  @param [out] Token      If not NULL, token allocated to this CmObj.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  An allocation has failed.
**/
EFI_STATUS
EFIAPI
DynPlatRepoAddObject (
  IN        DYNAMIC_PLATFORM_REPOSITORY_INFO  *This,
  IN  CONST CM_OBJ_DESCRIPTOR                 *CmObjDesc,
  OUT       CM_OBJECT_TOKEN                   *Token OPTIONAL
  );

/** Finalise the dynamic repository.

  Finalising means:
   - Preventing any further objects from being added.
   - Allowing to get objects from the dynamic repository
     (not possible before a call to this function).

  @param [in]  This       This dynamic platform repository.

  @retval EFI_SUCCESS           Success.
  @retval EFI_ALREADY_STARTED   Instance already initialised.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_BUFFER_TOO_SMALL  Buffer too small.
  @retval EFI_OUT_OF_RESOURCES  An allocation has failed.
**/
EFI_STATUS
EFIAPI
DynamicPlatRepoFinalise (
  IN  DYNAMIC_PLATFORM_REPOSITORY_INFO  *This
  );

/** Get a CmObj from the dynamic repository.

  @param [in]      This        Pointer to the Dynamic Platform Repository.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObjDesc   Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
DynamicPlatRepoGetObject (
  IN      DYNAMIC_PLATFORM_REPOSITORY_INFO  *This,
  IN      CM_OBJECT_ID                      CmObjectId,
  IN      CM_OBJECT_TOKEN                   Token OPTIONAL,
  IN  OUT CM_OBJ_DESCRIPTOR                 *CmObjDesc
  );

/** Initialize the dynamic platform repository.

  @param [out]  DynPlatRepo   If success, contains the initialised dynamic
                              platform repository.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  An allocation has failed.
**/
EFI_STATUS
EFIAPI
DynamicPlatRepoInit (
  OUT DYNAMIC_PLATFORM_REPOSITORY_INFO  **DynPlatRepo
  );

/** Shutdown the dynamic platform repository.

  Free all the memory allocated for the dynamic platform repository.

  @param [in]  DynPlatRepo    The dynamic platform repository.

  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_SUCCESS           Success.
**/
EFI_STATUS
EFIAPI
DynamicPlatRepoShutdown (
  IN  DYNAMIC_PLATFORM_REPOSITORY_INFO  *DynPlatRepo
  );

#endif // DYNAMIC_PLAT_REPO_H_
