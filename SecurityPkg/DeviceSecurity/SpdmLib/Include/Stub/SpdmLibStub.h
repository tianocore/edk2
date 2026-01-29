/** @file

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __LIBSPDM_STUB_H__
#define __LIBSPDM_STUB_H__

#include <library/spdm_common_lib.h>
#include <library/spdm_return_status.h>
#include <library/spdm_crypt_lib.h>
#include <library/spdm_requester_lib.h>
#include <library/spdm_responder_lib.h>
#include <library/spdm_transport_pcidoe_lib.h>

#pragma pack(1)

/* The layout of SPDM_RETURN is
 * [31:28] - Severity
 * [27:24] - Reserved
 * [23:16] - Source
 * [15:00] - Code
 */
typedef UINT32 SPDM_RETURN;

/*Interface of spdm.h*/
/* SPDM message header*/
typedef struct {
  UINT8    SPDMVersion;
  UINT8    RequestResponseCode;
  UINT8    Param1;
  UINT8    Param2;
} SPDM_MESSAGE_HEADER;

/* SPDM VERSION structure
 * Bit[15:12] MajorVersion
 * Bit[11:8]  MinorVersion
 * Bit[7:4]   UpdateVersionNumber
 * Bit[3:0]   Alpha*/
typedef UINT16 SPDM_VERSION_NUMBER;

typedef struct {
  /* Total length of the certificate chain, in bytes,
   * including all fields in this table.*/

  UINT16    Length;
  UINT16    Reserved;

  /* digest of the Root Certificate.
   * Note that Root Certificate is ASN.1 DER-encoded for this digest.
   * The hash size is determined by the SPDM device.*/

  /*UINT8    RootHash[HashSize];*/

  /* One or more ASN.1 DER-encoded X509v3 certificates where the first certificate is signed by the Root
   * Certificate or is the Root Certificate itself and each subsequent certificate is signed by the preceding
   * certificate. The last certificate is the Leaf Certificate.*/

  /*UINT8    Certificates[length - 4 - HashSize];*/
} SPDM_CERT_CHAIN;

/* SPDM MEASUREMENTS block common header */
typedef struct {
  UINT8     Index;
  UINT8     MeasurementSpecification;
  UINT16    MeasurementSize;
  /*UINT8                Measurement[MeasurementSize];*/
} SPDM_MEASUREMENT_BLOCK_COMMON_HEADER;

/* SPDM MEASUREMENTS block DMTF header */
typedef struct {
  UINT8     DMTFSpecMeasurementValueType;
  UINT16    DMTFSpecMeasurementValueSize;
  /*UINT8                DMTFSpecMeasurementValue[DMTFSpecMeasurementValueSize];*/
} SPDM_MEASUREMENT_BLOCK_DMTF_HEADER;

typedef struct {
  SPDM_MEASUREMENT_BLOCK_COMMON_HEADER    MeasurementBlockCommonHeader;
  SPDM_MEASUREMENT_BLOCK_DMTF_HEADER      MeasurementBlockDmtfHeader;
  /*UINT8                                 HashValue[HashSize];*/
} SPDM_MEASUREMENT_BLOCK_DMTF;

#define  SPDM_DATA_PARAMETER  libspdm_data_parameter_t

/* DOE header*/

typedef struct {
  UINT16    VendorId;
  UINT8     DataObjectType;
  UINT8     Reserved;

  /* length of the data object being transfered in number of DW, including the header (2 DW)
   * It only includes bit[0~17], bit[18~31] are reserved.
   * A value of 00000h indicate 2^18 DW == 2^20 byte.*/
  UINT32    Length;
  /*UINT32   DataObjectDw[Length];*/
} PCI_DOE_DATA_OBJECT_HEADER;

#pragma pack()

/* FUNCTION */
#define SpdmSetData                         libspdm_set_data
#define SpdmGetData                         libspdm_get_data
#define SpdmInitContext                     libspdm_init_context
#define SpdmGetContextSize                  libspdm_get_context_size
#define SpdmRegisterDeviceIoFunc            libspdm_register_device_io_func
#define SpdmRegisterTransportLayerFunc      libspdm_register_transport_layer_func
#define SpdmGetSizeofRequiredScratchBuffer  libspdm_get_sizeof_required_scratch_buffer
#define SpdmRegisterDeviceBufferFunc        libspdm_register_device_buffer_func
#define SpdmSetScratchBuffer                libspdm_set_scratch_buffer

#define SpdmGetHashSize               libspdm_get_hash_size
#define SpdmHashAll                   libspdm_hash_all
#define SpdmGetMeasurementHashSize    libspdm_get_measurement_hash_size
#define SpdmMeasurementHashAll        libspdm_measurement_hash_all
#define SpdmHmacAll                   libspdm_hmac_all
#define SpdmHkdfExpand                libspdm_hkdf_expand
#define SpdmAsymFree                  libspdm_asym_free
#define SpdmAsymGetPrivateKeyFromPem  libspdm_asym_get_private_key_from_pem
#define SpdmAsymSign                  libspdm_asym_sign
#define SpdmAsymSignHash              libspdm_asym_sign_hash

#define SpdmInitConnection                libspdm_init_connection
#define SpdmGetDigest                     libspdm_get_digest
#define SpdmGetCertificate                libspdm_get_certificate
#define SpdmGetCertificateEx              libspdm_get_certificate_ex
#define SpdmChallenge                     libspdm_challenge
#define SpdmChallengeEx                   libspdm_challenge_ex
#define SpdmGetMeasurement                libspdm_get_measurement
#define SpdmGetMeasurementEx              libspdm_get_measurement_ex
#define SpdmStartSession                  libspdm_start_session
#define SpdmStopSession                   libspdm_stop_session
#define SpdmSendReceiveData               libspdm_send_receive_data
#define SpdmRegisterGetResponseFunc       libspdm_register_get_response_func
#define SpdmProcessRequest                libspdm_process_request
#define SpdmBuildResponse                 libspdm_build_response
#define SpdmGenerateErrorResponse         libspdm_generate_error_response
#define SpdmTransportPciDoeEncodeMessage  libspdm_transport_pci_doe_encode_message
#define SpdmTransportPciDoeDecodeMessage  libspdm_transport_pci_doe_decode_message

#define SpdmMeasurementCollectionFunc         libspdm_measurement_collection
#define SpdmRequesterDataSignFunc             libspdm_requester_data_sign
#define SpdmResponderDataSignFunc             libspdm_responder_data_sign
#define SpdmGenerateMeasurementSummaryHash    libspdm_generate_measurement_summary_hash
#define SpdmPskMasterSecretHkdfExpandFunc     libspdm_psk_master_secret_hkdf_expand
#define SpdmPskHandshakeSecretHkdfExpandFunc  libspdm_psk_handshake_secret_hkdf_expand
#define SpdmMeasurementOpaqueData             libspdm_measurement_opaque_data
#define SpdmChallengeOpaqueData               libspdm_challenge_opaque_data

#endif
