/** @file
  Internal functions for Opal Core library.

Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _OPAL_INTERNAL_H_
#define _OPAL_INTERNAL_H_

#include <Library/TcgStorageOpalLib.h>

/**

  The function retrieves the MSID from the device specified

  @param[in]  AdminSpSession              OPAL_SESSION with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_ANYBODY_AUTHORITY
  @param[out] ActiveDataRemovalMechanism  Active Data Removal Mechanism that the device will use for Revert/RevertSP calls.

**/
TCG_RESULT
OpalPyrite2GetActiveDataRemovalMechanism (
  OPAL_SESSION  *AdminSpSession,
  UINT8         *ActiveDataRemovalMechanism
  );

/**

  Get the support attribute info.

  @param[in]      Session             OPAL_SESSION with OPAL_UID_LOCKING_SP to retrieve info.
  @param[in]      FeatureCode         The feature code user request.
  @param[in, out] DataSize            The data size.
  @param[out]     Data                The data buffer used to save the feature descriptor.

**/
TCG_RESULT
OpalGetFeatureDescriptor (
  IN     OPAL_SESSION  *Session,
  IN     UINT16        FeatureCode,
  IN OUT UINTN         *DataSize,
  OUT    VOID          *Data
  );

/**
  Get revert timeout value.

  @param[in]      Session                       The session info for one opal device.

**/
UINT32
GetRevertTimeOut (
  IN OPAL_SESSION  *Session
  );

/**

  Reverts device using Admin SP Revert method.

  @param[in]  AdminSpSession      OPAL_SESSION with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY to perform PSID revert.
  @param[in]  EstimateTimeCost    Input the timeout value.

**/
TCG_RESULT
OpalPyrite2PsidRevert (
  OPAL_SESSION  *AdminSpSession,
  UINT32        EstimateTimeCost
  );

/**

  The function calls the Admin SP RevertSP method on the Locking SP.  If KeepUserData is True, then the optional parameter
  to keep the user Data is set to True, otherwise the optional parameter is not provided.

  @param[in]      LockingSpSession    OPAL_SESSION with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY to revertSP
  @param[in]      KeepUserData        Specifies whether or not to keep user Data when performing RevertSP action. True = keeps user Data.
  @param[in/out]  MethodStatus        Method status of last action performed.  If action succeeded, it should be TCG_METHOD_STATUS_CODE_SUCCESS.
  @param[in]      EstimateTimeCost    Input the timeout value.

**/
TCG_RESULT
OpalPyrite2AdminRevert (
  OPAL_SESSION  *LockingSpSession,
  BOOLEAN       KeepUserData,
  UINT8         *MethodStatus,
  UINT32        EstimateTimeCost
  );

#endif // _OPAL_CORE_H_
