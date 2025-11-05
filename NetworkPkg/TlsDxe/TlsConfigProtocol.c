/** @file
  Implementation of EFI TLS Configuration Protocol Interfaces.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TlsImpl.h"

EFI_TLS_CONFIGURATION_PROTOCOL  mTlsConfigurationProtocol = {
  TlsConfigurationSetData,
  TlsConfigurationGetData
};

/**
  Set TLS configuration data.

  The SetData() function sets TLS configuration to non-volatile storage or volatile
  storage.

  @param[in]  This                Pointer to the EFI_TLS_CONFIGURATION_PROTOCOL instance.
  @param[in]  DataType            Configuration data type.
  @param[in]  Data                Pointer to configuration data.
  @param[in]  DataSize            Total size of configuration data.

  @retval EFI_SUCCESS             The TLS configuration data is set successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Data is NULL.
                                  DataSize is 0.
  @retval EFI_UNSUPPORTED         The DataType is unsupported.
  @retval EFI_OUT_OF_RESOURCES    Required system resources could not be allocated.

**/
EFI_STATUS
EFIAPI
TlsConfigurationSetData (
  IN     EFI_TLS_CONFIGURATION_PROTOCOL  *This,
  IN     EFI_TLS_CONFIG_DATA_TYPE        DataType,
  IN     VOID                            *Data,
  IN     UINTN                           DataSize
  )
{
  EFI_STATUS    Status;
  TLS_INSTANCE  *Instance;
  EFI_TPL       OldTpl;

  Status = EFI_SUCCESS;

  if ((This == NULL) ||  (Data == NULL) || (DataSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = TLS_INSTANCE_FROM_CONFIGURATION (This);

  switch (DataType) {
    case EfiTlsConfigDataTypeCACertificate:
      Status = TlsSetCaCertificate (Instance->TlsConn, Data, DataSize);
      break;
    case EfiTlsConfigDataTypeHostPublicCert:
      Status = TlsSetHostPublicCert (Instance->TlsConn, Data, DataSize);
      break;
    case EfiTlsConfigDataTypeHostPrivateKey:
      Status = TlsSetHostPrivateKey (Instance->TlsConn, Data, DataSize);
      break;
    case EfiTlsConfigDataTypeCertRevocationList:
      Status = TlsSetCertRevocationList (Data, DataSize);
      break;
    default:
      Status = EFI_UNSUPPORTED;
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Get TLS configuration data.

  The GetData() function gets TLS configuration.

  @param[in]       This           Pointer to the EFI_TLS_CONFIGURATION_PROTOCOL instance.
  @param[in]       DataType       Configuration data type.
  @param[in, out]  Data           Pointer to configuration data.
  @param[in, out]  DataSize       Total size of configuration data. On input, it means
                                  the size of Data buffer. On output, it means the size
                                  of copied Data buffer if EFI_SUCCESS, and means the
                                  size of desired Data buffer if EFI_BUFFER_TOO_SMALL.

  @retval EFI_SUCCESS             The TLS configuration data is got successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  DataSize is NULL.
                                  Data is NULL if *DataSize is not zero.
  @retval EFI_UNSUPPORTED         The DataType is unsupported.
  @retval EFI_NOT_FOUND           The TLS configuration data is not found.
  @retval EFI_BUFFER_TOO_SMALL    The buffer is too small to hold the data.
**/
EFI_STATUS
EFIAPI
TlsConfigurationGetData (
  IN     EFI_TLS_CONFIGURATION_PROTOCOL  *This,
  IN     EFI_TLS_CONFIG_DATA_TYPE        DataType,
  IN OUT VOID                            *Data  OPTIONAL,
  IN OUT UINTN                           *DataSize
  )
{
  EFI_STATUS    Status;
  TLS_INSTANCE  *Instance;

  EFI_TPL  OldTpl;

  Status = EFI_SUCCESS;

  if ((This == NULL) || (DataSize == NULL) || ((Data == NULL) && (*DataSize != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = TLS_INSTANCE_FROM_CONFIGURATION (This);

  switch (DataType) {
    case EfiTlsConfigDataTypeCACertificate:
      Status = TlsGetCaCertificate (Instance->TlsConn, Data, DataSize);
      break;
    case EfiTlsConfigDataTypeHostPublicCert:
      Status = TlsGetHostPublicCert (Instance->TlsConn, Data, DataSize);
      break;
    case EfiTlsConfigDataTypeHostPrivateKey:
      Status = TlsGetHostPrivateKey (Instance->TlsConn, Data, DataSize);
      break;
    case EfiTlsConfigDataTypeCertRevocationList:
      Status = TlsGetCertRevocationList (Data, DataSize);
      break;
    default:
      Status = EFI_UNSUPPORTED;
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}
