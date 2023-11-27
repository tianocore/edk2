/** @file
  FIT Load Image Support
Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <UniversalPayload/UniversalPayload.h>
#include <UniversalPayload/DeviceTree.h>
#include <Guid/UniversalPayloadBase.h>
#include <Guid/GraphicsInfoHob.h>
#include <UniversalPayload/ExtraData.h>
#include <UniversalPayload/DeviceTree.h>
#include <Ppi/LoadFile.h>

#include <Library/PciHostBridgeLib.h>
#include <Protocol/DevicePath.h>
#include <UniversalPayload/SerialPortInfo.h>
#include <UniversalPayload/PciRootBridges.h>
#include <UniversalPayload/AcpiTable.h>
#include <UniversalPayload/SmbiosTable.h>
#include <IndustryStandard/SmBios.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FdtLib.h>
#include <Library/PrintLib.h>
#include "FitLib.h"
#define STACK_SIZE  0x20000

CONST EFI_PEI_PPI_DESCRIPTOR  gReadyToPayloadSignalPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiReadyToPayloadPpiGuid,
  NULL
};

/**
  Notify ReadyToPayLoad signal.
  @param[in] PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param[in] NotifyDescriptor  Address of the notification descriptor data structure.
  @param[in] Ppi               Address of the PPI that was installed.
  @retval EFI_SUCCESS          Hobs data is discovered.
  @return Others               No Hobs data is discovered.
**/
EFI_STATUS
EFIAPI
EndOfPeiPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS  Status;

  //
  // Ready to Payload phase signal
  //
  Status = PeiServicesInstallPpi (&gReadyToPayloadSignalPpi);

  return Status;
}

EFI_PEI_NOTIFY_DESCRIPTOR  mEndOfPeiNotifyList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiEndOfPeiSignalPpiGuid,
    EndOfPeiPpiNotifyCallback
  }
};

EFI_PEI_PPI_DESCRIPTOR  mEndOfPeiSignalPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  NULL
};

#define MEMORY_ATTRIBUTE_DEFAULT  (EFI_RESOURCE_ATTRIBUTE_PRESENT                   | \
                                     EFI_RESOURCE_ATTRIBUTE_INITIALIZED             | \
                                     EFI_RESOURCE_ATTRIBUTE_TESTED                  | \
                                     EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE             | \
                                     EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE       | \
                                     EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE | \
                                     EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE    )

CHAR8  *mMemoryAllocType[] = {
  "Reserved",
  "LoaderCode",
  "LoaderData",
  "boot-code",
  "boot-data",
  "runtime-code",
  "runtime-data",
  "ConventionalMemory",
  "UnusableMemory",
  "acpi",
  "acpi-nvs",
  "mmio",
  "MemoryMappedIOPortSpace",
  "PalCode",
  "PersistentMemory",
};

/**
  The wrapper function of PeiLoadImageLoadImage().
  @param This            - Pointer to EFI_PEI_LOAD_FILE_PPI.
  @param FileHandle      - Pointer to the FFS file header of the image.
  @param ImageAddressArg - Pointer to PE/TE image.
  @param ImageSizeArg    - Size of PE/TE image.
  @param EntryPoint      - Pointer to entry point of specified image file for output.
  @param AuthenticationState - Pointer to attestation authentication state of image.
  @return Status of PeiLoadImageLoadImage().
**/
EFI_STATUS
EFIAPI
PeiLoadFileLoadPayload (
  IN     CONST EFI_PEI_LOAD_FILE_PPI  *This,
  IN     EFI_PEI_FILE_HANDLE          FileHandle,
  OUT    EFI_PHYSICAL_ADDRESS         *ImageAddressArg   OPTIONAL,
  OUT    UINT64                       *ImageSizeArg      OPTIONAL,
  OUT    EFI_PHYSICAL_ADDRESS         *EntryPoint,
  OUT    UINT32                       *AuthenticationState
  )
{
  EFI_STATUS              Status;
  FIT_IMAGE_CONTEXT       Context;
  UINTN                   Instance;
  VOID                    *Binary;
  FIT_RELOCATE_ITEM       *RelocateTable;
  UNIVERSAL_PAYLOAD_BASE  *PayloadBase;
  UINTN                   Length;
  UINTN                   Delta;
  UINTN                   Index;

 #if (FixedPcdGetBool (PcdHandOffFdtEnable))
  VOID                           *BaseOfStack;
  VOID                           *TopOfStack;
  UNIVERSAL_PAYLOAD_DEVICE_TREE  *Fdt;
  VOID                           *Hob;

  Fdt = NULL;
 #endif

  Instance = 0;
  do {
    Status = PeiServicesFfsFindSectionData3 (EFI_SECTION_RAW, Instance++, FileHandle, &Binary, AuthenticationState);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    ZeroMem (&Context, sizeof (Context));
    Status = ParseFitImage (Binary, &Context);
  } while (EFI_ERROR (Status));

  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  DEBUG (
    (
     DEBUG_INFO,
     "Before Rebase Payload File Base: 0x%08x, File Size: 0x%08X, EntryPoint: 0x%08x\n",
     Context.PayloadBaseAddress,
     Context.PayloadSize,
     Context.PayloadEntryPoint
    )
    );
  Context.PayloadBaseAddress = (EFI_PHYSICAL_ADDRESS)AllocatePages (EFI_SIZE_TO_PAGES (Context.PayloadSize));

  RelocateTable = (FIT_RELOCATE_ITEM *)(UINTN)(Context.PayloadBaseAddress + Context.RelocateTableOffset);
  CopyMem ((VOID *)Context.PayloadBaseAddress, Binary, Context.PayloadSize);

  if (Context.PayloadBaseAddress > Context.PayloadLoadAddress) {
    Delta                      = Context.PayloadBaseAddress - Context.PayloadLoadAddress;
    Context.PayloadEntryPoint += Delta;
    for (Index = 0; Index < Context.RelocateTableCount; Index++) {
      if ((RelocateTable[Index].RelocateType == 10) || (RelocateTable[Index].RelocateType == 3)) {
        *((UINT64 *)(Context.PayloadBaseAddress + RelocateTable[Index].Offset)) = *((UINT64 *)(Context.PayloadBaseAddress + RelocateTable[Index].Offset)) + Delta;
      }
    }
  } else {
    Delta                      = Context.PayloadLoadAddress - Context.PayloadBaseAddress;
    Context.PayloadEntryPoint -= Delta;
    for (Index = 0; Index < Context.RelocateTableCount; Index++) {
      if ((RelocateTable[Index].RelocateType == 10) || (RelocateTable[Index].RelocateType == 3)) {
        *((UINT64 *)(Context.PayloadBaseAddress + RelocateTable[Index].Offset)) = *((UINT64 *)(Context.PayloadBaseAddress + RelocateTable[Index].Offset)) - Delta;
      }
    }
  }

  DEBUG (
    (
     DEBUG_INFO,
     "After Rebase Payload File Base: 0x%08x, File Size: 0x%08X, EntryPoint: 0x%08x\n",
     Context.PayloadBaseAddress,
     Context.PayloadSize,
     Context.PayloadEntryPoint
    )
    );

  Length      = sizeof (UNIVERSAL_PAYLOAD_BASE);
  PayloadBase = BuildGuidHob (
                  &gUniversalPayloadBaseGuid,
                  Length
                  );
  PayloadBase->Entry = (EFI_PHYSICAL_ADDRESS)Context.ImageBase;

  *ImageAddressArg = Context.PayloadBaseAddress;
  *ImageSizeArg    = Context.PayloadSize;
  *EntryPoint      = Context.PayloadEntryPoint;

  Status = PeiServicesNotifyPpi (&mEndOfPeiNotifyList[0]);
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesInstallPpi (&mEndOfPeiSignalPpi);
  ASSERT_EFI_ERROR (Status);

 #if (FixedPcdGetBool (PcdHandOffFdtEnable))
  Hob = GetFirstGuidHob (&gUniversalPayloadDeviceTreeGuid);
  if (Hob != NULL) {
    Fdt =  (UNIVERSAL_PAYLOAD_DEVICE_TREE *)GET_GUID_HOB_DATA (Hob);
  }

  //
  // Allocate 128KB for the Stack
  //
  BaseOfStack = AllocatePages (EFI_SIZE_TO_PAGES (STACK_SIZE));
  ASSERT (BaseOfStack != NULL);

  //
  // Compute the top of the stack we were allocated. Pre-allocate a UINTN
  // for safety.
  //
  TopOfStack = (VOID *)((UINTN)BaseOfStack + EFI_SIZE_TO_PAGES (STACK_SIZE) * EFI_PAGE_SIZE - CPU_STACK_ALIGNMENT);
  TopOfStack = ALIGN_POINTER (TopOfStack, CPU_STACK_ALIGNMENT);

  //
  // Transfer the control to the entry point of UniveralPayloadEntry.
  //
  SwitchStack (
    (SWITCH_STACK_ENTRY_POINT)(UINTN)Context.PayloadEntryPoint,
    (VOID *)(Fdt->DeviceTreeAddress),
    NULL,
    TopOfStack
    );
 #endif

  return EFI_SUCCESS;
}

EFI_PEI_LOAD_FILE_PPI  mPeiLoadFilePpi = {
  PeiLoadFileLoadPayload
};

EFI_PEI_PPI_DESCRIPTOR  gPpiLoadFilePpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiLoadFilePpiGuid,
  &mPeiLoadFilePpi
};

#if (FixedPcdGetBool (PcdHandOffFdtEnable))

/**
  Discover Hobs data and report data into a FDT.
  @param[in] PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param[in] NotifyDescriptor  Address of the notification descriptor data structure.
  @param[in] Ppi               Address of the PPI that was installed.
  @retval EFI_SUCCESS          Hobs data is discovered.
  @return Others               No Hobs data is discovered.
**/
EFI_STATUS
EFIAPI
FdtPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR  mReadyToPayloadNotifyList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiReadyToPayloadPpiGuid,
    FdtPpiNotifyCallback
  }
};
#endif

/**
  Print FDT data.
  @param[in] FdtBase         Address of the Fdt data.
**/
VOID
PrintFdt (
  IN     VOID  *FdtBase
  )
{
  UINT8   *Fdt;
  UINT32  i;

  Fdt = NULL;
  i   = 0;

  DEBUG ((DEBUG_ERROR, "FDT DTB data:"));
  for (Fdt = FdtBase, i = 0; i < Fdt32ToCpu (((FDT_HEADER *)FdtBase)->TotalSize); i++, Fdt++) {
    if (i % 16 == 0) {
      DEBUG ((DEBUG_ERROR, "\n"));
    }

    DEBUG ((DEBUG_ERROR, "%02x ", *Fdt));
  }

  DEBUG ((DEBUG_ERROR, "\n"));
}

/**
  It will build FDT based on memory information from Hobs.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForMemory (
  IN     VOID  *FdtBase
  )
{
  EFI_STATUS                   Status;
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  VOID                         *HobStart;
  VOID                         *Fdt;
  INT32                        TempNode;
  CHAR8                        TempStr[32];
  UINT64                       RegTmp[2];

  Fdt = FdtBase;

  HobStart = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  //
  // Scan resource descriptor hobs to set memory nodes
  //
  for (Hob.Raw = HobStart; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      ResourceHob = Hob.ResourceDescriptor;
      // Memory
      if (ResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
        // DEBUG ((DEBUG_ERROR, "Found hob for memory: base %016lX  length %016lX\n", ResourceHob->PhysicalStart, ResourceHob->ResourceLength));

        Status   = AsciiSPrint (TempStr, sizeof (TempStr), "memory@%lX", ResourceHob->PhysicalStart);
        TempNode = FdtAddSubnode (Fdt, 0, TempStr);
        ASSERT (TempNode > 0);

        RegTmp[0] = CpuToFdt64 (ResourceHob->PhysicalStart);
        RegTmp[1] = CpuToFdt64 (ResourceHob->ResourceLength);
        Status    = FdtSetProp (Fdt, TempNode, "reg", &RegTmp, sizeof (RegTmp));
        ASSERT_EFI_ERROR (Status);

        Status = FdtSetProp (Fdt, TempNode, "device_type", "memory", (UINT32)(AsciiStrLen ("memory")+1));
        ASSERT_EFI_ERROR (Status);
      }
    }
  }

  return Status;
}

/**
  It will build FDT based on memory allocation information from Hobs.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForMemAlloc (
  IN     VOID  *FdtBase
  )
{
  EFI_STATUS            Status;
  EFI_PEI_HOB_POINTERS  Hob;
  VOID                  *HobStart;
  VOID                  *Fdt;
  INT32                 ParentNode;
  INT32                 TempNode;
  CHAR8                 TempStr[32];
  UINT64                RegTmp[2];
  UINT32                AllocMemType;
  EFI_GUID              *AllocMemName;
  UINT8                 IsStackHob;
  UINT8                 IsBspStore;
  UINT32                Data32;
  // UNIVERSAL_PAYLOAD_SMBIOS_TABLE  *SmbiosTable;
  EFI_HOB_GUID_TYPE             *GuidHob;
  UNIVERSAL_PAYLOAD_ACPI_TABLE  *AcpiTable;

  Fdt = FdtBase;

  ParentNode = FdtAddSubnode (Fdt, 0, "reserved-memory");
  ASSERT (ParentNode > 0);

  Data32 = CpuToFdt32 (2);
  Status = FdtSetProp (Fdt, ParentNode, "#address-cells", &Data32, sizeof (UINT32));
  Status = FdtSetProp (Fdt, ParentNode, "#size-cells", &Data32, sizeof (UINT32));

  HobStart = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  //
  // Scan memory allocation hobs to set memory type
  //
  for (Hob.Raw = HobStart; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      AllocMemName = NULL;
      IsStackHob   = 0;
      IsBspStore   = 0;
      if (CompareGuid (&(Hob.MemoryAllocationModule->MemoryAllocationHeader.Name), &gEfiHobMemoryAllocModuleGuid)) {
        continue;
      } else if (IsZeroGuid (&(Hob.MemoryAllocationModule->MemoryAllocationHeader.Name)) == FALSE) {
        AllocMemName = &(Hob.MemoryAllocationModule->MemoryAllocationHeader.Name);

        if (CompareGuid (AllocMemName, &gEfiHobMemoryAllocStackGuid)) {
          IsStackHob = 1;
        } else if (CompareGuid (AllocMemName, &gEfiHobMemoryAllocBspStoreGuid)) {
          IsBspStore = 1;
        }
      }

      DEBUG ((
        DEBUG_ERROR,
        "Found hob for rsvd memory alloc: base %016lX  length %016lX  type %x\n",
        Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,
        Hob.MemoryAllocation->AllocDescriptor.MemoryLength,
        Hob.MemoryAllocation->AllocDescriptor.MemoryType
        ));

      AllocMemType = Hob.MemoryAllocation->AllocDescriptor.MemoryType;
      if (IsStackHob == 1) {
        Status = AsciiSPrint (
                   TempStr,
                   sizeof (TempStr),
                   "%a@%lX",
                   "stackhob",
                   Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress
                   );
      } else if (IsBspStore == 1) {
        Status = AsciiSPrint (
                   TempStr,
                   sizeof (TempStr),
                   "%a@%lX",
                   "bspstore",
                   Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress
                   );
      } else {
        Status = AsciiSPrint (
                   TempStr,
                   sizeof (TempStr),
                   "%a@%lX",
                   mMemoryAllocType[AllocMemType],
                   Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress
                   );
      }

      if (AsciiStrCmp (mMemoryAllocType[AllocMemType], "ConventionalMemory") == 0) {
        continue;
      }

      if (AsciiStrCmp (mMemoryAllocType[AllocMemType], "acpi") == 0) {
        GuidHob = GetFirstGuidHob (&gUniversalPayloadAcpiTableGuid);
        if (GuidHob != NULL) {
          AcpiTable = (UNIVERSAL_PAYLOAD_ACPI_TABLE *)GET_GUID_HOB_DATA (GuidHob);
          RegTmp[0] = CpuToFdt64 ((UINTN)AcpiTable->Rsdp);
          RegTmp[1] = CpuToFdt64 (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress);
        }

        DEBUG ((DEBUG_INFO, "To build acpi memory FDT , Rsdp :%x, MemoryBaseAddress :%x\n", RegTmp[0], RegTmp[1]));
        if (RegTmp[0] != RegTmp[1]) {
          DEBUG ((DEBUG_INFO, "Not Match , skip \n"));
          continue;
        }

        DEBUG ((DEBUG_INFO, "To build acpi memory FDT \n"));
      }

      if (AsciiStrCmp (mMemoryAllocType[AllocMemType], "mmio") == 0) {
        Status = AsciiSPrint (TempStr, sizeof (TempStr), "mmio@%lX", Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress);
      } else {
        Status = AsciiSPrint (TempStr, sizeof (TempStr), "memory@%lX", Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress);
      }

      TempNode = FdtAddSubnode (Fdt, ParentNode, TempStr);
      DEBUG ((DEBUG_INFO, "FdtAddSubnode %x", TempNode));
      if (TempNode < 0) {
        continue;
      }

      RegTmp[0] = CpuToFdt64 (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress);
      RegTmp[1] = CpuToFdt64 (Hob.MemoryAllocation->AllocDescriptor.MemoryLength);
      Status    = FdtSetProp (Fdt, TempNode, "reg", &RegTmp, sizeof (RegTmp));
      ASSERT_EFI_ERROR (Status);

      // if (!(AsciiStrCmp (mMemoryAllocType[AllocMemType], "mmio") == 0)) {
      //  Status = FdtSetProp (Fdt, TempNode, "compatible", mMemoryAllocType[AllocMemType], (UINT32)(AsciiStrLen (mMemoryAllocType[AllocMemType])+1));
      //  ASSERT_EFI_ERROR (Status);
      // }

      if (AllocMemName != NULL) {
        AllocMemName->Data1 = CpuToFdt32 (AllocMemName->Data1);
        AllocMemName->Data2 = CpuToFdt16 (AllocMemName->Data2);
        AllocMemName->Data3 = CpuToFdt16 (AllocMemName->Data3);
        Status              = FdtSetProp (Fdt, TempNode, "guid", AllocMemName, sizeof (EFI_GUID));
      }
    }
  }

  // SmbiosTable = NULL;
  // SmbiosTable = (UNIVERSAL_PAYLOAD_SMBIOS_TABLE *)GetFirstGuidHob (&gUniversalPayloadSmbios3TableGuid);
  // if (SmbiosTable != NULL) {
  //  DEBUG ((DEBUG_INFO, "To build Smbios memory FDT\n"));
  //  Status   = AsciiSPrint (TempStr, sizeof (TempStr), "memory@%lX", SmbiosTable->SmBiosEntryPoint);
  //  TempNode = FdtAddSubnode (Fdt, ParentNode, TempStr);
  //  DEBUG ((DEBUG_INFO, "FdtAddSubnode %x", TempNode));
  //  RegTmp[0] = CpuToFdt64 (SmbiosTable->SmBiosEntryPoint);
  //  RegTmp[1] = CpuToFdt64 (sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE) + sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT));
  //  Status    = FdtSetProp (Fdt, TempNode, "reg", &RegTmp, sizeof (RegTmp));
  //  ASSERT_EFI_ERROR (Status);

  //  Status = FdtSetProp (Fdt, TempNode, "compatible", "smbios", (UINT32)(AsciiStrLen ("smbios")+1));
  //  ASSERT_EFI_ERROR (Status);
  // }
  return Status;
}

/**
  It will build FDT based on serial information.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForSerial (
  IN     VOID  *FdtBase
  )
{
  EFI_STATUS  Status;
  VOID        *Fdt;
  INT32       TempNode;
  UINT64      RegisterBase;
  CHAR8       TempStr[32];
  UINT64      RegData[3];
  UINT32      Data32;

  Fdt          = FdtBase;
  RegisterBase = 0;

  //
  // Create SerialPortInfo FDT node.
  //
  Status   = AsciiSPrint (TempStr, sizeof (TempStr), "serial@%lX", (RegisterBase == 0) ? PcdGet64 (PcdSerialRegisterBase) : RegisterBase);
  TempNode = FdtAddSubnode (Fdt, 0, TempStr);
  ASSERT (TempNode > 0);

  Data32 = CpuToFdt32 (PcdGet32 (PcdSerialBaudRate));
  Status = FdtSetProp (Fdt, TempNode, "current-speed", &Data32, sizeof (Data32));
  ASSERT_EFI_ERROR (Status);

  // Data32 = CpuToFdt32 (PcdGet32 (PcdSerialRegisterStride));
  // Status = FdtSetProp (Fdt, TempNode, "reg-shift", &Data32, sizeof (Data32));
  // ASSERT_EFI_ERROR (Status);

  //  Data32 = CpuToFdt32 (8);
  //  Status = FdtSetProp (Fdt, TempNode, "reg-width", &Data32, sizeof (Data32));
  //  ASSERT_EFI_ERROR (Status);

  if (PcdGetBool (PcdSerialUseMmio)) {
    RegData[0] = CpuToFdt64 (0);
    RegData[1] = CpuToFdt64 ((RegisterBase == 0) ? PcdGet64 (PcdSerialRegisterBase) : RegisterBase);
    RegData[2] = CpuToFdt64 (0x80);
    Status     = FdtSetProp (Fdt, TempNode, "reg", &RegData, sizeof (RegData));
    ASSERT_EFI_ERROR (Status);

    Data32 = CpuToFdt32 (4);
    Status = FdtSetProp (Fdt, TempNode, "reg-io-width", &Data32, sizeof (Data32));
    ASSERT_EFI_ERROR (Status);
  } else {
    RegData[0] = CpuToFdt64 (1);
    RegData[1] = CpuToFdt64 ((RegisterBase == 0) ? PcdGet64 (PcdSerialRegisterBase) : RegisterBase);
    RegData[2] = CpuToFdt64 (8);
    Status     = FdtSetProp (Fdt, TempNode, "reg", &RegData, sizeof (RegData));
    ASSERT_EFI_ERROR (Status);

    Data32 = CpuToFdt32 (1);
    Status = FdtSetProp (Fdt, TempNode, "reg-io-width", &Data32, sizeof (Data32));
    ASSERT_EFI_ERROR (Status);
  }

  Status = FdtSetProp (Fdt, TempNode, "compatible", "isa", (UINT32)(AsciiStrLen ("isa")+1));
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  It will build FDT based on BuildFdtForPciRootBridge information.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForPciRootBridge (
  IN     VOID  *FdtBase
  )
{
  EFI_STATUS                          Status;
  VOID                                *Fdt;
  INT32                               TempNode;
  CHAR8                               TempStr[32];
  UINT16                              RegTmp[2];
  UINT64                              RegData[16];
  UINT32                              Data32;
  UINT8                               BusNumber;
  EFI_HOB_GUID_TYPE                   *GuidHob;
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    *GenericHeader;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *PciRootBridgeInfo;

  Fdt               = FdtBase;
  BusNumber         = 0;
  Status            = EFI_SUCCESS;
  PciRootBridgeInfo = NULL;

  DEBUG ((DEBUG_INFO, "%a: #1 \n", __func__));
  //
  // Create BuildFdtForPciRootBridge FDT node.
  //

  GuidHob = GetFirstGuidHob (&gUniversalPayloadPciRootBridgeInfoGuid);
  if (GuidHob != NULL) {
    GenericHeader = (UNIVERSAL_PAYLOAD_GENERIC_HEADER *)GET_GUID_HOB_DATA (GuidHob);
    if ((sizeof (UNIVERSAL_PAYLOAD_GENERIC_HEADER) <= GET_GUID_HOB_DATA_SIZE (GuidHob)) && (GenericHeader->Length <= GET_GUID_HOB_DATA_SIZE (GuidHob))) {
      if ((GenericHeader->Revision == UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION) && (GenericHeader->Length >= sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES))) {
        DEBUG ((DEBUG_INFO, "%a: #2 \n", __func__));

        //
        // UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES structure is used when Revision equals to UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION
        //
        PciRootBridgeInfo = (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES *)GET_GUID_HOB_DATA (GuidHob);
      }
    }
  }

  DEBUG ((DEBUG_INFO, "PciRootBridgeInfo->Count %x\n", PciRootBridgeInfo->Count));
  DEBUG ((DEBUG_INFO, "PciRootBridge->Segment %x, \n", PciRootBridgeInfo->RootBridge[0].Segment));

  DEBUG ((DEBUG_INFO, "PciRootBridge->Bus.Base %x, \n", PciRootBridgeInfo->RootBridge[0].Bus.Base));
  DEBUG ((DEBUG_INFO, "PciRootBridge->Bus.limit %x, \n", PciRootBridgeInfo->RootBridge[0].Bus.Limit));

  DEBUG ((DEBUG_INFO, "PciRootBridge->Mem.Base %x, \n", PciRootBridgeInfo->RootBridge[0].Mem.Base));
  DEBUG ((DEBUG_INFO, "PciRootBridge->Mem.limit %x, \n", PciRootBridgeInfo->RootBridge[0].Mem.Limit));

  DEBUG ((DEBUG_INFO, "PciRootBridge->PMem.Base %x, \n", PciRootBridgeInfo->RootBridge[0].PMem.Base));
  DEBUG ((DEBUG_INFO, "PciRootBridge->PMem.limit %x, \n", PciRootBridgeInfo->RootBridge[0].PMem.Limit));

  if (PciRootBridgeInfo != NULL) {
    Status   = AsciiSPrint (TempStr, sizeof (TempStr), "pci-rb@%lX", PcdGet64 (PcdPciExpressBaseAddress));
    TempNode = FdtAddSubnode (Fdt, 0, TempStr);
    ASSERT (TempNode > 0);
    SetMem (RegData, sizeof (RegData), 0);

    RegData[0] = CpuToFdt64 (PciRootBridgeInfo->RootBridge[0].Mem.Base);
    RegData[3] = CpuToFdt64 (PciRootBridgeInfo->RootBridge[0].Mem.Limit);

    RegData[4] = CpuToFdt64 (PciRootBridgeInfo->RootBridge[0].PMem.Base);
    RegData[7] = CpuToFdt64 (PciRootBridgeInfo->RootBridge[0].PMem.Limit);

    RegData[8]  = CpuToFdt64 (PciRootBridgeInfo->RootBridge[0].Io.Base);
    RegData[11] = CpuToFdt64 (PciRootBridgeInfo->RootBridge[0].Io.Limit);

    Status = FdtSetProp (Fdt, TempNode, "ranges", &RegData, sizeof (RegData));
    ASSERT_EFI_ERROR (Status);

    Data32 = CpuToFdt32 (2);
    Status = FdtSetProp (Fdt, TempNode, "#size-cells", &Data32, sizeof (UINT32));

    Data32 = CpuToFdt32 (3);
    Status = FdtSetProp (Fdt, TempNode, "#address-cells", &Data32, sizeof (UINT32));

    BusNumber = PciRootBridgeInfo->RootBridge[0].Bus.Base & 0xFF;
    RegTmp[0] = CpuToFdt16 (BusNumber);
    BusNumber = PciRootBridgeInfo->RootBridge[0].Bus.Limit & 0xFF;
    RegTmp[1] = CpuToFdt16 (BusNumber);
    Status    = FdtSetProp (Fdt, TempNode, "bus-range", &RegTmp, sizeof (RegTmp));
    ASSERT_EFI_ERROR (Status);

    Status = FdtSetProp (Fdt, TempNode, "compatible", "pci", (UINT32)(AsciiStrLen ("pci")+1));
    ASSERT_EFI_ERROR (Status);
  }

  DEBUG ((DEBUG_INFO, "%a: #3 \n", __func__));

  return Status;
}

/**
  It will build FDT based on FrameBuffer.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForFrameBuffer (
  IN     VOID  *FdtBase
  )
{
  EFI_STATUS                 Status;
  VOID                       *Fdt;
  INT32                      TempNode;
  UINT32                     Data32;
  CHAR8                      TempStr[32];
  UINT64                     RegData[2];
  EFI_HOB_GUID_TYPE          *GuidHob;
  EFI_PEI_GRAPHICS_INFO_HOB  *GraphicsInfo;

  Fdt = FdtBase;

  GuidHob = GetFirstGuidHob (&gEfiGraphicsInfoHobGuid);
  if (GuidHob != NULL) {
    GraphicsInfo = (EFI_PEI_GRAPHICS_INFO_HOB *)(GET_GUID_HOB_DATA (GuidHob));
    Status       = AsciiSPrint (TempStr, sizeof (TempStr), "framebuffer@%lX", GraphicsInfo->FrameBufferBase);
    TempNode     = FdtAddSubnode (Fdt, 0, TempStr);
    ASSERT (TempNode > 0);

    Status = FdtSetProp (Fdt, TempNode, "format", "a8r8g8b8", (UINT32)(AsciiStrLen ("a8r8g8b8")+1));
    ASSERT_EFI_ERROR (Status);

    Data32 = CpuToFdt32 (GraphicsInfo->GraphicsMode.VerticalResolution);
    Status = FdtSetProp (Fdt, TempNode, "height", &Data32, sizeof (UINT32));
    ASSERT_EFI_ERROR (Status);

    Data32 = CpuToFdt32 (GraphicsInfo->GraphicsMode.HorizontalResolution);
    Status = FdtSetProp (Fdt, TempNode, "width", &Data32, sizeof (UINT32));
    ASSERT_EFI_ERROR (Status);

    RegData[0] = CpuToFdt64 (GraphicsInfo->FrameBufferBase);
    RegData[1] = CpuToFdt64 (GraphicsInfo->FrameBufferSize);
    Status     = FdtSetProp (Fdt, TempNode, "reg", &RegData, sizeof (RegData));
    ASSERT_EFI_ERROR (Status);

    Status = FdtSetProp (Fdt, TempNode, "compatible", "simple-framebuffer", (UINT32)(AsciiStrLen ("simple-framebuffer")+1));
    ASSERT_EFI_ERROR (Status);
  } else {
    Status   = AsciiSPrint (TempStr, sizeof (TempStr), "framebuffer@%lX", 0xB0000000);
    TempNode = FdtAddSubnode (Fdt, 0, TempStr);
    ASSERT (TempNode > 0);

    Status = FdtSetProp (Fdt, TempNode, "format", "a8r8g8b8", (UINT32)(AsciiStrLen ("a8r8g8b8")+1));
    ASSERT_EFI_ERROR (Status);

    Data32 = CpuToFdt32 (1024);
    Status = FdtSetProp (Fdt, TempNode, "height", &Data32, sizeof (UINT32));
    ASSERT_EFI_ERROR (Status);

    Data32 = CpuToFdt32 (1280);
    Status = FdtSetProp (Fdt, TempNode, "width", &Data32, sizeof (UINT32));
    ASSERT_EFI_ERROR (Status);

    RegData[0] = CpuToFdt64 (0xB0000000);
    RegData[1] = CpuToFdt64 (0x500000);
    Status     = FdtSetProp (Fdt, TempNode, "reg", &RegData, sizeof (RegData));
    ASSERT_EFI_ERROR (Status);

    Status = FdtSetProp (Fdt, TempNode, "compatible", "simple-framebuffer", (UINT32)(AsciiStrLen ("simple-framebuffer")+1));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  It will build FDT for UPL required data.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForUplRequired (
  IN     VOID  *FdtBase
  )
{
  EFI_STATUS     Status;
  VOID           *Fdt;
  INT32          ParentNode;
  INT32          CustomNode;
  INT32          UPLParaNode;
  INT32          UPLImageNode;
  EFI_HOB_CPU    *CpuHob;
  UINT64         Data64;
  UINT32         Data32;
  UINT16         Data16;
  VOID           *HobPtr;
  EFI_BOOT_MODE  BootMode;
  CHAR8          TempStr[32];
  UINT64         RegTmp[2];

  Fdt = FdtBase;

  //
  // Create Hob list FDT node.
  //
  ParentNode = FdtAddSubnode (Fdt, 0, "options");
  ASSERT (ParentNode > 0);

  UPLParaNode = FdtAddSubnode (Fdt, ParentNode, "upl-params");
  ASSERT (UPLParaNode > 0);

  //
  // Create CPU info FDT node
  //
  CpuHob = GetFirstHob (EFI_HOB_TYPE_CPU);
  ASSERT (CpuHob != NULL);

  Data16 = CpuToFdt16 (0);
  Status = FdtSetProp (Fdt, UPLParaNode, "pci-enum-done", &Data16, sizeof (Data16));
  ASSERT_EFI_ERROR (Status);

  BootMode = GetBootModeHob ();

  Data32 = CpuToFdt32 ((UINT32)CpuHob->SizeOfMemorySpace);
  Status = FdtSetProp (Fdt, UPLParaNode, "addr-width", &Data32, sizeof (Data32));
  ASSERT_EFI_ERROR (Status);

  if (BootMode == BOOT_WITH_FULL_CONFIGURATION) {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "normal", (UINT32)(AsciiStrLen ("normal")+1));
  } else if (BootMode == BOOT_WITH_MINIMAL_CONFIGURATION) {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "fast", (UINT32)(AsciiStrLen ("fast")+1));
  } else if (BootMode == BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS) {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "full", (UINT32)(AsciiStrLen ("full")+1));
  } else if (BootMode == BOOT_WITH_DEFAULT_SETTINGS) {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "default", (UINT32)(AsciiStrLen ("default")+1));
  } else if (BootMode == BOOT_ON_S4_RESUME) {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "s4", (UINT32)(AsciiStrLen ("s4")+1));
  } else if (BootMode == BOOT_ON_S3_RESUME) {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "s3", (UINT32)(AsciiStrLen ("s3")+1));
  } else {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "na", (UINT32)(AsciiStrLen ("na")+1));
  }

  ASSERT_EFI_ERROR (Status);

  Status = FdtSetProp (Fdt, UPLParaNode, "compatible", "upl", (UINT32)(AsciiStrLen ("upl")+1));
  ASSERT_EFI_ERROR (Status);

  Status       = AsciiSPrint (TempStr, sizeof (TempStr), "upl-images@%lX", (UINTN)(FdtBase));
  UPLImageNode = FdtAddSubnode (Fdt, ParentNode, TempStr);

  RegTmp[0] = CpuToFdt64 ((UINTN)FdtBase);
  RegTmp[1] = CpuToFdt64 (((FDT_HEADER *)FdtBase)->TotalSize);
  Status    = FdtSetProp (FdtBase, UPLImageNode, "addr", &RegTmp, sizeof (RegTmp));

  CustomNode = FdtAddSubnode (Fdt, ParentNode, "upl-custom");
  ASSERT (CustomNode > 0);

  HobPtr = GetHobList ();
  Data64 = CpuToFdt64 ((UINT64)(EFI_PHYSICAL_ADDRESS)HobPtr);
  Status = FdtSetProp (Fdt, CustomNode, "hoblistptr", &Data64, sizeof (Data64));
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  It will build FDT for UPL consumed.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForUPL (
  IN     VOID  *FdtBase
  )
{
  EFI_STATUS  Status;

  //
  // Build FDT for memory related
  //
  Status = BuildFdtForMemory (FdtBase);
  ASSERT_EFI_ERROR (Status);

  // Status = BuildFdtForReservedMemory (FdtBase);
  // ASSERT_EFI_ERROR (Status);

  Status = BuildFdtForMemAlloc (FdtBase);
  ASSERT_EFI_ERROR (Status);

  Status = BuildFdtForSerial (FdtBase);
  ASSERT_EFI_ERROR (Status);

  Status = BuildFdtForPciRootBridge (FdtBase);
  ASSERT_EFI_ERROR (Status);

  Status = BuildFdtForFrameBuffer (FdtBase);
  ASSERT_EFI_ERROR (Status);

  Status = BuildFdtForUplRequired (FdtBase);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

#if (FixedPcdGetBool (PcdHandOffFdtEnable))

/**
  Discover Hobs data and report data into a FDT.
  @param[in] PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param[in] NotifyDescriptor  Address of the notification descriptor data structure.
  @param[in] Ppi               Address of the PPI that was installed.
  @retval EFI_SUCCESS          Hobs data is discovered.
  @return Others               No Hobs data is discovered.
**/
EFI_STATUS
EFIAPI
FdtPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                     Status;
  UNIVERSAL_PAYLOAD_DEVICE_TREE  *Fdt;
  UINT32                         FdtSize;
  UINTN                          FdtPages;
  VOID                           *FdtBase;
  UINT32                         Data32;

  Fdt      = NULL;
  FdtSize  = 4 * EFI_PAGE_SIZE;
  FdtPages = EFI_SIZE_TO_PAGES (FdtSize);
  FdtBase  = AllocatePages (FdtPages);
  if (FdtBase == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: AllocatePages failed\n", __func__));
    return EFI_NOT_FOUND;
  }

  Status = FdtCreateEmptyTree (FdtBase, (UINT32)FdtSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: cannot create FDT\n", __func__));
  }

  // Set cell property of root node
  Data32 = CpuToFdt32 (2);
  Status = FdtSetProp (FdtBase, 0, "#address-cells", &Data32, sizeof (UINT32));
  Status = FdtSetProp (FdtBase, 0, "#size-cells", &Data32, sizeof (UINT32));

  Status = BuildFdtForUPL (FdtBase);
  ASSERT_EFI_ERROR (Status);

  PrintFdt (FdtBase);

  Fdt = BuildGuidHob (&gUniversalPayloadDeviceTreeGuid, sizeof (UNIVERSAL_PAYLOAD_DEVICE_TREE));
  if (Fdt == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Build FDT Hob failed\n", __func__));
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: fdt at 0x%x (size %d)\n",
    __func__,
    FdtBase,
    Fdt32ToCpu (((FDT_HEADER *)FdtBase)->TotalSize)
    ));

  Fdt->Header.Revision   = UNIVERSAL_PAYLOAD_DEVICE_TREE_REVISION;
  Fdt->Header.Length     = sizeof (UNIVERSAL_PAYLOAD_DEVICE_TREE);
  Fdt->DeviceTreeAddress = (UINT64)FdtBase;

  return Status;
}

#endif

/**
  Install Pei Load File PPI.
  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.
  @retval EFI_SUCESS  The entry point executes successfully.
  @retval Others      Some error occurs during the execution of this function.
**/
EFI_STATUS
EFIAPI
InitializeFitPayloadLoaderPeim (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = PeiServicesInstallPpi (&gPpiLoadFilePpiList);

 #if (FixedPcdGetBool (PcdHandOffFdtEnable))

  //
  // Build FDT in end of PEI notify callback.
  //
  Status = PeiServicesNotifyPpi (&mReadyToPayloadNotifyList[0]);
  ASSERT_EFI_ERROR (Status);
 #endif
  return Status;
}
