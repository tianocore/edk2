/** @file
This file contains Madt Talbe initialized work.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

//
// Statements that include other files
//

#include "AcpiPlatform.h"

VOID
InitMadtConfigData (MADT_CONFIG_DATA *mConfigData)
{
  mConfigData->MadtInterruptSetting[0].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable0Enable);
  mConfigData->MadtInterruptSetting[0].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable0SourceIrq);
  mConfigData->MadtInterruptSetting[0].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable0Polarity);
  mConfigData->MadtInterruptSetting[0].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable0TrigerMode);
  mConfigData->MadtInterruptSetting[0].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable0GlobalIrq);

  mConfigData->MadtInterruptSetting[1].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable1Enable);
  mConfigData->MadtInterruptSetting[1].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable1SourceIrq);
  mConfigData->MadtInterruptSetting[1].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable1Polarity);
  mConfigData->MadtInterruptSetting[1].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable1TrigerMode);
  mConfigData->MadtInterruptSetting[1].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable1GlobalIrq);

  mConfigData->MadtInterruptSetting[2].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable2Enable);
  mConfigData->MadtInterruptSetting[2].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable2SourceIrq);
  mConfigData->MadtInterruptSetting[2].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable2Polarity);
  mConfigData->MadtInterruptSetting[2].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable2TrigerMode);
  mConfigData->MadtInterruptSetting[2].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable2GlobalIrq);

  mConfigData->MadtInterruptSetting[3].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable3Enable);
  mConfigData->MadtInterruptSetting[3].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable3SourceIrq);
  mConfigData->MadtInterruptSetting[3].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable3Polarity);
  mConfigData->MadtInterruptSetting[3].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable3TrigerMode);
  mConfigData->MadtInterruptSetting[3].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable3GlobalIrq);

  mConfigData->MadtInterruptSetting[4].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable4Enable);
  mConfigData->MadtInterruptSetting[4].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable4SourceIrq);
  mConfigData->MadtInterruptSetting[4].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable4Polarity);
  mConfigData->MadtInterruptSetting[4].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable4TrigerMode);
  mConfigData->MadtInterruptSetting[4].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable4GlobalIrq);

  mConfigData->MadtInterruptSetting[5].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable5Enable);
  mConfigData->MadtInterruptSetting[5].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable5SourceIrq);
  mConfigData->MadtInterruptSetting[5].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable5Polarity);
  mConfigData->MadtInterruptSetting[5].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable5TrigerMode);
  mConfigData->MadtInterruptSetting[5].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable5GlobalIrq);

  mConfigData->MadtInterruptSetting[6].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable6Enable);
  mConfigData->MadtInterruptSetting[6].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable6SourceIrq);
  mConfigData->MadtInterruptSetting[6].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable6Polarity);
  mConfigData->MadtInterruptSetting[6].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable6TrigerMode);
  mConfigData->MadtInterruptSetting[6].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable6GlobalIrq);

  mConfigData->MadtInterruptSetting[7].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable7Enable);
  mConfigData->MadtInterruptSetting[7].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable7SourceIrq);
  mConfigData->MadtInterruptSetting[7].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable7Polarity);
  mConfigData->MadtInterruptSetting[7].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable7TrigerMode);
  mConfigData->MadtInterruptSetting[7].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable7GlobalIrq);

  mConfigData->MadtInterruptSetting[8].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable8Enable);
  mConfigData->MadtInterruptSetting[8].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable8SourceIrq);
  mConfigData->MadtInterruptSetting[8].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable8Polarity);
  mConfigData->MadtInterruptSetting[8].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable8TrigerMode);
  mConfigData->MadtInterruptSetting[8].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable8GlobalIrq);

  mConfigData->MadtInterruptSetting[9].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable9Enable);
  mConfigData->MadtInterruptSetting[9].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable9SourceIrq);
  mConfigData->MadtInterruptSetting[9].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable9Polarity);
  mConfigData->MadtInterruptSetting[9].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable9TrigerMode);
  mConfigData->MadtInterruptSetting[9].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable9GlobalIrq);

  mConfigData->MadtInterruptSetting[10].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable10Enable);
  mConfigData->MadtInterruptSetting[10].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable10SourceIrq);
  mConfigData->MadtInterruptSetting[10].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable10Polarity);
  mConfigData->MadtInterruptSetting[10].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable10TrigerMode);
  mConfigData->MadtInterruptSetting[10].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable10GlobalIrq);

  mConfigData->MadtInterruptSetting[11].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable11Enable);
  mConfigData->MadtInterruptSetting[11].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable11SourceIrq);
  mConfigData->MadtInterruptSetting[11].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable11Polarity);
  mConfigData->MadtInterruptSetting[11].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable11TrigerMode);
  mConfigData->MadtInterruptSetting[11].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable11GlobalIrq);

  mConfigData->MadtInterruptSetting[12].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable12Enable);
  mConfigData->MadtInterruptSetting[12].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable12SourceIrq);
  mConfigData->MadtInterruptSetting[12].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable12Polarity);
  mConfigData->MadtInterruptSetting[12].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable12TrigerMode);
  mConfigData->MadtInterruptSetting[12].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable12GlobalIrq);

  mConfigData->MadtInterruptSetting[13].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable13Enable);
  mConfigData->MadtInterruptSetting[13].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable13SourceIrq);
  mConfigData->MadtInterruptSetting[13].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable13Polarity);
  mConfigData->MadtInterruptSetting[13].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable13TrigerMode);
  mConfigData->MadtInterruptSetting[13].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable13GlobalIrq);

  mConfigData->MadtInterruptSetting[14].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable14Enable);
  mConfigData->MadtInterruptSetting[14].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable14SourceIrq);
  mConfigData->MadtInterruptSetting[14].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable14Polarity);
  mConfigData->MadtInterruptSetting[14].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable14TrigerMode);
  mConfigData->MadtInterruptSetting[14].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable14GlobalIrq);

  mConfigData->MadtInterruptSetting[15].Enable     = PcdGet8 (PcdInterruptOverrideSettingTable15Enable);
  mConfigData->MadtInterruptSetting[15].SourceIrq  = PcdGet8 (PcdInterruptOverrideSettingTable15SourceIrq);
  mConfigData->MadtInterruptSetting[15].Polarity   = PcdGet8 (PcdInterruptOverrideSettingTable15Polarity);
  mConfigData->MadtInterruptSetting[15].TrigerMode = PcdGet8 (PcdInterruptOverrideSettingTable15TrigerMode);
  mConfigData->MadtInterruptSetting[15].GlobalIrq  = PcdGet32 (PcdInterruptOverrideSettingTable15GlobalIrq);

  mConfigData->MadtIoApicSetting.IoApicAddress       = (UINT32)PcdGet64(PcdIoApicBaseAddress);
  mConfigData->MadtIoApicSetting.GlobalInterruptBase = PcdGet32 (PcdIoApicSettingGlobalInterruptBase);
  mConfigData->MadtIoApicSetting.IoApicId            = PcdGet8 (PcdIoApicSettingIoApicId);
  mConfigData->MadtIoApicSetting.NmiEnable           = PcdGet8 (PcdIoApicSettingNmiEnable);
  mConfigData->MadtIoApicSetting.NmiSource           = PcdGet8 (PcdIoApicSettingNmiSource);
  mConfigData->MadtIoApicSetting.Polarity            = PcdGet8 (PcdIoApicSettingPolarity);
  mConfigData->MadtIoApicSetting.TrigerMode          = PcdGet8 (PcdIoApicSettingTrigerMode);

  mConfigData->MadtLocalApicSetting.NmiEnabelApicIdMask   = PcdGet8 (PcdLocalApicSettingNmiEnabelApicIdMask);
  mConfigData->MadtLocalApicSetting.AddressOverrideEnable = PcdGet8 (PcdLocalApicSettingAddressOverrideEnable);
  mConfigData->MadtLocalApicSetting.Polarity              = PcdGet8 (PcdLocalApicSettingPolarity);
  mConfigData->MadtLocalApicSetting.TrigerMode            = PcdGet8 (PcdLocalApicSettingTrigerMode);
  mConfigData->MadtLocalApicSetting.LocalApicLint         = PcdGet8 (PcdLocalApicSettingLocalApicLint);
  mConfigData->MadtLocalApicSetting.LocalApicAddressOverride      = PcdGet64 (PcdLocalApicAddressOverride);
  mConfigData->MadtLocalApicSetting.LocalApicAddress              = PcdGet32 (PcdCpuLocalApicBaseAddress);
}
UINT32
GetAcutalMadtTableSize (
  IN MADT_CONFIG_DATA * MadtConfigData,
  IN INTN               NumberOfCPUs
  )
{
  UINT32 MadtSize;
  UINT8  Index;
  MadtSize = (UINT32)(sizeof (EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER) +
             sizeof (EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE) * NumberOfCPUs +
             sizeof (EFI_ACPI_2_0_IO_APIC_STRUCTURE) +
             sizeof (EFI_ACPI_2_0_LOCAL_APIC_ADDRESS_OVERRIDE_STRUCTURE) * (MadtConfigData->MadtLocalApicSetting.AddressOverrideEnable != 0?1:0)
             );
  for (Index = 0; Index < EFI_ACPI_INTERRUPT_SOURCE_OVERRIDE_COUNT_MAX; Index ++ ) {
    if (MadtConfigData->MadtInterruptSetting[Index].Enable != 0) {
      MadtSize += sizeof (EFI_ACPI_2_0_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE);
    }
  }
  for (Index = 0; Index < NumberOfCPUs; Index ++ ) {
    if (0 != (MadtConfigData->MadtLocalApicSetting.NmiEnabelApicIdMask & (1 << Index))) {
      MadtSize += sizeof (EFI_ACPI_2_0_LOCAL_APIC_NMI_STRUCTURE);
    }
  }
  if (0 != MadtConfigData->MadtIoApicSetting.NmiEnable) {
    MadtSize += sizeof (EFI_ACPI_2_0_NON_MASKABLE_INTERRUPT_SOURCE_STRUCTURE);
  }
  return MadtSize;
}

//
// Init Multiple APIC Description Table
//
EFI_STATUS
MadtTableInitialize (
  OUT   EFI_ACPI_COMMON_HEADER  **MadtTable,
  OUT   UINTN                   *Size
  )
{
  EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *Madt;
  EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE         *ProcLocalApic;
  EFI_ACPI_2_0_IO_APIC_STRUCTURE                      *IoApic;
  EFI_ACPI_2_0_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE    *InterruptSourceOverride;
  EFI_ACPI_2_0_NON_MASKABLE_INTERRUPT_SOURCE_STRUCTURE  *IoApicNmiSource;
  EFI_ACPI_2_0_LOCAL_APIC_NMI_STRUCTURE                 *LocalApicNmiSource;
  EFI_ACPI_2_0_LOCAL_APIC_ADDRESS_OVERRIDE_STRUCTURE    *LocalApicAddressOverride;

  EFI_MP_SERVICES_PROTOCOL                            *MpService;
  UINTN                                               NumberOfCPUs;
  UINTN                                               NumberOfEnabledCPUs;
  MADT_CONFIG_DATA                                    MadtConfigData;

  UINT32      MadtSize;
  UINTN       Index;
  EFI_STATUS  Status;


  ASSERT (NULL != MadtTable);
  ASSERT (NULL != Size);
  //
  // Init Madt table data
  //
  InitMadtConfigData (&MadtConfigData);
  //
  // Find the MP Protocol. This is an MP platform, so MP protocol must be
  // there.
  //
  Status = gBS->LocateProtocol (
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  (VOID **)&MpService
                  );
  ASSERT_EFI_ERROR (Status);
  //
  // Determine the number of processors
  //
  MpService->GetNumberOfProcessors (
              MpService,
              &NumberOfCPUs,
              &NumberOfEnabledCPUs
              );
  //ASSERT (NumberOfCPUs <= 2 && NumberOfCPUs > 0);
  MadtSize = GetAcutalMadtTableSize (&MadtConfigData, NumberOfCPUs);
  Madt = (EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)AllocateZeroPool (MadtSize);
  ASSERT (Madt != NULL);
  //
  // Initialize MADT Header information
  //
  Madt->Header.Signature    = EFI_ACPI_2_0_MULTIPLE_SAPIC_DESCRIPTION_TABLE_SIGNATURE;
  Madt->Header.Length       = MadtSize;
  Madt->Header.Revision     = EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION;
  Madt->Header.OemTableId   = EFI_ACPI_OEM_TABLE_ID;
  Madt->Header.OemRevision  = EFI_ACPI_OEM_MADT_REVISION;
  Madt->Header.CreatorId    = EFI_ACPI_CREATOR_ID;
  Madt->LocalApicAddress    = MadtConfigData.MadtLocalApicSetting.LocalApicAddress;
  Madt->Flags               = EFI_ACPI_2_0_MULTIPLE_APIC_FLAGS;
  CopyMem (Madt->Header.OemId, EFI_ACPI_OEM_ID, 6);

  ProcLocalApic = (EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE *) (Madt + 1);
  //
  // Initialization of Processor's local APICs
  //
  for (Index = 0;Index < NumberOfCPUs; Index++) {
    ProcLocalApic[Index].Type             = EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC;
    ProcLocalApic[Index].Length           = sizeof (EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE);
    ProcLocalApic[Index].AcpiProcessorId  = (UINT8)(Index + 1);
    ProcLocalApic[Index].ApicId           = 0xff;
    ProcLocalApic[Index].Flags            = 0;
  }
  //
  // Initialization of IO APIC.
  // Note: Here assumes that there must be one and only one IO APIC in platform.
  //
  IoApic = (EFI_ACPI_2_0_IO_APIC_STRUCTURE *) (&ProcLocalApic[Index]);
  IoApic->Type                      = EFI_ACPI_2_0_IO_APIC;
  IoApic->Length                    = sizeof (EFI_ACPI_2_0_IO_APIC_STRUCTURE);
  IoApic->IoApicId                  = MadtConfigData.MadtIoApicSetting.IoApicId;
  IoApic->IoApicAddress             = MadtConfigData.MadtIoApicSetting.IoApicAddress;
  IoApic->GlobalSystemInterruptBase = MadtConfigData.MadtIoApicSetting.GlobalInterruptBase;

  InterruptSourceOverride = (EFI_ACPI_2_0_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE *) (IoApic + 1);
  for (Index = 0;Index < EFI_ACPI_INTERRUPT_SOURCE_OVERRIDE_COUNT_MAX; Index++ ){
    if (MadtConfigData.MadtInterruptSetting[Index].Enable) {
      InterruptSourceOverride->Type   = EFI_ACPI_2_0_INTERRUPT_SOURCE_OVERRIDE;
      InterruptSourceOverride->Length = sizeof (EFI_ACPI_2_0_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE);
      InterruptSourceOverride->Bus    = 0;
      InterruptSourceOverride->Source = MadtConfigData.MadtInterruptSetting[Index].SourceIrq;
      InterruptSourceOverride->Flags  = ((MadtConfigData.MadtInterruptSetting[Index].TrigerMode & 0x03) << 2) | (MadtConfigData.MadtInterruptSetting[Index].Polarity & 0x03);
      InterruptSourceOverride->GlobalSystemInterrupt  = MadtConfigData.MadtInterruptSetting[Index].GlobalIrq;
      InterruptSourceOverride++;
    }
  }
  //
  // support NMI source configuration.
  //
  IoApicNmiSource = (EFI_ACPI_2_0_NON_MASKABLE_INTERRUPT_SOURCE_STRUCTURE *) InterruptSourceOverride;
  if ((BOOLEAN) MadtConfigData.MadtIoApicSetting.NmiEnable) {
    IoApicNmiSource->Type   = EFI_ACPI_2_0_NON_MASKABLE_INTERRUPT_SOURCE;
    IoApicNmiSource->Length = sizeof (EFI_ACPI_2_0_NON_MASKABLE_INTERRUPT_SOURCE_STRUCTURE);
    IoApicNmiSource->Flags  = ((MadtConfigData.MadtIoApicSetting.TrigerMode & 0x03) << 2) | (MadtConfigData.MadtIoApicSetting.Polarity & 0x03);
    IoApicNmiSource->GlobalSystemInterrupt = MadtConfigData.MadtIoApicSetting.NmiSource;
    IoApicNmiSource ++;
  }
  //
  // Assume each processor has same NMI interrupt source.
  //
  LocalApicNmiSource = (EFI_ACPI_2_0_LOCAL_APIC_NMI_STRUCTURE *) IoApicNmiSource;
  for (Index = 0;Index < NumberOfCPUs; Index++) {
    if (0 != (MadtConfigData.MadtLocalApicSetting.NmiEnabelApicIdMask & (1 << Index))){
      LocalApicNmiSource->Type          = EFI_ACPI_2_0_LOCAL_APIC_NMI;
      LocalApicNmiSource->Length        = sizeof (EFI_ACPI_2_0_LOCAL_APIC_NMI_STRUCTURE);
      LocalApicNmiSource->LocalApicLint = MadtConfigData.MadtLocalApicSetting.LocalApicLint;
      LocalApicNmiSource->Flags         = ((MadtConfigData.MadtLocalApicSetting.TrigerMode & 0x03) << 2) | (MadtConfigData.MadtLocalApicSetting.Polarity & 0x03);
      LocalApicNmiSource->AcpiProcessorId = (UINT8)(Index + 1);
      LocalApicNmiSource++;
    }
  }

  LocalApicAddressOverride = (EFI_ACPI_2_0_LOCAL_APIC_ADDRESS_OVERRIDE_STRUCTURE *) LocalApicNmiSource;
  if ((BOOLEAN) MadtConfigData.MadtLocalApicSetting.AddressOverrideEnable) {
    LocalApicAddressOverride->Type    = EFI_ACPI_2_0_LOCAL_APIC_ADDRESS_OVERRIDE;
    LocalApicAddressOverride->Length  = sizeof (EFI_ACPI_2_0_LOCAL_APIC_ADDRESS_OVERRIDE_STRUCTURE);
    LocalApicAddressOverride->LocalApicAddress = MadtConfigData.MadtLocalApicSetting.LocalApicAddressOverride;
    LocalApicAddressOverride++;
  }
  *Size       = MadtSize;
  *MadtTable  = (EFI_ACPI_COMMON_HEADER *) Madt;

  return EFI_SUCCESS;
}
