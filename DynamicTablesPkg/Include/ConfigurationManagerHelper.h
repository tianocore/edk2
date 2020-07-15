/** @file

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef CONFIGURATION_MANAGER_HELPER_H_
#define CONFIGURATION_MANAGER_HELPER_H_

/** The GET_OBJECT_LIST macro expands to a function that is used to retrieve
    an object or an object list from the Configuration Manager using the
    Configuration Manager Protocol interface.

  The macro expands to a function which has the following prototype:

  STATIC
  EFI_STATUS
  EFIAPI
  Get<CmObjectId> (
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL * CONST CfgMgrProtocol,
    IN  CONST CM_OBJECT_TOKEN                              Token OPTIONAL,
    OUT       Type                                **       List,
    OUT       UINT32                               *       Count OPTIONAL
    );

  Generated function parameters:
  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager protocol
                              interface.
  @param [in]  Token          Reference token for the Object.
  @param [out] List           Pointer to the Object list.
  @param [out] Count          Count of the objects returned in the list.

  Macro Parameters:
  @param [in] CmObjectNameSpace The Object Namespace
  @param [in] CmObjectId        Object Id.
  @param [in] Type              Structure used to describe the Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
#define GET_OBJECT_LIST(CmObjectNameSpace, CmObjectId, Type)                  \
STATIC                                                                        \
EFI_STATUS                                                                    \
EFIAPI                                                                        \
Get##CmObjectId (                                                             \
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol,     \
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,     \
  OUT       Type                                 **       List,               \
  OUT       UINT32                                * CONST Count OPTIONAL      \
  )                                                                           \
{                                                                             \
  EFI_STATUS         Status;                                                  \
  CM_OBJ_DESCRIPTOR  CmObjectDesc;                                            \
  UINT32             ObjCount = 0;                                            \
  if (List == NULL) {                                                         \
    Status = EFI_INVALID_PARAMETER;                                           \
    DEBUG ((                                                                  \
      DEBUG_ERROR,                                                            \
      "ERROR: Get" #CmObjectId ": Invalid out parameter for"                  \
      " object list. Status = %r\n",                                          \
      Status                                                                  \
      ));                                                                     \
    goto error_handler;                                                       \
  }                                                                           \
  Status = CfgMgrProtocol->GetObject (                                        \
                             CfgMgrProtocol,                                  \
                             CREATE_CM_OBJECT_ID (                            \
                               CmObjectNameSpace,                             \
                               CmObjectId                                     \
                               ),                                             \
                             Token,                                           \
                             &CmObjectDesc                                    \
                             );                                               \
  if (EFI_ERROR (Status)) {                                                   \
    DEBUG ((                                                                  \
      DEBUG_INFO,                                                             \
      "INFO: Get" #CmObjectId ": Platform does not implement "                \
      #CmObjectId ". Status = %r\n",                                          \
      Status                                                                  \
      ));                                                                     \
    *List = NULL;                                                             \
    goto error_handler;                                                       \
  }                                                                           \
  if (CmObjectDesc.ObjectId !=                                                \
      CREATE_CM_OBJECT_ID (CmObjectNameSpace, CmObjectId)) {                  \
    DEBUG ((                                                                  \
      DEBUG_ERROR,                                                            \
      "ERROR: Get" #CmObjectId ": " #CmObjectId                               \
      ": Invalid ObjectId = 0x%x\n, expected Id = 0x%x\n",                    \
      CmObjectDesc.ObjectId,                                                  \
      CREATE_CM_OBJECT_ID (CmObjectNameSpace, CmObjectId)                     \
      ));                                                                     \
    ASSERT (FALSE);                                                           \
    Status = EFI_INVALID_PARAMETER;                                           \
    goto error_handler;                                                       \
  }                                                                           \
  if (CmObjectDesc.Size < (sizeof (Type) * CmObjectDesc.Count)) {             \
    DEBUG ((                                                                  \
      DEBUG_ERROR,                                                            \
      "ERROR: Get" #CmObjectId ": " #CmObjectId                               \
      ": Buffer too small, size = 0x%x\n",                                    \
      CmObjectDesc.Size                                                       \
      ));                                                                     \
    ASSERT (FALSE);                                                           \
    Status = EFI_BAD_BUFFER_SIZE;                                             \
    goto error_handler;                                                       \
  }                                                                           \
  ObjCount = CmObjectDesc.Count;                                              \
  *List = (Type*)CmObjectDesc.Data;                                           \
error_handler:                                                                \
  if (Count != NULL) {                                                        \
    *Count = ObjCount;                                                        \
  }                                                                           \
  return Status;                                                              \
}

#endif // CONFIGURATION_MANAGER_HELPER_H_
