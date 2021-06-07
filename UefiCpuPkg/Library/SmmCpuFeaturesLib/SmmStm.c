/** @file
  SMM STM support functions

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/TpmMeasurementLib.h>
#include <Register/Intel/Cpuid.h>
#include <Register/Intel/ArchitecturalMsr.h>
#include <Register/Intel/SmramSaveStateMap.h>

#include <Protocol/MpService.h>

#include "CpuFeaturesLib.h"
#include "SmmStm.h"

#define TXT_EVTYPE_BASE                  0x400
#define TXT_EVTYPE_STM_HASH              (TXT_EVTYPE_BASE + 14)

#define RDWR_ACCS             3
#define FULL_ACCS             7

EFI_HANDLE  mStmSmmCpuHandle = NULL;

BOOLEAN mLockLoadMonitor = FALSE;

//
// Template of STM_RSC_END structure for copying.
//
GLOBAL_REMOVE_IF_UNREFERENCED STM_RSC_END mRscEndNode = {
  {END_OF_RESOURCES, sizeof (STM_RSC_END)},
};

GLOBAL_REMOVE_IF_UNREFERENCED UINT8  *mStmResourcesPtr         = NULL;
GLOBAL_REMOVE_IF_UNREFERENCED UINTN  mStmResourceTotalSize     = 0x0;
GLOBAL_REMOVE_IF_UNREFERENCED UINTN  mStmResourceSizeUsed      = 0x0;
GLOBAL_REMOVE_IF_UNREFERENCED UINTN  mStmResourceSizeAvailable = 0x0;

GLOBAL_REMOVE_IF_UNREFERENCED UINT32  mStmState = 0;

//
// System Configuration Table pointing to STM Configuration Table
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_SM_MONITOR_INIT_PROTOCOL mSmMonitorInitProtocol = {
  LoadMonitor,
  AddPiResource,
  DeletePiResource,
  GetPiResource,
  GetMonitorState,
};




#define   CPUID1_EDX_XD_SUPPORT      0x100000

//
// External global variables associated with SMI Handler Template
//
extern CONST TXT_PROCESSOR_SMM_DESCRIPTOR  gcStmPsd;
extern UINT32                              gStmSmbase;
extern volatile UINT32                     gStmSmiStack;
extern UINT32                              gStmSmiCr3;
extern volatile UINT8                      gcStmSmiHandlerTemplate[];
extern CONST UINT16                        gcStmSmiHandlerSize;
extern UINT16                              gcStmSmiHandlerOffset;
extern BOOLEAN                             gStmXdSupported;

//
// Variables used by SMI Handler
//
IA32_DESCRIPTOR  gStmSmiHandlerIdtr;

//
// MP Services Protocol
//
EFI_MP_SERVICES_PROTOCOL  *mSmmCpuFeaturesLibMpService = NULL;

//
// MSEG Base and Length in SMRAM
//
UINTN  mMsegBase = 0;
UINTN  mMsegSize = 0;

BOOLEAN  mStmConfigurationTableInitialized = FALSE;

/**
  The constructor function for the Traditional MM library instance with STM.

  @param[in]  ImageHandle  The firmware allocated handle for the EFI image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS      The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmCpuFeaturesLibStmConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS              Status;
  CPUID_VERSION_INFO_ECX  RegEcx;
  EFI_HOB_GUID_TYPE       *GuidHob;
  EFI_SMRAM_DESCRIPTOR    *SmramDescriptor;

  //
  // Initialize address fixup
  //
  SmmCpuFeaturesLibStmSmiEntryFixupAddress ();

  //
  // Perform library initialization common across all instances
  //
  CpuFeaturesLibInitialization ();

  //
  // Lookup the MP Services Protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  (VOID **)&mSmmCpuFeaturesLibMpService
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // If CPU supports VMX, then determine SMRAM range for MSEG.
  //
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, &RegEcx.Uint32, NULL);
  if (RegEcx.Bits.VMX == 1) {
    GuidHob = GetFirstGuidHob (&gMsegSmramGuid);
    if (GuidHob != NULL) {
      //
      // Retrieve MSEG location from MSEG SRAM HOB
      //
      SmramDescriptor = (EFI_SMRAM_DESCRIPTOR *) GET_GUID_HOB_DATA (GuidHob);
      if (SmramDescriptor->PhysicalSize > 0) {
        mMsegBase       = (UINTN)SmramDescriptor->CpuStart;
        mMsegSize       = (UINTN)SmramDescriptor->PhysicalSize;
      }
    } else if (PcdGet32 (PcdCpuMsegSize) > 0) {
      //
      // Allocate MSEG from SMRAM memory
      //
      mMsegBase = (UINTN)AllocatePages (EFI_SIZE_TO_PAGES (PcdGet32 (PcdCpuMsegSize)));
      if (mMsegBase > 0) {
        mMsegSize = ALIGN_VALUE (PcdGet32 (PcdCpuMsegSize), EFI_PAGE_SIZE);
      } else {
        DEBUG ((DEBUG_ERROR, "Not enough SMRAM resource to allocate MSEG size %08x\n", PcdGet32 (PcdCpuMsegSize)));
      }
    }
    if (mMsegBase > 0) {
      DEBUG ((DEBUG_INFO, "MsegBase: 0x%08x, MsegSize: 0x%08x\n", mMsegBase, mMsegSize));
    }
  }

  return EFI_SUCCESS;
}

/**
  Internal worker function that is called to complete CPU initialization at the
  end of SmmCpuFeaturesInitializeProcessor().

**/
VOID
FinishSmmCpuFeaturesInitializeProcessor (
  VOID
  )
{
  MSR_IA32_SMM_MONITOR_CTL_REGISTER  SmmMonitorCtl;

  //
  // Set MSEG Base Address in SMM Monitor Control MSR.
  //
  if (mMsegBase > 0) {
    SmmMonitorCtl.Uint64        = 0;
    SmmMonitorCtl.Bits.MsegBase = (UINT32)mMsegBase >> 12;
    SmmMonitorCtl.Bits.Valid    = 1;
    AsmWriteMsr64 (MSR_IA32_SMM_MONITOR_CTL, SmmMonitorCtl.Uint64);
  }
}

/**
  Return the size, in bytes, of a custom SMI Handler in bytes.  If 0 is
  returned, then a custom SMI handler is not provided by this library,
  and the default SMI handler must be used.

  @retval 0    Use the default SMI handler.
  @retval > 0  Use the SMI handler installed by SmmCpuFeaturesInstallSmiHandler()
               The caller is required to allocate enough SMRAM for each CPU to
               support the size of the custom SMI handler.
**/
UINTN
EFIAPI
SmmCpuFeaturesGetSmiHandlerSize (
  VOID
  )
{
  return gcStmSmiHandlerSize;
}

/**
  Install a custom SMI handler for the CPU specified by CpuIndex.  This function
  is only called if SmmCpuFeaturesGetSmiHandlerSize() returns a size is greater
  than zero and is called by the CPU that was elected as monarch during System
  Management Mode initialization.

  @param[in] CpuIndex   The index of the CPU to install the custom SMI handler.
                        The value must be between 0 and the NumberOfCpus field
                        in the System Management System Table (SMST).
  @param[in] SmBase     The SMBASE address for the CPU specified by CpuIndex.
  @param[in] SmiStack   The stack to use when an SMI is processed by the
                        the CPU specified by CpuIndex.
  @param[in] StackSize  The size, in bytes, if the stack used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] GdtBase    The base address of the GDT to use when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] GdtSize    The size, in bytes, of the GDT used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] IdtBase    The base address of the IDT to use when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] IdtSize    The size, in bytes, of the IDT used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] Cr3        The base address of the page tables to use when an SMI
                        is processed by the CPU specified by CpuIndex.
**/
VOID
EFIAPI
SmmCpuFeaturesInstallSmiHandler (
  IN UINTN   CpuIndex,
  IN UINT32  SmBase,
  IN VOID    *SmiStack,
  IN UINTN   StackSize,
  IN UINTN   GdtBase,
  IN UINTN   GdtSize,
  IN UINTN   IdtBase,
  IN UINTN   IdtSize,
  IN UINT32  Cr3
  )
{
  EFI_STATUS                     Status;
  TXT_PROCESSOR_SMM_DESCRIPTOR   *Psd;
  VOID                           *Hob;
  UINT32                         RegEax;
  UINT32                         RegEdx;
  EFI_PROCESSOR_INFORMATION      ProcessorInfo;

  CopyMem ((VOID *)((UINTN)SmBase + TXT_SMM_PSD_OFFSET), &gcStmPsd, sizeof (gcStmPsd));
  Psd = (TXT_PROCESSOR_SMM_DESCRIPTOR *)(VOID *)((UINTN)SmBase + TXT_SMM_PSD_OFFSET);
  Psd->SmmGdtPtr = GdtBase;
  Psd->SmmGdtSize = (UINT32)GdtSize;

  //
  // Initialize values in template before copy
  //
  gStmSmiStack             = (UINT32)((UINTN)SmiStack + StackSize - sizeof (UINTN));
  gStmSmiCr3               = Cr3;
  gStmSmbase               = SmBase;
  gStmSmiHandlerIdtr.Base  = IdtBase;
  gStmSmiHandlerIdtr.Limit = (UINT16)(IdtSize - 1);

  if (gStmXdSupported) {
    AsmCpuid (CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
    if (RegEax <= CPUID_EXTENDED_FUNCTION) {
      //
      // Extended CPUID functions are not supported on this processor.
      //
      gStmXdSupported = FALSE;
    }

    AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, NULL, &RegEdx);
    if ((RegEdx & CPUID1_EDX_XD_SUPPORT) == 0) {
      //
      // Execute Disable Bit feature is not supported on this processor.
      //
      gStmXdSupported = FALSE;
    }
  }

  //
  // Set the value at the top of the CPU stack to the CPU Index
  //
  *(UINTN*)(UINTN)gStmSmiStack = CpuIndex;

  //
  // Copy template to CPU specific SMI handler location
  //
  CopyMem (
    (VOID*)((UINTN)SmBase + SMM_HANDLER_OFFSET),
    (VOID*)gcStmSmiHandlerTemplate,
    gcStmSmiHandlerSize
    );

  Psd->SmmSmiHandlerRip = SmBase + SMM_HANDLER_OFFSET + gcStmSmiHandlerOffset;
  Psd->SmmSmiHandlerRsp = (UINTN)SmiStack + StackSize - sizeof(UINTN);
  Psd->SmmCr3           = Cr3;

  DEBUG((DEBUG_INFO, "CpuSmmStmExceptionStackSize - %x\n", PcdGet32(PcdCpuSmmStmExceptionStackSize)));
  DEBUG((DEBUG_INFO, "Pages - %x\n", EFI_SIZE_TO_PAGES(PcdGet32(PcdCpuSmmStmExceptionStackSize))));
  Psd->StmProtectionExceptionHandler.SpeRsp = (UINT64)(UINTN)AllocatePages (EFI_SIZE_TO_PAGES (PcdGet32 (PcdCpuSmmStmExceptionStackSize)));
  Psd->StmProtectionExceptionHandler.SpeRsp += EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (PcdGet32 (PcdCpuSmmStmExceptionStackSize)));

  Psd->BiosHwResourceRequirementsPtr        = (UINT64)(UINTN)GetStmResource ();

  //
  // Get the APIC ID for the CPU specified by CpuIndex
  //
  Status = mSmmCpuFeaturesLibMpService->GetProcessorInfo (
             mSmmCpuFeaturesLibMpService,
             CpuIndex,
             &ProcessorInfo
             );
  ASSERT_EFI_ERROR (Status);

  Psd->LocalApicId = (UINT32)ProcessorInfo.ProcessorId;
  Psd->AcpiRsdp = 0;

  Hob = GetFirstHob (EFI_HOB_TYPE_CPU);
  if (Hob != NULL) {
    Psd->PhysicalAddressBits = ((EFI_HOB_CPU *) Hob)->SizeOfMemorySpace;
  } else {
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000008) {
      AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
      Psd->PhysicalAddressBits = (UINT8) RegEax;
    } else {
      Psd->PhysicalAddressBits = 36;
    }
  }

  if (!mStmConfigurationTableInitialized) {
    StmSmmConfigurationTableInit ();
    mStmConfigurationTableInitialized = TRUE;
  }
}

/**
  SMM End Of Dxe event notification handler.

  STM support need patch AcpiRsdp in TXT_PROCESSOR_SMM_DESCRIPTOR.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS   Notification handler runs successfully.
**/
EFI_STATUS
EFIAPI
SmmEndOfDxeEventNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  VOID                          *Rsdp;
  UINTN                         Index;
  TXT_PROCESSOR_SMM_DESCRIPTOR  *Psd;

  DEBUG ((DEBUG_INFO, "SmmEndOfDxeEventNotify\n"));

  //
  // found ACPI table RSD_PTR from system table
  //
  Rsdp = NULL;
  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpi20TableGuid)) {
      //
      // A match was found.
      //
      Rsdp = gST->ConfigurationTable[Index].VendorTable;
      break;
    }
  }
  if (Rsdp == NULL) {
    for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
      if (CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpi10TableGuid)) {
        //
        // A match was found.
        //
        Rsdp = gST->ConfigurationTable[Index].VendorTable;
        break;
      }
    }
  }

  for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
    Psd = (TXT_PROCESSOR_SMM_DESCRIPTOR *)((UINTN)gSmst->CpuSaveState[Index] - SMRAM_SAVE_STATE_MAP_OFFSET + TXT_SMM_PSD_OFFSET);
    DEBUG ((DEBUG_INFO, "Index=%d  Psd=%p  Rsdp=%p\n", Index, Psd, Rsdp));
    Psd->AcpiRsdp = (UINT64)(UINTN)Rsdp;
  }

  mLockLoadMonitor = TRUE;

  return EFI_SUCCESS;
}

/**
  This function initializes the STM configuration table.
**/
VOID
StmSmmConfigurationTableInit (
  VOID
  )
{
  EFI_STATUS    Status;
    VOID        *Registration;

  Status = gSmst->SmmInstallProtocolInterface (
                    &mStmSmmCpuHandle,
                    &gEfiSmMonitorInitProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mSmMonitorInitProtocol
                    );
  ASSERT_EFI_ERROR (Status);

  //
  //
  // Register SMM End of DXE Event
  //
  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiSmmEndOfDxeProtocolGuid,
                    SmmEndOfDxeEventNotify,
                    &Registration
                    );
  ASSERT_EFI_ERROR (Status);
}

/**

  Get STM state.

  @return STM state

**/
EFI_SM_MONITOR_STATE
EFIAPI
GetMonitorState (
  VOID
  )
{
  return mStmState;
}

/**

  Handle single Resource to see if it can be merged into Record.

  @param Resource  A pointer to resource node to be added
  @param Record    A pointer to record node to be merged

  @retval TRUE  resource handled
  @retval FALSE resource is not handled

**/
BOOLEAN
HandleSingleResource (
  IN  STM_RSC      *Resource,
  IN  STM_RSC      *Record
  )
{
  UINT64      ResourceLo;
  UINT64      ResourceHi;
  UINT64      RecordLo;
  UINT64      RecordHi;

  ResourceLo = 0;
  ResourceHi = 0;
  RecordLo = 0;
  RecordHi = 0;

  //
  // Calling code is responsible for making sure that
  // Resource->Header.RscType == (*Record)->Header.RscType
  // thus we use just one of them as switch variable.
  //
  switch (Resource->Header.RscType) {
  case MEM_RANGE:
  case MMIO_RANGE:
    ResourceLo = Resource->Mem.Base;
    ResourceHi = Resource->Mem.Base + Resource->Mem.Length;
    RecordLo = Record->Mem.Base;
    RecordHi = Record->Mem.Base + Record->Mem.Length;
    if (Resource->Mem.RWXAttributes != Record->Mem.RWXAttributes) {
      if ((ResourceLo == RecordLo) && (ResourceHi == RecordHi)) {
        Record->Mem.RWXAttributes = Resource->Mem.RWXAttributes | Record->Mem.RWXAttributes;
        return TRUE;
      } else {
        return FALSE;
      }
    }
    break;
  case IO_RANGE:
  case TRAPPED_IO_RANGE:
    ResourceLo = (UINT64) Resource->Io.Base;
    ResourceHi = (UINT64) Resource->Io.Base + (UINT64) Resource->Io.Length;
    RecordLo = (UINT64) Record->Io.Base;
    RecordHi = (UINT64) Record->Io.Base + (UINT64) Record->Io.Length;
    break;
  case PCI_CFG_RANGE:
    if ((Resource->PciCfg.OriginatingBusNumber != Record->PciCfg.OriginatingBusNumber) ||
        (Resource->PciCfg.LastNodeIndex != Record->PciCfg.LastNodeIndex)) {
      return FALSE;
    }
    if (CompareMem (Resource->PciCfg.PciDevicePath, Record->PciCfg.PciDevicePath, sizeof(STM_PCI_DEVICE_PATH_NODE) * (Resource->PciCfg.LastNodeIndex + 1)) != 0) {
      return FALSE;
    }
    ResourceLo = (UINT64) Resource->PciCfg.Base;
    ResourceHi = (UINT64) Resource->PciCfg.Base + (UINT64) Resource->PciCfg.Length;
    RecordLo = (UINT64) Record->PciCfg.Base;
    RecordHi = (UINT64) Record->PciCfg.Base + (UINT64) Record->PciCfg.Length;
    if (Resource->PciCfg.RWAttributes != Record->PciCfg.RWAttributes) {
      if ((ResourceLo == RecordLo) && (ResourceHi == RecordHi)) {
        Record->PciCfg.RWAttributes = Resource->PciCfg.RWAttributes | Record->PciCfg.RWAttributes;
        return TRUE;
      } else {
        return FALSE;
      }
    }
    break;
  case MACHINE_SPECIFIC_REG:
    //
    // Special case - merge MSR masks in place.
    //
    if (Resource->Msr.MsrIndex != Record->Msr.MsrIndex) {
      return FALSE;
    }
    Record->Msr.ReadMask |= Resource->Msr.ReadMask;
    Record->Msr.WriteMask |= Resource->Msr.WriteMask;
    return TRUE;
  default:
    return FALSE;
  }
  //
  // If resources are disjoint
  //
  if ((ResourceHi < RecordLo) || (ResourceLo > RecordHi)) {
    return FALSE;
  }

  //
  // If resource is consumed by record.
  //
  if ((ResourceLo >= RecordLo) && (ResourceHi <= RecordHi)) {
    return TRUE;
  }
  //
  // Resources are overlapping.
  // Resource and record are merged.
  //
  ResourceLo = (ResourceLo < RecordLo) ? ResourceLo : RecordLo;
  ResourceHi = (ResourceHi > RecordHi) ? ResourceHi : RecordHi;

  switch (Resource->Header.RscType) {
  case MEM_RANGE:
  case MMIO_RANGE:
    Record->Mem.Base = ResourceLo;
    Record->Mem.Length = ResourceHi - ResourceLo;
    break;
  case IO_RANGE:
  case TRAPPED_IO_RANGE:
    Record->Io.Base = (UINT16) ResourceLo;
    Record->Io.Length = (UINT16) (ResourceHi - ResourceLo);
    break;
  case PCI_CFG_RANGE:
    Record->PciCfg.Base = (UINT16) ResourceLo;
    Record->PciCfg.Length = (UINT16) (ResourceHi - ResourceLo);
    break;
  default:
    return FALSE;
  }

  return TRUE;
}

/**

  Add resource node.

  @param Resource  A pointer to resource node to be added

**/
VOID
AddSingleResource (
  IN  STM_RSC    *Resource
  )
{
  STM_RSC    *Record;

  Record = (STM_RSC *)mStmResourcesPtr;

  while (TRUE) {
    if (Record->Header.RscType == END_OF_RESOURCES) {
      break;
    }
    //
    // Go to next record if resource and record types don't match.
    //
    if (Resource->Header.RscType != Record->Header.RscType) {
      Record = (STM_RSC *)((UINTN)Record + Record->Header.Length);
      continue;
    }
    //
    // Record is handled inside of procedure - don't adjust.
    //
    if (HandleSingleResource (Resource, Record)) {
      return ;
    }
    Record = (STM_RSC *)((UINTN)Record + Record->Header.Length);
  }

  //
  // Add resource to the end of area.
  //
  CopyMem (
    mStmResourcesPtr + mStmResourceSizeUsed - sizeof(mRscEndNode),
    Resource,
    Resource->Header.Length
    );
  CopyMem (
    mStmResourcesPtr + mStmResourceSizeUsed - sizeof(mRscEndNode) + Resource->Header.Length,
    &mRscEndNode,
    sizeof(mRscEndNode)
    );
  mStmResourceSizeUsed += Resource->Header.Length;
  mStmResourceSizeAvailable = mStmResourceTotalSize - mStmResourceSizeUsed;

  return ;
}

/**

  Add resource list.

  @param ResourceList  A pointer to resource list to be added
  @param NumEntries    Optional number of entries.
                       If 0, list must be terminated by END_OF_RESOURCES.

**/
VOID
AddResource (
  IN  STM_RSC    *ResourceList,
  IN  UINT32      NumEntries OPTIONAL
  )
{
  UINT32      Count;
  UINTN       Index;
  STM_RSC    *Resource;

  if (NumEntries == 0) {
    Count = 0xFFFFFFFF;
  } else {
    Count = NumEntries;
  }

  Resource = ResourceList;

  for (Index = 0; Index < Count; Index++) {
    if (Resource->Header.RscType == END_OF_RESOURCES) {
      return ;
    }
    AddSingleResource (Resource);
    Resource = (STM_RSC *)((UINTN)Resource + Resource->Header.Length);
  }
  return ;
}

/**

  Validate resource list.

  @param ResourceList  A pointer to resource list to be added
  @param NumEntries    Optional number of entries.
                       If 0, list must be terminated by END_OF_RESOURCES.

  @retval TRUE  resource valid
  @retval FALSE resource invalid

**/
BOOLEAN
ValidateResource (
  IN  STM_RSC    *ResourceList,
  IN  UINT32      NumEntries OPTIONAL
  )
{
  UINT32      Count;
  UINTN       Index;
  STM_RSC    *Resource;
  UINTN       SubIndex;

  //
  // If NumEntries == 0 make it very big. Scan will be terminated by
  // END_OF_RESOURCES.
  //
  if (NumEntries == 0) {
    Count = 0xFFFFFFFF;
  } else {
    Count = NumEntries;
  }

  //
  // Start from beginning of resource list.
  //
  Resource = ResourceList;

  for (Index = 0; Index < Count; Index++) {
    DEBUG ((DEBUG_INFO, "ValidateResource (%d) - RscType(%x)\n", Index, Resource->Header.RscType));
    //
    // Validate resource.
    //
    switch (Resource->Header.RscType) {
      case END_OF_RESOURCES:
        if (Resource->Header.Length != sizeof (STM_RSC_END)) {
          return  FALSE;
        }
        //
        // If we are passed actual number of resources to add,
        // END_OF_RESOURCES structure between them is considered an
        // error. If NumEntries == 0 END_OF_RESOURCES is a termination.
        //
        if (NumEntries != 0) {
          return  FALSE;
        } else {
          //
          // If NumEntries == 0 and list reached end - return success.
          //
          return TRUE;
        }
        break;

      case MEM_RANGE:
      case MMIO_RANGE:
        if (Resource->Header.Length != sizeof (STM_RSC_MEM_DESC)) {
          return FALSE;
        }

        if (Resource->Mem.RWXAttributes > FULL_ACCS) {
          return FALSE;
        }
        break;

      case IO_RANGE:
      case TRAPPED_IO_RANGE:
        if (Resource->Header.Length != sizeof (STM_RSC_IO_DESC)) {
          return FALSE;
        }

        if ((Resource->Io.Base + Resource->Io.Length) > 0xFFFF) {
          return FALSE;
        }
        break;

      case PCI_CFG_RANGE:
        DEBUG ((DEBUG_INFO, "ValidateResource - PCI (0x%02x, 0x%08x, 0x%02x, 0x%02x)\n", Resource->PciCfg.OriginatingBusNumber, Resource->PciCfg.LastNodeIndex, Resource->PciCfg.PciDevicePath[0].PciDevice, Resource->PciCfg.PciDevicePath[0].PciFunction));
        if (Resource->Header.Length != sizeof (STM_RSC_PCI_CFG_DESC) + (sizeof(STM_PCI_DEVICE_PATH_NODE) * Resource->PciCfg.LastNodeIndex)) {
          return FALSE;
        }
        for (SubIndex = 0; SubIndex <= Resource->PciCfg.LastNodeIndex; SubIndex++) {
          if ((Resource->PciCfg.PciDevicePath[SubIndex].PciDevice > 0x1F) || (Resource->PciCfg.PciDevicePath[SubIndex].PciFunction > 7)) {
            return FALSE;
          }
        }
        if ((Resource->PciCfg.Base + Resource->PciCfg.Length) > 0x1000) {
          return FALSE;
        }
        break;

      case MACHINE_SPECIFIC_REG:
        if (Resource->Header.Length != sizeof (STM_RSC_MSR_DESC)) {
          return FALSE;
        }
        break;

      default :
        DEBUG ((DEBUG_ERROR, "ValidateResource - Unknown RscType(%x)\n", Resource->Header.RscType));
        return FALSE;
    }
    Resource = (STM_RSC *)((UINTN)Resource + Resource->Header.Length);
  }
  return TRUE;
}

/**

  Get resource list.
  EndResource is excluded.

  @param ResourceList  A pointer to resource list to be added
  @param NumEntries    Optional number of entries.
                       If 0, list must be terminated by END_OF_RESOURCES.

  @retval TRUE  resource valid
  @retval FALSE resource invalid

**/
UINTN
GetResourceSize (
  IN  STM_RSC    *ResourceList,
  IN  UINT32      NumEntries OPTIONAL
  )
{
  UINT32      Count;
  UINTN       Index;
  STM_RSC    *Resource;

  Resource = ResourceList;

  //
  // If NumEntries == 0 make it very big. Scan will be terminated by
  // END_OF_RESOURCES.
  //
  if (NumEntries == 0) {
    Count = 0xFFFFFFFF;
  } else {
    Count = NumEntries;
  }

  //
  // Start from beginning of resource list.
  //
  Resource = ResourceList;

  for (Index = 0; Index < Count; Index++) {
    if (Resource->Header.RscType == END_OF_RESOURCES) {
      break;
    }
    Resource = (STM_RSC *)((UINTN)Resource + Resource->Header.Length);
  }

  return (UINTN)Resource - (UINTN)ResourceList;
}

/**

  Add resources in list to database. Allocate new memory areas as needed.

  @param ResourceList  A pointer to resource list to be added
  @param NumEntries    Optional number of entries.
                       If 0, list must be terminated by END_OF_RESOURCES.

  @retval EFI_SUCCESS            If resources are added
  @retval EFI_INVALID_PARAMETER  If nested procedure detected resource failer
  @retval EFI_OUT_OF_RESOURCES   If nested procedure returned it and we cannot allocate more areas.

**/
EFI_STATUS
EFIAPI
AddPiResource (
  IN  STM_RSC    *ResourceList,
  IN  UINT32      NumEntries OPTIONAL
  )
{
  EFI_STATUS            Status;
  UINTN                 ResourceSize;
  EFI_PHYSICAL_ADDRESS  NewResource;
  UINTN                 NewResourceSize;

  DEBUG ((DEBUG_INFO, "AddPiResource - Enter\n"));

  if (!ValidateResource (ResourceList, NumEntries)) {
    return EFI_INVALID_PARAMETER;
  }

  ResourceSize = GetResourceSize (ResourceList, NumEntries);
  DEBUG ((DEBUG_INFO, "ResourceSize - 0x%08x\n", ResourceSize));
  if (ResourceSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (mStmResourcesPtr == NULL) {
    //
    // First time allocation
    //
    NewResourceSize = EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (ResourceSize + sizeof(mRscEndNode)));
    DEBUG ((DEBUG_INFO, "Allocate - 0x%08x\n", NewResourceSize));
    Status = gSmst->SmmAllocatePages (
                      AllocateAnyPages,
                      EfiRuntimeServicesData,
                      EFI_SIZE_TO_PAGES (NewResourceSize),
                      &NewResource
                      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Copy EndResource for initialization
    //
    mStmResourcesPtr = (UINT8 *)(UINTN)NewResource;
    mStmResourceTotalSize = NewResourceSize;
    CopyMem (mStmResourcesPtr, &mRscEndNode, sizeof(mRscEndNode));
    mStmResourceSizeUsed      = sizeof(mRscEndNode);
    mStmResourceSizeAvailable = mStmResourceTotalSize - sizeof(mRscEndNode);

    //
    // Let SmmCore change resource ptr
    //
    NotifyStmResourceChange (mStmResourcesPtr);
  } else if (mStmResourceSizeAvailable < ResourceSize) {
    //
    // Need enlarge
    //
    NewResourceSize = mStmResourceTotalSize + (ResourceSize - mStmResourceSizeAvailable);
    NewResourceSize = EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (NewResourceSize));
    DEBUG ((DEBUG_INFO, "ReAllocate - 0x%08x\n", NewResourceSize));
    Status = gSmst->SmmAllocatePages (
                      AllocateAnyPages,
                      EfiRuntimeServicesData,
                      EFI_SIZE_TO_PAGES (NewResourceSize),
                      &NewResource
                      );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    CopyMem ((VOID *)(UINTN)NewResource, mStmResourcesPtr, mStmResourceSizeUsed);
    mStmResourceSizeAvailable = NewResourceSize - mStmResourceSizeUsed;

    gSmst->SmmFreePages (
             (EFI_PHYSICAL_ADDRESS)(UINTN)mStmResourcesPtr,
             EFI_SIZE_TO_PAGES (mStmResourceTotalSize)
             );

    mStmResourceTotalSize = NewResourceSize;
    mStmResourcesPtr = (UINT8 *)(UINTN)NewResource;

    //
    // Let SmmCore change resource ptr
    //
    NotifyStmResourceChange (mStmResourcesPtr);
  }

  //
  // Check duplication
  //
  AddResource (ResourceList, NumEntries);

  return EFI_SUCCESS;
}

/**

  Delete resources in list to database.

  @param ResourceList  A pointer to resource list to be deleted
                       NULL means delete all resources.
  @param NumEntries    Optional number of entries.
                       If 0, list must be terminated by END_OF_RESOURCES.

  @retval EFI_SUCCESS            If resources are deleted
  @retval EFI_INVALID_PARAMETER  If nested procedure detected resource failer

**/
EFI_STATUS
EFIAPI
DeletePiResource (
  IN  STM_RSC    *ResourceList,
  IN  UINT32      NumEntries OPTIONAL
  )
{
  if (ResourceList != NULL) {
    // TBD
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }
  //
  // Delete all
  //
  CopyMem (mStmResourcesPtr, &mRscEndNode, sizeof(mRscEndNode));
  mStmResourceSizeUsed      = sizeof(mRscEndNode);
  mStmResourceSizeAvailable = mStmResourceTotalSize - sizeof(mRscEndNode);
  return EFI_SUCCESS;
}

/**

  Get BIOS resources.

  @param ResourceList  A pointer to resource list to be filled
  @param ResourceSize  On input it means size of resource list input.
                       On output it means size of resource list filled,
                       or the size of resource list to be filled if size of too small.

  @retval EFI_SUCCESS            If resources are returned.
  @retval EFI_BUFFER_TOO_SMALL   If resource list buffer is too small to hold the whole resources.

**/
EFI_STATUS
EFIAPI
GetPiResource (
  OUT    STM_RSC *ResourceList,
  IN OUT UINT32  *ResourceSize
  )
{
  if (*ResourceSize < mStmResourceSizeUsed) {
    *ResourceSize = (UINT32)mStmResourceSizeUsed;
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (ResourceList, mStmResourcesPtr, mStmResourceSizeUsed);
  *ResourceSize = (UINT32)mStmResourceSizeUsed;
  return EFI_SUCCESS;
}

/**

  Set valid bit for MSEG MSR.

  @param Buffer Ap function buffer. (not used)

**/
VOID
EFIAPI
EnableMsegMsr (
  IN VOID  *Buffer
  )
{
  MSR_IA32_SMM_MONITOR_CTL_REGISTER  SmmMonitorCtl;

  SmmMonitorCtl.Uint64 = AsmReadMsr64 (MSR_IA32_SMM_MONITOR_CTL);
  SmmMonitorCtl.Bits.Valid = 1;
  AsmWriteMsr64 (MSR_IA32_SMM_MONITOR_CTL, SmmMonitorCtl.Uint64);
}

/**

  Get 4K page aligned VMCS size.

  @return 4K page aligned VMCS size

**/
UINT32
GetVmcsSize (
  VOID
  )
{
  MSR_IA32_VMX_BASIC_REGISTER  VmxBasic;

  //
  // Read VMCS size and and align to 4KB
  //
  VmxBasic.Uint64 = AsmReadMsr64 (MSR_IA32_VMX_BASIC);
  return ALIGN_VALUE (VmxBasic.Bits.VmcsSize, SIZE_4KB);
}

/**

  Check STM image size.

  @param StmImage      STM image
  @param StmImageSize  STM image size

  @retval TRUE  check pass
  @retval FALSE check fail
**/
BOOLEAN
StmCheckStmImage (
  IN EFI_PHYSICAL_ADDRESS StmImage,
  IN UINTN                StmImageSize
  )
{
  UINTN                     MinMsegSize;
  STM_HEADER                *StmHeader;
  IA32_VMX_MISC_REGISTER    VmxMiscMsr;

  //
  // Check to see if STM image is compatible with CPU
  //
  StmHeader = (STM_HEADER *)(UINTN)StmImage;
  VmxMiscMsr.Uint64 = AsmReadMsr64 (MSR_IA32_VMX_MISC);
  if (StmHeader->HwStmHdr.MsegHeaderRevision != VmxMiscMsr.Bits.MsegRevisionIdentifier) {
    DEBUG ((DEBUG_ERROR, "STM Image not compatible with CPU\n"));
    DEBUG ((DEBUG_ERROR, "  StmHeader->HwStmHdr.MsegHeaderRevision = %08x\n", StmHeader->HwStmHdr.MsegHeaderRevision));
    DEBUG ((DEBUG_ERROR, "  VmxMiscMsr.Bits.MsegRevisionIdentifier = %08x\n", VmxMiscMsr.Bits.MsegRevisionIdentifier));
    return FALSE;
  }

  //
  // Get Minimal required Mseg size
  //
  MinMsegSize = (EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (StmHeader->SwStmHdr.StaticImageSize)) +
                 StmHeader->SwStmHdr.AdditionalDynamicMemorySize +
                 (StmHeader->SwStmHdr.PerProcDynamicMemorySize + GetVmcsSize () * 2) * gSmst->NumberOfCpus);
  if (MinMsegSize < StmImageSize) {
    MinMsegSize = StmImageSize;
  }

  if (StmHeader->HwStmHdr.Cr3Offset >= StmHeader->SwStmHdr.StaticImageSize) {
    //
    // We will create page table, just in case that SINIT does not create it.
    //
    if (MinMsegSize < StmHeader->HwStmHdr.Cr3Offset + EFI_PAGES_TO_SIZE(6)) {
      MinMsegSize = StmHeader->HwStmHdr.Cr3Offset + EFI_PAGES_TO_SIZE(6);
    }
  }

  //
  // Check if it exceeds MSEG size
  //
  if (MinMsegSize > mMsegSize) {
    DEBUG ((DEBUG_ERROR, "MSEG too small.  Min MSEG Size = %08x  Current MSEG Size = %08x\n", MinMsegSize, mMsegSize));
    DEBUG ((DEBUG_ERROR, "  StmHeader->SwStmHdr.StaticImageSize             = %08x\n", StmHeader->SwStmHdr.StaticImageSize));
    DEBUG ((DEBUG_ERROR, "  StmHeader->SwStmHdr.AdditionalDynamicMemorySize = %08x\n", StmHeader->SwStmHdr.AdditionalDynamicMemorySize));
    DEBUG ((DEBUG_ERROR, "  StmHeader->SwStmHdr.PerProcDynamicMemorySize    = %08x\n", StmHeader->SwStmHdr.PerProcDynamicMemorySize));
    DEBUG ((DEBUG_ERROR, "  VMCS Size                                       = %08x\n", GetVmcsSize ()));
    DEBUG ((DEBUG_ERROR, "  Max CPUs                                        = %08x\n", gSmst->NumberOfCpus));
    DEBUG ((DEBUG_ERROR, "  StmHeader->HwStmHdr.Cr3Offset                   = %08x\n", StmHeader->HwStmHdr.Cr3Offset));
    return FALSE;
  }

  return TRUE;
}

/**

  Load STM image to MSEG.

  @param StmImage      STM image
  @param StmImageSize  STM image size

**/
VOID
StmLoadStmImage (
  IN EFI_PHYSICAL_ADDRESS StmImage,
  IN UINTN                StmImageSize
  )
{
  MSR_IA32_SMM_MONITOR_CTL_REGISTER  SmmMonitorCtl;
  UINT32                             MsegBase;
  STM_HEADER                         *StmHeader;

  //
  // Get MSEG base address from MSR_IA32_SMM_MONITOR_CTL
  //
  SmmMonitorCtl.Uint64 = AsmReadMsr64 (MSR_IA32_SMM_MONITOR_CTL);
  MsegBase = SmmMonitorCtl.Bits.MsegBase << 12;

  //
  // Zero all of MSEG base address
  //
  ZeroMem ((VOID *)(UINTN)MsegBase, mMsegSize);

  //
  // Copy STM Image into MSEG
  //
  CopyMem ((VOID *)(UINTN)MsegBase, (VOID *)(UINTN)StmImage, StmImageSize);

  //
  // STM Header is at the beginning of the STM Image
  //
  StmHeader = (STM_HEADER *)(UINTN)StmImage;

  StmGen4GPageTable ((UINTN)MsegBase + StmHeader->HwStmHdr.Cr3Offset);
}

/**

  Load STM image to MSEG.

  @param StmImage      STM image
  @param StmImageSize  STM image size

  @retval EFI_SUCCESS            Load STM to MSEG successfully
  @retval EFI_ALREADY_STARTED    STM image is already loaded to MSEG
  @retval EFI_BUFFER_TOO_SMALL   MSEG is smaller than minimal requirement of STM image
  @retval EFI_UNSUPPORTED        MSEG is not enabled

**/
EFI_STATUS
EFIAPI
LoadMonitor (
  IN EFI_PHYSICAL_ADDRESS StmImage,
  IN UINTN                StmImageSize
  )
{
  MSR_IA32_SMM_MONITOR_CTL_REGISTER  SmmMonitorCtl;

  if (mLockLoadMonitor) {
    return EFI_ACCESS_DENIED;
  }

  SmmMonitorCtl.Uint64 = AsmReadMsr64 (MSR_IA32_SMM_MONITOR_CTL);
  if (SmmMonitorCtl.Bits.MsegBase == 0) {
    return EFI_UNSUPPORTED;
  }

  if (!StmCheckStmImage (StmImage, StmImageSize)) {
    return EFI_BUFFER_TOO_SMALL;
  }

  // Record STM_HASH to PCR 0, just in case it is NOT TXT launch, we still need provide the evidence.
  TpmMeasureAndLogData(
    0,                        // PcrIndex
    TXT_EVTYPE_STM_HASH,      // EventType
    NULL,                     // EventLog
    0,                        // LogLen
    (VOID *)(UINTN)StmImage,  // HashData
    StmImageSize              // HashDataLen
    );

  StmLoadStmImage (StmImage, StmImageSize);

  mStmState |= EFI_SM_MONITOR_STATE_ENABLED;

  return EFI_SUCCESS;
}

/**
  This function return BIOS STM resource.
  Produced by SmmStm.
  Consumed by SmmMpService when Init.

  @return BIOS STM resource

**/
VOID *
GetStmResource(
  VOID
  )
{
  return mStmResourcesPtr;
}

/**
  This function notify STM resource change.

  @param StmResource BIOS STM resource

**/
VOID
NotifyStmResourceChange (
  VOID *StmResource
  )
{
  UINTN                         Index;
  TXT_PROCESSOR_SMM_DESCRIPTOR  *Psd;

  for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
    Psd = (TXT_PROCESSOR_SMM_DESCRIPTOR *)((UINTN)gSmst->CpuSaveState[Index] - SMRAM_SAVE_STATE_MAP_OFFSET + TXT_SMM_PSD_OFFSET);
    Psd->BiosHwResourceRequirementsPtr = (UINT64)(UINTN)StmResource;
  }
  return ;
}


/**
  This is STM setup BIOS callback.
**/
VOID
EFIAPI
SmmStmSetup (
  VOID
  )
{
  mStmState |= EFI_SM_MONITOR_STATE_ACTIVATED;
}

/**
  This is STM teardown BIOS callback.
**/
VOID
EFIAPI
SmmStmTeardown (
  VOID
  )
{
  mStmState &= ~EFI_SM_MONITOR_STATE_ACTIVATED;
}

