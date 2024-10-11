/** @file
  Entry point to the Standalone MM Foundation when initialized during the SEC
  phase on ARM platforms

  Copyright (c) 2017 - 2024, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - SPM_MM - An implementation where the Secure Partition Manager resides at EL3
              with management services running from an isolated Secure Partitions
              at S-EL0, and the communication protocol is the Management Mode(MM)
              interface.
    - FF-A - Firmware Framework for Arm A-profile
    - TL   - Transfer List

  @par Reference(s):
    - Transfer List [https://github.com/FirmwareHandoff/firmware_handoff]
    - Secure Partition Manager [https://trustedfirmware-a.readthedocs.io/en/latest/components/secure-partition-manager-mm.html].
    - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]

**/

#ifndef __STANDALONEMMCORE_ENTRY_POINT_H__
#define __STANDALONEMMCORE_ENTRY_POINT_H__

#include <Library/ArmSvcLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/PeCoffLib.h>
#include <Library/FvLib.h>

#define CPU_INFO_FLAG_PRIMARY_CPU  0x00000001

/*
 * Index information for mm range descriptors in
 * gEfiMmPeiMmramMemoryReserveGuid Guid Hob.
 */
#define MMRAM_DESC_IDX_IMAGE                 0
#define MMRAM_DESC_IDX_SECURE_SHARED_BUFFER  1
#define MMRAM_DESC_IDX_NORMAL_SHARED_BUFFER  2
#define MMRAM_DESC_IDX_HEAP                  3
#define MMRAM_DESC_MIN_COUNT                 4

/*
 * Communication ABI protocol to communicate between normal/secure partition.
 */
typedef enum {
  /// Unknown Communication ABI protocol
  CommProtocolUnknown,

  /// Communicate via SPM_MM ABI protocol
  CommProtocolSpmMm,

  /// Communicate via FF-A ABI protocol
  CommProtocolFfa,

  CommProtocolMax,
} COMM_PROTOCOL;

/** When using FF-A ABI, there're ways to request service to StandaloneMm
      - FF-A with MmCommunication protocol.
      - FF-A service with each specification.
   MmCommunication Protocol can use FFA_MSG_SEND_DIRECT_REQ or REQ2,
   Other FF-A services should use FFA_MSG_SEND_DIRECT_REQ2.
   In case of FF-A with MmCommunication protocol via FFA_MSG_SEND_DIRECT_REQ,
   register x3 saves Communication Buffer with gEfiMmCommunication2ProtocolGuid.
   In case of FF-A with MmCommunication protocol via FFA_MSG_SEND_DIRECT_REQ2,
   register x2/x3 save gEfiMmCommunication2ProtocolGuid and
   register x4 saves Communication Buffer with Service Guid.

   Other FF-A services (ServiceTypeMisc) delivers register values according to
   there own service specification.
   That means it doesn't use MmCommunication Buffer with MmCommunication Header
   format.
   (i.e) Tpm service via FF-A or Firmware Update service via FF-A.
   To support latter services by StandaloneMm, it defines SERVICE_TYPE_MISC.
   So that StandaloneMmEntryPointCore.c generates MmCommunication Header
   with delivered register values to dispatch service provided StandaloneMmCore.
   So that service handler can get proper information from delivered register.

   In case of SPM_MM Abi, it only supports MmCommunication service.
 */
typedef enum {
  /// Unknown
  ServiceTypeUnknown,

  /// MmCommunication services
  ServiceTypeMmCommunication,

  /// Misc services
  ServiceTypeMisc,

  ServiceTypeMax,
} SERVICE_TYPE;

/** Direct message request/response version
 */
typedef enum {
  /// Direct message version 1. Use FFA_DIRECT_MSG_REQ/RESP
  DirectMsgV1,

  /// Direct message version 2. Use FFA_DIRECT_MSG_REQ2/RESP2
  DirectMsgV2,

  DirectMsgMax,
} DIRECT_MSG_VERSION;

/** Service table entry to return service type matched with service guid
 */
typedef struct ServiceTableEntry {
  /// Service Guid
  EFI_GUID        *ServiceGuid;

  /// Service Type
  SERVICE_TYPE    ServiceType;
} SERVICE_TABLE_ENTRY;

/** Ffa Abi data used in FFA_MSG_SEND_DIRECT_RESP/RESP2.
 */
typedef struct FfaMsgInfo {
  /// Source partition id
  UINT16                SourcePartId;

  /// Destination partition id
  UINT16                DestPartId;

  /// Direct Message version
  DIRECT_MSG_VERSION    DirectMsgVersion;

  /// Service Type
  SERVICE_TYPE          ServiceType;
} FFA_MSG_INFO;

/** MmCommunication Header for Misc service.
    Misc service doesn't use MmCommunication Buffer.
    This structure is used to dispatch Misc services by StandaloneMm.
 */
typedef struct {
  /// Service guid
  EFI_GUID           HeaderGuid;

  /// Length of Message. In case of misc service, sizeof (EventSvcArgs)
  UINTN              MessageLength;

  /// Delivered register values.
  DIRECT_MSG_ARGS    DirectMsgArgs;
} MISC_MM_COMMUNICATE_BUFFER;

typedef struct {
  UINT8     Type;    /* type of the structure */
  UINT8     Version; /* version of this structure */
  UINT16    Size;    /* size of this structure in bytes */
  UINT32    Attr;    /* attributes: unused bits SBZ */
} EFI_PARAM_HEADER;

typedef RETURN_STATUS (*REGION_PERMISSION_UPDATE_FUNC) (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

/**
  Privileged firmware assigns RO & Executable attributes to all memory occupied
  by the Boot Firmware Volume. This function sets the correct permissions of
  sections in the Standalone MM Core module to be able to access RO and RW data
  and make further progress in the boot process.

  @param  [in] ImageContext           Pointer to PE/COFF image context
  @param  [in] ImageBase              Base of image in memory
  @param  [in] SectionHeaderOffset    Offset of PE/COFF image section header
  @param  [in] NumberOfSections       Number of Sections
  @param  [in] TextUpdater            Function to change code permissions
  @param  [in] ReadOnlyUpdater        Function to change RO permissions
  @param  [in] ReadWriteUpdater       Function to change RW permissions

**/
EFI_STATUS
EFIAPI
UpdateMmFoundationPeCoffPermissions (
  IN  CONST PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  IN  EFI_PHYSICAL_ADDRESS                ImageBase,
  IN  UINT32                              SectionHeaderOffset,
  IN  CONST  UINT16                       NumberOfSections,
  IN  REGION_PERMISSION_UPDATE_FUNC       TextUpdater,
  IN  REGION_PERMISSION_UPDATE_FUNC       ReadOnlyUpdater,
  IN  REGION_PERMISSION_UPDATE_FUNC       ReadWriteUpdater
  );

/**
  Privileged firmware assigns RO & Executable attributes to all memory occupied
  by the Boot Firmware Volume. This function locates the section information of
  the Standalone MM Core module to be able to change permissions of the
  individual sections later in the boot process.

  @param  [in]      TeData                Pointer to PE/COFF image data
  @param  [in, out] ImageContext          Pointer to PE/COFF image context
  @param  [out]     ImageBase             Pointer to ImageBase variable
  @param  [in, out] SectionHeaderOffset   Offset of PE/COFF image section header
  @param  [in, out] NumberOfSections      Number of Sections

**/
EFI_STATUS
EFIAPI
GetStandaloneMmCorePeCoffSections (
  IN        VOID                          *TeData,
  IN  OUT   PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  OUT   EFI_PHYSICAL_ADDRESS              *ImageBase,
  IN  OUT   UINT32                        *SectionHeaderOffset,
  IN  OUT   UINT16                        *NumberOfSections
  );

/**
  Privileged firmware assigns RO & Executable attributes to all memory occupied
  by the Boot Firmware Volume. This function locates the Standalone MM Core
  module PE/COFF image in the BFV and returns this information.

  @param  [in]      BfvAddress         Base Address of Boot Firmware Volume
  @param  [in, out] TeData             Pointer to address for allocating memory
                                       for PE/COFF image data
  @param  [in, out] TeDataSize         Pointer to size of PE/COFF image data

**/
EFI_STATUS
EFIAPI
LocateStandaloneMmCorePeCoffData (
  IN        EFI_FIRMWARE_VOLUME_HEADER  *BfvAddress,
  IN  OUT   VOID                        **TeData,
  IN  OUT   UINTN                       *TeDataSize
  );

/**
  The handoff between the SPMC to StandaloneMM depends on the
  communication interface between the SPMC and StandaloneMM.
  When SpmMM is used, the handoff is implemented using the
  Firmware Handoff protocol. When FF-A is used the FF-A boot
  protocol is used.

  @param  [in]  Arg0        In case of FF-A, address of FF-A boot information
                            In case of SPM_MM, this parameter must be zero
  @param  [in]  Arg1        In case of FF-A, this parameter must be zero
                            In case of SPM_MM, Signature and register convention version
  @param  [in]  Arg2        Must be zero
  @param  [in]  Arg3        In case of FF-A, this parameter must be zero
                            In case of SPM_MM, address of transfer list

**/
VOID
EFIAPI
CEntryPoint (
  IN UINTN  Arg0,
  IN UINTN  Arg1,
  IN UINTN  Arg2,
  IN UINTN  Arg3
  );

/**
  Auto generated function that calls the library constructors for all of the module's dependent libraries.

  This function must be called by CEntryPoint().
  This function calls the set of library constructors for the set of library instances
  that a module depends on.  This includes library instances that a module depends on
  directly and library instances that a module depends on indirectly through other
  libraries. This function is auto generated by build tools and those build tools are
  responsible for collecting the set of library instances, determine which ones have
  constructors, and calling the library constructors in the proper order based upon
  each of the library instances own dependencies.

  @param  ImageHandle  The image handle of the DXE Core.
  @param  SystemTable  A pointer to the EFI System Table.

**/
VOID
EFIAPI
ProcessLibraryConstructorList (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  );

/**
  Auto generated function that calls a set of module entry points.

  This function must be called by CEntryPoint().
  This function calls the set of module entry points.
  This function is auto generated by build tools and those build tools are responsible
  for collecting the module entry points and calling them in a specified order.

  @param  HobStart  Pointer to the beginning of the HOB List passed in from the PEI Phase.

**/
VOID
EFIAPI
ProcessModuleEntryPointList (
  IN VOID  *HobStart
  );

#endif
