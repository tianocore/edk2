/** @file
  Dynamic Platform Info Repository Internal

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef DYNAMIC_PLAT_REPO_INTERNAL_H_
#define DYNAMIC_PLAT_REPO_INTERNAL_H_

#include "TokenMapper.h"

#pragma pack(1)

/** CmObj node.

  This is a node wrapper around the CM_OBJ_DESCRIPTOR structure.
  It also allows to bind a token to the CM_OBJ_DESCRIPTOR.
*/
typedef struct CmObjectNode {
  /// This must be the first field in this structure.
  LIST_ENTRY           Link;

  /// Token associated with the CmObjDesc.
  CM_OBJECT_TOKEN      Token;

  /// CmObjDesc wrapped.
  /// Note: the CM_OBJ_DESCRIPTOR.Data field is allocated and copied.
  CM_OBJ_DESCRIPTOR    CmObjDesc;
} CM_OBJ_NODE;

/** Dynamic repository states.

  The states must progress as:
  UnInitialised -> Transient -> Finalized
*/
typedef enum DynRepoState {
  DynRepoUnInitialised, ///< Un-Initialised state
  DynRepoTransient,     ///< Transient state - CmObjects can be added.
  DynRepoFinalized,     ///< Repo Locked - No further CmObjects can be added.
                        ///< Getting objects is now possible.
  DynRepoMax            ///< Max value.
} EDYNAMIC_REPO_STATE;

/** A structure describing the platform configuration
    manager repository information
*/
typedef struct DynamicPlatformRepositoryInfo {
  /// Repo state machine.
  EDYNAMIC_REPO_STATE    RepoState;

  /// Count of all the objects added to the Dynamic Platform Repo
  /// during the Transient state.
  UINTN                  ObjectCount;

  /// Link lists of CmObj from the ArmNameSpace
  /// that are added in the Transient state.
  LIST_ENTRY             ArmCmObjList[EArmObjMax];

  /// Structure Members used in Finalized state.
  /// An array of CmObj Descriptors from the ArmNameSpace
  /// This array is populated when the Repo is finalized.
  CM_OBJ_DESCRIPTOR      ArmCmObjArray[EArmObjMax];

  /// A token mapper for the objects in the ArmNamespaceObjectArray
  /// The Token mapper is populated when the Repo is finalized in
  /// a call to DynamicPlatRepoFinalise ().
  TOKEN_MAPPER           TokenMapper;
} DYNAMIC_PLATFORM_REPOSITORY_INFO;

#pragma pack()

#endif // DYNAMIC_PLAT_REPO_INTERNAL_H_
