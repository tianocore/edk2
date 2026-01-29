/** @file
  The internal header file that declared a data structure that is shared
  between the SMM IPL and the SMM Core.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PI_SMM_CORE_PRIVATE_DATA_H_
#define _PI_SMM_CORE_PRIVATE_DATA_H_

///
/// Define values for the communications buffer used when gEfiEventDxeDispatchGuid is
/// event signaled.  This event is signaled by the DXE Core each time the DXE Core
/// dispatcher has completed its work.  When this event is signaled, the SMM Core
/// if notified, so the SMM Core can dispatch SMM drivers.  If COMM_BUFFER_SMM_DISPATCH_ERROR
/// is returned in the communication buffer, then an error occurred dispatching SMM
/// Drivers.  If COMM_BUFFER_SMM_DISPATCH_SUCCESS is returned, then the SMM Core
/// dispatched all the drivers it could.  If COMM_BUFFER_SMM_DISPATCH_RESTART is
/// returned, then the SMM Core just dispatched the SMM Driver that registered
/// the SMM Entry Point enabling the use of SMM Mode.  In this case, the SMM Core
/// should be notified again to dispatch more SMM Drivers using SMM Mode.
///
#define COMM_BUFFER_SMM_DISPATCH_ERROR    0x00
#define COMM_BUFFER_SMM_DISPATCH_SUCCESS  0x01
#define COMM_BUFFER_SMM_DISPATCH_RESTART  0x02

///
/// Signature for the private structure shared between the SMM IPL and the SMM Core
///
#define SMM_CORE_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('s', 'm', 'm', 'c')

///
/// Private structure that is used to share information between the SMM IPL and
/// the SMM Core.  This structure is allocated from memory of type EfiRuntimeServicesData.
/// Since runtime memory types are converted to available memory when a legacy boot
/// is performed, the SMM Core must not access any fields of this structure if a legacy
/// boot is performed.  As a result, the SMM IPL must create an event notification
/// for the Legacy Boot event and notify the SMM Core that a legacy boot is being
/// performed.  The SMM Core can then use this information to filter accesses to
/// thos structure.
///
typedef struct {
  UINTN                   Signature;

  ///
  /// The ImageHandle passed into the entry point of the SMM IPL.  This ImageHandle
  /// is used by the SMM Core to fill in the ParentImageHandle field of the Loaded
  /// Image Protocol for each SMM Driver that is dispatched by the SMM Core.
  ///
  EFI_HANDLE              SmmIplImageHandle;

  ///
  /// The number of SMRAM ranges passed from the SMM IPL to the SMM Core.  The SMM
  /// Core uses these ranges of SMRAM to initialize the SMM Core memory manager.
  ///
  UINTN                   SmramRangeCount;

  ///
  /// A table of SMRAM ranges passed from the SMM IPL to the SMM Core.  The SMM
  /// Core uses these ranges of SMRAM to initialize the SMM Core memory manager.
  ///
  EFI_SMRAM_DESCRIPTOR    *SmramRanges;

  ///
  /// The SMM Foundation Entry Point.  The SMM Core fills in this field when the
  /// SMM Core is initialized.  The SMM IPL is responsible for registering this entry
  /// point with the SMM Configuration Protocol.  The SMM Configuration Protocol may
  /// not be available at the time the SMM IPL and SMM Core are started, so the SMM IPL
  /// sets up a protocol notification on the SMM Configuration Protocol and registers
  /// the SMM Foundation Entry Point as soon as the SMM Configuration Protocol is
  /// available.
  ///
  EFI_SMM_ENTRY_POINT      SmmEntryPoint;

  ///
  /// Boolean flag set to TRUE while an SMI is being processed by the SMM Core.
  ///
  BOOLEAN                  SmmEntryPointRegistered;

  ///
  /// Boolean flag set to TRUE while an SMI is being processed by the SMM Core.
  ///
  BOOLEAN                  InSmm;

  ///
  /// This field is set by the SMM Core then the SMM Core is initialized.  This field is
  /// used by the SMM Base 2 Protocol and SMM Communication Protocol implementations in
  /// the SMM IPL.
  ///
  EFI_SMM_SYSTEM_TABLE2    *Smst;

  ///
  /// This field is used by the SMM Communication Protocol to pass a buffer into
  /// a software SMI handler and for the software SMI handler to pass a buffer back to
  /// the caller of the SMM Communication Protocol.
  ///
  VOID                     *CommunicationBuffer;

  ///
  /// This field is used by the SMM Communication Protocol to pass the size of a buffer,
  /// in bytes, into a software SMI handler and for the software SMI handler to pass the
  /// size, in bytes, of a buffer back to the caller of the SMM Communication Protocol.
  ///
  UINTN                    BufferSize;

  ///
  /// This field is used by the SMM Communication Protocol to pass the return status from
  /// a software SMI handler back to the caller of the SMM Communication Protocol.
  ///
  EFI_STATUS               ReturnStatus;

  EFI_PHYSICAL_ADDRESS     PiSmmCoreImageBase;
  UINT64                   PiSmmCoreImageSize;
  EFI_PHYSICAL_ADDRESS     PiSmmCoreEntryPoint;
} SMM_CORE_PRIVATE_DATA;

#endif
