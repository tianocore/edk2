/** @file
  MM Core data.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2018 - 2021, Arm Limited. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MM_CORE_DATA_H__
#define __MM_CORE_DATA_H__

#define MM_CORE_DATA_HOB_GUID \
  { 0xa160bf99, 0x2aa4, 0x4d7d, { 0x99, 0x93, 0x89, 0x9c, 0xb1, 0x2d, 0xf3, 0x76 }}

extern EFI_GUID gMmCoreDataHobGuid;

typedef struct {
  //
  // Address pointer to MM_CORE_PRIVATE_DATA
  //
  EFI_PHYSICAL_ADDRESS   Address;
} MM_CORE_DATA_HOB_DATA;


///
/// Define values for the communications buffer used when gEfiEventDxeDispatchGuid is
/// event signaled.  This event is signaled by the DXE Core each time the DXE Core
/// dispatcher has completed its work.  When this event is signaled, the MM Core
/// if notified, so the MM Core can dispatch MM drivers.  If COMM_BUFFER_MM_DISPATCH_ERROR
/// is returned in the communication buffer, then an error occurred dispatching MM
/// Drivers.  If COMM_BUFFER_MM_DISPATCH_SUCCESS is returned, then the MM Core
/// dispatched all the drivers it could.  If COMM_BUFFER_MM_DISPATCH_RESTART is
/// returned, then the MM Core just dispatched the MM Driver that registered
/// the MM Entry Point enabling the use of MM Mode.  In this case, the MM Core
/// should be notified again to dispatch more MM Drivers using MM Mode.
///
#define COMM_BUFFER_MM_DISPATCH_ERROR    0x00
#define COMM_BUFFER_MM_DISPATCH_SUCCESS  0x01
#define COMM_BUFFER_MM_DISPATCH_RESTART  0x02

///
/// Signature for the private structure shared between the MM IPL and the MM Core
///
#define MM_CORE_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('m', 'm', 'i', 'c')

///
/// Private structure that is used to share information between the MM IPL and
/// the MM Core.  This structure is allocated from memory of type EfiRuntimeServicesData.
/// Since runtime memory types are converted to available memory when a legacy boot
/// is performed, the MM Core must not access any fields of this structure if a legacy
/// boot is performed.  As a result, the MM IPL must create an event notification
/// for the Legacy Boot event and notify the MM Core that a legacy boot is being
/// performed.  The MM Core can then use this information to filter accesses to
/// thos structure.
///
typedef struct {
  UINT64                          Signature;

  ///
  /// The number of MMRAM ranges passed from the MM IPL to the MM Core.  The MM
  /// Core uses these ranges of MMRAM to initialize the MM Core memory manager.
  ///
  UINT64                          MmramRangeCount;

  ///
  /// A table of MMRAM ranges passed from the MM IPL to the MM Core.  The MM
  /// Core uses these ranges of MMRAM to initialize the MM Core memory manager.
  ///
  EFI_PHYSICAL_ADDRESS            MmramRanges;

  ///
  /// The MM Foundation Entry Point.  The MM Core fills in this field when the
  /// MM Core is initialized.  The MM IPL is responsbile for registering this entry
  /// point with the MM Configuration Protocol.  The MM Configuration Protocol may
  /// not be available at the time the MM IPL and MM Core are started, so the MM IPL
  /// sets up a protocol notification on the MM Configuration Protocol and registers
  /// the MM Foundation Entry Point as soon as the MM Configuration Protocol is
  /// available.
  ///
  EFI_PHYSICAL_ADDRESS            MmEntryPoint;

  ///
  /// Boolean flag set to TRUE while an MMI is being processed by the MM Core.
  ///
  BOOLEAN                         MmEntryPointRegistered;

  ///
  /// Boolean flag set to TRUE while an MMI is being processed by the MM Core.
  ///
  BOOLEAN                         InMm;

  ///
  /// This field is set by the MM Core then the MM Core is initialized.  This field is
  /// used by the MM Base 2 Protocol and MM Communication Protocol implementations in
  /// the MM IPL.
  ///
  EFI_PHYSICAL_ADDRESS            Mmst;

  ///
  /// This field is used by the MM Communication Protocol to pass a buffer into
  /// a software MMI handler and for the software MMI handler to pass a buffer back to
  /// the caller of the MM Communication Protocol.
  ///
  EFI_PHYSICAL_ADDRESS            CommunicationBuffer;

  ///
  /// This field is used by the MM Communication Protocol to pass the size of a buffer,
  /// in bytes, into a software MMI handler and for the software MMI handler to pass the
  /// size, in bytes, of a buffer back to the caller of the MM Communication Protocol.
  ///
  UINT64                          BufferSize;

  ///
  /// This field is used by the MM Communication Protocol to pass the return status from
  /// a software MMI handler back to the caller of the MM Communication Protocol.
  ///
  UINT64                          ReturnStatus;

  EFI_PHYSICAL_ADDRESS            MmCoreImageBase;
  UINT64                          MmCoreImageSize;
  EFI_PHYSICAL_ADDRESS            MmCoreEntryPoint;

  EFI_PHYSICAL_ADDRESS            StandaloneBfvAddress;
} MM_CORE_PRIVATE_DATA;

#endif
