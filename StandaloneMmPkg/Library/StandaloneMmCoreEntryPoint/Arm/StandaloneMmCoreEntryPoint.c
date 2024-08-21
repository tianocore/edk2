/** @file
  Entry point to the Standalone MM Foundation when initialized during the SEC
  phase on ARM platforms

  Copyright (c) 2017 - 2024, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - SpmMM - An implementation where the Secure Partition Manager resides at EL3
              with management services running from an isolated Secure Partitions
              at S-EL0, and the communication protocol is the Management Mode(MM)
              interface.

  @par Reference(s):
  - Secure Partition Manager [https://trustedfirmware-a.readthedocs.io/en/latest/components/secure-partition-manager-mm.html].

**/

#include <PiMm.h>

#include <Library/Arm/StandaloneMmCoreEntryPoint.h>

#include <PiPei.h>
#include <Guid/MmramMemoryReserve.h>
#include <Guid/MmCoreData.h>
#include <Guid/MpInformation.h>

#include <Library/ArmLib.h>
#include <Library/ArmSvcLib.h>
#include <Library/ArmTransferListLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SerialPortLib.h>
#include <Library/StandaloneMmMmuLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/ArmStdSmc.h>
#include <IndustryStandard/ArmMmSvc.h>
#include <IndustryStandard/ArmFfaSvc.h>

#include <Protocol/PiMmCpuDriverEp.h>

extern EFI_MM_SYSTEM_TABLE  gMmCoreMmst;

VOID  *gHobList = NULL;

STATIC PI_MM_CPU_DRIVER_ENTRYPOINT  CpuDriverEntryPoint = NULL;
STATIC MP_INFORMATION_HOB_DATA      *mMpInfo            = NULL;

/**
  Get ABI protocol.

  @retval         AbiProtocolSpmMm         SPM_MM ABI
  @retval         AbiProtocolFfa           FF-A ABI
  @retval         AbiProtocolUnknown       Invalid ABI.

**/
STATIC
ABI_PROTOCOL
EFIAPI
GetAbiProtocol (
  IN VOID
  )
{
  UINT16        RequestMajorVersion;
  UINT16        RequestMinorVersion;
  UINT16        CurrentMajorVersion;
  UINT16        CurrentMinorVersion;
  ARM_SVC_ARGS  SvcArgs;
  ABI_PROTOCOL  AbiProtocol;

  ZeroMem (&SvcArgs, sizeof (ARM_SVC_ARGS));
  SvcArgs.Arg0 = ARM_SVC_ID_FFA_VERSION_AARCH32;
  SvcArgs.Arg1 = ((SPM_MAJOR_VERSION_FFA <<  16) | (SPM_MINOR_VERSION_FFA));

  ArmCallSvc (&SvcArgs);

  if (SvcArgs.Arg0 != ARM_FFA_SPM_RET_NOT_SUPPORTED) {
    AbiProtocol         = AbiProtocolFfa;
    RequestMajorVersion = SPM_MAJOR_VERSION_FFA;
    RequestMinorVersion = SPM_MINOR_VERSION_FFA;
    CurrentMajorVersion = ((SvcArgs.Arg0 >> 16) & 0xffff);
    CurrentMinorVersion = (SvcArgs.Arg0 & 0xffff);
  } else {
    ZeroMem (&SvcArgs, sizeof (ARM_SVC_ARGS));
    SvcArgs.Arg0 = ARM_FID_SPM_MM_VERSION_AARCH32;

    ArmCallSvc (&SvcArgs);

    if (SvcArgs.Arg0 == ARM_SPM_MM_RET_NOT_SUPPORTED) {
      return AbiProtocolUnknown;
    }

    AbiProtocol         = AbiProtocolSpmMm;
    RequestMajorVersion = ARM_SPM_MM_SUPPORT_MAJOR_VERSION;
    RequestMinorVersion = ARM_SPM_MM_SUPPORT_MINOR_VERSION;
    CurrentMajorVersion =
      ((SvcArgs.Arg0 >> ARM_SPM_MM_MAJOR_VERSION_SHIFT) & ARM_SPM_MM_VERSION_MASK);
    CurrentMinorVersion =
      ((SvcArgs.Arg0 >> ARM_SPM_MM_MINOR_VERSION_SHIFT) & ARM_SPM_MM_VERSION_MASK);
  }

  // Different major revision values indicate possibly incompatible functions.
  // For two revisions, A and B, for which the major revision values are
  // identical, if the minor revision value of revision B is greater than
  // the minor revision value of revision A, then every function in
  // revision A must work in a compatible way with revision B.
  // However, it is possible for revision B to have a higher
  // function count than revision A
  if ((RequestMajorVersion != CurrentMajorVersion) ||
      (RequestMinorVersion > CurrentMinorVersion))
  {
    DEBUG ((
      DEBUG_INFO,
      "Incompatible %s Versions.\n" \
      "Request Version: Major=0x%x, Minor>=0x%x.\n" \
      "Current Version: Major=0x%x, Minor=0x%x.\n",
      (AbiProtocol == AbiProtocolFfa) ? L"FF-A" : L"SPM_MM",
      RequestMajorVersion,
      RequestMinorVersion,
      CurrentMajorVersion,
      CurrentMinorVersion
      ));
    AbiProtocol = AbiProtocolUnknown;
  } else {
    DEBUG ((
      DEBUG_INFO,
      "%s Version: Major=0x%x, Minor=0x%x\n",
      (AbiProtocol == AbiProtocolFfa) ? L"FF-A" : L"SPM_MM",
      CurrentMajorVersion,
      CurrentMinorVersion
      ));
  }

  return AbiProtocol;
}

/**
  Get Boot protocol.

  @param[in]      Arg0     Arg0 passed from firmware
  @param[in]      Arg1     Arg1 passed from firmware
  @param[in]      Arg2     Arg2 passed from firmware
  @param[in]      Arg3     Arg3 passed from firmware

  @retval         BootProtocolTl64            64 bits register convention transfer list
  @retval         BootProtocolTl32            32 bits register convention transfer list
  @retval         BootProtocolUnknown         Invalid Boot protocol

**/
STATIC
BOOT_PROTOCOL
EFIAPI
GetBootProtocol (
  IN UINTN  Arg0,
  IN UINTN  Arg1,
  IN UINTN  Arg2,
  IN UINTN  Arg3
  )
{
  UINTN  RegVersion;
  UINT64 Fields;

  Fields = Arg1;

  /*
   * The signature value in x1's [23:0] bits is the same regardless of
   * architecture when using Transfer list.
   * That's why it need to check signature value in x1 again with [31:0] bits
   * to discern 32 or 64 bits architecture after checking x1 value in [23:0].
   * Please see:
   *     https://github.com/FirmwareHandoff/firmware_handoff/blob/main/source/register_conventions.rst
   */
  if ((Fields & TRANSFER_LIST_SIGNATURE_MASK_32) ==
      (TRANSFER_LIST_SIGNATURE & TRANSFER_LIST_SIGNATURE_MASK_32))
  {
    if ((Fields & TRANSFER_LIST_SIGNATURE_MASK_64) ==
        (TRANSFER_LIST_SIGNATURE & TRANSFER_LIST_SIGNATURE_MASK_64))
    {
      RegVersion = (Fields >> REGISTER_CONVENTION_VERSION_SHIFT_64) &
                   REGISTER_CONVENTION_VERSION_MASK;

      if ((RegVersion != 1) || (Arg2 != 0x00) || (Arg3 == 0x00)) {
        goto err_out;
      }

      return BootProtocolTl64;
    } else {
      RegVersion = (Fields >> REGISTER_CONVENTION_VERSION_SHIFT_32) &
                   REGISTER_CONVENTION_VERSION_MASK;

      if ((RegVersion != 1) || (Arg0 != 0x00) || (Arg3 == 0x00)) {
        goto err_out;
      }

      return BootProtocolTl32;
    }
  }

err_out:
  DEBUG ((DEBUG_ERROR, "Error: Failed to get boot protocol!\n"));

  return BootProtocolUnknown;
}

/**
  Get PHIT hob information from firmware handoff transfer list protocol.

  @param[in]      TlhAddr     Transfer list header address

  @retval         NULL                    Failed to get PHIT hob
  @retval         Address                 PHIT hob address

**/
STATIC
VOID *
EFIAPI
GetPhitHobFromTransferList (
  IN UINTN  TlhAddr
  )
{
  TRANSFER_LIST_HEADER   *Tlh;
  TRANSFER_ENTRY_HEADER  *Te;
  VOID                   *HobStart;

  Tlh = (TRANSFER_LIST_HEADER *)TlhAddr;

  Te = TlFindFirstEntry (Tlh, TRANSFER_ENTRY_TAG_ID_HOB_LIST);
  if (Te == NULL) {
    DEBUG ((DEBUG_ERROR, "Error: No Phit hob is present in transfer list...\n"));

    return NULL;
  }

  HobStart = TlGetEntryData (Te);

  return HobStart;
}

/**
  Get logical Cpu Number.

  @param  [in] AbiProtocol            Abi Protocol.
  @param  [in] EventCompleteSvcArgs   Pointer to the event completion arguments.

  @retval         CpuNumber               Cpu Number
**/
STATIC
UINTN
EFIAPI
GetCpuNumber (
  IN ABI_PROTOCOL  AbiProtocol,
  IN ARM_SVC_ARGS  *EventCompleteSvcArgs
  )
{
  UINTN  Idx;

  if (AbiProtocol == AbiProtocolSpmMm) {
    Idx = EventCompleteSvcArgs->Arg3;
  } else {
    Idx = EventCompleteSvcArgs->Arg6;
  }

  ASSERT (Idx < mMpInfo->NumberOfProcessors);

  return Idx;
}

/**
  Dump mp information descriptor.

  @param[in]      ProcessorInfo       Mp information
  @param[in]      Idx                 Cpu index

**/
STATIC
VOID
EFIAPI
DumpMpInfoDescriptor (
  EFI_PROCESSOR_INFORMATION  *ProcessorInfo,
  UINTN                      Idx
  )
{
  if (ProcessorInfo == NULL) {
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "CPU[%d]: MpIdr - 0x%lx\n",
    Idx,
    ProcessorInfo->ProcessorId
    ));
  DEBUG ((
    DEBUG_INFO,
    "CPU[%d]: StatusFlag - 0x%lx\n",
    Idx,
    ProcessorInfo->StatusFlag
    ));
  DEBUG ((
    DEBUG_INFO,
    "CPU[%d]: Location[P:C:T] - :%d:%d:%d\n",
    Idx,
    ProcessorInfo->Location.Package,
    ProcessorInfo->Location.Core,
    ProcessorInfo->Location.Thread
    ));
}

/**
  Dump mmram descriptor.

  @param[in]      Name                Name
  @param[in]      MmramDesc           Mmram descriptor

**/
STATIC
VOID
EFIAPI
DumpMmramDescriptor (
  IN CHAR16                *Name,
  IN EFI_MMRAM_DESCRIPTOR  *MmramDesc
  )
{
  if (MmramDesc == NULL) {
    return;
  }

  if (Name == NULL) {
    Name = L"Unknown";
  }

  DEBUG ((
    DEBUG_INFO,
    "MmramDescriptor[%s]: PhysicalStart - 0x%lx\n",
    Name,
    MmramDesc->PhysicalStart
    ));
  DEBUG ((
    DEBUG_INFO,
    "MmramDescriptors[%s]: CpuStart - 0x%lx\n",
    Name,
    MmramDesc->CpuStart
    ));
  DEBUG ((
    DEBUG_INFO,
    "MmramDescriptors[%s]: PhysicalSize - %ld\n",
    Name,
    MmramDesc->PhysicalSize
    ));
  DEBUG ((
    DEBUG_INFO,
    "MmramDescriptors[%s]: RegionState - 0x%lx\n",
    Name,
    MmramDesc->RegionState
    ));
}

/**
  Dump PHIT hob information.

  @param[in]      HobStart            PHIT hob start address.

**/
STATIC
VOID
EFIAPI
DumpPhitHob (
  IN VOID  *HobStart
  )
{
  EFI_HOB_FIRMWARE_VOLUME         *FvHob;
  EFI_HOB_GUID_TYPE               *GuidHob;
  MP_INFORMATION_HOB_DATA         *MpInfo;
  EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *MmramRangesHobData;
  EFI_MMRAM_DESCRIPTOR            *MmramDesc;
  UINTN                           Idx;

  FvHob = GetNextHob (EFI_HOB_TYPE_FV, HobStart);
  if (FvHob == NULL) {
    DEBUG ((DEBUG_ERROR, "Error: No Firmware Volume Hob is present.\n"));
    return;
  }

  DEBUG ((DEBUG_INFO, "FvHob: BaseAddress - 0x%lx\n", FvHob->BaseAddress));
  DEBUG ((DEBUG_INFO, "FvHob: Length - %ld\n", FvHob->Length));

  GuidHob = GetNextGuidHob (&gMpInformationHobGuid, HobStart);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_ERROR, "Error: No MpInformation Guid Hob is present.\n"));
    return;
  }

  MpInfo = GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((DEBUG_INFO, "Number of Cpus - %d\n", MpInfo->NumberOfProcessors));
  DEBUG ((
    DEBUG_INFO,
    "Number of Enabled Cpus - %d\n",
    MpInfo->NumberOfEnabledProcessors
    ));
  for (Idx = 0; Idx < MpInfo->NumberOfProcessors; Idx++) {
    DumpMpInfoDescriptor (&MpInfo->ProcessorInfoBuffer[Idx], Idx);
  }

  GuidHob = GetNextGuidHob (&gEfiStandaloneMmNonSecureBufferGuid, HobStart);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_ERROR, "Error: No Ns Buffer Guid Hob is present.\n"));
    return;
  }

  DumpMmramDescriptor (L"NsBuffer", GET_GUID_HOB_DATA (GuidHob));

  GuidHob = GetNextGuidHob (&gEfiMmPeiMmramMemoryReserveGuid, HobStart);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_ERROR, "Error: No Pei Mmram Memory Reserved Guid Hob is present.\n"));
    return;
  }

  MmramRangesHobData = GET_GUID_HOB_DATA (GuidHob);
  if ((MmramRangesHobData == NULL) ||
      (MmramRangesHobData->NumberOfMmReservedRegions == 0))
  {
    DEBUG ((DEBUG_ERROR, "Error: No Pei Mmram Memory Reserved information is present.\n"));
    return;
  }

  for (Idx = 0; Idx < MmramRangesHobData->NumberOfMmReservedRegions; Idx++) {
    MmramDesc = &MmramRangesHobData->Descriptor[Idx];
    DumpMmramDescriptor (L"PeiMemReserved", MmramDesc);
  }
}

/**
  Convert EFI_STATUS to SPM_MM return code.

  @param [in] Status          edk2 status code.

  @retval ARM_SPM_MM_RET_*    return value correspond to EFI_STATUS.

**/
STATIC
UINTN
EFIAPI
EfiStatusToSpmMmStatus (
  IN EFI_STATUS  Status
  )
{
  switch (Status) {
    case EFI_SUCCESS:
      return ARM_SPM_MM_RET_SUCCESS;
    case EFI_INVALID_PARAMETER:
      return ARM_SPM_MM_RET_INVALID_PARAMS;
    case EFI_ACCESS_DENIED:
      return ARM_SPM_MM_RET_DENIED;
    case EFI_OUT_OF_RESOURCES:
      return ARM_SPM_MM_RET_NO_MEMORY;
    default:
      return ARM_SPM_MM_RET_NOT_SUPPORTED;
  }
}

/**
  Convert EFI_STATUS to FFA return code.

  @param [in] Status          edk2 status code.

  @retval ARM_FFA_SPM_RET_*   return value correspond to EFI_STATUS.

**/
STATIC
UINTN
EFIAPI
EfiStatusToFfaStatus (
  IN EFI_STATUS  Status
  )
{
  switch (Status) {
    case EFI_SUCCESS:
      return ARM_FFA_SPM_RET_SUCCESS;
    case EFI_INVALID_PARAMETER:
      return ARM_FFA_SPM_RET_INVALID_PARAMETERS;
    case EFI_OUT_OF_RESOURCES:
      return ARM_FFA_SPM_RET_NO_MEMORY;
    case EFI_ALREADY_STARTED:
      return ARM_FFA_SPM_RET_BUSY;
    case EFI_INTERRUPT_PENDING:
      return ARM_FFA_SPM_RET_INTERRUPTED;
    case EFI_ACCESS_DENIED:
      return ARM_FFA_SPM_RET_DENIED;
    case EFI_ABORTED:
      return ARM_FFA_SPM_RET_ABORTED;
    default:
      return ARM_FFA_SPM_RET_NOT_SUPPORTED;
  }
}

/**
  Set Event Complete arguments to be returned via SVC call.

  @param[in]      AbiProtocol               ABI Protocol.
  @param[in]      Status                    Result of StandaloneMm service.
  @param[out]     EventCompleteSvcArgs      Args structure.

**/
STATIC
VOID
SetEventCompleteSvcArgs (
  IN ABI_PROTOCOL   AbiProtocol,
  IN EFI_STATUS     Status,
  OUT ARM_SVC_ARGS  *EventCompleteSvcArgs
  )
{
  ZeroMem (EventCompleteSvcArgs, sizeof (ARM_SVC_ARGS));

  if (AbiProtocol == AbiProtocolFfa) {
    EventCompleteSvcArgs->Arg0 = ARM_SVC_ID_FFA_MSG_SEND_DIRECT_RESP;
    EventCompleteSvcArgs->Arg3 = ARM_FID_SPM_MM_SP_EVENT_COMPLETE;
    EventCompleteSvcArgs->Arg4 = EfiStatusToFfaStatus (Status);
  } else {
    EventCompleteSvcArgs->Arg0 = ARM_FID_SPM_MM_SP_EVENT_COMPLETE;
    EventCompleteSvcArgs->Arg1 = EfiStatusToSpmMmStatus (Status);
  }
}

/**
  A loop to delegated events.

  @param  [in] AbiProtocol            Abi Protocol.
  @param  [in] EventCompleteSvcArgs   Pointer to the event completion arguments.

**/
STATIC
VOID
EFIAPI
DelegatedEventLoop (
  IN ABI_PROTOCOL  AbiProtocol,
  IN ARM_SVC_ARGS  *EventCompleteSvcArgs
  )
{
  EFI_STATUS  Status;
  UINTN       CpuNumber;
  UINTN       CommBufferAddr;

  while (TRUE) {
    ArmCallSvc (EventCompleteSvcArgs);

    DEBUG ((DEBUG_INFO, "Received delegated event\n"));
    DEBUG ((DEBUG_INFO, "X0 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg0));
    DEBUG ((DEBUG_INFO, "X1 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg1));
    DEBUG ((DEBUG_INFO, "X2 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg2));
    DEBUG ((DEBUG_INFO, "X3 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg3));
    DEBUG ((DEBUG_INFO, "X4 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg4));
    DEBUG ((DEBUG_INFO, "X5 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg5));
    DEBUG ((DEBUG_INFO, "X6 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg6));
    DEBUG ((DEBUG_INFO, "X7 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg7));

    CpuNumber = GetCpuNumber (AbiProtocol, EventCompleteSvcArgs);
    DEBUG ((DEBUG_INFO, "CpuNumber: %d\n", CpuNumber));

    if (AbiProtocol == AbiProtocolFfa) {
      if (EventCompleteSvcArgs->Arg0 != ARM_SVC_ID_FFA_MSG_SEND_DIRECT_REQ) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "Error: Unrecognized FF-A Id: 0x%x\n",
          EventCompleteSvcArgs->Arg0
          ));
        goto event_complete;
      }

      CommBufferAddr = EventCompleteSvcArgs->Arg3;
    } else {
      /*
       * Register Convention for SPM_MM
       *   Arg0: ARM_SMC_ID_MM_COMMUNICATE
       *   Arg1: Communication Buffer
       *   Arg2: Size of Communication Buffer
       *   Arg3: Cpu number where StandaloneMm running on.
       *
       *   See tf-a/services/std_svc/spm/spm_mm/spm_mm_main.c
       */
      if (EventCompleteSvcArgs->Arg0 != ARM_SMC_ID_MM_COMMUNICATE) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "Error: Unrecognized SPM_MM Id: 0x%x\n",
          EventCompleteSvcArgs->Arg0
          ));
        goto event_complete;
      }

      CommBufferAddr = EventCompleteSvcArgs->Arg1;
    }

    Status = CpuDriverEntryPoint (EventCompleteSvcArgs->Arg0, CpuNumber, CommBufferAddr);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "Error: Failed delegated event 0x%x, Status 0x%x\n",
        CommBufferAddr,
        Status
        ));
    }

event_complete:
    SetEventCompleteSvcArgs (
      AbiProtocol,
      Status,
      EventCompleteSvcArgs
      );
  }
}

/**
  The entry point of Standalone MM Foundation.

  @param  [in]  Arg0        Boot information passed according to boot protocol.
  @param  [in]  Arg1        Boot information passed according to boot protocol.
  @param  [in]  Arg2        Boot information passed according to boot protocol.
  @param  [in]  Arg3        Boot information passed according to boot protocol.

**/
VOID
EFIAPI
_ModuleEntryPoint (
  IN UINTN  Arg0,
  IN UINTN  Arg1,
  IN UINTN  Arg2,
  IN UINTN  Arg3
  )
{
  PE_COFF_LOADER_IMAGE_CONTEXT        ImageContext;
  ARM_SVC_ARGS                        EventCompleteSvcArgs;
  EFI_STATUS                          Status;
  UINT32                              SectionHeaderOffset;
  UINT16                              NumberOfSections;
  BOOT_PROTOCOL                       BootProtocol;
  ABI_PROTOCOL                        AbiProtocol;
  VOID                                *HobStart;
  VOID                                *TeData;
  UINTN                               TeDataSize;
  EFI_PHYSICAL_ADDRESS                ImageBase;
  EDKII_PI_MM_CPU_DRIVER_EP_PROTOCOL  *PiMmCpuDriverEpProtocol;
  EFI_HOB_FIRMWARE_VOLUME             *FvHob;
  EFI_HOB_GUID_TYPE                   *GuidHob;
  EFI_CONFIGURATION_TABLE             *ConfigurationTable;
  UINTN                               Idx;

  AbiProtocol = GetAbiProtocol ();
  if (AbiProtocol == AbiProtocolUnknown) {
    Status = EFI_UNSUPPORTED;
    goto finish;
  }

  /**
   * Check boot information
   */
  BootProtocol = GetBootProtocol (Arg0, Arg1, Arg2, Arg3);
  if (BootProtocol == BootProtocolUnknown) {
    Status = EFI_UNSUPPORTED;
    goto finish;
  }

  HobStart = GetPhitHobFromTransferList (Arg3);
  if (HobStart == NULL) {
    Status = EFI_UNSUPPORTED;
    goto finish;
  }

  DEBUG ((DEBUG_INFO, "Start Dump Hob: %lx\n", (unsigned long)HobStart));
  DumpPhitHob (HobStart);
  DEBUG ((DEBUG_INFO, "End Dump Hob: %lx\n", (unsigned long)HobStart));

  FvHob = GetNextHob (EFI_HOB_TYPE_FV, HobStart);
  if (FvHob == NULL) {
    DEBUG ((DEBUG_ERROR, "Error: No Firmware Volume Hob is present.\n"));
    Status = EFI_INVALID_PARAMETER;

    goto finish;
  }

  // Locate PE/COFF File information for the Standalone MM core module
  Status = LocateStandaloneMmCorePeCoffData (
             (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FvHob->BaseAddress,
             &TeData,
             &TeDataSize
             );

  if (EFI_ERROR (Status)) {
    goto finish;
  }

  // Obtain the PE/COFF Section information for the Standalone MM core module
  Status = GetStandaloneMmCorePeCoffSections (
             TeData,
             &ImageContext,
             &ImageBase,
             &SectionHeaderOffset,
             &NumberOfSections
             );

  if (EFI_ERROR (Status)) {
    goto finish;
  }

  //
  // ImageBase may deviate from ImageContext.ImageAddress if we are dealing
  // with a TE image, in which case the latter points to the actual offset
  // of the image, whereas ImageBase refers to the address where the image
  // would start if the stripped PE headers were still in place. In either
  // case, we need to fix up ImageBase so it refers to the actual current
  // load address.
  //
  ImageBase += (UINTN)TeData - ImageContext.ImageAddress;

  // Update the memory access permissions of individual sections in the
  // Standalone MM core module
  Status = UpdateMmFoundationPeCoffPermissions (
             &ImageContext,
             ImageBase,
             SectionHeaderOffset,
             NumberOfSections,
             ArmSetMemoryRegionNoExec,
             ArmSetMemoryRegionReadOnly,
             ArmClearMemoryRegionReadOnly
             );
  if (EFI_ERROR (Status)) {
    goto finish;
  }

  if (ImageContext.ImageAddress != (UINTN)TeData) {
    ImageContext.ImageAddress = (UINTN)TeData;
    ArmSetMemoryRegionNoExec (ImageBase, SIZE_4KB);
    ArmClearMemoryRegionReadOnly (ImageBase, SIZE_4KB);

    Status = PeCoffLoaderRelocateImage (&ImageContext);
    ASSERT_EFI_ERROR (Status);
  }

  gHobList = HobStart;

  //
  // Call the MM Core entry point
  //
  ProcessModuleEntryPointList (HobStart);

  //
  // Find HobList to check gEfiHobList is installed.
  //
  Status             = EFI_NOT_FOUND;
  ConfigurationTable = gMmCoreMmst.MmConfigurationTable;
  for (Idx = 0; Idx < gMmCoreMmst.NumberOfTableEntries; Idx++) {
    if (CompareGuid (&gEfiHobListGuid, &ConfigurationTable[Idx].VendorGuid)) {
      Status = EFI_SUCCESS;
      break;
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error: Hoblist not found in MmConfigurationTable\n"));
    goto finish;
  }

  //
  // Find MpInformation Hob in HobList.
  // It couldn't save address of mp information in gHobList
  // because that memory area will be reused after StandaloneMm finishing
  // initialization.
  //
  HobStart = ConfigurationTable[Idx].VendorTable;
  GuidHob  = GetNextGuidHob (&gMpInformationHobGuid, HobStart);
  if (GuidHob == NULL) {
    Status = EFI_NOT_FOUND;
    DEBUG ((DEBUG_ERROR, "Error: No MpInformation hob ...\n"));
    goto finish;
  }

  mMpInfo = GET_GUID_HOB_DATA (GuidHob);

  //
  // Find out cpu driver entry point used in DelegatedEventLoop
  // to handle MMI request.
  //
  Status = gMmCoreMmst.MmLocateProtocol (
                         &gEdkiiPiMmCpuDriverEpProtocolGuid,
                         NULL,
                         (VOID **)&PiMmCpuDriverEpProtocol
                         );
  if (EFI_ERROR (Status)) {
    goto finish;
  }

  CpuDriverEntryPoint = PiMmCpuDriverEpProtocol->PiMmCpuDriverEntryPoint;

  DEBUG ((
    DEBUG_INFO,
    "Shared Cpu Driver EP %p\n",
    CpuDriverEntryPoint
    ));

finish:
  SetEventCompleteSvcArgs (AbiProtocol, Status, &EventCompleteSvcArgs);
  DelegatedEventLoop (AbiProtocol, &EventCompleteSvcArgs);
}
