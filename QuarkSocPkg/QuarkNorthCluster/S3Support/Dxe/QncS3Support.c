/** @file
This is the driver that implements the QNC S3 Support protocol

Copyright (c) 2013-2016 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "QncS3Support.h"

//
// Global Variables
//
EFI_QNC_S3_SUPPORT_PROTOCOL mQncS3SupportProtocol;
QNC_S3_PARAMETER_HEADER     *mS3Parameter;
UINT32                      mQncS3ImageEntryPoint;
VOID                        *mQncS3ImageAddress;
UINTN                       mQncS3ImageSize;

extern EFI_GUID gQncS3CodeInLockBoxGuid;
extern EFI_GUID gQncS3ContextInLockBoxGuid;

/**

    Create a buffer that is used to store context information for use with
    dispatch functions.

    @retval EFI_SUCCESS - Buffer allocated and initialized.

**/
EFI_STATUS
CreateContextBuffer (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;
  UINT32                ContextStoreSize;

  ContextStoreSize = EFI_PAGE_SIZE;

  //
  // Allcoate <4G EfiReservedMemory
  //
  Address = 0xFFFFFFFF;
  Status = gBS->AllocatePages (AllocateMaxAddress, EfiReservedMemoryType, EFI_SIZE_TO_PAGES (ContextStoreSize), &Address);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  mS3Parameter  = (QNC_S3_PARAMETER_HEADER *) (UINTN) Address;

  //
  // Determine the maximum number of context entries that can be stored in this
  // table.
  //
  mS3Parameter->MaxContexts = ((ContextStoreSize - sizeof(QNC_S3_PARAMETER_HEADER)) / sizeof(EFI_DISPATCH_CONTEXT_UNION)) + 1;
  mS3Parameter->StorePosition = 0;

  return Status;
}

//
// Functions
//
EFI_STATUS
EFIAPI
QncS3SupportEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

  Routine Description:

    QNC S3 support driver entry point

  Arguments:

    ImageHandle     - Handle for the image of this driver
    SystemTable     - Pointer to the EFI System Table

  Returns:

    EFI_STATUS

--*/
{
  EFI_STATUS  Status;
  VOID        *TmpPtr;
  EFI_EVENT   Event;

  //
  // If the protocol is found execution is happening in ACPI NVS memory.  If it
  // is not found copy the driver into ACPI NVS memory and pass control to it.
  //
  Status = gBS->LocateProtocol (&gEfiCallerIdGuid, NULL, &TmpPtr);

  //
  // Load the QNC S3 image
  //
  if (EFI_ERROR (Status)) {
    Status = LoadQncS3Image (SystemTable);
    ASSERT_EFI_ERROR (Status);

  } else {
    DEBUG ((DEBUG_INFO, "QncS3SupportEntryPoint() in reserved memory - Begin\n"));
    //
    // Allocate and initialize context buffer.
    //
    Status = CreateContextBuffer ();

    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Install the QNC S3 Support protocol
    //
    mQncS3SupportProtocol.SetDispatchItem = QncS3SetDispatchItem;
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &ImageHandle,
                    &gEfiQncS3SupportProtocolGuid,
                    &mQncS3SupportProtocol,
                    NULL
                    );

    mQncS3ImageAddress = (VOID *)(UINTN)PcdGet64(PcdQncS3CodeInLockBoxAddress);
    mQncS3ImageSize    = (UINTN)PcdGet64(PcdQncS3CodeInLockBoxSize);
    DEBUG ((DEBUG_INFO, "QncS3SupportEntry Code = %08x, Size = %08x\n", (UINTN)mQncS3ImageAddress, mQncS3ImageSize));
    DEBUG ((DEBUG_INFO, "QncS3SupportEntry Contex = %08x, Size = %08x\n", (UINTN)mS3Parameter, EFI_PAGE_SIZE));
    ASSERT (mQncS3ImageAddress != 0);

    //
    // Register EFI_END_OF_DXE_EVENT_GROUP_GUID event.
    //
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    QncS3BootEvent,
                    NULL,
                    &gEfiEndOfDxeEventGroupGuid,
                    &Event
                    );
    ASSERT_EFI_ERROR (Status);

    DEBUG ((DEBUG_INFO, "QncS3SupportEntryPoint() in reserved memory - End\n"));
  }



  return Status;
}

EFI_STATUS
EFIAPI
QncS3SetDispatchItem (
  IN     EFI_QNC_S3_SUPPORT_PROTOCOL   *This,
  IN     EFI_QNC_S3_DISPATCH_ITEM      *DispatchItem,
  OUT  VOID                            **S3DispatchEntryPoint,
  OUT  VOID                            **Context
  )
/*++

Routine Description:

  Set an item to be dispatched at S3 resume time. At the same time, the entry point
  of the QNC S3 support image is returned to be used in subsequent boot script save
  call

Arguments:

  This                    - Pointer to the protocol instance.
  DispatchItem            - The item to be dispatched.
  S3DispatchEntryPoint    - The entry point of the QNC S3 support image.

Returns:

  EFI_STATUS              - Successfully completed.
  EFI_OUT_OF_RESOURCES    - Out of resources.

--*/
{

  DEBUG ((DEBUG_INFO, "QncS3SetDispatchItem() Start\n"));

  //
  // Set default values.
  //
  *S3DispatchEntryPoint = NULL;
  *Context = NULL;

  //
  // Determine if this entry will fit.
  //
  if (mS3Parameter->StorePosition >= mS3Parameter->MaxContexts) {
    DEBUG ((DEBUG_INFO, "QncS3SetDispatchItem exceeds max length - 0x%08x\n", (UINTN)mS3Parameter->MaxContexts));
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Calculate the size required;
  // ** Always round up to be 8 byte aligned
  //
  switch (DispatchItem->Type) {
    case QncS3ItemTypeInitPcieRootPortDownstream:
      *S3DispatchEntryPoint = (VOID*) (UINTN)QncS3InitPcieRootPortDownstream;
      *Context = &mS3Parameter->Contexts[mS3Parameter->StorePosition];
       CopyMem (&mS3Parameter->Contexts[mS3Parameter->StorePosition], DispatchItem->Parameter, sizeof(UINT32));
      DEBUG ((DEBUG_INFO, "QncS3InitPcieRootPortDownstream @ 0x%08x - context  0x%08x\n", (UINTN)*S3DispatchEntryPoint, (UINTN)*Context));
       break;

    default:
      return EFI_UNSUPPORTED;

  }

  mS3Parameter->StorePosition ++;
  DEBUG ((DEBUG_INFO, "QncS3SetDispatchItem() End\n"));

  return EFI_SUCCESS;
}

EFI_STATUS
LoadQncS3Image (
  IN  EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:

  Load the QNC S3 Image into Efi Reserved Memory below 4G.

Arguments:

  ImageEntryPoint     the ImageEntryPoint after success loading

Returns:

  EFI_STATUS

--*/
{
  EFI_STATUS                                    Status;
  UINT8                                         *Buffer;
  UINTN                                         BufferSize;
  VOID                                          *FfsBuffer;
  PE_COFF_LOADER_IMAGE_CONTEXT                  ImageContext;
  EFI_HANDLE                                    NewImageHandle;

  //
  // Install NULL protocol on module file handle to indicate that the entry point
  // has been called for the first time.
  //
  NewImageHandle = NULL;
  Status = gBS->InstallProtocolInterface (
    &NewImageHandle,
    &gEfiCallerIdGuid,
    EFI_NATIVE_INTERFACE,
    NULL
    );


  //
  // Find this module so it can be loaded again.
  //
  Status = GetSectionFromAnyFv  (
             &gEfiCallerIdGuid,
             EFI_SECTION_PE32,
             0,
             (VOID**) &Buffer,
             &BufferSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }


  //
  // Get information about the image being loaded.
  //
  ImageContext.Handle = Buffer;
  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

  //
  // Get information about the image being loaded
  //
  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->AllocatePool (
                EfiReservedMemoryType,
                BufferSize + ImageContext.SectionAlignment,
                &FfsBuffer
                );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "LoadQncS3Image failed for no enough space! \n"));
    return EFI_OUT_OF_RESOURCES;
  }

  mQncS3ImageAddress = FfsBuffer;
  mQncS3ImageSize    = BufferSize + ImageContext.SectionAlignment;
  Status = PcdSet64S (PcdQncS3CodeInLockBoxAddress, (UINT64)(UINTN)mQncS3ImageAddress);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet64S (PcdQncS3CodeInLockBoxSize, (UINT64)mQncS3ImageSize);
  ASSERT_EFI_ERROR (Status);
  //
  // Align buffer on section boundary
  //
  ImageContext.ImageAddress = (PHYSICAL_ADDRESS)(UINTN)FfsBuffer;
  if (ImageContext.SectionAlignment != 0) {
    ImageContext.ImageAddress += ImageContext.SectionAlignment - 1;
    ImageContext.ImageAddress &= ~(ImageContext.SectionAlignment - 1);
  }

  //
  // Load the image to our new buffer
  //
  Status = PeCoffLoaderLoadImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    gBS->FreePool (FfsBuffer);
    DEBUG ((DEBUG_INFO, "LoadQncS3Image failed for PeCoffLoaderLoadImage failure! \n"));
    return Status;
  }

  //
  // Relocate the image in our new buffer
  //
  Status = PeCoffLoaderRelocateImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    PeCoffLoaderUnloadImage (&ImageContext);
    gBS->FreePool (FfsBuffer);
    DEBUG ((DEBUG_INFO, "LoadQncS3Image failed for PeCoffLoaderRelocateImage failure! \n"));
    return Status;
  }

  //
  // Invalidate instruction cache and pass control to the image.  This will perform
  // the initialization of the module and publish the supporting protocols.
  //
  InvalidateInstructionCacheRange ((VOID *)(UINTN)ImageContext.ImageAddress, (UINTN)ImageContext.ImageSize);
  Status = ((EFI_IMAGE_ENTRY_POINT)(UINTN)(ImageContext.EntryPoint)) (NewImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    gBS->FreePool (FfsBuffer);
    return Status;
  }

  return EFI_SUCCESS;

}

EFI_STATUS
QncS3InitPcieRootPortDownstream (
  IN EFI_HANDLE ImageHandle,
  IN VOID       *Context
  )
/*++

  Routine Description:
    Perform Init Root Port Downstream devices on S3 resume

  Arguments:
    Parameter         Parameters passed in from DXE

  Returns:
    EFI_STATUS

--*/
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "QncS3InitPcieRootPortDownstream() Begin\n"));

  //
  // Initialize the device behind the root port.
  //
  Status = PciExpressInit ();

  //
  // Not checking the error status here - downstream device not present does not
  // mean an error of this root port. Our return status of EFI_SUCCESS means this
  // port is enabled and outer function depends on this return status to do
  // subsequent initializations.
  //

  if (Status != EFI_SUCCESS){
    DEBUG ((DEBUG_INFO, "QncS3InitPcieRootPortDownstream() failed\n"));
  }

  DEBUG ((DEBUG_INFO, "QncS3InitPcieRootPortDownstream() End\n"));
  return Status;
}

VOID
EFIAPI
QncS3BootEvent (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_STATUS  Status;

  //
  // These 2 boxes will be restored by RestoreAllLockBoxInPlace in S3Resume automatically
  //
  DEBUG ((DEBUG_INFO, "SaveLockBox QncS3Code = %08x, Size = %08x\n", (UINTN)mQncS3ImageAddress, mQncS3ImageSize));
  SaveLockBox(&gQncS3CodeInLockBoxGuid, mQncS3ImageAddress, mQncS3ImageSize);
  Status = SetLockBoxAttributes (&gQncS3CodeInLockBoxGuid, LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "SaveLockBox QncS3Context = %08x, Size = %08x\n", (UINTN)mS3Parameter, EFI_PAGE_SIZE));
  SaveLockBox(&gQncS3ContextInLockBoxGuid, (VOID *)mS3Parameter, EFI_PAGE_SIZE);
  Status = SetLockBoxAttributes (&gQncS3ContextInLockBoxGuid, LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE);
  ASSERT_EFI_ERROR (Status);
}

