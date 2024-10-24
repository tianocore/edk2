/** @file MockTcgPpiLib.h
  Google Test mocks for TcgPpi

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_TCG_PPI_H__

#define __MOCK_TCG_PPI_H__

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <PiPei.h>

  // #include <Ppi/Tcg.h>
  // NOTE: Directly includes Ppi/Tcg.h will cause a compilation error when building
  // google test driver. Because of in MdePkg/Include/IndustryStandard/Tpm12.h,
  // there is a C++ keyword `operator` is used in the TPM_PERMANENT_FLAGS structure.
  // To workaround this I moved all necessary definitions for TcgPpi into this file.
  #define EV_EFI_EVENT_BASE                                  ((UINT32) 0x80000000)
  #define EV_EFI_PLATFORM_FIRMWARE_BLOB                      (EV_EFI_EVENT_BASE + 8)
  #define EV_EFI_PLATFORM_FIRMWARE_BLOB2                     (EV_EFI_EVENT_BASE + 0xA)
  #define TCG_EfiSpecIDEventStruct_SPEC_ERRATA_TPM2          0
  #define TCG_EfiSpecIDEventStruct_SPEC_ERRATA_TPM2_REV_105  105

  ///
  /// TCG_PCR_EVENT_HDR
  ///
  #define TPM_SHA1_160_HASH_LEN  0x14

  typedef struct tdTPM_DIGEST {
    UINT8    digest[TPM_SHA1_160_HASH_LEN];
  } TPM_DIGEST;
  typedef TPM_DIGEST TCG_DIGEST;

  typedef struct tdTCG_PCR_EVENT_HDR {
    UINT32        PCRIndex;
    UINT32        EventType;
    TCG_DIGEST    Digest;
    UINT32        EventSize;
  } TCG_PCR_EVENT_HDR;

  typedef struct tdEFI_PLATFORM_FIRMWARE_BLOB {
    EFI_PHYSICAL_ADDRESS    BlobBase;
    UINT64                  BlobLength;
  } EFI_PLATFORM_FIRMWARE_BLOB;

  typedef struct _EDKII_TCG_PPI EDKII_TCG_PPI;

  //
  // This bit is shall be set when HashData is the pre-hash digest.
  //
  #define EDKII_TCG_PRE_HASH  0x0000000000000001

  //
  // This bit is shall be set when HashData is the pre-hash digest and log only.
  //
  #define EDKII_TCG_PRE_HASH_LOG_ONLY  0x0000000000000002

  /**
    Tpm measure and log data, and extend the measurement result into a specific PCR.

    @param[in]      This          Indicates the calling context
    @param[in]      Flags         Bitmap providing additional information
    @param[in]      HashData      If BIT0 of Flags is 0, it is physical address of the
                                  start of the data buffer to be hashed, extended, and logged.
                                  If BIT0 of Flags is 1, it is physical address of the
                                  start of the pre-hash data buffter to be extended, and logged.
                                  The pre-hash data format is TPML_DIGEST_VALUES.
    @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData.
    @param[in]      NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.
    @param[in]      NewEventData  Pointer to the new event data.

    @retval EFI_SUCCESS           Operation completed successfully.
    @retval EFI_UNSUPPORTED       TPM device not available.
    @retval EFI_OUT_OF_RESOURCES  Out of memory.
    @retval EFI_DEVICE_ERROR      The operation was unsuccessful.
  **/
  typedef
    EFI_STATUS
  (EFIAPI *EDKII_TCG_HASH_LOG_EXTEND_EVENT)(
  IN      EDKII_TCG_PPI             *This,
  IN      UINT64                    Flags,
  IN      UINT8                     *HashData,
  IN      UINTN                     HashDataLen,
  IN      TCG_PCR_EVENT_HDR         *NewEventHdr,
  IN      UINT8                     *NewEventData
  );

  ///
  /// The EFI_TCG Protocol abstracts TCG activity.
  ///
  struct _EDKII_TCG_PPI {
    EDKII_TCG_HASH_LOG_EXTEND_EVENT    HashLogExtendEvent;
  };

  extern EFI_GUID  gEdkiiTcgPpiGuid;
}

struct MockTcgPpiLib {
  MOCK_INTERFACE_DECLARATION (MockTcgPpiLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    HashLogExtendEvent,
    (IN      EDKII_TCG_PPI             *This,
     IN      UINT64                    Flags,
     IN      UINT8                     *HashData,
     IN      UINTN                     HashDataLen,
     IN      TCG_PCR_EVENT_HDR         *NewEventHdr,
     IN      UINT8                     *NewEventData)
    );
};

#endif
