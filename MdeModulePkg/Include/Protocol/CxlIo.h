/** @file
  EFI CXL I/O Protocol provides the interfaces to interact with the CXL-specific
  subsystems of CXL devices. Other interactions with CXL devices should be
  routed through EFI_PCI_IO_PROTOCOL

  Copyright (c) 2026, Google, LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CXL_IO_H__
#define __CXL_IO_H__

#include <Uefi/UefiBaseType.h>
#include "IndustryStandard/Cxl20.h"
#include "Protocol/PciIo.h"

#include <IndustryStandard/Cxl.h>
#include <IndustryStandard/CxlCdat.h>

///
/// Global ID for the CXL I/O Protocol
///
// 8fac60b2-bbc1-41de-a797-9875cdd3a8f8

#define EDKII_CXL_IO_PROTOCOL_GUID \
  { \
    0x8fac60b2, 0xbbc1, 0x41de, {0xa7, 0x97, 0x98, 0x75, 0xcd, 0xd3, 0xa8, 0xf8 } \
  }

typedef struct _EDKII_CXL_IO_PROTOCOL EDKII_CXL_IO_PROTOCOL;

/**
  Performs a CXL DOE transaction.

  @param  This                  A pointer to the EDKII_CXL_IO_PROTOCOL instance.
  @param  RequestBuffer         Payload of the request. This buffer should contain anything needed
                                for the DOE request beyond the standard 2 DWORD header. If not extra
                                data is required, this may be NULL.
  @param  RequestBufferSize     Size of the request buffer in bytes. If RequestBuffer is NULL,
                                RequestBufferSize must be 0.
  @param  ResponseBuffer        Destination buffer for the response.
  @param  ResponseBufferSize    As an input parameter, the maximum size of the buffer.
                                As an output parameter, the actual size of the response.

  @retval EFI_SUCCESS           The data was read from or written to the CXL controller.
  @retval EFI_UNSUPPORTED       This device does not support DOE.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_CXL_IO_PROTOCOL_IO_DOE_TRANSACT)(
  IN     EDKII_CXL_IO_PROTOCOL          *This,
  IN CONST VOID                       *RequestBuffer,
  IN     UINTN                        RequestBufferSize,
  OUT VOID                            *ResponseBuffer,
  IN OUT UINTN                        *ResponseBufferSize
  );

/**
  Transacts a set of regblock registers.

  @param[in]      This           A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]      RegBlock       The type of regblock regs.
  @param[in]      Offset         Offset from the start of the regblock.
  @param[in, out] Buf            The bufer to write or fill.
  @param[in]      Size           The size of the buffer.

  @retval EFI_SUCCESS All bytes were written or read successfully.
  @retval other       Something went wrong.
*/
typedef
EFI_STATUS
(EFIAPI *EDKII_CXL_IO_PROTOCOL_IO_REGBLOCK)(
  IN EDKII_CXL_IO_PROTOCOL          *This,
  IN UINT32     RegBlock,
  IN UINT64                       Offset,
  IN VOID                         *Buf,
  IN UINTN                        Size
  );

typedef struct {
  ///
  /// Read CXL DVSEC.
  ///
  EDKII_CXL_IO_PROTOCOL_IO_REGBLOCK    Read;
  ///
  /// Write CXL DVSEC.
  ///
  EDKII_CXL_IO_PROTOCOL_IO_REGBLOCK    Write;
} EDKII_CXL_IO_PROTOCOL_REGBLOCK_ACCESS;

/**
  Reads a set of DVSEC registers.

  @param[in]      This           A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]      CxlDvsecId     The DVSEC to read or write. See #defines prefixed with CXL_DVSEC_ID_.
  @param[in]      Offset         Offset from the start of the DVSEC.
  @param[in, out] Buf            The buffer to write or fill.
  @param[in]      Size           The size of the buffer.

  @retval EFI_SUCCESS All bytes were written or read successfully.
  @retval other       Something went wrong.
*/
typedef
EFI_STATUS
(EFIAPI *EDKII_CXL_IO_PROTOCOL_IO_DVSEC)(
  IN EDKII_CXL_IO_PROTOCOL          *This,
  IN UINT32                       CxlDvsecId,
  IN UINT32                       Offset,
  IN VOID                         *Buf,
  IN UINTN                        Size
  );

typedef struct {
  ///
  /// Read CXL DVSEC.
  ///
  EDKII_CXL_IO_PROTOCOL_IO_DVSEC    Read;
  ///
  /// Write CXL DVSEC.
  ///
  EDKII_CXL_IO_PROTOCOL_IO_DVSEC    Write;
} EDKII_CXL_IO_PROTOCOL_DVSEC_ACCESS;

///
///  EFI CXL I/O Protocol provides the interfaces to interact with the CXL-specific
///  subsystems of CXL devices. Other interactions with CXL devices should be
///  routed through EFI_PCI_IO_PROTOCOL.
///
struct _EDKII_CXL_IO_PROTOCOL {
  ///
  /// Access to the associated PciIo protocol for this CXL device.
  ///
  EFI_PCI_IO_PROTOCOL                      *PciIo;
  ///
  /// Data object exchange request-response transaction.
  ///
  EDKII_CXL_IO_PROTOCOL_IO_DOE_TRANSACT    DoeTransact;
  ///
  /// Regblock register getter/setter.
  ///
  EDKII_CXL_IO_PROTOCOL_REGBLOCK_ACCESS    Regblock;
  ///
  /// DVSEC register getter/setter.
  ///
  EDKII_CXL_IO_PROTOCOL_DVSEC_ACCESS       Dvsec;
};

extern EFI_GUID  gEdkiiCxlIoProtocolGuid;

#endif
