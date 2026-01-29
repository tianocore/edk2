/** @file

  Copyright (c) 2017 - 2018, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef CONFIGURATION_MANAGER_PROTOCOL_H_
#define CONFIGURATION_MANAGER_PROTOCOL_H_

#include <ConfigurationManagerObject.h>

/** This macro defines the Configuration Manager Protocol GUID.

  GUID: {D85A4835-5A82-4894-AC02-706F43D5978E}
*/
#define EDKII_CONFIGURATION_MANAGER_PROTOCOL_GUID       \
  { 0xd85a4835, 0x5a82, 0x4894,                         \
    { 0xac, 0x2, 0x70, 0x6f, 0x43, 0xd5, 0x97, 0x8e }   \
  };

/** This macro defines the Configuration Manager Protocol Revision.
*/
#define EDKII_CONFIGURATION_MANAGER_PROTOCOL_REVISION  CREATE_REVISION (1, 0)

#pragma pack(1)

/**
  Forward declarations:
*/
typedef struct ConfigurationManagerProtocol  EDKII_CONFIGURATION_MANAGER_PROTOCOL;
typedef struct PlatformRepositoryInfo        EDKII_PLATFORM_REPOSITORY_INFO;

/** The GetObject function defines the interface implemented by the
    Configuration Manager Protocol for returning the Configuration
    Manager Objects.

  @param [in]  This        Pointer to the Configuration Manager Protocol.
  @param [in]  CmObjectId  The Configuration Manager Object ID.
  @param [in]  Token       An optional token identifying the object. If
                           unused this must be CM_NULL_TOKEN.
  @param [out] CmObject    Pointer to the Configuration Manager Object
                           descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration Manager
                                is less than the Object size for the requested
                                object.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_CONFIGURATION_MANAGER_GET_OBJECT)(
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST CmObject
  );

/** The SetObject function defines the interface implemented by the
    Configuration Manager Protocol for updating the Configuration
    Manager Objects.

  @param [in]  This        Pointer to the Configuration Manager Protocol.
  @param [in]  CmObjectId  The Configuration Manager Object ID.
  @param [in]  Token       An optional token identifying the object. If
                           unused this must be CM_NULL_TOKEN.
  @param [out] CmObject    Pointer to the Configuration Manager Object
                           descriptor describing the Object.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration Manager
                                is less than the Object size for the requested
                                object.
  @retval EFI_UNSUPPORTED       This operation is not supported.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_CONFIGURATION_MANAGER_SET_OBJECT)(
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN        CM_OBJ_DESCRIPTOR                     *CONST CmObject
  );

/** The EDKII_CONFIGURATION_MANAGER_PROTOCOL structure describes the
    Configuration Manager Protocol interface.
*/
typedef struct ConfigurationManagerProtocol {
  /// The Configuration Manager Protocol revision.
  UINT32                                    Revision;

  /** The interface used to request information about
      the Configuration Manager Objects.
  */
  EDKII_CONFIGURATION_MANAGER_GET_OBJECT    GetObject;

  /** The interface used to update the information stored
      in the Configuration Manager repository.
  */
  EDKII_CONFIGURATION_MANAGER_SET_OBJECT    SetObject;

  /** Pointer to an implementation defined abstract repository
      provisioned by the Configuration Manager.
  */
  EDKII_PLATFORM_REPOSITORY_INFO            *PlatRepoInfo;
} EDKII_CONFIGURATION_MANAGER_PROTOCOL;

/** The Configuration Manager Protocol GUID.
*/
extern EFI_GUID  gEdkiiConfigurationManagerProtocolGuid;

#pragma pack()

#endif // CONFIGURATION_MANAGER_PROTOCOL_H_
