/** @file
  MM Communication buffer data.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_COMM_BUFFER_H_
#define MM_COMM_BUFFER_H_

#define MM_COMM_BUFFER_HOB_GUID \
  { 0x6c2a2520, 0x0131, 0x4aee, { 0xa7, 0x50, 0xcc, 0x38, 0x4a, 0xac, 0xe8, 0xc6 }}

///
/// This structure is allocated from memory of type EfiRuntimeServicesData.
/// Since runtime memory types are converted to available memory when a legacy boot
/// is performed, there are three parameters, ReturnStatus is the status from
/// MM handler, ReturnBufferSize is the buffer size returned from MM handler,
/// IsCommBufferValid is used to determine whether the software SMI has a valid communication buffer:
/// If TRUE, it indicates a valid communication buffer,
/// If FALSE, it indicates an invalid communication buffer.
///
typedef struct {
  UINT64     ReturnStatus;
  UINT64     ReturnBufferSize;
  BOOLEAN    IsCommBufferValid;
} COMMUNICATION_IN_OUT;

///
/// Communicate buffer structure that is used to share information between the MM IPL and
/// the MM Core.  This structure is allocated from memory of type EfiRuntimeServicesData.
/// Since runtime memory types are converted to available memory when a legacy boot
/// is performed, the MM Core must not access any fields of this structure if a legacy
/// boot is performed.  As a result, the MM IPL must create an event notification
/// for the Legacy Boot event and notify the MM Core that a legacy boot is being
/// performed.  The MM Core can then use this information to filter accesses to
/// those structure.
///
typedef struct {
  ///
  /// This field is used by the MM Communication Protocol to pass a buffer into
  /// a software MMI handler and for the software MMI handler to pass a buffer back to
  /// the caller of the MM Communication Protocol.
  ///
  EFI_PHYSICAL_ADDRESS    FixedCommBuffer;

  ///
  /// This field is used by the MM Communication Protocol to pass the size of a buffer,
  /// in bytes, into a software MMI handler and for the software MMI handler to pass the
  /// size, in bytes, of a buffer back to the caller of the MM Communication Protocol.
  ///
  UINT64                  FixedCommBufferSize;

  ///
  /// This field is used by the MM Communication Protocol to pass the return status from
  /// a software MMI handler back to the caller of the MM Communication Protocol.
  ///
  EFI_PHYSICAL_ADDRESS    CommunicationInOut;
} MM_COMM_BUFFER;

extern EFI_GUID  gEdkiiCommunicationBufferGuid;

#endif
