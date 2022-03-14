/** @file

@copyright
  INTEL CONFIDENTIAL
  Copyright 2022 Intel Corporation.

  The source code contained or described herein and all documents related to the
  source code ("Material") are owned by Intel Corporation or its suppliers or
  licensors. Title to the Material remains with Intel Corporation or its suppliers
  and licensors. The Material may contain trade secrets and proprietary and
  confidential information of Intel Corporation and its suppliers and licensors,
  and is protected by worldwide copyright and trade secret laws and treaty
  provisions. No part of the Material may be used, copied, reproduced, modified,
  published, uploaded, posted, transmitted, distributed, or disclosed in any way
  without Intel's prior express written permission.

  No license under any patent, copyright, trade secret or other intellectual
  property right is granted to or conferred upon you by disclosure or delivery
  of the Materials, either expressly, by implication, inducement, estoppel or
  otherwise. Any license under such intellectual property rights must be
  express and approved by Intel in writing.

  Unless otherwise agreed by Intel in writing, you may not remove or alter
  this notice or any other notice embedded in Materials by Intel or
  Intel's suppliers or licensors in any way.

  This file contains a 'Sample Driver' and is licensed as such under the terms
  of your license agreement with Intel or your vendor. This file may be modified
  by the user, subject to the additional terms of the license agreement.

@par Specification Reference:
**/

#ifndef _PEI_KEY_SERVICE_PPI_H
#define _PEI_KEY_SERVICE_PPI_H
///
/// KEY SERVICE PPI GUID
///
extern EFI_GUID gKeyServicePpiGuid;

/**
  Generate a new key from root key.

  @param[in]   Salt                     Pointer to the salt(non-secret) value.
  @param[in]   SaltSize                 Salt size in bytes.
  @param[out]  NewKey                   Pointer to buffer to receive new key.
  @param[in]   NewKeySize               Size of new key bytes to generate.

  @retval EFI_SUCCESS                   The function completed successfully
  @retval OTHER                         The function completed with failure.
**/
typedef
EFI_STATUS
(EFIAPI *KEY_SERVICE_GEN_KEY) (
  IN   UINT8        *Salt,
  IN   UINTN        SaltSize,
  OUT  UINT8        *NewKey,
  IN   UINTN        NewKeySize
  );

#define KEY_SERVICE_PPI_REVISION  1
#define ROOT_KEY_LEN              64
#define SALT_SIZE_MIN_LEN         64
#define KEY_SERVICE_KEY_NAME      L"KEY_SERVICE_KEY"

typedef struct {
  UINT8 RootKey[ROOT_KEY_LEN];
  UINT8 PreviousRootKey[ROOT_KEY_LEN];
} KEY_SERVICE_DATA;

typedef struct _KEY_SERVICE_PPI  KEY_SERVICE_PPI;

///
/// KEY SERVICE PPI
/// The interface functions are for Key Service in PEI Phase
///
struct _KEY_SERVICE_PPI {
  KEY_SERVICE_GEN_KEY    GenerateKey; /// Generate Key
};
#endif

