/** @file
  Provides Key Services.

Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

@par Specification Reference:
**/

#ifndef PEI_KEY_SERVICE_PPI_H_
#define PEI_KEY_SERVICE_PPI_H_
///
/// KEY SERVICE PPI GUID
///
extern EFI_GUID  gKeyServicePpiGuid;

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
(EFIAPI *KEY_SERVICE_GEN_KEY)(
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
  UINT8    RootKey[ROOT_KEY_LEN];
  UINT8    PreviousRootKey[ROOT_KEY_LEN];
} KEY_SERVICE_DATA;

typedef struct _KEY_SERVICE_PPI KEY_SERVICE_PPI;

///
/// KEY SERVICE PPI
/// The interface functions are for Key Service in PEI Phase
///
struct _KEY_SERVICE_PPI {
  KEY_SERVICE_GEN_KEY    GenerateKey; /// Generate Key
};

#endif
