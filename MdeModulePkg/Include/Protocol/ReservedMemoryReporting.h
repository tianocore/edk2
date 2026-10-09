/** @file  // [CODE_FIRST] 13253
  Defines the Reserved-Memory Reporting (RMEM) registration protocol.  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Copyright (c) Microsoft Corporation.  // [CODE_FIRST] 13253
  SPDX-License-Identifier: BSD-2-Clause-Patent  // [CODE_FIRST] 13253
**/  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#pragma once  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#include <Guid/ReservedMemoryReportingTable.h>  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
/* [CODE_FIRST] 13253 */ #define EDKII_RMEM_REGISTRATION_PROTOCOL_GUID \
  { 0x0cb661d3, 0x3c81, 0x4074, { 0xa6, 0xa8, 0xc9, 0x75, 0x9c, 0xc5, 0x12, 0x07 } }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#define EDKII_RMEM_REGISTRATION_PROTOCOL_REVISION  1  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
typedef struct _EDKII_RMEM_REGISTRATION_PROTOCOL EDKII_RMEM_REGISTRATION_PROTOCOL;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
/**  // [CODE_FIRST] 13253
  Registers a reserved physical-memory range for inclusion in the RMEM ACPI  // [CODE_FIRST] 13253
  table.  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  The protocol copies the label before returning. The caller retains ownership  // [CODE_FIRST] 13253
  of the label buffer.  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  @param[in] This      A pointer to the EDKII_RMEM_REGISTRATION_PROTOCOL  // [CODE_FIRST] 13253
                       instance.  // [CODE_FIRST] 13253
  @param[in] Base      The actual physical address of the first byte in the  // [CODE_FIRST] 13253
                       range. This must be aligned to EFI_PAGE_SIZE.  // [CODE_FIRST] 13253
  @param[in] Size      The size of the range in bytes. This must be aligned to  // [CODE_FIRST] 13253
                       EFI_PAGE_SIZE.  // [CODE_FIRST] 13253
  @param[in] Category  The purpose category assigned to the range.  // [CODE_FIRST] 13253
  @param[in] Flags     RMEM entry flags. Unsupported bits must be zero.  // [CODE_FIRST] 13253
  @param[in] Label     An optional null-terminated ASCII diagnostic label. The  // [CODE_FIRST] 13253
                       label, including its null terminator, must fit within  // [CODE_FIRST] 13253
                       RMEM_LABEL_MAX_LEN bytes. Therefore, the label may  // [CODE_FIRST] 13253
                       contain at most RMEM_LABEL_MAX_LEN - 1 characters.  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  @retval EFI_SUCCESS           The range was registered.  // [CODE_FIRST] 13253
  @retval EFI_INVALID_PARAMETER A parameter, category, or flag value is invalid.  // [CODE_FIRST] 13253
  @retval EFI_BAD_BUFFER_SIZE   The label and its null terminator do not fit  // [CODE_FIRST] 13253
                                within RMEM_LABEL_MAX_LEN bytes.  // [CODE_FIRST] 13253
  @retval EFI_ACCESS_DENIED     Registration is finalized or the range overlaps  // [CODE_FIRST] 13253
                                an existing entry.  // [CODE_FIRST] 13253
  @retval EFI_OUT_OF_RESOURCES  The registration capacity has been reached.  // [CODE_FIRST] 13253
**/  // [CODE_FIRST] 13253
typedef  // [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
(EFIAPI *EDKII_RMEM_ADD_RESERVED_RANGE)(  // [CODE_FIRST] 13253
  IN EDKII_RMEM_REGISTRATION_PROTOCOL  *This,  // [CODE_FIRST] 13253
  IN EFI_PHYSICAL_ADDRESS              Base,  // [CODE_FIRST] 13253
  IN UINT64                            Size,  // [CODE_FIRST] 13253
  IN RMEM_CATEGORY                     Category,  // [CODE_FIRST] 13253
  IN UINT16                            Flags,  // [CODE_FIRST] 13253
  IN CONST CHAR8                       *Label OPTIONAL  // [CODE_FIRST] 13253
  );  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
/// Provides the interface used by DXE producers to register reserved-memory  // [CODE_FIRST] 13253
/// ranges with the RMEM ACPI table publisher.  // [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
struct _EDKII_RMEM_REGISTRATION_PROTOCOL {  // [CODE_FIRST] 13253
  UINT32                           Revision;  // [CODE_FIRST] 13253
  EDKII_RMEM_ADD_RESERVED_RANGE    AddReservedRange;  // [CODE_FIRST] 13253
};  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
extern EFI_GUID  gEdkiiRmemRegistrationProtocolGuid;  // [CODE_FIRST] 13253
