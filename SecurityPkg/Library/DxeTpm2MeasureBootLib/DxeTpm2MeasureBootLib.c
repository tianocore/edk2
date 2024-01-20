/** @file
  The library instance provides security service of TPM2 measure boot and
  Confidential Computing (CC) measure boot.

  Caution: This file requires additional review when modified.
  This library will have external input - PE/COFF image and GPT partition.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  DxeTpm2MeasureBootLibImageRead() function will make sure the PE/COFF image content
  read is within the image buffer.

  Tcg2MeasurePeImage() function will accept untrusted PE/COFF image and validate its
  data structure within this image buffer before use.

  Tcg2MeasureGptTable() function will receive untrusted GPT partition table, and parse
  partition data carefully.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>

#include <Protocol/Tcg2Protocol.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include <Guid/MeasuredFvHob.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/PeCoffLib.h>
#include <Library/SecurityManagementLib.h>
#include <Library/HobLib.h>
#include <Protocol/CcMeasurement.h>

#include "DxeTpm2MeasureBootLibSanitization.h"

typedef struct {
  EFI_TCG2_PROTOCOL              *Tcg2Protocol;
  EFI_CC_MEASUREMENT_PROTOCOL    *CcProtocol;
} MEASURE_BOOT_PROTOCOLS;

//
// Flag to check GPT partition. It only need be measured once.
//
BOOLEAN  mTcg2MeasureGptTableFlag = FALSE;
UINTN    mTcg2MeasureGptCount     = 0;
VOID     *mTcg2FileBuffer;
UINTN    mTcg2ImageSize;
//
// Measured FV handle cache
//
EFI_HANDLE         mTcg2CacheMeasuredHandle = NULL;
MEASURED_HOB_DATA  *mTcg2MeasuredHobData    = NULL;

/**
  Reads contents of a PE/COFF image in memory buffer.

  Caution: This function may receive untrusted input.
  PE/COFF image is external input, so this function will make sure the PE/COFF image content
  read is within the image buffer.

  @param  FileHandle      Pointer to the file handle to read the PE/COFF image.
  @param  FileOffset      Offset into the PE/COFF image to begin the read operation.
  @param  ReadSize        On input, the size in bytes of the requested read operation.
                          On output, the number of bytes actually read.
  @param  Buffer          Output buffer that contains the data read from the PE/COFF image.

  @retval EFI_SUCCESS     The specified portion of the PE/COFF image was read and the size
**/
EFI_STATUS
EFIAPI
DxeTpm2MeasureBootLibImageRead (
  IN     VOID   *FileHandle,
  IN     UINTN  FileOffset,
  IN OUT UINTN  *ReadSize,
  OUT    VOID   *Buffer
  )
{
  UINTN  EndPosition;

  if ((FileHandle == NULL) || (ReadSize == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (MAX_ADDRESS - FileOffset < *ReadSize) {
    return EFI_INVALID_PARAMETER;
  }

  EndPosition = FileOffset + *ReadSize;
  if (EndPosition > mTcg2ImageSize) {
    *ReadSize = (UINT32)(mTcg2ImageSize - FileOffset);
  }

  if (FileOffset >= mTcg2ImageSize) {
    *ReadSize = 0;
  }

  CopyMem (Buffer, (UINT8 *)((UINTN)FileHandle + FileOffset), *ReadSize);

  return EFI_SUCCESS;
}

/**
  Measure GPT table data into TPM log.

  Caution: This function may receive untrusted input.
  The GPT partition table is external input, so this function should parse partition data carefully.

  @param MeasureBootProtocols    Pointer to the located MeasureBoot protocol instances (i.e. TCG2/CC protocol).
  @param GptHandle               Handle that GPT partition was installed.

  @retval EFI_SUCCESS            Successfully measure GPT table.
  @retval EFI_UNSUPPORTED        Not support GPT table on the given handle.
  @retval EFI_DEVICE_ERROR       Can't get GPT table because device error.
  @retval EFI_OUT_OF_RESOURCES   No enough resource to measure GPT table.
  @retval other error value
**/
EFI_STATUS
EFIAPI
Tcg2MeasureGptTable (
  IN  MEASURE_BOOT_PROTOCOLS  *MeasureBootProtocols,
  IN  EFI_HANDLE              GptHandle
  )
{
  EFI_STATUS                   Status;
  EFI_BLOCK_IO_PROTOCOL        *BlockIo;
  EFI_DISK_IO_PROTOCOL         *DiskIo;
  EFI_PARTITION_TABLE_HEADER   *PrimaryHeader;
  EFI_PARTITION_ENTRY          *PartitionEntry;
  UINT8                        *EntryPtr;
  UINTN                        NumberOfPartition;
  UINT32                       Index;
  UINT8                        *EventPtr;
  EFI_TCG2_EVENT               *Tcg2Event;
  EFI_CC_EVENT                 *CcEvent;
  EFI_GPT_DATA                 *GptData;
  UINT32                       TcgEventSize;
  EFI_TCG2_PROTOCOL            *Tcg2Protocol;
  EFI_CC_MEASUREMENT_PROTOCOL  *CcProtocol;
  EFI_CC_MR_INDEX              MrIndex;
  UINT32                       AllocSize;

  if (mTcg2MeasureGptCount > 0) {
    return EFI_SUCCESS;
  }

  PrimaryHeader = NULL;
  EntryPtr      = NULL;
  EventPtr      = NULL;

  Tcg2Protocol = MeasureBootProtocols->Tcg2Protocol;
  CcProtocol   = MeasureBootProtocols->CcProtocol;

  if ((Tcg2Protocol == NULL) && (CcProtocol == NULL)) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  if (sizeof (EFI_CC_EVENT) != sizeof (EFI_TCG2_EVENT)) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  Status = gBS->HandleProtocol (GptHandle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->HandleProtocol (GptHandle, &gEfiDiskIoProtocolGuid, (VOID **)&DiskIo);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Read the EFI Partition Table Header
  //
  PrimaryHeader = (EFI_PARTITION_TABLE_HEADER *)AllocatePool (BlockIo->Media->BlockSize);
  if (PrimaryHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = DiskIo->ReadDisk (
                     DiskIo,
                     BlockIo->Media->MediaId,
                     1 * BlockIo->Media->BlockSize,
                     BlockIo->Media->BlockSize,
                     (UINT8 *)PrimaryHeader
                     );
  if (EFI_ERROR (Status) || EFI_ERROR (Tpm2SanitizeEfiPartitionTableHeader (PrimaryHeader, BlockIo))) {
    DEBUG ((DEBUG_ERROR, "Failed to read Partition Table Header or invalid Partition Table Header!\n"));
    FreePool (PrimaryHeader);
    return EFI_DEVICE_ERROR;
  }

  //
  // Read the partition entry.
  //
  Status = Tpm2SanitizePrimaryHeaderAllocationSize (PrimaryHeader, &AllocSize);
  if (EFI_ERROR (Status)) {
    FreePool (PrimaryHeader);
    return EFI_BAD_BUFFER_SIZE;
  }

  EntryPtr = (UINT8 *)AllocatePool (AllocSize);
  if (EntryPtr == NULL) {
    FreePool (PrimaryHeader);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = DiskIo->ReadDisk (
                     DiskIo,
                     BlockIo->Media->MediaId,
                     MultU64x32 (PrimaryHeader->PartitionEntryLBA, BlockIo->Media->BlockSize),
                     AllocSize,
                     EntryPtr
                     );
  if (EFI_ERROR (Status)) {
    FreePool (PrimaryHeader);
    FreePool (EntryPtr);
    return EFI_DEVICE_ERROR;
  }

  //
  // Count the valid partition
  //
  PartitionEntry    = (EFI_PARTITION_ENTRY *)EntryPtr;
  NumberOfPartition = 0;
  for (Index = 0; Index < PrimaryHeader->NumberOfPartitionEntries; Index++) {
    if (!IsZeroGuid (&PartitionEntry->PartitionTypeGUID)) {
      NumberOfPartition++;
    }

    PartitionEntry = (EFI_PARTITION_ENTRY *)((UINT8 *)PartitionEntry + PrimaryHeader->SizeOfPartitionEntry);
  }

  //
  // Prepare Data for Measurement (CcProtocol and Tcg2Protocol)
  //
  Status = Tpm2SanitizePrimaryHeaderGptEventSize (PrimaryHeader, NumberOfPartition, &TcgEventSize);
  if (EFI_ERROR (Status)) {
    FreePool (PrimaryHeader);
    FreePool (EntryPtr);
    return EFI_DEVICE_ERROR;
  }

  EventPtr = (UINT8 *)AllocateZeroPool (TcgEventSize);
  if (EventPtr == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  Tcg2Event                       = (EFI_TCG2_EVENT *)EventPtr;
  Tcg2Event->Size                 = TcgEventSize;
  Tcg2Event->Header.HeaderSize    = sizeof (EFI_TCG2_EVENT_HEADER);
  Tcg2Event->Header.HeaderVersion = EFI_TCG2_EVENT_HEADER_VERSION;
  Tcg2Event->Header.PCRIndex      = 5;
  Tcg2Event->Header.EventType     = EV_EFI_GPT_EVENT;
  GptData                         = (EFI_GPT_DATA *)Tcg2Event->Event;

  //
  // Copy the EFI_PARTITION_TABLE_HEADER and NumberOfPartition
  //
  CopyMem ((UINT8 *)GptData, (UINT8 *)PrimaryHeader, sizeof (EFI_PARTITION_TABLE_HEADER));
  GptData->NumberOfPartitions = NumberOfPartition;
  //
  // Copy the valid partition entry
  //
  PartitionEntry    = (EFI_PARTITION_ENTRY *)EntryPtr;
  NumberOfPartition = 0;
  for (Index = 0; Index < PrimaryHeader->NumberOfPartitionEntries; Index++) {
    if (!IsZeroGuid (&PartitionEntry->PartitionTypeGUID)) {
      CopyMem (
        (UINT8 *)&GptData->Partitions + NumberOfPartition * PrimaryHeader->SizeOfPartitionEntry,
        (UINT8 *)PartitionEntry,
        PrimaryHeader->SizeOfPartitionEntry
        );
      NumberOfPartition++;
    }

    PartitionEntry = (EFI_PARTITION_ENTRY *)((UINT8 *)PartitionEntry + PrimaryHeader->SizeOfPartitionEntry);
  }

  //
  // Only one of TCG2_PROTOCOL or CC_MEASUREMENT_PROTOCOL is exposed.
  // So Measure the GPT data with one of the protocol.
  //
  if (CcProtocol != NULL) {
    //
    // EFI_CC_EVENT share the same data structure with EFI_TCG2_EVENT
    // except the MrIndex and PCRIndex in Header.
    // Tcg2Event has been created and initialized before. So only the MrIndex need
    // be adjusted.
    //
    Status = CcProtocol->MapPcrToMrIndex (CcProtocol, Tcg2Event->Header.PCRIndex, &MrIndex);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Cannot map PcrIndex(%d) to MrIndex\n", Tcg2Event->Header.PCRIndex));
      goto Exit;
    }

    CcEvent                 = (EFI_CC_EVENT *)EventPtr;
    CcEvent->Header.MrIndex = MrIndex;
    Status                  = CcProtocol->HashLogExtendEvent (
                                            CcProtocol,
                                            0,
                                            (EFI_PHYSICAL_ADDRESS)(UINTN)(VOID *)GptData,
                                            (UINT64)TcgEventSize - OFFSET_OF (EFI_TCG2_EVENT, Event),
                                            CcEvent
                                            );
    if (!EFI_ERROR (Status)) {
      mTcg2MeasureGptCount++;
    }

    DEBUG ((DEBUG_INFO, "DxeTpm2MeasureBootHandler - Cc MeasureGptTable - %r\n", Status));
  } else if (Tcg2Protocol != NULL) {
    //
    // If Tcg2Protocol is installed, then Measure GPT data with this protocol.
    //
    Status = Tcg2Protocol->HashLogExtendEvent (
                             Tcg2Protocol,
                             0,
                             (EFI_PHYSICAL_ADDRESS)(UINTN)(VOID *)GptData,
                             (UINT64)TcgEventSize -  OFFSET_OF (EFI_TCG2_EVENT, Event),
                             Tcg2Event
                             );
    if (!EFI_ERROR (Status)) {
      mTcg2MeasureGptCount++;
    }

    DEBUG ((DEBUG_INFO, "DxeTpm2MeasureBootHandler - Tcg2 MeasureGptTable - %r\n", Status));
  }

Exit:
  if (PrimaryHeader != NULL) {
    FreePool (PrimaryHeader);
  }

  if (EntryPtr != NULL) {
    FreePool (EntryPtr);
  }

  if (EventPtr != NULL) {
    FreePool (EventPtr);
  }

  return Status;
}

/**
  Measure PE image into TPM log based on the authenticode image hashing in
  PE/COFF Specification 8.0 Appendix A.

  Caution: This function may receive untrusted input.
  PE/COFF image is external input, so this function will validate its data structure
  within this image buffer before use.

  @param[in] MeasureBootProtocols   Pointer to the located MeasureBoot protocol instances.
  @param[in] ImageAddress           Start address of image buffer.
  @param[in] ImageSize              Image size
  @param[in] LinkTimeBase           Address that the image is loaded into memory.
  @param[in] ImageType              Image subsystem type.
  @param[in] FilePath               File path is corresponding to the input image.

  @retval EFI_SUCCESS            Successfully measure image.
  @retval EFI_OUT_OF_RESOURCES   No enough resource to measure image.
  @retval EFI_UNSUPPORTED        ImageType is unsupported or PE image is mal-format.
  @retval other error value
**/
EFI_STATUS
EFIAPI
Tcg2MeasurePeImage (
  IN  MEASURE_BOOT_PROTOCOLS    *MeasureBootProtocols,
  IN  EFI_PHYSICAL_ADDRESS      ImageAddress,
  IN  UINTN                     ImageSize,
  IN  UINTN                     LinkTimeBase,
  IN  UINT16                    ImageType,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  EFI_STATUS                   Status;
  EFI_TCG2_EVENT               *Tcg2Event;
  EFI_IMAGE_LOAD_EVENT         *ImageLoad;
  UINT32                       FilePathSize;
  UINT32                       EventSize;
  EFI_CC_EVENT                 *CcEvent;
  EFI_CC_MEASUREMENT_PROTOCOL  *CcProtocol;
  EFI_TCG2_PROTOCOL            *Tcg2Protocol;
  UINT8                        *EventPtr;
  EFI_CC_MR_INDEX              MrIndex;

  Status    = EFI_UNSUPPORTED;
  ImageLoad = NULL;
  EventPtr  = NULL;
  Tcg2Event = NULL;

  Tcg2Protocol = MeasureBootProtocols->Tcg2Protocol;
  CcProtocol   = MeasureBootProtocols->CcProtocol;

  if ((Tcg2Protocol == NULL) && (CcProtocol == NULL)) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  if (sizeof (EFI_CC_EVENT) != sizeof (EFI_TCG2_EVENT)) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  FilePathSize = (UINT32)GetDevicePathSize (FilePath);
  Status       = Tpm2SanitizePeImageEventSize (FilePathSize, &EventSize);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Determine destination PCR by BootPolicy
  //
  // from a malicious GPT disk partition
  EventPtr = AllocateZeroPool (EventSize);
  if (EventPtr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Tcg2Event                       = (EFI_TCG2_EVENT *)EventPtr;
  Tcg2Event->Size                 = EventSize;
  Tcg2Event->Header.HeaderSize    = sizeof (EFI_TCG2_EVENT_HEADER);
  Tcg2Event->Header.HeaderVersion = EFI_TCG2_EVENT_HEADER_VERSION;
  ImageLoad                       = (EFI_IMAGE_LOAD_EVENT *)Tcg2Event->Event;

  switch (ImageType) {
    case EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION:
      Tcg2Event->Header.EventType = EV_EFI_BOOT_SERVICES_APPLICATION;
      Tcg2Event->Header.PCRIndex  = 4;
      break;
    case EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
      Tcg2Event->Header.EventType = EV_EFI_BOOT_SERVICES_DRIVER;
      Tcg2Event->Header.PCRIndex  = 2;
      break;
    case EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
      Tcg2Event->Header.EventType = EV_EFI_RUNTIME_SERVICES_DRIVER;
      Tcg2Event->Header.PCRIndex  = 2;
      break;
    default:
      DEBUG (
        (
         DEBUG_ERROR,
         "Tcg2MeasurePeImage: Unknown subsystem type %d",
         ImageType
        )
        );
      goto Finish;
  }

  ImageLoad->ImageLocationInMemory = ImageAddress;
  ImageLoad->ImageLengthInMemory   = ImageSize;
  ImageLoad->ImageLinkTimeAddress  = LinkTimeBase;
  ImageLoad->LengthOfDevicePath    = FilePathSize;
  if ((FilePath != NULL) && (FilePathSize != 0)) {
    CopyMem (ImageLoad->DevicePath, FilePath, FilePathSize);
  }

  //
  // Log the PE data
  //
  if (CcProtocol != NULL) {
    Status = CcProtocol->MapPcrToMrIndex (CcProtocol, Tcg2Event->Header.PCRIndex, &MrIndex);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Cannot map PcrIndex(%d) to MrIndex\n", Tcg2Event->Header.PCRIndex));
      goto Finish;
    }

    CcEvent                 = (EFI_CC_EVENT *)EventPtr;
    CcEvent->Header.MrIndex = MrIndex;

    Status = CcProtocol->HashLogExtendEvent (
                           CcProtocol,
                           PE_COFF_IMAGE,
                           ImageAddress,
                           ImageSize,
                           CcEvent
                           );
    DEBUG ((DEBUG_INFO, "DxeTpm2MeasureBootHandler - Cc MeasurePeImage - %r\n", Status));
  } else if (Tcg2Protocol != NULL) {
    Status = Tcg2Protocol->HashLogExtendEvent (
                             Tcg2Protocol,
                             PE_COFF_IMAGE,
                             ImageAddress,
                             ImageSize,
                             Tcg2Event
                             );
    DEBUG ((DEBUG_INFO, "DxeTpm2MeasureBootHandler - Tcg2 MeasurePeImage - %r\n", Status));
  }

  if (Status == EFI_VOLUME_FULL) {
    //
    // Volume full here means the image is hashed and its result is extended to PCR.
    // But the event log can't be saved since log area is full.
    // Just return EFI_SUCCESS in order not to block the image load.
    //
    Status = EFI_SUCCESS;
  }

Finish:
  if (EventPtr != NULL) {
    FreePool (EventPtr);
  }

  return Status;
}

/**
  Get the measure boot protocols.

  There are 2 measure boot, TCG2 protocol based and Cc measurement protocol based.

  @param  MeasureBootProtocols  Pointer to the located measure boot protocol instances.

  @retval EFI_SUCCESS           Successfully locate the measure boot protocol instances (at least one instance).
  @retval EFI_UNSUPPORTED       Measure boot is not supported.
**/
EFI_STATUS
EFIAPI
GetMeasureBootProtocols (
  MEASURE_BOOT_PROTOCOLS  *MeasureBootProtocols
  )
{
  EFI_STATUS                        Status;
  EFI_TCG2_PROTOCOL                 *Tcg2Protocol;
  EFI_CC_MEASUREMENT_PROTOCOL       *CcProtocol;
  EFI_TCG2_BOOT_SERVICE_CAPABILITY  Tcg2ProtocolCapability;
  EFI_CC_BOOT_SERVICE_CAPABILITY    CcProtocolCapability;

  CcProtocol = NULL;
  Status     = gBS->LocateProtocol (&gEfiCcMeasurementProtocolGuid, NULL, (VOID **)&CcProtocol);
  if (EFI_ERROR (Status)) {
    //
    // Cc Measurement protocol is not installed.
    //
    DEBUG ((DEBUG_VERBOSE, "CcMeasurementProtocol is not installed. - %r\n", Status));
  } else {
    ZeroMem (&CcProtocolCapability, sizeof (CcProtocolCapability));
    CcProtocolCapability.Size = sizeof (CcProtocolCapability);
    Status                    = CcProtocol->GetCapability (CcProtocol, &CcProtocolCapability);
    if (EFI_ERROR (Status) || (CcProtocolCapability.CcType.Type == EFI_CC_TYPE_NONE)) {
      DEBUG ((DEBUG_ERROR, " CcProtocol->GetCapability returns : %x, %r\n", CcProtocolCapability.CcType.Type, Status));
      CcProtocol = NULL;
    }
  }

  Tcg2Protocol = NULL;
  Status       = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **)&Tcg2Protocol);
  if (EFI_ERROR (Status)) {
    //
    // Tcg2 protocol is not installed. So, TPM2 is not present.
    //
    DEBUG ((DEBUG_VERBOSE, "Tcg2Protocol is not installed. - %r\n", Status));
  } else {
    Tcg2ProtocolCapability.Size = (UINT8)sizeof (Tcg2ProtocolCapability);
    Status                      = Tcg2Protocol->GetCapability (Tcg2Protocol, &Tcg2ProtocolCapability);
    if (EFI_ERROR (Status) || (!Tcg2ProtocolCapability.TPMPresentFlag)) {
      //
      // TPM device doesn't work or activate.
      //
      DEBUG ((DEBUG_ERROR, "TPMPresentFlag=FALSE %r\n", Status));
      Tcg2Protocol = NULL;
    }
  }

  MeasureBootProtocols->Tcg2Protocol = Tcg2Protocol;
  MeasureBootProtocols->CcProtocol   = CcProtocol;

  return (Tcg2Protocol == NULL && CcProtocol == NULL) ? EFI_UNSUPPORTED : EFI_SUCCESS;
}

/**
  The security handler is used to abstract platform-specific policy
  from the DXE core response to an attempt to use a file that returns a
  given status for the authentication check from the section extraction protocol.

  The possible responses in a given SAP implementation may include locking
  flash upon failure to authenticate, attestation logging for all signed drivers,
  and other exception operations.  The File parameter allows for possible logging
  within the SAP of the driver.

  If the file specified by File with an authentication status specified by
  AuthenticationStatus is safe for the DXE Core to use, then EFI_SUCCESS is returned.

  If the file specified by File with an authentication status specified by
  AuthenticationStatus is not safe for the DXE Core to use under any circumstances,
  then EFI_ACCESS_DENIED is returned.

  If the file specified by File with an authentication status specified by
  AuthenticationStatus is not safe for the DXE Core to use right now, but it
  might be possible to use it at a future time, then EFI_SECURITY_VIOLATION is
  returned.

  If check image specified by FileBuffer and File is NULL meanwhile, return EFI_ACCESS_DENIED.

  @param[in]      AuthenticationStatus  This is the authentication status returned
                                        from the securitymeasurement services for the
                                        input file.
  @param[in]      File       This is a pointer to the device path of the file that is
                             being dispatched. This will optionally be used for logging.
  @param[in]      FileBuffer File buffer matches the input file device path.
  @param[in]      FileSize   Size of File buffer matches the input file device path.
  @param[in]      BootPolicy A boot policy that was used to call LoadImage() UEFI service.

  @retval EFI_SUCCESS             The file specified by DevicePath and non-NULL
                                  FileBuffer did authenticate, and the platform policy dictates
                                  that the DXE Foundation may use the file.
  @retval other error value
**/
EFI_STATUS
EFIAPI
DxeTpm2MeasureBootHandler (
  IN  UINT32                          AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *File  OPTIONAL,
  IN  VOID                            *FileBuffer,
  IN  UINTN                           FileSize,
  IN  BOOLEAN                         BootPolicy
  )
{
  MEASURE_BOOT_PROTOCOLS              MeasureBootProtocols;
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL            *OrigDevicePathNode;
  EFI_HANDLE                          Handle;
  EFI_HANDLE                          TempHandle;
  BOOLEAN                             ApplicationRequired;
  PE_COFF_LOADER_IMAGE_CONTEXT        ImageContext;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvbProtocol;
  EFI_PHYSICAL_ADDRESS                FvAddress;
  UINT32                              Index;

  MeasureBootProtocols.Tcg2Protocol = NULL;
  MeasureBootProtocols.CcProtocol   = NULL;

  Status = GetMeasureBootProtocols (&MeasureBootProtocols);

  if (EFI_ERROR (Status)) {
    //
    // None of Measured boot protocols (Tcg2, Cc) is installed.
    // Don't do any measurement, and directly return EFI_SUCCESS.
    //
    DEBUG ((DEBUG_INFO, "None of Tcg2Protocol/CcMeasurementProtocol is installed.\n"));
    return EFI_SUCCESS;
  }

  DEBUG (
    (
     DEBUG_INFO,
     "Tcg2Protocol = %p, CcMeasurementProtocol = %p\n",
     MeasureBootProtocols.Tcg2Protocol,
     MeasureBootProtocols.CcProtocol
    )
    );

  //
  // Copy File Device Path
  //
  OrigDevicePathNode = DuplicateDevicePath (File);

  //
  // 1. Check whether this device path support BlockIo protocol.
  // Is so, this device path may be a GPT device path.
  //
  DevicePathNode = OrigDevicePathNode;
  Status         = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &DevicePathNode, &Handle);
  if (!EFI_ERROR (Status) && !mTcg2MeasureGptTableFlag) {
    //
    // Find the gpt partition on the given devicepath
    //
    DevicePathNode = OrigDevicePathNode;
    ASSERT (DevicePathNode != NULL);
    while (!IsDevicePathEnd (DevicePathNode)) {
      //
      // Find the Gpt partition
      //
      if ((DevicePathType (DevicePathNode) == MEDIA_DEVICE_PATH) &&
          (DevicePathSubType (DevicePathNode) == MEDIA_HARDDRIVE_DP))
      {
        //
        // Check whether it is a gpt partition or not
        //
        if ((((HARDDRIVE_DEVICE_PATH *)DevicePathNode)->MBRType == MBR_TYPE_EFI_PARTITION_TABLE_HEADER) &&
            (((HARDDRIVE_DEVICE_PATH *)DevicePathNode)->SignatureType == SIGNATURE_TYPE_GUID))
        {
          //
          // Change the partition device path to its parent device path (disk) and get the handle.
          //
          DevicePathNode->Type    = END_DEVICE_PATH_TYPE;
          DevicePathNode->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
          DevicePathNode          = OrigDevicePathNode;
          Status                  = gBS->LocateDevicePath (
                                           &gEfiDiskIoProtocolGuid,
                                           &DevicePathNode,
                                           &Handle
                                           );
          if (!EFI_ERROR (Status)) {
            //
            // Measure GPT disk.
            //
            Status = Tcg2MeasureGptTable (&MeasureBootProtocols, Handle);

            if (!EFI_ERROR (Status)) {
              //
              // GPT disk check done.
              //
              mTcg2MeasureGptTableFlag = TRUE;
            }
          }

          FreePool (OrigDevicePathNode);
          OrigDevicePathNode = DuplicateDevicePath (File);
          ASSERT (OrigDevicePathNode != NULL);
          break;
        }
      }

      DevicePathNode = NextDevicePathNode (DevicePathNode);
    }
  }

  //
  // 2. Measure PE image.
  //
  ApplicationRequired = FALSE;

  //
  // Check whether this device path support FVB protocol.
  //
  DevicePathNode = OrigDevicePathNode;
  Status         = gBS->LocateDevicePath (&gEfiFirmwareVolumeBlockProtocolGuid, &DevicePathNode, &Handle);
  if (!EFI_ERROR (Status)) {
    //
    // Don't check FV image, and directly return EFI_SUCCESS.
    // It can be extended to the specific FV authentication according to the different requirement.
    //
    if (IsDevicePathEnd (DevicePathNode)) {
      return EFI_SUCCESS;
    }

    //
    // The PE image from unmeasured Firmware volume need be measured
    // The PE image from measured Firmware volume will be measured according to policy below.
    //   If it is driver, do not measure
    //   If it is application, still measure.
    //
    ApplicationRequired = TRUE;

    if ((mTcg2CacheMeasuredHandle != Handle) && (mTcg2MeasuredHobData != NULL)) {
      //
      // Search for Root FV of this PE image
      //
      TempHandle = Handle;
      do {
        Status = gBS->HandleProtocol (
                        TempHandle,
                        &gEfiFirmwareVolumeBlockProtocolGuid,
                        (VOID **)&FvbProtocol
                        );
        TempHandle = FvbProtocol->ParentHandle;
      } while (!EFI_ERROR (Status) && FvbProtocol->ParentHandle != NULL);

      //
      // Search in measured FV Hob
      //
      Status = FvbProtocol->GetPhysicalAddress (FvbProtocol, &FvAddress);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      ApplicationRequired = FALSE;

      for (Index = 0; Index < mTcg2MeasuredHobData->Num; Index++) {
        if (mTcg2MeasuredHobData->MeasuredFvBuf[Index].BlobBase == FvAddress) {
          //
          // Cache measured FV for next measurement
          //
          mTcg2CacheMeasuredHandle = Handle;
          ApplicationRequired      = TRUE;
          break;
        }
      }
    }
  }

  //
  // File is not found.
  //
  if (FileBuffer == NULL) {
    Status = EFI_SECURITY_VIOLATION;
    goto Finish;
  }

  mTcg2ImageSize  = FileSize;
  mTcg2FileBuffer = FileBuffer;

  //
  // Measure PE Image
  //
  DevicePathNode = OrigDevicePathNode;
  ZeroMem (&ImageContext, sizeof (ImageContext));
  ImageContext.Handle    = (VOID *)FileBuffer;
  ImageContext.ImageRead = (PE_COFF_LOADER_READ_FILE)DxeTpm2MeasureBootLibImageRead;

  //
  // Get information about the image being loaded
  //
  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    //
    // Check for invalid parameters.
    //
    if (File == NULL) {
      Status = EFI_ACCESS_DENIED;
    }

    //
    // The information can't be got from the invalid PeImage
    //
    goto Finish;
  }

  //
  // Measure only application if Application flag is set
  // Measure drivers and applications if Application flag is not set
  //
  if ((!ApplicationRequired) ||
      (ApplicationRequired && (ImageContext.ImageType == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION)))
  {
    //
    // Print the image path to be measured.
    //
    DEBUG_CODE_BEGIN ();
    CHAR16  *ToText;
    ToText = ConvertDevicePathToText (
               DevicePathNode,
               FALSE,
               TRUE
               );
    if (ToText != NULL) {
      DEBUG ((DEBUG_INFO, "The measured image path is %s.\n", ToText));
      FreePool (ToText);
    }

    DEBUG_CODE_END ();

    //
    // Measure PE image into TPM log.
    //
    Status = Tcg2MeasurePeImage (
               &MeasureBootProtocols,
               (EFI_PHYSICAL_ADDRESS)(UINTN)FileBuffer,
               FileSize,
               (UINTN)ImageContext.ImageAddress,
               ImageContext.ImageType,
               DevicePathNode
               );
  }

  //
  // Done, free the allocated resource.
  //
Finish:
  if (OrigDevicePathNode != NULL) {
    FreePool (OrigDevicePathNode);
  }

  DEBUG ((DEBUG_INFO, "DxeTpm2MeasureBootHandler - %r\n", Status));

  return Status;
}

/**
  Register the security handler to provide TPM measure boot service.

  @param  ImageHandle  ImageHandle of the loaded driver.
  @param  SystemTable  Pointer to the EFI System Table.

  @retval  EFI_SUCCESS            Register successfully.
  @retval  EFI_OUT_OF_RESOURCES   No enough memory to register this handler.
**/
EFI_STATUS
EFIAPI
DxeTpm2MeasureBootLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  GuidHob = NULL;

  GuidHob = GetFirstGuidHob (&gMeasuredFvHobGuid);

  if (GuidHob != NULL) {
    mTcg2MeasuredHobData = GET_GUID_HOB_DATA (GuidHob);
  }

  return RegisterSecurity2Handler (
           DxeTpm2MeasureBootHandler,
           EFI_AUTH_OPERATION_MEASURE_IMAGE | EFI_AUTH_OPERATION_IMAGE_REQUIRED
           );
}
