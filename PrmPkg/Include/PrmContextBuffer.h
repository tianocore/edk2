/** @file

  Definitions for the Platform Runtime Mechanism (PRM) context buffer structures.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_CONTEXT_BUFFER_H_
#define PRM_CONTEXT_BUFFER_H_

#include <PrmDataBuffer.h>
#include <PrmMmio.h>
#include <Uefi.h>

#define PRM_CONTEXT_BUFFER_SIGNATURE          SIGNATURE_32('P','R','M','C')
#define PRM_CONTEXT_BUFFER_INTERFACE_VERSION  1

#pragma pack(push, 1)

//
// Associates an ACPI parameter buffer with a particular PRM handler in
// a PRM module.
//
// If either the GUID or address are zero then neither value is used to
// copy the ACPI parameter buffer address to the PRMT ACPI table.
//
typedef struct {
  EFI_GUID    HandlerGuid;
  UINT64      AcpiParameterBufferAddress;
} ACPI_PARAMETER_BUFFER_DESCRIPTOR;

//
// This is the context buffer structure that is passed to a PRM handler.
//
// At OS runtime, the OS will allocate and populate this structure and
// place virtual addresses in the pointer fields.
//
// It is also reused internally in FW (in the PRM_MODULE_CONTEXT_BUFFERS structure)
// to track context buffers within a given PRM module. In that internal usage,
// the addresses will be physical addresses.
//
typedef struct {
  ///
  /// Signature of this interface.
  ///
  UINT32      Signature;

  ///
  /// Version of this interface.
  ///
  UINT16      Version;

  ///
  /// Reserved field.
  ///
  UINT16      Reserved;

  ///
  /// The GUID of the PRM handler represented by this context instance.
  ///
  EFI_GUID    HandlerGuid;

  ///
  /// A virtual address pointer to the static data buffer allocated for
  /// the PRM handler represented by this context instance.
  ///
  /// The static buffer is intended to be populated in the PRM module
  /// configuration library and treated as read-only data at OS runtime.
  ///
  /// This pointer may be NULL if a static data buffer is not needed.
  ///
  PRM_DATA_BUFFER    *StaticDataBuffer;

  ///
  /// A virtual address pointer to an array of PRM_RUNTIME_MMIO_RANGE
  /// structures that describe MMIO physical address ranges mapped to
  /// virtual memory addresses for access at OS runtime.
  ///
  /// This pointer is ignored in firmware internal usage of this structure
  /// as this field is present to allow a PRM handler to get the list
  /// of MMIO ranges described as accessible by its PRM module.
  ///
  /// The module list of MMIO ranges is specified by the PRM configuration
  /// code as a single array in PRM_MODULE_CONTEXT_BUFFERS.
  ///
  /// The OS is responsible for ensuring the pointer to the array in this
  /// structure is converted to a virtual address during construction of
  /// of the context buffer in the OS.
  ///
  /// This pointer may be NULL if runtime memory ranges are not needed.
  ///
  PRM_RUNTIME_MMIO_RANGES    *RuntimeMmioRanges;
} PRM_CONTEXT_BUFFER;

//
// A firmware internal data structure used to track context buffer and
// runtime MMIO range usage across a PRM module.
//
typedef struct {
  ///
  /// The GUID of the PRM module.
  ///
  EFI_GUID              ModuleGuid;

  ///
  /// The number of PRM context buffers in ContextBuffers[].
  /// This count should equal the number of PRM handlers in the module being configured.
  ///
  UINTN                 BufferCount;

  ///
  /// A pointer to an array of PRM context buffers
  ///
  PRM_CONTEXT_BUFFER    *Buffer;

  /// The MMIO ranges are defined in the firmware boot environment.
  /// The addresses within the PRM_RUNTIME_MMIO_RANGES structure will
  /// be converted to virtual addresses by firmware.

  ///
  /// A physical address pointer to an array of PRM_RUNTIME_MMIO_RANGE
  /// structures that describe memory ranges that need to be mapped to
  /// virtual memory addresses for access at OS runtime.
  ///
  /// This is a consolidated array of MMIO ranges accessed by any PRM
  /// handler in the PRM module at OS runtime. The MMIO range physical
  /// addresses registered here will automatically be converted to the
  /// corresponding virtual address in the structure by PRM infrastructure
  /// code. No action is required to convert MMIO range base physical
  /// addresses to virtual addresses by either the PRM configuration code
  /// or the OS.
  ///
  /// This pointer may be NULL if runtime memory ranges are not needed.
  ///
  PRM_RUNTIME_MMIO_RANGES    *RuntimeMmioRanges;

  ///
  /// The number of ACPI parameter buffer descriptors in the array
  /// AcpiParameterBufferDescriptors
  ///
  UINTN                      AcpiParameterBufferDescriptorCount;

  ///
  /// A pointer to an array of ACPI parameter buffer descriptors. PRM module
  /// configuration code uses this structure to associate a specific PRM
  /// handler with an ACPI parameter buffer.
  ///
  /// An ACPI parameter buffer is a parameter buffer allocated by the PRM
  /// module configuration code to be used by ACPI as a parameter buffer
  /// to the associated PRM handler at OS runtime.
  ///
  /// This buffer is not required if:
  /// 1. A parameter buffer is not used by a PRM handler at all
  /// 2. A parameter buffer is used but the PRM handler is never invoked
  ///    from ACPI (it is directly called by an OS device driver for example)
  ///
  /// In case #2 above, the direct PRM handler is responsible for allocating
  /// a parameter buffer and passing that buffer to the PRM handler.
  ///
  /// A PRM module only needs to provide an ACPI_PARAMETER_BUFFER_DESCRIPTOR
  /// for each PRM handler that actually uses an ACPI parameter buffer. If
  /// no handlers use an ACPI parameter buffer this pointer should be NULL.
  ///
  ACPI_PARAMETER_BUFFER_DESCRIPTOR    *AcpiParameterBufferDescriptors;
} PRM_MODULE_CONTEXT_BUFFERS;

#pragma pack(pop)

#endif
