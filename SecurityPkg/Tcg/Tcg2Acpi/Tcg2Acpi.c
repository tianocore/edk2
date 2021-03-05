/** @file
  This driver implements TPM 2.0 definition block in ACPI table and
  populates registered SMI callback functions for Tcg2 physical presence
  and MemoryClear to handle the requests for ACPI method. It needs to be
  used together with Tcg2 MM drivers to exchange information on registered
  SwSmiValue and allocated NVS region address.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable and ACPINvs data in SMM mode.
  This external input must be validated carefully to avoid security issue.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <IndustryStandard/Tpm2Acpi.h>

#include <Guid/TpmInstance.h>
#include <Guid/TpmNvsMm.h>
#include <Guid/PiSmmCommunicationRegionTable.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/Tcg2Protocol.h>
#include <Protocol/MmCommunication.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/TpmMeasurementLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/UefiLib.h>
#include <Library/MmUnblockMemoryLib.h>

//
// Physical Presence Interface Version supported by Platform
//
#define PHYSICAL_PRESENCE_VERSION_TAG                              "$PV"
#define PHYSICAL_PRESENCE_VERSION_SIZE                             4

//
// PNP _HID for TPM2 device
//
#define TPM_HID_TAG                                                "NNNN0000"
#define TPM_HID_PNP_SIZE                                           8
#define TPM_HID_ACPI_SIZE                                          9

#define TPM_PRS_RESL                                               "RESL"
#define TPM_PRS_RESS                                               "RESS"
#define TPM_PRS_RES_NAME_SIZE                                      4
//
// Minimum PRS resource template size
//  1 byte    for  BufferOp
//  1 byte    for  PkgLength
//  2 bytes   for  BufferSize
//  12 bytes  for  Memory32Fixed descriptor
//  5 bytes   for  Interrupt descriptor
//  2 bytes   for  END Tag
//
#define TPM_POS_RES_TEMPLATE_MIN_SIZE                              (1 + 1 + 2 + 12 + 5 + 2)

//
// Max Interrupt buffer size for PRS interrupt resource
// Now support 15 interrupts in maxmum
//
#define MAX_PRS_INT_BUF_SIZE                                       (15*4)

#pragma pack(1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER Header;
  // Flags field is replaced in version 4 and above
  //    BIT0~15:  PlatformClass      This field is only valid for version 4 and above
  //    BIT16~31: Reserved
  UINT32                      Flags;
  UINT64                      AddressOfControlArea;
  UINT32                      StartMethod;
  UINT8                       PlatformSpecificParameters[12];  // size up to 12
  UINT32                      Laml;                          // Optional
  UINT64                      Lasa;                          // Optional
} EFI_TPM2_ACPI_TABLE_V4;

#pragma pack()

EFI_TPM2_ACPI_TABLE_V4  mTpm2AcpiTemplate = {
  {
    EFI_ACPI_5_0_TRUSTED_COMPUTING_PLATFORM_2_TABLE_SIGNATURE,
    sizeof (mTpm2AcpiTemplate),
    EFI_TPM2_ACPI_TABLE_REVISION,
    //
    // Compiler initializes the remaining bytes to 0
    // These fields should be filled in in production
    //
  },
  0, // BIT0~15:  PlatformClass
     // BIT16~31: Reserved
  0, // Control Area
  EFI_TPM2_ACPI_TABLE_START_METHOD_TIS, // StartMethod
};

TCG_NVS                    *mTcgNvs;

/**
  Find the operation region in TCG ACPI table by given Name and Size,
  and initialize it if the region is found.

  @param[in, out] Table          The TPM item in ACPI table.
  @param[in]      Name           The name string to find in TPM table.
  @param[in]      Size           The size of the region to find.

  @return                        The allocated address for the found region.

**/
VOID *
AssignOpRegion (
  EFI_ACPI_DESCRIPTION_HEADER    *Table,
  UINT32                         Name,
  UINT16                         Size
  )
{
  EFI_STATUS                     Status;
  AML_OP_REGION_32_8             *OpRegion;
  EFI_PHYSICAL_ADDRESS           MemoryAddress;

  MemoryAddress = SIZE_4GB - 1;

  //
  // Patch some pointers for the ASL code before loading the SSDT.
  //
  for (OpRegion  = (AML_OP_REGION_32_8 *) (Table + 1);
       OpRegion <= (AML_OP_REGION_32_8 *) ((UINT8 *) Table + Table->Length);
       OpRegion  = (AML_OP_REGION_32_8 *) ((UINT8 *) OpRegion + 1)) {
    if ((OpRegion->OpRegionOp  == AML_EXT_REGION_OP) &&
        (OpRegion->NameString  == Name) &&
        (OpRegion->DWordPrefix == AML_DWORD_PREFIX) &&
        (OpRegion->BytePrefix  == AML_BYTE_PREFIX)) {

      Status = gBS->AllocatePages(AllocateMaxAddress, EfiACPIMemoryNVS, EFI_SIZE_TO_PAGES (Size), &MemoryAddress);
      ASSERT_EFI_ERROR (Status);
      ZeroMem ((VOID *)(UINTN)MemoryAddress, Size);
      OpRegion->RegionOffset = (UINT32) (UINTN) MemoryAddress;
      OpRegion->RegionLen    = (UINT8) Size;
      // Request to unblock this region from MM core
      Status = MmUnblockMemoryRequest (MemoryAddress, EFI_SIZE_TO_PAGES (Size));
      if (Status != EFI_UNSUPPORTED && EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
      }
      break;
    }
  }

  return (VOID *) (UINTN) MemoryAddress;
}

/**
  Locate the MM communication buffer and protocol, then use it to exchange information with
  Tcg2StandaloneMmm on NVS address and SMI value.

  @param[in, out] TcgNvs         The NVS subject to send to MM environment.

  @return                        The status for locating MM common buffer, communicate to MM, etc.

**/
EFI_STATUS
EFIAPI
ExchangeCommonBuffer (
  IN OUT  TCG_NVS                 *TcgNvs
)
{
  EFI_STATUS                                Status;
  EFI_MM_COMMUNICATION_PROTOCOL             *MmCommunication;
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE   *PiSmmCommunicationRegionTable;
  EFI_MEMORY_DESCRIPTOR                     *MmCommMemRegion;
  EFI_MM_COMMUNICATE_HEADER                 *CommHeader;
  TPM_NVS_MM_COMM_BUFFER                    *CommBuffer;
  UINTN                                     CommBufferSize;
  UINTN                                     Index;

  // Step 0: Sanity check for input argument
  if (TcgNvs == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Input argument is NULL!\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: Grab the common buffer header
  Status = EfiGetSystemConfigurationTable (&gEdkiiPiSmmCommunicationRegionTableGuid, (VOID**) &PiSmmCommunicationRegionTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to locate SMM communciation common buffer - %r!\n", __FUNCTION__, Status));
    return Status;
  }

  // Step 2: Grab one that is large enough to hold TPM_NVS_MM_COMM_BUFFER, the IPL one should be sufficient
  CommBufferSize = 0;
  MmCommMemRegion = (EFI_MEMORY_DESCRIPTOR*) (PiSmmCommunicationRegionTable + 1);
  for (Index = 0; Index < PiSmmCommunicationRegionTable->NumberOfEntries; Index++) {
    if (MmCommMemRegion->Type == EfiConventionalMemory) {
      CommBufferSize = EFI_PAGES_TO_SIZE ((UINTN)MmCommMemRegion->NumberOfPages);
      if (CommBufferSize >= (sizeof (TPM_NVS_MM_COMM_BUFFER) + OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data))) {
        break;
      }
    }
    MmCommMemRegion = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)MmCommMemRegion + PiSmmCommunicationRegionTable->DescriptorSize);
  }

  if (Index >= PiSmmCommunicationRegionTable->NumberOfEntries) {
    // Could not find one that meets our goal...
    DEBUG ((DEBUG_ERROR, "%a - Could not find a common buffer that is big enough for NVS!\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  // Step 3: Start to populate contents
  // Step 3.1: MM Communication common header
  CommHeader = (EFI_MM_COMMUNICATE_HEADER *) (UINTN) MmCommMemRegion->PhysicalStart;
  CommBufferSize = sizeof (TPM_NVS_MM_COMM_BUFFER) + OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data);
  ZeroMem (CommHeader, CommBufferSize);
  CopyGuid (&CommHeader->HeaderGuid, &gTpmNvsMmGuid);
  CommHeader->MessageLength = sizeof (TPM_NVS_MM_COMM_BUFFER);

  // Step 3.2: TPM_NVS_MM_COMM_BUFFER content per our needs
  CommBuffer = (TPM_NVS_MM_COMM_BUFFER *) (CommHeader->Data);
  CommBuffer->Function = TpmNvsMmExchangeInfo;
  CommBuffer->TargetAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) TcgNvs;

  // Step 4: Locate the protocol and signal Mmi.
  Status = gBS->LocateProtocol (&gEfiMmCommunicationProtocolGuid, NULL, (VOID**) &MmCommunication);
  if (!EFI_ERROR (Status)) {
    Status = MmCommunication->Communicate (MmCommunication, CommHeader, &CommBufferSize);
    DEBUG ((DEBUG_INFO, "%a - Communicate() = %r\n", __FUNCTION__, Status));
  }
  else {
    DEBUG ((DEBUG_ERROR, "%a - Failed to locate MmCommunication protocol - %r\n", __FUNCTION__, Status));
    return Status;
  }

  // Step 5: If everything goes well, populate the channel number
  if (!EFI_ERROR (CommBuffer->ReturnStatus)) {
    // Need to demote to UINT8 according to SMI value definition
    TcgNvs->PhysicalPresence.SoftwareSmi = (UINT8) CommBuffer->RegisteredPpSwiValue;
    TcgNvs->MemoryClear.SoftwareSmi = (UINT8) CommBuffer->RegisteredMcSwiValue;
    DEBUG ((
      DEBUG_INFO,
      "%a Communication returned software SMI value. PP: 0x%x; MC: 0x%x.\n",
      __FUNCTION__,
      TcgNvs->PhysicalPresence.SoftwareSmi,
      TcgNvs->MemoryClear.SoftwareSmi
      ));
  }

  return (EFI_STATUS) CommBuffer->ReturnStatus;
}

/**
  Patch version string of Physical Presence interface supported by platform. The initial string tag in TPM
ACPI table is "$PV".

  @param[in, out] Table          The TPM item in ACPI table.
  @param[in]      PPVer          Version string of Physical Presence interface supported by platform.

  @return                        The allocated address for the found region.

**/
EFI_STATUS
UpdatePPVersion (
  EFI_ACPI_DESCRIPTION_HEADER    *Table,
  CHAR8                          *PPVer
  )
{
  EFI_STATUS  Status;
  UINT8       *DataPtr;

  //
  // Patch some pointers for the ASL code before loading the SSDT.
  //
  for (DataPtr  = (UINT8 *)(Table + 1);
       DataPtr <= (UINT8 *) ((UINT8 *) Table + Table->Length - PHYSICAL_PRESENCE_VERSION_SIZE);
       DataPtr += 1) {
    if (AsciiStrCmp((CHAR8 *)DataPtr,  PHYSICAL_PRESENCE_VERSION_TAG) == 0) {
      Status = AsciiStrCpyS((CHAR8 *)DataPtr, PHYSICAL_PRESENCE_VERSION_SIZE, PPVer);
      DEBUG((DEBUG_INFO, "TPM2 Physical Presence Interface Version update status 0x%x\n", Status));
      return Status;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Patch interrupt resources returned by TPM _PRS. ResourceTemplate to patch is determined by input
  interrupt buffer size. BufferSize, PkgLength and interrupt descriptor in ByteList need to be patched

  @param[in, out] Table            The TPM item in ACPI table.
  @param[in]      IrqBuffer        Input new IRQ buffer.
  @param[in]      IrqBuffserSize   Input new IRQ buffer size.
  @param[out]     IsShortFormPkgLength   If _PRS returns Short length Package(ACPI spec 20.2.4).

  @return                          patch status.

**/
EFI_STATUS
UpdatePossibleResource (
  IN OUT  EFI_ACPI_DESCRIPTION_HEADER    *Table,
  IN      UINT32                         *IrqBuffer,
  IN      UINT32                         IrqBuffserSize,
  OUT     BOOLEAN                        *IsShortFormPkgLength
  )
{
  UINT8       *DataPtr;
  UINT8       *DataEndPtr;
  UINT32      NewPkgLength;
  UINT32      OrignalPkgLength;

  NewPkgLength     = 0;
  OrignalPkgLength = 0;
  DataEndPtr       = NULL;

  //
  // Follow ACPI spec
  //           6.4.3   Extend Interrupt Descriptor.
  //           19.3.3 ASL Resource Template
  //           20      AML specification
  // to patch TPM ACPI object _PRS returned ResourceTemplate() containing 2 resource descriptors and an auto appended End Tag
  //
  //  AML data is organized by following rule.
  //  Code need to patch BufferSize and PkgLength and interrupt descriptor in ByteList
  //
  // =============  Buffer ====================
  //           DefBuffer := BufferOp PkgLength BufferSize ByteList
  //            BufferOp := 0x11
  //
  // ==============PkgLength==================
  //          PkgLength := PkgLeadByte |
  //                              <PkgLeadByte ByteData> |
  //                              <PkgLeadByte ByteData ByteData> |
  //                              <PkgLeadByte ByteData ByteData ByteData>
  //
  //       PkgLeadByte := <bit 7-6: ByteData count that follows (0-3)>
  //                               <bit 5-4: Only used if PkgLength <= 63 >
  //                               <bit 3-0: Least significant package length nybble>
  //
  //==============BufferSize==================
  //        BufferSize := Integer
  //           Integer := ByteConst|WordConst|DwordConst....
  //
  //           ByteConst := BytePrefix ByteData
  //
  //==============ByteList===================
  //          ByteList := ByteData ByteList
  //
  //=========================================

  //
  // 1. Check TPM_PRS_RESS with PkgLength <=63 can hold the input interrupt number buffer for patching
  //
  for (DataPtr  = (UINT8 *)(Table + 1);
       DataPtr < (UINT8 *) ((UINT8 *) Table + Table->Length - (TPM_PRS_RES_NAME_SIZE + TPM_POS_RES_TEMPLATE_MIN_SIZE));
       DataPtr += 1) {
    if (CompareMem(DataPtr, TPM_PRS_RESS, TPM_PRS_RES_NAME_SIZE) == 0) {
      //
      // Jump over object name & BufferOp
      //
      DataPtr += TPM_PRS_RES_NAME_SIZE + 1;

      if ((*DataPtr & (BIT7|BIT6)) == 0) {
        OrignalPkgLength = (UINT32)*DataPtr;
        DataEndPtr       = DataPtr + OrignalPkgLength;

        //
        // Jump over PkgLength = PkgLeadByte only
        //
        NewPkgLength++;

        //
        // Jump over BufferSize
        //
        if (*(DataPtr + 1) == AML_BYTE_PREFIX) {
          NewPkgLength += 2;
        } else if (*(DataPtr + 1) == AML_WORD_PREFIX) {
          NewPkgLength += 3;
        } else if (*(DataPtr + 1) == AML_DWORD_PREFIX) {
          NewPkgLength += 5;
        } else {
          ASSERT(FALSE);
          return EFI_UNSUPPORTED;
        }
      } else {
        ASSERT(FALSE);
        return EFI_UNSUPPORTED;
      }

      //
      // Include Memory32Fixed Descriptor (12 Bytes) + Interrupt Descriptor header(5 Bytes) + End Tag(2 Bytes)
      //
      NewPkgLength += 19 + IrqBuffserSize;
      if (NewPkgLength > 63) {
        break;
      }

      if (NewPkgLength > OrignalPkgLength) {
        ASSERT(FALSE);
        return EFI_INVALID_PARAMETER;
      }

      //
      // 1.1 Patch PkgLength
      //
      *DataPtr = (UINT8)NewPkgLength;

      //
      // 1.2 Patch BufferSize = sizeof(Memory32Fixed Descriptor + Interrupt Descriptor + End Tag).
      //      It is Little endian. So only patch lowest byte of BufferSize due to current interrupt number limit.
      //
      *(DataPtr + 2) = (UINT8)(IrqBuffserSize + 19);

      //
      // Notify _PRS to report short formed ResourceTemplate
      //
      *IsShortFormPkgLength = TRUE;

      break;
    }
  }

  //
  // 2. Use TPM_PRS_RESL with PkgLength > 63 to hold longer input interrupt number buffer for patching
  //
  if (NewPkgLength > 63) {
    NewPkgLength     = 0;
    OrignalPkgLength = 0;
    for (DataPtr  = (UINT8 *)(Table + 1);
         DataPtr < (UINT8 *) ((UINT8 *) Table + Table->Length - (TPM_PRS_RES_NAME_SIZE + TPM_POS_RES_TEMPLATE_MIN_SIZE));
         DataPtr += 1) {
      if (CompareMem(DataPtr, TPM_PRS_RESL, TPM_PRS_RES_NAME_SIZE) == 0) {
        //
        // Jump over object name & BufferOp
        //
        DataPtr += TPM_PRS_RES_NAME_SIZE + 1;

        if ((*DataPtr & (BIT7|BIT6)) != 0) {
          OrignalPkgLength = (UINT32)(*(DataPtr + 1) << 4) + (*DataPtr & 0x0F);
          DataEndPtr       = DataPtr + OrignalPkgLength;
          //
          // Jump over PkgLength = PkgLeadByte + ByteData length
          //
          NewPkgLength += 1 + ((*DataPtr & (BIT7|BIT6)) >> 6);

          //
          // Jump over BufferSize
          //
          if (*(DataPtr + NewPkgLength) == AML_BYTE_PREFIX) {
            NewPkgLength += 2;
          } else if (*(DataPtr + NewPkgLength) == AML_WORD_PREFIX) {
            NewPkgLength += 3;
          } else if (*(DataPtr + NewPkgLength) == AML_DWORD_PREFIX) {
            NewPkgLength += 5;
          } else {
            ASSERT(FALSE);
            return EFI_UNSUPPORTED;
          }
        } else {
          ASSERT(FALSE);
          return EFI_UNSUPPORTED;
        }

        //
        // Include Memory32Fixed Descriptor (12 Bytes) + Interrupt Descriptor header(5 Bytes) + End Tag(2  Bytes)
        //
        NewPkgLength += 19 + IrqBuffserSize;

        if (NewPkgLength > OrignalPkgLength) {
          ASSERT(FALSE);
          return EFI_INVALID_PARAMETER;
        }

        //
        // 2.1 Patch PkgLength. Only patch PkgLeadByte and first ByteData
        //
        *DataPtr = (UINT8)((*DataPtr) & 0xF0) | (NewPkgLength & 0x0F);
        *(DataPtr + 1) = (UINT8)((NewPkgLength & 0xFF0) >> 4);

        //
        // 2.2 Patch BufferSize = sizeof(Memory32Fixed Descriptor + Interrupt Descriptor + End Tag).
        //     It is Little endian. Only patch lowest byte of BufferSize due to current interrupt number limit.
        //
        *(DataPtr + 2 + ((*DataPtr & (BIT7|BIT6)) >> 6)) = (UINT8)(IrqBuffserSize + 19);

        //
        // Notify _PRS to report long formed ResourceTemplate
        //
        *IsShortFormPkgLength = FALSE;
        break;
      }
    }
  }

  if (DataPtr >= (UINT8 *) ((UINT8 *) Table + Table->Length - (TPM_PRS_RES_NAME_SIZE + TPM_POS_RES_TEMPLATE_MIN_SIZE))) {
    return EFI_NOT_FOUND;
  }

  //
  // 3. Move DataPtr to Interrupt descriptor header and patch interrupt descriptor.
  //     5 bytes for interrupt descriptor header, 2 bytes for End Tag
  //
  DataPtr += NewPkgLength - (5 + IrqBuffserSize + 2);
  //
  //   3.1 Patch Length bit[7:0] of Interrupt descriptor patch interrupt descriptor
  //
  *(DataPtr + 1) = (UINT8)(2 + IrqBuffserSize);
  //
  //   3.2 Patch Interrupt Table Length
  //
  *(DataPtr + 4) = (UINT8)(IrqBuffserSize / sizeof(UINT32));
  //
  //   3.3 Copy patched InterruptNumBuffer
  //
  CopyMem(DataPtr + 5, IrqBuffer, IrqBuffserSize);

  //
  // 4. Jump over Interrupt descriptor and Patch END Tag, set Checksum field to 0
  //
  DataPtr       += 5 + IrqBuffserSize;
  *DataPtr       = ACPI_END_TAG_DESCRIPTOR;
  *(DataPtr + 1) = 0;

  //
  // 5. Jump over new ResourceTemplate. Stuff rest bytes to NOOP
  //
  DataPtr += 2;
  if (DataPtr < DataEndPtr) {
    SetMem(DataPtr, (UINTN)DataEndPtr - (UINTN)DataPtr, AML_NOOP_OP);
  }

  return EFI_SUCCESS;
}

/**
  Patch TPM2 device HID string.  The initial string tag in TPM2 ACPI table is "NNN0000".

  @param[in, out] Table          The TPM2 SSDT ACPI table.

  @return                               HID Update status.

**/
EFI_STATUS
UpdateHID (
  EFI_ACPI_DESCRIPTION_HEADER    *Table
  )
{
  EFI_STATUS  Status;
  UINT8       *DataPtr;
  CHAR8       Hid[TPM_HID_ACPI_SIZE];
  UINT32      ManufacturerID;
  UINT32      FirmwareVersion1;
  UINT32      FirmwareVersion2;
  BOOLEAN     PnpHID;

  PnpHID = TRUE;

  //
  // Initialize HID with Default PNP string
  //
  ZeroMem(Hid, TPM_HID_ACPI_SIZE);

  //
  // Get Manufacturer ID
  //
  Status = Tpm2GetCapabilityManufactureID(&ManufacturerID);
  if (!EFI_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "TPM_PT_MANUFACTURER 0x%08x\n", ManufacturerID));
    //
    // ManufacturerID defined in TCG Vendor ID Registry
    // may tailed with 0x00 or 0x20
    //
    if ((ManufacturerID >> 24) == 0x00 || ((ManufacturerID >> 24) == 0x20)) {
      //
      //  HID containing PNP ID "NNN####"
      //   NNN is uppercase letter for Vendor ID specified by manufacturer
      //
      CopyMem(Hid, &ManufacturerID, 3);
    } else {
      //
      //  HID containing ACP ID "NNNN####"
      //   NNNN is uppercase letter for Vendor ID specified by manufacturer
      //
      CopyMem(Hid, &ManufacturerID, 4);
      PnpHID = FALSE;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Get TPM_PT_MANUFACTURER failed %x!\n", Status));
    ASSERT(FALSE);
    return Status;
  }

  Status = Tpm2GetCapabilityFirmwareVersion(&FirmwareVersion1, &FirmwareVersion2);
  if (!EFI_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "TPM_PT_FIRMWARE_VERSION_1 0x%x\n", FirmwareVersion1));
    DEBUG((DEBUG_INFO, "TPM_PT_FIRMWARE_VERSION_2 0x%x\n", FirmwareVersion2));
    //
    //   #### is Firmware Version 1
    //
    if (PnpHID) {
      AsciiSPrint(Hid + 3, TPM_HID_PNP_SIZE - 3, "%02d%02d", ((FirmwareVersion1 & 0xFFFF0000) >> 16), (FirmwareVersion1 & 0x0000FFFF));
    } else {
      AsciiSPrint(Hid + 4, TPM_HID_ACPI_SIZE - 4, "%02d%02d", ((FirmwareVersion1 & 0xFFFF0000) >> 16), (FirmwareVersion1 & 0x0000FFFF));
    }

  } else {
    DEBUG ((DEBUG_ERROR, "Get TPM_PT_FIRMWARE_VERSION_X failed %x!\n", Status));
    ASSERT(FALSE);
    return Status;
  }

  //
  // Patch HID in ASL code before loading the SSDT.
  //
  for (DataPtr  = (UINT8 *)(Table + 1);
       DataPtr <= (UINT8 *) ((UINT8 *) Table + Table->Length - TPM_HID_PNP_SIZE);
       DataPtr += 1) {
    if (AsciiStrCmp((CHAR8 *)DataPtr,  TPM_HID_TAG) == 0) {
      if (PnpHID) {
        CopyMem(DataPtr, Hid, TPM_HID_PNP_SIZE);
        //
        // if HID is PNP ID, patch the last byte in HID TAG to Noop
        //
        *(DataPtr + TPM_HID_PNP_SIZE) = AML_NOOP_OP;
      } else {

        CopyMem(DataPtr, Hid, TPM_HID_ACPI_SIZE);
      }
      DEBUG((DEBUG_INFO, "TPM2 ACPI _HID is patched to %a\n", DataPtr));

      return Status;
    }
  }

  DEBUG((DEBUG_ERROR, "TPM2 ACPI HID TAG for patch not found!\n"));
  return EFI_NOT_FOUND;
}

/**
  Initialize and publish TPM items in ACPI table.

  @retval   EFI_SUCCESS     The TCG ACPI table is published successfully.
  @retval   Others          The TCG ACPI table is not published.

**/
EFI_STATUS
PublishAcpiTable (
  VOID
  )
{
  EFI_STATUS                     Status;
  EFI_ACPI_TABLE_PROTOCOL        *AcpiTable;
  UINTN                          TableKey;
  EFI_ACPI_DESCRIPTION_HEADER    *Table;
  UINTN                          TableSize;
  UINT32                         *PossibleIrqNumBuf;
  UINT32                         PossibleIrqNumBufSize;
  BOOLEAN                        IsShortFormPkgLength;

  IsShortFormPkgLength = FALSE;

  Status = GetSectionFromFv (
             &gEfiCallerIdGuid,
             EFI_SECTION_RAW,
             0,
             (VOID **) &Table,
             &TableSize
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Measure to PCR[0] with event EV_POST_CODE ACPI DATA.
  // The measurement has to be done before any update.
  // Otherwise, the PCR record would be different after TPM FW update
  // or the PCD configuration change.
  //
  TpmMeasureAndLogData(
    0,
    EV_POST_CODE,
    EV_POSTCODE_INFO_ACPI_DATA,
    ACPI_DATA_LEN,
    Table,
    TableSize
    );

  //
  // Update Table version before measuring it to PCR
  //
  Status = UpdatePPVersion(Table, (CHAR8 *)PcdGetPtr(PcdTcgPhysicalPresenceInterfaceVer));
  ASSERT_EFI_ERROR (Status);

  DEBUG ((
    DEBUG_INFO,
    "Current physical presence interface version - %a\n",
    (CHAR8 *) PcdGetPtr(PcdTcgPhysicalPresenceInterfaceVer)
    ));

  //
  // Update TPM2 HID after measuring it to PCR
  //
  Status = UpdateHID(Table);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  if (PcdGet32(PcdTpm2CurrentIrqNum) != 0) {
    //
    // Patch _PRS interrupt resource only when TPM interrupt is supported
    //
    PossibleIrqNumBuf     = (UINT32 *)PcdGetPtr(PcdTpm2PossibleIrqNumBuf);
    PossibleIrqNumBufSize = (UINT32)PcdGetSize(PcdTpm2PossibleIrqNumBuf);

    if (PossibleIrqNumBufSize <= MAX_PRS_INT_BUF_SIZE && (PossibleIrqNumBufSize % sizeof(UINT32)) == 0) {
      Status = UpdatePossibleResource(Table, PossibleIrqNumBuf, PossibleIrqNumBufSize, &IsShortFormPkgLength);
      DEBUG ((
        DEBUG_INFO,
        "UpdatePossibleResource status - %x. TPM2 service may not ready in OS.\n",
        Status
        ));
    } else {
      DEBUG ((
        DEBUG_INFO,
        "PcdTpm2PossibleIrqNumBuf size %x is not correct. TPM2 service may not ready in OS.\n",
        PossibleIrqNumBufSize
      ));
    }
  }

  ASSERT (Table->OemTableId == SIGNATURE_64 ('T', 'p', 'm', '2', 'T', 'a', 'b', 'l'));
  CopyMem (Table->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (Table->OemId) );
  mTcgNvs = AssignOpRegion (Table, SIGNATURE_32 ('T', 'N', 'V', 'S'), (UINT16) sizeof (TCG_NVS));
  ASSERT (mTcgNvs != NULL);
  mTcgNvs->TpmIrqNum            = PcdGet32(PcdTpm2CurrentIrqNum);
  mTcgNvs->IsShortFormPkgLength = IsShortFormPkgLength;

  Status = ExchangeCommonBuffer (mTcgNvs);

  //
  // Publish the TPM ACPI table. Table is re-checksummed.
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **) &AcpiTable);
  ASSERT_EFI_ERROR (Status);

  TableKey = 0;
  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        Table,
                        TableSize,
                        &TableKey
                        );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Publish TPM2 ACPI table

  @retval   EFI_SUCCESS     The TPM2 ACPI table is published successfully.
  @retval   Others          The TPM2 ACPI table is not published.

**/
EFI_STATUS
PublishTpm2 (
  VOID
  )
{
  EFI_STATUS                     Status;
  EFI_ACPI_TABLE_PROTOCOL        *AcpiTable;
  UINTN                          TableKey;
  UINT64                         OemTableId;
  EFI_TPM2_ACPI_CONTROL_AREA     *ControlArea;
  TPM2_PTP_INTERFACE_TYPE        InterfaceType;

  //
  // Measure to PCR[0] with event EV_POST_CODE ACPI DATA.
  // The measurement has to be done before any update.
  // Otherwise, the PCR record would be different after event log update
  // or the PCD configuration change.
  //
  TpmMeasureAndLogData(
    0,
    EV_POST_CODE,
    EV_POSTCODE_INFO_ACPI_DATA,
    ACPI_DATA_LEN,
    &mTpm2AcpiTemplate,
    mTpm2AcpiTemplate.Header.Length
    );

  mTpm2AcpiTemplate.Header.Revision = PcdGet8(PcdTpm2AcpiTableRev);
  DEBUG((DEBUG_INFO, "Tpm2 ACPI table revision is %d\n", mTpm2AcpiTemplate.Header.Revision));

  //
  // PlatformClass is only valid for version 4 and above
  //    BIT0~15:  PlatformClass
  //    BIT16~31: Reserved
  //
  if (mTpm2AcpiTemplate.Header.Revision >= EFI_TPM2_ACPI_TABLE_REVISION_4) {
    mTpm2AcpiTemplate.Flags = (mTpm2AcpiTemplate.Flags & 0xFFFF0000) | PcdGet8(PcdTpmPlatformClass);
    DEBUG((DEBUG_INFO, "Tpm2 ACPI table PlatformClass is %d\n", (mTpm2AcpiTemplate.Flags & 0x0000FFFF)));
  }

  mTpm2AcpiTemplate.Laml = PcdGet32(PcdTpm2AcpiTableLaml);
  mTpm2AcpiTemplate.Lasa = PcdGet64(PcdTpm2AcpiTableLasa);
  if ((mTpm2AcpiTemplate.Header.Revision < EFI_TPM2_ACPI_TABLE_REVISION_4) ||
      (mTpm2AcpiTemplate.Laml == 0) || (mTpm2AcpiTemplate.Lasa == 0)) {
    //
    // If version is smaller than 4 or Laml/Lasa is not valid, rollback to original Length.
    //
    mTpm2AcpiTemplate.Header.Length = sizeof(EFI_TPM2_ACPI_TABLE);
  }

  InterfaceType = PcdGet8(PcdActiveTpmInterfaceType);
  switch (InterfaceType) {
  case Tpm2PtpInterfaceCrb:
    mTpm2AcpiTemplate.StartMethod = EFI_TPM2_ACPI_TABLE_START_METHOD_COMMAND_RESPONSE_BUFFER_INTERFACE;
    mTpm2AcpiTemplate.AddressOfControlArea = PcdGet64 (PcdTpmBaseAddress) + 0x40;
    ControlArea = (EFI_TPM2_ACPI_CONTROL_AREA *)(UINTN)mTpm2AcpiTemplate.AddressOfControlArea;
    ControlArea->CommandSize  = 0xF80;
    ControlArea->ResponseSize = 0xF80;
    ControlArea->Command      = PcdGet64 (PcdTpmBaseAddress) + 0x80;
    ControlArea->Response     = PcdGet64 (PcdTpmBaseAddress) + 0x80;
    break;
  case Tpm2PtpInterfaceFifo:
  case Tpm2PtpInterfaceTis:
    break;
  default:
    DEBUG((DEBUG_ERROR, "TPM2 InterfaceType get error! %d\n", InterfaceType));
    break;
  }

  CopyMem (mTpm2AcpiTemplate.Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (mTpm2AcpiTemplate.Header.OemId));
  OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  CopyMem (&mTpm2AcpiTemplate.Header.OemTableId, &OemTableId, sizeof (UINT64));
  mTpm2AcpiTemplate.Header.OemRevision      = PcdGet32 (PcdAcpiDefaultOemRevision);
  mTpm2AcpiTemplate.Header.CreatorId        = PcdGet32 (PcdAcpiDefaultCreatorId);
  mTpm2AcpiTemplate.Header.CreatorRevision  = PcdGet32 (PcdAcpiDefaultCreatorRevision);

  //
  // Construct ACPI table
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **) &AcpiTable);
  ASSERT_EFI_ERROR (Status);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        &mTpm2AcpiTemplate,
                        mTpm2AcpiTemplate.Header.Length,
                        &TableKey
                        );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  The driver's entry point.

  It patches and installs ACPI tables used for handling TPM physical presence
  and Memory Clear requests through ACPI method.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval Others          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeTcgAcpi (
  IN EFI_HANDLE                  ImageHandle,
  IN EFI_SYSTEM_TABLE            *SystemTable
  )
{
  EFI_STATUS                     Status;

  if (!CompareGuid (PcdGetPtr(PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm20DtpmGuid)){
    DEBUG ((DEBUG_ERROR, "No TPM2 DTPM instance required!\n"));
    return EFI_UNSUPPORTED;
  }

  Status = PublishAcpiTable ();
  ASSERT_EFI_ERROR (Status);

  //
  // Set TPM2 ACPI table
  //
  Status = PublishTpm2 ();
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

