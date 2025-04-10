/** @file
  Provides function interfaces to communicate with TPM 2.0 service through FF-A.

  This header follows the TPM over FF-A specification:
  https://developer.arm.com/documentation/den0138/latest/

  Copyright (c), Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef TPM2_SERVICE_FFA_H_
#define TPM2_SERVICE_FFA_H_

#define TPM2_SERVICE_FFA_GUID \
  { 0x17b862a4, 0x1806, 0x4faf, { 0x86, 0xb3, 0x08, 0x9a, 0x58, 0x35, 0x38, 0x61 } }

#define TPM2_FFA_GET_INTERFACE_VERSION         0x0f000001
#define TPM2_FFA_GET_FEATURE_INFO              0x0f000101
#define TPM2_FFA_START                         0x0f000201
#define TPM2_FFA_REGISTER_FOR_NOTIFICATION     0x0f000301
#define TPM2_FFA_UNREGISTER_FROM_NOTIFICATION  0x0f000401
#define TPM2_FFA_FINISH_NOTIFIED               0x0f000501

#define TPM2_FFA_SUCCESS_OK                   0x05000001
#define TPM2_FFA_SUCCESS_OK_RESULTS_RETURNED  0x05000002

#define TPM2_FFA_ERROR_NOFUNC             0x8e000001
#define TPM2_FFA_ERROR_NOTSUP             0x8e000002
#define TPM2_FFA_ERROR_INVARG             0x8e000005
#define TPM2_FFA_ERROR_INV_CRB_CTRL_DATA  0x8e000006
#define TPM2_FFA_ERROR_ALREADY            0x8e000009
#define TPM2_FFA_ERROR_DENIED             0x8e00000a
#define TPM2_FFA_ERROR_NOMEM              0x8e00000b

#define TPM_SERVICE_FEATURE_SUPPORT_NOTIFICATION  0xfea70000

#define TPM2_FFA_START_FUNC_QUALIFIER_COMMAND   0x0
#define TPM2_FFA_START_FUNC_QUALIFIER_LOCALITY  0x1

extern EFI_GUID  gTpm2ServiceFfaGuid;

#endif /* TPM2_SERVICE_FFA_H_ */
