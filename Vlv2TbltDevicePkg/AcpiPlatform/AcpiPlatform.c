/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   

  This program and the accompanying materials are licensed and made available under

  the terms and conditions of the BSD License that accompanies this distribution.  

  The full text of the license may be found at                                     

  http://opensource.org/licenses/bsd-license.php.                                  

                                                                                   

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            

  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    

                                                                                   




Module Name:

  AcpiPlatform.c

Abstract:

  ACPI Platform Driver


--*/

#include <PiDxe.h>
#include <Protocol/TcgService.h>
#include <Protocol/FirmwareVolume.h>
#include "AcpiPlatform.h"
#include "AcpiPlatformHooks.h"
#include "AcpiPlatformHooksLib.h"
#include "Platform.h"
#include <Hpet.h>
#include <Mcfg.h>
#include "Osfr.h"
#include <Guid/GlobalVariable.h>
#include <Guid/SetupVariable.h>
#include <Guid/PlatformInfo.h>
#include <Protocol/CpuIo.h>
#include <Guid/BoardFeatures.h>
#include <Protocol/AcpiSupport.h>
#include <Protocol/AcpiS3Save.h>
#include <Protocol/Ps2Policy.h>
#include <Library/CpuIA32.h>
#include <SetupMode.h>
#include <Guid/AcpiTableStorage.h>
#include <Guid/EfiVpdData.h>
#include <PchAccess.h>
#include <Guid/Vlv2Variable.h>
#include <Guid/PlatformCpuInfo.h>


CHAR16    EfiPlatformCpuInfoVariable[] = L"PlatformCpuInfo";
CHAR16    gACPIOSFRModelStringVariableName[] = ACPI_OSFR_MODEL_STRING_VARIABLE_NAME;
CHAR16    gACPIOSFRRefDataBlockVariableName[] = ACPI_OSFR_REF_DATA_BLOCK_VARIABLE_NAME;
CHAR16    gACPIOSFRMfgStringVariableName[] = ACPI_OSFR_MFG_STRING_VARIABLE_NAME;

EFI_CPU_IO_PROTOCOL                    *mCpuIo;
#ifndef __GNUC__
#pragma optimize("", off)
#endif
BOOLEAN                   mFirstNotify;
EFI_PLATFORM_INFO_HOB     *mPlatformInfo;
EFI_GUID                  mSystemConfigurationGuid = SYSTEM_CONFIGURATION_GUID;
SYSTEM_CONFIGURATION      mSystemConfiguration;
SYSTEM_CONFIGURATION      mSystemConfig;

UINT8 mSmbusRsvdAddresses[] = PLATFORM_SMBUS_RSVD_ADDRESSES;
UINT8 mNumberSmbusAddress = sizeof( mSmbusRsvdAddresses ) / sizeof( mSmbusRsvdAddresses[0] );

/**
  Locate the first instance of a protocol.  If the protocol requested is an
  FV protocol, then it will return the first FV that contains the ACPI table
  storage file.

  @param[in]  Protocol            The protocol to find.
  @param[in]  Instance            Return pointer to the first instance of the protocol.
  @param[in]  Type                The type of protocol to locate.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_NOT_FOUND          The protocol could not be located.
  @retval  EFI_OUT_OF_RESOURCES   There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateSupportProtocol (
  IN   EFI_GUID       *Protocol,
  OUT  VOID           **Instance,
  IN   UINT32         Type
  )
{
  EFI_STATUS              Status;
  EFI_HANDLE              *HandleBuffer;
  UINTN                   NumberOfHandles;
  EFI_FV_FILETYPE         FileType;
  UINT32                  FvStatus;
  EFI_FV_FILE_ATTRIBUTES  Attributes;
  UINTN                   Size;
  UINTN                   Index;

  FvStatus = 0;

  //
  // Locate protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  Protocol,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    //
    // Defined errors at this time are not found and out of resources.
    //
    return Status;
  }

  //
  // Looking for FV with ACPI storage file.
  //
  for (Index = 0; Index < NumberOfHandles; Index++) {
    //
    // Get the protocol on this handle.
    // This should not fail because of LocateHandleBuffer.
    //
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    Protocol,
                    Instance
                    );
    ASSERT (!EFI_ERROR (Status));

    if (!Type) {
      //
      // Not looking for the FV protocol, so find the first instance of the
      // protocol.  There should not be any errors because our handle buffer
      // should always contain at least one or LocateHandleBuffer would have
      // returned not found.
      //
      break;
    }

    //
    // See if it has the ACPI storage file.
    //
    Status = ((EFI_FIRMWARE_VOLUME_PROTOCOL *) (*Instance))->ReadFile (
                                                              *Instance,
                                                              &gEfiAcpiTableStorageGuid,
                                                              NULL,
                                                              &Size,
                                                              &FileType,
                                                              &Attributes,
                                                              &FvStatus
                                                              );

    //
    // If we found it, then we are done.
    //
    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  //
  // Our exit status is determined by the success of the previous operations.
  // If the protocol was found, Instance already points to it.
  //
  //
  // Free any allocated buffers.
  //
  gBS->FreePool (HandleBuffer);

  return Status;
}

/**
  This function will update any runtime platform specific information.
  This currently includes:
    Setting OEM table values, ID, table ID, creator ID and creator revision.
    Enabling the proper processor entries in the APIC tables.

  @param[in]  Table       The table to update.

  @retval  EFI_SUCCESS    The function completed successfully.

**/
EFI_STATUS
PlatformUpdateTables (
  IN OUT EFI_ACPI_COMMON_HEADER  *Table
  )
{
  EFI_ACPI_DESCRIPTION_HEADER                                 *TableHeader;
  UINT8                                                       *CurrPtr;
  UINT8                                                       *EndPtr;
  ACPI_APIC_STRUCTURE_PTR                                     *ApicPtr;
  UINT8                                                       CurrProcessor;
  EFI_STATUS                                                  Status;
  EFI_MP_SERVICES_PROTOCOL                                    *MpService;
  UINTN                                                       MaximumNumberOfCPUs;
  UINTN                                                       NumberOfEnabledCPUs;
  UINTN                                                       BufferSize;
  ACPI_APIC_STRUCTURE_PTR                                     *ProcessorLocalApicEntry;
  UINTN                                                       BspIndex;
  EFI_ACPI_1_0_ASF_DESCRIPTION_TABLE                          *AsfEntry;
  EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER            *HpetTbl;
  UINT64                                                      OemIdValue;
  UINT8                                                       Index;
  EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE                   *Facp;
  EFI_ACPI_OSFR_TABLE                                         *OsfrTable;
  EFI_ACPI_OSFR_OCUR_OBJECT                                   *pOcurObject;
  EFI_ACPI_OSFR_OCUR_OBJECT                                   OcurObject = {{0xB46F133D, 0x235F, 0x4634, 0x9F, 0x03, 0xB1, 0xC0, 0x1C, 0x54, 0x78, 0x5B}, 0, 0, 0, 0, 0};
  CHAR16                                                      *OcurMfgStringBuffer = NULL;
  CHAR16                                                      *OcurModelStringBuffer = NULL;
  UINT8                                                       *OcurRefDataBlockBuffer = NULL;
  UINTN                                                       OcurMfgStringBufferSize;
  UINTN                                                       OcurModelStringBufferSize;
  UINTN                                                       OcurRefDataBlockBufferSize;
#if defined (IDCC2_SUPPORTED) && IDCC2_SUPPORTED
  EFI_ACPI_ASPT_TABLE                                         *pSpttTable;
#endif
  UINT16                                                      NumberOfHpets;
  UINT16                                                      HpetCapIdValue;
  UINT32                                                      HpetBlockID;
  UINTN                                                       LocalApicCounter;
  EFI_PROCESSOR_INFORMATION                                   ProcessorInfoBuffer;
  UINT8                                                       TempVal;
  EFI_ACPI_3_0_IO_APIC_STRUCTURE                              *IOApicType;
  EFI_ACPI_3_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER         *APICTableHeader;

  CurrPtr                 = NULL;
  EndPtr                  = NULL;
  ApicPtr                 = NULL;
  LocalApicCounter        = 0;
  CurrProcessor           = 0;
  ProcessorLocalApicEntry = NULL;


 if (Table->Signature != EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE) {
    TableHeader = (EFI_ACPI_DESCRIPTION_HEADER *) Table;
    //
    // Update the OEMID.
    //
    OemIdValue = mPlatformInfo->AcpiOemId;

    *(UINT32 *)(TableHeader->OemId)     = (UINT32)OemIdValue;
    *(UINT16 *)(TableHeader->OemId + 4) = *(UINT16*)(((UINT8 *)&OemIdValue) + 4);

    if ((Table->Signature != EFI_ACPI_2_0_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE)) {
    //
    // Update the OEM Table ID.
    //
      TableHeader->OemTableId = mPlatformInfo->AcpiOemTableId;
    }

    //
    // Update the OEM Table ID.
    //
    TableHeader->OemRevision = EFI_ACPI_OEM_REVISION;

    //
    // Update the creator ID.
    //
    TableHeader->CreatorId = EFI_ACPI_CREATOR_ID;

    //
    // Update the creator revision.
    //
    TableHeader->CreatorRevision = EFI_ACPI_CREATOR_REVISION;
  }

  //
  // Complete this function.
  //
  //
  // Locate the MP services protocol.
  //
  //
  // Find the MP Protocol. This is an MP platform, so MP protocol must be
  // there.
  //
  Status = gBS->LocateProtocol (
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  (VOID **) &MpService
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Determine the number of processors.
  //
  MpService->GetNumberOfProcessors (
              MpService,
              &MaximumNumberOfCPUs,
              &NumberOfEnabledCPUs
              );

  ASSERT (MaximumNumberOfCPUs <= MAX_CPU_NUM && NumberOfEnabledCPUs >= 1);


  //
  // Assign a invalid intial value for update.
  //
  //
  // Update the processors in the APIC table.
  //
  switch (Table->Signature) {
    case EFI_ACPI_1_0_ASF_DESCRIPTION_TABLE_SIGNATURE:
      //
      // Update the table if ASF is enabled. Otherwise, return error so caller will not install.
      //
      if (mSystemConfig.Asf == 1) {
        return  EFI_UNSUPPORTED;
      }
      AsfEntry = (EFI_ACPI_1_0_ASF_DESCRIPTION_TABLE *) Table;
      TempVal = (mNumberSmbusAddress < ASF_ADDR_DEVICE_ARRAY_LENGTH)? mNumberSmbusAddress : ASF_ADDR_DEVICE_ARRAY_LENGTH;
      for (Index = 0; Index < TempVal; Index++) {
        AsfEntry->AsfAddr.FixedSmbusAddresses[Index] = mSmbusRsvdAddresses[Index];
      }
      break;

    case EFI_ACPI_3_0_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE:

      Status = MpService->WhoAmI (
                            MpService,
                            &BspIndex
                            );

      //
      // PCAT_COMPAT Set to 1 indicate 8259 vectors should be disabled.
      //
      APICTableHeader = (EFI_ACPI_3_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)Table;
      APICTableHeader->Flags |= EFI_ACPI_3_0_PCAT_COMPAT;

      CurrPtr = (UINT8 *) &((EFI_ACPI_DESCRIPTION_HEADER *) Table)[1];
      CurrPtr = CurrPtr + 8;

      //
      // Size of Local APIC Address & Flag.
      //
      EndPtr  = (UINT8 *) Table;
      EndPtr  = EndPtr + Table->Length;
      while (CurrPtr < EndPtr) {
        ApicPtr = (ACPI_APIC_STRUCTURE_PTR *) CurrPtr;
        switch (ApicPtr->AcpiApicCommon.Type) {
          case EFI_ACPI_3_0_PROCESSOR_LOCAL_APIC:
            //
            // ESS override
            // Fix for Ordering of MADT to be maintained as it is in MADT table.
            //
            // Update processor enabled or disabled and keep the local APIC
            // order in MADT intact.
            //
            // Sanity check to make sure proc-id is not arbitrary.
            //
            DEBUG ((EFI_D_ERROR, "ApicPtr->AcpiLocalApic.AcpiProcessorId = %x, MaximumNumberOfCPUs = %x\n", \
            ApicPtr->AcpiLocalApic.AcpiProcessorId, MaximumNumberOfCPUs));
            if(ApicPtr->AcpiLocalApic.AcpiProcessorId > MaximumNumberOfCPUs) {
              ApicPtr->AcpiLocalApic.AcpiProcessorId = (UINT8)MaximumNumberOfCPUs;
            }

            BufferSize                    = 0;
            ApicPtr->AcpiLocalApic.Flags  = 0;

            for (CurrProcessor = 0; CurrProcessor < MaximumNumberOfCPUs; CurrProcessor++) {
              Status = MpService->GetProcessorInfo (
                                    MpService,
                                    CurrProcessor,
                                    &ProcessorInfoBuffer
                                    );

              if (Status == EFI_SUCCESS && ProcessorInfoBuffer.ProcessorId == ApicPtr->AcpiLocalApic.ApicId) {
                //
                // Check to see whether or not a processor (or thread) is enabled.
                //
                if ((BspIndex == CurrProcessor) || ((ProcessorInfoBuffer.StatusFlag & PROCESSOR_ENABLED_BIT) != 0)) {
                  //
                  // Go on and check if Hyper Threading is enabled. If HT not enabled
                  // hide this thread from OS by not setting the flag to 1.  This is the
                  // software way to disable Hyper Threading.  Basically we just hide it
                  // from the OS.
                  //
                  ApicPtr->AcpiLocalApic.Flags = EFI_ACPI_1_0_LOCAL_APIC_ENABLED;


                  if(ProcessorInfoBuffer.Location.Thread != 0) {
                    ApicPtr->AcpiLocalApic.Flags = 0;
                  }

                  AppendCpuMapTableEntry (&(ApicPtr->AcpiLocalApic));
                }
                break;
              }
            }

            //
            // If no APIC-ID match, the cpu may not be populated.
            //
            break;

          case EFI_ACPI_3_0_IO_APIC:

            IOApicType = (EFI_ACPI_3_0_IO_APIC_STRUCTURE *)CurrPtr;
            IOApicType->IoApicId = 0x02;
            //
            // IO APIC entries can be patched here.
            //
            break;
        }

        CurrPtr = CurrPtr + ApicPtr->AcpiApicCommon.Length;
      }
      break;

    case EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE:

       Facp = (EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *) Table;
       Facp->Flags &= (UINT32)(~(3<<2));

      break;

    case EFI_ACPI_3_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
      //
      // Patch the memory resource.
      //
      PatchDsdtTable ((EFI_ACPI_DESCRIPTION_HEADER *) Table);
      break;

    case EFI_ACPI_3_0_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
      //
      // Gv3 support
      //
      // TBD: Need re-design based on the ValleyTrail platform.
      //
      break;

    case EFI_ACPI_3_0_HIGH_PRECISION_EVENT_TIMER_TABLE_SIGNATURE:
      //
      // Adjust HPET Table to correct the Base Address.
      //
      // Enable HPET always as Hpet.asi always indicates that Hpet is enabled.
      //
      MmioOr8 (R_PCH_PCH_HPET + R_PCH_PCH_HPET_GCFG, B_PCH_PCH_HPET_GCFG_EN);


      HpetTbl = (EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER *) Table;
      HpetTbl->BaseAddressLower32Bit.Address = HPET_BASE_ADDRESS;
      HpetTbl->EventTimerBlockId = *((UINT32*)(UINTN)HPET_BASE_ADDRESS);

      HpetCapIdValue = *(UINT16 *)(UINTN)(HPET_BASE_ADDRESS);
      NumberOfHpets = HpetCapIdValue & B_PCH_PCH_HPET_GCID_NT;  // Bits [8:12] contains the number of Hpets
      HpetBlockID = EFI_ACPI_EVENT_TIMER_BLOCK_ID;

      if((NumberOfHpets) && (NumberOfHpets & B_PCH_PCH_HPET_GCID_NT)) {
        HpetBlockID |= (NumberOfHpets);
      }
      HpetTbl->EventTimerBlockId = HpetBlockID;

      break;

    case EFI_ACPI_3_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE:
      //
      // Update MCFG base and end bus number.
      //
      ((EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE *) Table)->Segment[0].BaseAddress
        = mPlatformInfo->PciData.PciExpressBase;
      ((EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE *) Table)->Segment[0].EndBusNumber
        = (UINT8)RShiftU64 (mPlatformInfo->PciData.PciExpressSize, 20) - 1;
      break;


    case EFI_ACPI_OSFR_TABLE_SIGNATURE:
      //
      // Get size of OSFR variable.
      //
      OcurMfgStringBufferSize = 0;
      Status = gRT->GetVariable (
                      gACPIOSFRMfgStringVariableName,
                      &gACPIOSFRMfgStringVariableGuid,
                      NULL,
                      &OcurMfgStringBufferSize,
                      NULL
                      );
      if (Status != EFI_BUFFER_TOO_SMALL) {
        //
        // Variable must not be present on the system.
        //
        return EFI_UNSUPPORTED;
      }

      //
      // Allocate memory for variable data.
      //
      OcurMfgStringBuffer = AllocatePool (OcurMfgStringBufferSize);
      Status = gRT->GetVariable (
                      gACPIOSFRMfgStringVariableName,
                      &gACPIOSFRMfgStringVariableGuid,
                      NULL,
                      &OcurMfgStringBufferSize,
                      OcurMfgStringBuffer
                      );
      if (!EFI_ERROR (Status)) {
        OcurModelStringBufferSize = 0;
        Status = gRT->GetVariable (
                        gACPIOSFRModelStringVariableName,
                        &gACPIOSFRModelStringVariableGuid,
                        NULL,
                        &OcurModelStringBufferSize,
                        NULL
                        );
        if (Status != EFI_BUFFER_TOO_SMALL) {
          //
          // Variable must not be present on the system.
          //
          return EFI_UNSUPPORTED;
        }

        //
        // Allocate memory for variable data.
        //
        OcurModelStringBuffer = AllocatePool (OcurModelStringBufferSize);
        Status = gRT->GetVariable (
                        gACPIOSFRModelStringVariableName,
                        &gACPIOSFRModelStringVariableGuid,
                        NULL,
                        &OcurModelStringBufferSize,
                        OcurModelStringBuffer
                        );
        if (!EFI_ERROR (Status)) {
          OcurRefDataBlockBufferSize = 0;
          Status = gRT->GetVariable (
                          gACPIOSFRRefDataBlockVariableName,
                          &gACPIOSFRRefDataBlockVariableGuid,
                          NULL,
                          &OcurRefDataBlockBufferSize,
                          NULL
                          );
          if (Status == EFI_BUFFER_TOO_SMALL) {
            //
            // Allocate memory for variable data.
            //
            OcurRefDataBlockBuffer = AllocatePool (OcurRefDataBlockBufferSize);
            Status = gRT->GetVariable (
                            gACPIOSFRRefDataBlockVariableName,
                            &gACPIOSFRRefDataBlockVariableGuid,
                            NULL,
                            &OcurRefDataBlockBufferSize,
                            OcurRefDataBlockBuffer
                            );
          }
          OsfrTable = (EFI_ACPI_OSFR_TABLE *) Table;
          //
          // Currently only one object is defined: OCUR_OSFR_TABLE.
          //
          OsfrTable->ObjectCount = 1;
          //
          // Initialize table length to fixed portion of the ACPI OSFR table.
          //
          OsfrTable->Header.Length = sizeof (EFI_ACPI_OSFR_TABLE_FIXED_PORTION);
          *(UINT32 *)((UINTN) OsfrTable + sizeof (EFI_ACPI_OSFR_TABLE_FIXED_PORTION)) = \
            (UINT32) (sizeof (EFI_ACPI_OSFR_TABLE_FIXED_PORTION) + sizeof (UINT32));
          pOcurObject = (EFI_ACPI_OSFR_OCUR_OBJECT *)((UINTN) OsfrTable + sizeof (EFI_ACPI_OSFR_TABLE_FIXED_PORTION) + \
            sizeof (UINT32));
          CopyMem (pOcurObject, &OcurObject, sizeof (EFI_ACPI_OSFR_OCUR_OBJECT));
          pOcurObject->ManufacturerNameStringOffset = (UINT32)((UINTN) pOcurObject - (UINTN) OsfrTable + \
            sizeof (EFI_ACPI_OSFR_OCUR_OBJECT));
          pOcurObject->ModelNameStringOffset = (UINT32)((UINTN) pOcurObject - (UINTN) OsfrTable + \
            sizeof (EFI_ACPI_OSFR_OCUR_OBJECT) + OcurMfgStringBufferSize);
          if (OcurRefDataBlockBufferSize > 0) {
            pOcurObject->MicrosoftReferenceOffset = (UINT32)((UINTN) pOcurObject - (UINTN) OsfrTable + \
              sizeof (EFI_ACPI_OSFR_OCUR_OBJECT) + OcurMfgStringBufferSize + OcurModelStringBufferSize);
          }
          CopyMem ((UINTN *)((UINTN) pOcurObject + sizeof (EFI_ACPI_OSFR_OCUR_OBJECT)), OcurMfgStringBuffer, \
            OcurMfgStringBufferSize);
          CopyMem ((UINTN *)((UINTN) pOcurObject + sizeof (EFI_ACPI_OSFR_OCUR_OBJECT) + OcurMfgStringBufferSize), \
            OcurModelStringBuffer, OcurModelStringBufferSize);
          if (OcurRefDataBlockBufferSize > 0) {
            CopyMem ((UINTN *)((UINTN) pOcurObject + sizeof (EFI_ACPI_OSFR_OCUR_OBJECT) + OcurMfgStringBufferSize + \
            OcurModelStringBufferSize),OcurRefDataBlockBuffer, OcurRefDataBlockBufferSize);
          }
          OsfrTable->Header.Length += (UINT32)(OcurMfgStringBufferSize + OcurModelStringBufferSize + OcurRefDataBlockBufferSize);
          OsfrTable->Header.Length += sizeof (EFI_ACPI_OSFR_OCUR_OBJECT) + sizeof (UINT32);
        }
      }
      gBS->FreePool (OcurMfgStringBuffer);
      gBS->FreePool (OcurModelStringBuffer);
      gBS->FreePool (OcurRefDataBlockBuffer);
      break;
    default:
      break;
  }

  //
  //
  // Update the hardware signature in the FACS structure.
  //
  //
  // Locate the SPCR table and update based on current settings.
  // The user may change CR settings via setup or other methods.
  // The SPCR table must match.
  //
  return EFI_SUCCESS;
}

/**

Routine Description:

  GC_TODO: Add function description.

Arguments:

  Event   - GC_TODO: add argument description
  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

**/
STATIC
VOID
EFIAPI
OnReadyToBoot (
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  EFI_STATUS                  Status;
  EFI_ACPI_TABLE_VERSION      TableVersion;
  EFI_ACPI_SUPPORT_PROTOCOL   *AcpiSupport;
  EFI_ACPI_S3_SAVE_PROTOCOL   *AcpiS3Save;
  SYSTEM_CONFIGURATION        SetupVarBuffer;
  UINTN                       VariableSize;
  EFI_PLATFORM_CPU_INFO       *PlatformCpuInfoPtr = NULL;
  EFI_PLATFORM_CPU_INFO       PlatformCpuInfo;
  EFI_PEI_HOB_POINTERS          GuidHob;

  if (mFirstNotify) {
    return;
  }

  mFirstNotify = TRUE;

  //
  // To avoid compiler warning of "C4701: potentially uninitialized local variable 'PlatformCpuInfo' used".
  //
  PlatformCpuInfo.CpuVersion.FullCpuId = 0;

  //
  // Get Platform CPU Info HOB.
  //
  PlatformCpuInfoPtr = NULL;
  ZeroMem (&PlatformCpuInfo, sizeof(EFI_PLATFORM_CPU_INFO));
  VariableSize = sizeof(EFI_PLATFORM_CPU_INFO);
  Status = gRT->GetVariable(
                  EfiPlatformCpuInfoVariable,
                  &gEfiVlv2VariableGuid,
                  NULL,
                  &VariableSize,
                  PlatformCpuInfoPtr
                  );
  if (EFI_ERROR(Status)) {
    GuidHob.Raw = GetHobList ();
    if (GuidHob.Raw != NULL) {
      if ((GuidHob.Raw = GetNextGuidHob (&gEfiPlatformCpuInfoGuid, GuidHob.Raw)) != NULL) {
        PlatformCpuInfoPtr = GET_GUID_HOB_DATA (GuidHob.Guid);
      }
    }
  }

  if ((PlatformCpuInfoPtr != NULL)) {
    CopyMem(&PlatformCpuInfo, PlatformCpuInfoPtr, sizeof(EFI_PLATFORM_CPU_INFO));
  }

  //
  // Update the ACPI parameter blocks finally.
  //
  VariableSize = sizeof (SYSTEM_CONFIGURATION);
  Status = gRT->GetVariable (
                  L"Setup",
                  &mSystemConfigurationGuid,
                  NULL,
                  &VariableSize,
                  &SetupVarBuffer
                  );
  if (EFI_ERROR (Status) || VariableSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VariableSize = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
              L"SetupRecovery",
              &mSystemConfigurationGuid,
              NULL,
              &VariableSize,
              &SetupVarBuffer
              );
    ASSERT_EFI_ERROR (Status);
  }    

  //
  // Find the AcpiSupport protocol.
  //
  Status = LocateSupportProtocol (&gEfiAcpiSupportProtocolGuid, (VOID **) &AcpiSupport, 0);
  ASSERT_EFI_ERROR (Status);

  TableVersion = EFI_ACPI_TABLE_VERSION_2_0;

  //
  // Publish ACPI 1.0 or 2.0 Tables.
  //
  Status = AcpiSupport->PublishTables (
                          AcpiSupport,
                          TableVersion
                          );
  ASSERT_EFI_ERROR (Status);

  //
  // S3 script save.
  //
  Status = gBS->LocateProtocol (&gEfiAcpiS3SaveProtocolGuid, NULL, (VOID **) &AcpiS3Save);
  if (!EFI_ERROR (Status)) {
    AcpiS3Save->S3Save (AcpiS3Save, NULL);
  }

}

VOID
PR1FSASetting (
  IN VOID
  )
{
  //
  // for FSA on  PR1.
  //
  if (mPlatformInfo->BoardId == BOARD_ID_BL_FFRD && mPlatformInfo->BoardRev >= PR1) {
    DEBUG((EFI_D_ERROR, "Set FSA status = 1 for FFRD PR1\n"));
    mGlobalNvsArea.Area->FsaStatus  = mSystemConfiguration.PchFSAOn;
  }
  if (mPlatformInfo->BoardId == BOARD_ID_BL_FFRD8) {
    DEBUG((EFI_D_ERROR, "Set FSA status = 1 for FFRD8\n"));
    mGlobalNvsArea.Area->FsaStatus  = mSystemConfiguration.PchFSAOn;
  }

}

/**
  Entry point for Acpi platform driver.

  @param[in]  ImageHandle        A handle for the image that is initializing this driver.
  @param[in]  SystemTable        A pointer to the EFI system table.

  @retval  EFI_SUCCESS           Driver initialized successfully.
  @retval  EFI_LOAD_ERROR        Failed to Initialize or has been loaded.
  @retval  EFI_OUT_OF_RESOURCES  Could not allocate needed resources.

**/
EFI_STATUS
EFIAPI
AcpiPlatformEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_STATUS                    AcpiStatus;
  EFI_ACPI_SUPPORT_PROTOCOL     *AcpiSupport;
  EFI_FIRMWARE_VOLUME_PROTOCOL  *FwVol;
  INTN                          Instance;
  EFI_ACPI_COMMON_HEADER        *CurrentTable;
  UINTN                         TableHandle;
  UINT32                        FvStatus;
  UINT32                        Size;
  EFI_EVENT                     Event;
  EFI_ACPI_TABLE_VERSION        TableVersion;
  UINTN                         VarSize;
  UINTN                         SysCfgSize;
  EFI_HANDLE                    Handle;
  EFI_PS2_POLICY_PROTOCOL       *Ps2Policy;
  EFI_PEI_HOB_POINTERS          GuidHob;
  UINT8                         PortData;
  EFI_MP_SERVICES_PROTOCOL      *MpService;
  UINTN                         MaximumNumberOfCPUs;
  UINTN                         NumberOfEnabledCPUs;
  UINT32                        Data32;
  PCH_STEPPING                  pchStepping;

  mFirstNotify      = FALSE;

  TableVersion      = EFI_ACPI_TABLE_VERSION_2_0;
  Instance          = 0;
  CurrentTable      = NULL;
  TableHandle       = 0;
  Data32            = 0;

  //
  // Update HOB variable for PCI resource information.
  // Get the HOB list.  If it is not present, then ASSERT.
  //
  GuidHob.Raw = GetHobList ();
  if (GuidHob.Raw != NULL) {
    if ((GuidHob.Raw = GetNextGuidHob (&gEfiPlatformInfoGuid, GuidHob.Raw)) != NULL) {
      mPlatformInfo = GET_GUID_HOB_DATA (GuidHob.Guid);
    }
  }

  //
  // Search for the Memory Configuration GUID HOB.  If it is not present, then
  // there's nothing we can do. It may not exist on the update path.
  //
  VarSize = sizeof(SYSTEM_CONFIGURATION);
  Status = gRT->GetVariable(
                  L"Setup",
                  &mSystemConfigurationGuid,
                  NULL,
                  &VarSize,
                  &mSystemConfiguration
                  );
  if (EFI_ERROR (Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VarSize = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
              L"SetupRecovery",
              &mSystemConfigurationGuid,
              NULL,
              &VarSize,
              &mSystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Find the AcpiSupport protocol.
  //
  Status = LocateSupportProtocol (&gEfiAcpiSupportProtocolGuid, (VOID **) &AcpiSupport, 0);
  ASSERT_EFI_ERROR (Status);

  //
  // Locate the firmware volume protocol.
  //
  Status = LocateSupportProtocol (&gEfiFirmwareVolumeProtocolGuid, (VOID **) &FwVol, 1);
  ASSERT_EFI_ERROR (Status);

  //
  // Read the current system configuration variable store.
  //
  SysCfgSize = sizeof(SYSTEM_CONFIGURATION);
  Status = gRT->GetVariable (
                  L"Setup",
                  &gEfiNormalSetupGuid,
                  NULL,
                  &SysCfgSize,
                  &mSystemConfig
                  );
  if (EFI_ERROR (Status) || SysCfgSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    SysCfgSize = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
              L"SetupRecovery",
              &gEfiNormalSetupGuid,
              NULL,
              &SysCfgSize,
              &mSystemConfig
              );
    ASSERT_EFI_ERROR (Status);
  }


  Status    = EFI_SUCCESS;
  Instance  = 0;

  //
  // TBD: Need re-design based on the ValleyTrail platform.
  //
  Status = gBS->LocateProtocol (
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  (VOID **) &MpService
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Determine the number of processors.
  //
  MpService->GetNumberOfProcessors (
               MpService,
               &MaximumNumberOfCPUs,
               &NumberOfEnabledCPUs
               );

  //
  // Allocate and initialize the NVS area for SMM and ASL communication.
  //
  Status = gBS->AllocatePool (
                  EfiACPIMemoryNVS,
                  sizeof (EFI_GLOBAL_NVS_AREA),
                  (void **)&mGlobalNvsArea.Area
                  );
  ASSERT_EFI_ERROR (Status);
  gBS->SetMem (
         mGlobalNvsArea.Area,
         sizeof (EFI_GLOBAL_NVS_AREA),
         0
         );
  DEBUG((EFI_D_ERROR, "mGlobalNvsArea.Area is at 0x%X\n", mGlobalNvsArea.Area));

  //
  // Update global NVS area for ASL and SMM init code to use.
  //
  mGlobalNvsArea.Area->ApicEnable                 = 1;
  mGlobalNvsArea.Area->EmaEnable                  = 0;

  mGlobalNvsArea.Area->NumberOfBatteries          = 1;
  mGlobalNvsArea.Area->BatteryCapacity0           = 100;
  mGlobalNvsArea.Area->BatteryStatus0             = 84;
  mGlobalNvsArea.Area->OnboardCom                 = 1;
  mGlobalNvsArea.Area->IdeMode                    = 0;
  mGlobalNvsArea.Area->PowerState                 = 0;

  mGlobalNvsArea.Area->LogicalProcessorCount    = (UINT8)NumberOfEnabledCPUs;

  mGlobalNvsArea.Area->PassiveThermalTripPoint  = mSystemConfiguration.PassiveThermalTripPoint;
  mGlobalNvsArea.Area->PassiveTc1Value          = mSystemConfiguration.PassiveTc1Value;
  mGlobalNvsArea.Area->PassiveTc2Value          = mSystemConfiguration.PassiveTc2Value;
  mGlobalNvsArea.Area->PassiveTspValue          = mSystemConfiguration.PassiveTspValue;
  mGlobalNvsArea.Area->CriticalThermalTripPoint = mSystemConfiguration.CriticalThermalTripPoint;

  mGlobalNvsArea.Area->IgdPanelType             = mSystemConfiguration.IgdFlatPanel;
  mGlobalNvsArea.Area->IgdPanelScaling          = mSystemConfiguration.PanelScaling;
  mGlobalNvsArea.Area->IgdSciSmiMode            = 0;
  mGlobalNvsArea.Area->IgdTvFormat              = 0;
  mGlobalNvsArea.Area->IgdTvMinor               = 0;
  mGlobalNvsArea.Area->IgdSscConfig             = 1;
  mGlobalNvsArea.Area->IgdBiaConfig             = mSystemConfiguration.IgdLcdIBia;
  mGlobalNvsArea.Area->IgdBlcConfig             = mSystemConfiguration.IgdLcdIGmchBlc;
  mGlobalNvsArea.Area->IgdDvmtMemSize           =  mSystemConfiguration.IgdDvmt50TotalAlloc;
  mGlobalNvsArea.Area->IgdPAVP                  = mSystemConfiguration.PavpMode;

  mGlobalNvsArea.Area->AlsEnable                = mSystemConfiguration.AlsEnable;
  mGlobalNvsArea.Area->BacklightControlSupport  = 2;
  mGlobalNvsArea.Area->BrightnessPercentage    = 100;
  mGlobalNvsArea.Area->IgdState = 1;
  mGlobalNvsArea.Area->LidState = 1;

  mGlobalNvsArea.Area->DeviceId1 = 0x80000100 ;
  mGlobalNvsArea.Area->DeviceId2 = 0x80000400 ;
  mGlobalNvsArea.Area->DeviceId3 = 0x80000200 ;
  mGlobalNvsArea.Area->DeviceId4 = 0x04;
  mGlobalNvsArea.Area->DeviceId5 = 0x05;
  mGlobalNvsArea.Area->NumberOfValidDeviceId = 4 ;
  mGlobalNvsArea.Area->CurrentDeviceList = 0x0F ;
  mGlobalNvsArea.Area->PreviousDeviceList = 0x0F ;

  mGlobalNvsArea.Area->UartSelection = mSystemConfiguration.UartInterface;
  mGlobalNvsArea.Area->PcuUart1Enable = mSystemConfiguration.PcuUart1;
  mGlobalNvsArea.Area->NativePCIESupport = 1;





  //
  // Update BootMode: 0:ACPI mode; 1:PCI mode
  //
  mGlobalNvsArea.Area->LpssSccMode = mSystemConfiguration.LpssPciModeEnabled;
  if (mSystemConfiguration.LpssMipiHsi == 0) {
    mGlobalNvsArea.Area->MipiHsiAddr  = 0;
    mGlobalNvsArea.Area->MipiHsiLen   = 0;
    mGlobalNvsArea.Area->MipiHsi1Addr = 0;
    mGlobalNvsArea.Area->MipiHsi1Len  = 0;
  }

  //
  // Platform Flavor
  //
  mGlobalNvsArea.Area->PlatformFlavor = mPlatformInfo->PlatformFlavor;

  //
  // Update the Platform id
  //
  mGlobalNvsArea.Area->BoardID = mPlatformInfo->BoardId;

  //
  // Update the  Board Revision
  //
  mGlobalNvsArea.Area->FabID = mPlatformInfo->BoardRev;

  //
  // Update SOC Stepping
  //
  mGlobalNvsArea.Area->SocStepping = (UINT8)(PchStepping());

  mGlobalNvsArea.Area->OtgMode = mSystemConfiguration.PchUsbOtg;

  pchStepping = PchStepping();
  if (mSystemConfiguration.UsbAutoMode == 1) {
    //
    // Auto mode is enabled.
    //
    if (PchA0 == pchStepping) {
      //
      //  For A0, EHCI is enabled as default.
      //
      mSystemConfiguration.PchUsb20       = 1;
      mSystemConfiguration.PchUsb30Mode   = 0;
      mSystemConfiguration.UsbXhciSupport = 0;
      DEBUG ((EFI_D_INFO, "EHCI is enabled as default. SOC 0x%x\n", pchStepping));
    } else {
      //
      //  For A1 and later, XHCI is enabled as default.
      //
      mSystemConfiguration.PchUsb20       = 0;
      mSystemConfiguration.PchUsb30Mode   = 1;
      mSystemConfiguration.UsbXhciSupport = 1;
      DEBUG ((EFI_D_INFO, "XHCI is enabled as default. SOC 0x%x\n", pchStepping));
    }
  }

  mGlobalNvsArea.Area->XhciMode = mSystemConfiguration.PchUsb30Mode;

  mGlobalNvsArea.Area->Stepping = mPlatformInfo->IchRevision;

  //
  // Override invalid Pre-Boot Driver and XhciMode combination.
  //
  if ((mSystemConfiguration.UsbXhciSupport == 0) && (mSystemConfiguration.PchUsb30Mode == 3)) {
    mGlobalNvsArea.Area->XhciMode = 2;
  }
  if ((mSystemConfiguration.UsbXhciSupport == 1) && (mSystemConfiguration.PchUsb30Mode == 2)) {
    mGlobalNvsArea.Area->XhciMode = 3;
  }

  DEBUG ((EFI_D_ERROR, "ACPI NVS XHCI:0x%x\n", mGlobalNvsArea.Area->XhciMode));

  mGlobalNvsArea.Area->PmicEnable                       = GLOBAL_NVS_DEVICE_DISABLE;
  mGlobalNvsArea.Area->BatteryChargingSolution          = GLOBAL_NVS_DEVICE_DISABLE;
  mGlobalNvsArea.Area->ISPDevSel                        = mSystemConfiguration.ISPDevSel;
  mGlobalNvsArea.Area->LpeEnable                        = mSystemConfiguration.Lpe;

  if (mSystemConfiguration.ISPEn == 0) {
    mGlobalNvsArea.Area->ISPDevSel                      = GLOBAL_NVS_DEVICE_DISABLE;
  }

  mGlobalNvsArea.Area->WittEnable                       = mSystemConfiguration.WittEnable;
  mGlobalNvsArea.Area->UtsEnable                        = mSystemConfiguration.UtsEnable;
  mGlobalNvsArea.Area->SarEnable                        = mSystemConfiguration.SAR1;


  mGlobalNvsArea.Area->ReservedO                        = 1;

  SettingI2CTouchAddress();
  mGlobalNvsArea.Area->IdleReserve= mSystemConfiguration.IdleReserve;
  //
  // Read BMBOUND and store it in GlobalNVS to pass into ASL.
  //
  // BUGBUG: code was moved into silicon reference code.
  //
  if (mSystemConfiguration.eMMCBootMode== 1) {
    //
    // Auto detect mode.
    //
    DEBUG ((EFI_D_ERROR, "Auto detect mode------------start\n"));

    //
    // Silicon Steppings.
    //
    switch (PchStepping()) {
      case PchA0: // A0/A1
      case PchA1:
        DEBUG ((EFI_D_ERROR, "SOC A0/A1: eMMC 4.41 Configuration\n"));
        mSystemConfiguration.LpsseMMCEnabled            = 1;
        mSystemConfiguration.LpsseMMC45Enabled          = 0;
        break;

      case PchB0: // B0 and later.
      default:
        DEBUG ((EFI_D_ERROR, "SOC B0 and later: eMMC 4.5 Configuration\n"));
        mSystemConfiguration.LpsseMMCEnabled            = 0;
        mSystemConfiguration.LpsseMMC45Enabled          = 1;
        break;
   }
  } else if (mSystemConfiguration.eMMCBootMode == 2) {
      //
      // eMMC 4.41
      //
      DEBUG ((EFI_D_ERROR, "Force to eMMC 4.41 Configuration\n"));
      mSystemConfiguration.LpsseMMCEnabled            = 1;
      mSystemConfiguration.LpsseMMC45Enabled          = 0;
  } else if (mSystemConfiguration.eMMCBootMode == 3) {
      //
      // eMMC 4.5
      //
      DEBUG ((EFI_D_ERROR, "Force to eMMC 4.5 Configuration\n"));
      mSystemConfiguration.LpsseMMCEnabled            = 0;
      mSystemConfiguration.LpsseMMC45Enabled          = 1;

  } else {
      //
      // Disable eMMC controllers.
      //
      DEBUG ((EFI_D_ERROR, "Disable eMMC controllers\n"));
      mSystemConfiguration.LpsseMMCEnabled            = 0;
      mSystemConfiguration.LpsseMMC45Enabled          = 0;
  }

  mGlobalNvsArea.Area->emmcVersion = 0;
  if (mSystemConfiguration.LpsseMMCEnabled) {
     DEBUG ((EFI_D_ERROR, "mGlobalNvsArea.Area->emmcVersion = 0\n"));
     mGlobalNvsArea.Area->emmcVersion = 0;
  }

  if (mSystemConfiguration.LpsseMMC45Enabled) {
     DEBUG ((EFI_D_ERROR, "mGlobalNvsArea.Area->emmcVersion = 1\n"));
     mGlobalNvsArea.Area->emmcVersion = 1;
  }

  mGlobalNvsArea.Area->SdCardRemovable = mSystemConfiguration.SdCardRemovable;
  
  //
  // Microsoft IOT
  //
  if ((mSystemConfiguration.LpssHsuart0FlowControlEnabled == 1) && \
      (mSystemConfiguration.LpssPwm0Enabled == 0) && \
      (mSystemConfiguration.LpssPwm1Enabled == 0)) {
    mGlobalNvsArea.Area->MicrosoftIoT = GLOBAL_NVS_DEVICE_ENABLE;
    DEBUG ((EFI_D_ERROR, "JP1 is set to be MSFT IOT configuration.\n"));
  } else {
    mGlobalNvsArea.Area->MicrosoftIoT = GLOBAL_NVS_DEVICE_DISABLE;
    DEBUG ((EFI_D_ERROR, "JP1 is not set to be MSFT IOT configuration.\n"));
  }
  
  //
  // SIO related option.
  //
  Status = gBS->LocateProtocol (&gEfiCpuIoProtocolGuid, NULL, (void **)&mCpuIo);
  ASSERT_EFI_ERROR (Status);

  mGlobalNvsArea.Area->WPCN381U = GLOBAL_NVS_DEVICE_DISABLE;

  mGlobalNvsArea.Area->DockedSioPresent = GLOBAL_NVS_DEVICE_DISABLE;

  if (mGlobalNvsArea.Area->DockedSioPresent != GLOBAL_NVS_DEVICE_ENABLE) {
    //
    // Check ID for SIO WPCN381U.
    //
    Status = mCpuIo->Io.Read (
                          mCpuIo,
                          EfiCpuIoWidthUint8,
                          WPCN381U_CONFIG_INDEX,
                          1,
                          &PortData
                          );
    ASSERT_EFI_ERROR (Status);
    if (PortData != 0xFF) {
      PortData = 0x20;
      Status = mCpuIo->Io.Write (
                            mCpuIo,
                            EfiCpuIoWidthUint8,
                            WPCN381U_CONFIG_INDEX,
                            1,
                            &PortData
                            );
      ASSERT_EFI_ERROR (Status);
      Status = mCpuIo->Io.Read (
                            mCpuIo,
                            EfiCpuIoWidthUint8,
                            WPCN381U_CONFIG_DATA,
                            1,
                            &PortData
                            );
      ASSERT_EFI_ERROR (Status);
      if ((PortData == WPCN381U_CHIP_ID) || (PortData == WDCP376_CHIP_ID)) {
        mGlobalNvsArea.Area->WPCN381U = GLOBAL_NVS_DEVICE_ENABLE;
        mGlobalNvsArea.Area->OnboardCom = GLOBAL_NVS_DEVICE_ENABLE;
        mGlobalNvsArea.Area->OnboardComCir = GLOBAL_NVS_DEVICE_DISABLE;
      }
    }
  }



  //
  // Get Ps2 policy to set. Will be use if present.
  //
  Status =  gBS->LocateProtocol (
                   &gEfiPs2PolicyProtocolGuid,
                   NULL,
                   (VOID **)&Ps2Policy
                   );
  if (!EFI_ERROR (Status)) {
          Status = Ps2Policy->Ps2InitHardware (ImageHandle);
  }

  mGlobalNvsArea.Area->SDIOMode = mSystemConfiguration.LpssSdioMode;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiGlobalNvsAreaProtocolGuid,
                  &mGlobalNvsArea,
                  NULL
                  );

  //
  // Read tables from the storage file.
  //
  while (!EFI_ERROR (Status)) {
    CurrentTable = NULL;

    Status = FwVol->ReadSection (
                      FwVol,
                      &gEfiAcpiTableStorageGuid,
                      EFI_SECTION_RAW,
                      Instance,
                      (VOID **) &CurrentTable,
                      (UINTN *) &Size,
                      &FvStatus
                      );

    if (!EFI_ERROR (Status)) {
      //
      // Allow platform specific code to reject the table or update it.
      //
      AcpiStatus = AcpiPlatformHooksIsActiveTable (CurrentTable);

      if (!EFI_ERROR (AcpiStatus)) {
        //
        // Perform any table specific updates.
        //
        AcpiStatus = PlatformUpdateTables (CurrentTable);
        if (!EFI_ERROR (AcpiStatus)) {
          //
          // Add the table.
          //
          TableHandle = 0;
          AcpiStatus = AcpiSupport->SetAcpiTable (
                                      AcpiSupport,
                                      CurrentTable,
                                      TRUE,
                                      TableVersion,
                                      &TableHandle
                                      );
          ASSERT_EFI_ERROR (AcpiStatus);
        }
      }

      //
      // Increment the instance.
      //
      Instance++;
    }
  }

  Status = EfiCreateEventReadyToBootEx (
             TPL_NOTIFY,
             OnReadyToBoot,
             NULL,
             &Event
             );

  //
  // Finished.
  //
  return EFI_SUCCESS;
}

UINT8
ReadCmosBank1Byte (
  IN  UINT8                           Index
  )
{
  UINT8                               Data;

  IoWrite8(0x72, Index);
  Data = IoRead8 (0x73);
  return Data;
}

VOID
WriteCmosBank1Byte (
  IN  UINT8                           Index,
  IN  UINT8                           Data
  )
{
  IoWrite8 (0x72, Index);
  IoWrite8 (0x73, Data);
}



VOID
SettingI2CTouchAddress (
  IN VOID
  )
{
  if (mSystemConfiguration.I2CTouchAd == 0) {
    //
    // If setup menu select auto set I2C Touch Address base on board id.
    //
    if (mPlatformInfo->BoardId == BOARD_ID_BL_RVP ||
        mPlatformInfo->BoardId == BOARD_ID_BL_STHI ||
        mPlatformInfo->BoardId == BOARD_ID_BL_RVP_DDR3L ) {
      //
      //RVP
      //
      mGlobalNvsArea.Area->I2CTouchAddress = 0x4B;
    } else if (mPlatformInfo->BoardId == BOARD_ID_BL_FFRD) {
      //
      //FFRD
      //
      mGlobalNvsArea.Area->I2CTouchAddress = 0x4A;
    } else if (mPlatformInfo->BoardId == BOARD_ID_BB_RVP) {
      mGlobalNvsArea.Area->I2CTouchAddress = 0x4C;
    } else if (mPlatformInfo->BoardId == BOARD_ID_CVH) {
      mGlobalNvsArea.Area->I2CTouchAddress = 0x4C;
    } else if (mPlatformInfo->BoardId == BOARD_ID_BL_FFRD8) {
      //
      //FFRD8 uses 0x4A.
      //
      mGlobalNvsArea.Area->I2CTouchAddress = 0x4A;
    }
  } else {
    mGlobalNvsArea.Area->I2CTouchAddress = mSystemConfiguration.I2CTouchAd;
  }
  DEBUG((EFI_D_ERROR, "GlobalNvsArea.Area->I2CTouchAddress: [%02x]\n", mGlobalNvsArea.Area->I2CTouchAddress));
}


