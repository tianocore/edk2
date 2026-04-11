/** @file
  This is the code for Boot Script Executer module.

  This driver is dispatched by Dxe core and the driver will reload itself to ACPI reserved memory
  in the entry point. The functionality is to interpret and restore the S3 boot script

Copyright (c) 2006 - 2022, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ScriptExecute.h"

EFI_GUID  mBootScriptExecutorImageGuid = {
  0x9a8d3433, 0x9fe8, 0x42b6, { 0x87, 0xb, 0x1e, 0x31, 0xc8, 0x4e, 0xbe, 0x3b }
};

BOOLEAN  mPage1GSupport  = FALSE;
UINT64   mAddressEncMask = 0;

/**
  Entry function of Boot script exector. This function will be executed in
  S3 boot path.
  This function should not return, because it is invoked by switch stack.

  @param  AcpiS3Context    a pointer to a structure of ACPI_S3_CONTEXT
  @param  PeiS3ResumeState a pointer to a structure of PEI_S3_RESUME_STATE

  @retval EFI_INVALID_PARAMETER - OS waking vector not found
  @retval EFI_UNSUPPORTED - something wrong when we resume to OS
**/
EFI_STATUS
EFIAPI
S3BootScriptExecutorEntryFunction (
  IN ACPI_S3_CONTEXT      *AcpiS3Context,
  IN PEI_S3_RESUME_STATE  *PeiS3ResumeState
  )
{
  EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *Facs;
  EFI_STATUS                                    Status;
  UINTN                                         TempStackTop;
  UINTN                                         TempStack[0x10];
  UINTN                                         AsmTransferControl16Address;
  IA32_DESCRIPTOR                               IdtDescriptor;

  //
  // Disable interrupt of Debug timer, since new IDT table cannot handle it.
  //
  SaveAndSetDebugTimerInterrupt (FALSE);

  AsmReadIdtr (&IdtDescriptor);
  //
  // Restore IDT for debug
  //
  SetIdtEntry (AcpiS3Context);

  //
  // Initialize Debug Agent to support source level debug in S3 path, it will disable interrupt and Debug Timer.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_S3, (VOID *)&IdtDescriptor, NULL);

  //
  // Because not install BootScriptExecute PPI(used just in this module), So just pass NULL
  // for that parameter.
  //
  Status = S3BootScriptExecute ();

  //
  // If invalid script table or opcode in S3 boot script table.
  //
  ASSERT_EFI_ERROR (Status);

  if (EFI_ERROR (Status)) {
    CpuDeadLoop ();
    return Status;
  }

  AsmWbinvd ();

  //
  // Get ACPI Table Address
  //
  Facs = (EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)((UINTN)(AcpiS3Context->AcpiFacsTable));

  //
  // We need turn back to S3Resume - install boot script done ppi and report status code on S3resume.
  //
  if (PeiS3ResumeState != 0) {
    //
    // Need report status back to S3ResumePeim.
    // If boot script execution is failed, S3ResumePeim wil report the error status code.
    //
    PeiS3ResumeState->ReturnStatus = (UINT64)(UINTN)Status;
    if (FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
      //
      // X64 DXE to IA32 PEI S3 Resume
      //
      DEBUG ((DEBUG_INFO, "Call AsmDisablePaging64() to return to S3 Resume in PEI Phase\n"));
      PeiS3ResumeState->AsmTransferControl = (EFI_PHYSICAL_ADDRESS)(UINTN)AsmTransferControl32;

      if ((Facs != NULL) &&
          (Facs->Signature == EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE) &&
          (Facs->FirmwareWakingVector != 0))
      {
        //
        // more step needed - because relative address is handled differently between X64 and IA32.
        //
        AsmTransferControl16Address = (UINTN)AsmTransferControl16;
        AsmFixAddress16             = (UINT32)AsmTransferControl16Address;
        AsmJmpAddr32                = (UINT32)((Facs->FirmwareWakingVector & 0xF) | ((Facs->FirmwareWakingVector & 0xFFFF0) << 12));
      }

      AsmDisablePaging64 (
        PeiS3ResumeState->ReturnCs,
        (UINT32)PeiS3ResumeState->ReturnEntryPoint,
        (UINT32)(UINTN)AcpiS3Context,
        (UINT32)(UINTN)PeiS3ResumeState,
        (UINT32)PeiS3ResumeState->ReturnStackPointer
        );
    } else {
      //
      // IA32 DXE to IA32 PEI S3 Resume / X64 DXE to X64 PEI S3 Resume
      //
      DEBUG ((DEBUG_INFO, "Call SwitchStack() to return to S3 Resume in PEI Phase\n"));
      PeiS3ResumeState->AsmTransferControl = (EFI_PHYSICAL_ADDRESS)(UINTN)AsmTransferControl;

      SwitchStack (
        (SWITCH_STACK_ENTRY_POINT)(UINTN)PeiS3ResumeState->ReturnEntryPoint,
        (VOID *)(UINTN)AcpiS3Context,
        (VOID *)(UINTN)PeiS3ResumeState,
        (VOID *)(UINTN)PeiS3ResumeState->ReturnStackPointer
        );
    }

    //
    // Never run to here
    //
    CpuDeadLoop ();
    return EFI_UNSUPPORTED;
  }

  //
  // S3ResumePeim does not provide a way to jump back to itself, so resume to OS here directly
  //
  if (Facs->XFirmwareWakingVector != 0) {
    //
    // Switch to native waking vector
    //
    TempStackTop = (UINTN)&TempStack + sizeof (TempStack);
    if ((Facs->Version == EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION) &&
        ((Facs->Flags & EFI_ACPI_4_0_64BIT_WAKE_SUPPORTED_F) != 0) &&
        ((Facs->OspmFlags & EFI_ACPI_4_0_OSPM_64BIT_WAKE__F) != 0))
    {
      //
      // X64 long mode waking vector
      //
      DEBUG ((DEBUG_INFO, "Transfer from 64bit DXE to 64bit OS waking vector - %x\r\n", (UINTN)Facs->XFirmwareWakingVector));
      if (sizeof (UINTN) == sizeof (UINT64)) {
        //
        // 64bit DXE calls to 64bit OS S3 waking vector
        //
        SwitchStack (
          (SWITCH_STACK_ENTRY_POINT)(UINTN)Facs->XFirmwareWakingVector,
          NULL,
          NULL,
          (VOID *)(UINTN)TempStackTop
          );
      } else {
        // Unsupported for 32bit DXE, 64bit OS vector
        DEBUG ((DEBUG_ERROR, "Unsupported for 32bit DXE transfer to 64bit OS waking vector!\r\n"));
        ASSERT (FALSE);
      }
    } else {
      //
      // IA32 protected mode waking vector (Page disabled)
      //
      DEBUG ((DEBUG_INFO, "Transfer to 32bit OS waking vector - %x\r\n", (UINTN)Facs->XFirmwareWakingVector));
      if (sizeof (UINTN) == sizeof (UINT64)) {
        //
        // 64bit DXE calls to 32bit OS S3 waking vector
        //
        AsmDisablePaging64 (
          0x10,
          (UINT32)Facs->XFirmwareWakingVector,
          0,
          0,
          (UINT32)TempStackTop
          );
      } else {
        //
        // 32bit DXE calls to 32bit OS S3 waking vector
        //
        SwitchStack (
          (SWITCH_STACK_ENTRY_POINT)(UINTN)Facs->XFirmwareWakingVector,
          NULL,
          NULL,
          (VOID *)(UINTN)TempStackTop
          );
      }
    }
  } else {
    //
    // 16bit Realmode waking vector
    //
    DEBUG ((DEBUG_INFO, "Transfer to 16bit OS waking vector - %x\r\n", (UINTN)Facs->FirmwareWakingVector));
    AsmTransferControl (Facs->FirmwareWakingVector, 0x0);
  }

  //
  // Never run to here
  //
  CpuDeadLoop ();
  return EFI_UNSUPPORTED;
}

/**
  Register image to memory profile.

  @param FileName       File name of the image.
  @param ImageBase      Image base address.
  @param ImageSize      Image size.
  @param FileType       File type of the image.

**/
VOID
RegisterMemoryProfileImage (
  IN EFI_GUID          *FileName,
  IN PHYSICAL_ADDRESS  ImageBase,
  IN UINT64            ImageSize,
  IN EFI_FV_FILETYPE   FileType
  )
{
  EFI_STATUS                         Status;
  EDKII_MEMORY_PROFILE_PROTOCOL      *ProfileProtocol;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  *FilePath;
  UINT8                              TempBuffer[sizeof (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH) + sizeof (EFI_DEVICE_PATH_PROTOCOL)];

  if ((PcdGet8 (PcdMemoryProfilePropertyMask) & BIT0) != 0) {
    FilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)TempBuffer;
    Status   = gBS->LocateProtocol (&gEdkiiMemoryProfileGuid, NULL, (VOID **)&ProfileProtocol);
    if (!EFI_ERROR (Status)) {
      EfiInitializeFwVolDevicepathNode (FilePath, FileName);
      SetDevicePathEndNode (FilePath + 1);

      Status = ProfileProtocol->RegisterImage (
                                  ProfileProtocol,
                                  (EFI_DEVICE_PATH_PROTOCOL *)FilePath,
                                  ImageBase,
                                  ImageSize,
                                  FileType
                                  );
    }
  }
}

/**
  This is the Event notification function to reload BootScriptExecutor image
  to RESERVED mem and save it to LockBox.

  @param    Event   Pointer to this event
  @param    Context Event handler private data
 **/
VOID
EFIAPI
ReadyToLockEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                       Status;
  VOID                             *Interface;
  UINT8                            *Buffer;
  UINTN                            BufferSize;
  EFI_HANDLE                       NewImageHandle;
  UINTN                            Pages;
  EFI_PHYSICAL_ADDRESS             FfsBuffer;
  PE_COFF_LOADER_IMAGE_CONTEXT     ImageContext;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  MemDesc;

  Status = gBS->LocateProtocol (&gEfiDxeSmmReadyToLockProtocolGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // A workaround: Here we install a dummy handle
  //
  NewImageHandle = NULL;
  Status         = gBS->InstallProtocolInterface (
                          &NewImageHandle,
                          &gEfiCallerIdGuid,
                          EFI_NATIVE_INTERFACE,
                          NULL
                          );
  ASSERT_EFI_ERROR (Status);

  //
  // Reload BootScriptExecutor image itself to RESERVED mem
  //
  Status = GetSectionFromAnyFv (
             &gEfiCallerIdGuid,
             EFI_SECTION_PE32,
             0,
             (VOID **)&Buffer,
             &BufferSize
             );
  ASSERT_EFI_ERROR (Status);
  ImageContext.Handle    = Buffer;
  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;
  //
  // Get information about the image being loaded
  //
  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  ASSERT_EFI_ERROR (Status);
  if (ImageContext.SectionAlignment > EFI_PAGE_SIZE) {
    Pages = EFI_SIZE_TO_PAGES ((UINTN)(ImageContext.ImageSize + ImageContext.SectionAlignment));
  } else {
    Pages = EFI_SIZE_TO_PAGES ((UINTN)ImageContext.ImageSize);
  }

  FfsBuffer = 0xFFFFFFFF;
  Status    = gBS->AllocatePages (
                     AllocateMaxAddress,
                     EfiReservedMemoryType,
                     Pages,
                     &FfsBuffer
                     );
  ASSERT_EFI_ERROR (Status);

  //
  // Make sure that the buffer can be used to store code.
  //
  Status = gDS->GetMemorySpaceDescriptor (FfsBuffer, &MemDesc);
  if (!EFI_ERROR (Status) && ((MemDesc.Attributes & EFI_MEMORY_XP) != 0)) {
    gDS->SetMemorySpaceAttributes (
           FfsBuffer,
           EFI_PAGES_TO_SIZE (Pages),
           MemDesc.Attributes & (~EFI_MEMORY_XP)
           );
  }

  ImageContext.ImageAddress = (PHYSICAL_ADDRESS)(UINTN)FfsBuffer;
  //
  // Align buffer on section boundary
  //
  ImageContext.ImageAddress += ImageContext.SectionAlignment - 1;
  ImageContext.ImageAddress &= ~((EFI_PHYSICAL_ADDRESS)ImageContext.SectionAlignment - 1);
  //
  // Load the image to our new buffer
  //
  Status = PeCoffLoaderLoadImage (&ImageContext);
  ASSERT_EFI_ERROR (Status);

  //
  // Relocate the image in our new buffer
  //
  Status = PeCoffLoaderRelocateImage (&ImageContext);
  ASSERT_EFI_ERROR (Status);

  //
  // Free the buffer allocated by ReadSection since the image has been relocated in the new buffer
  //
  gBS->FreePool (Buffer);

  //
  // Flush the instruction cache so the image data is written before we execute it
  //
  InvalidateInstructionCacheRange ((VOID *)(UINTN)ImageContext.ImageAddress, (UINTN)ImageContext.ImageSize);

  RegisterMemoryProfileImage (
    &gEfiCallerIdGuid,
    ImageContext.ImageAddress,
    ImageContext.ImageSize,
    EFI_FV_FILETYPE_DRIVER
    );

  Status = ((EFI_IMAGE_ENTRY_POINT)(UINTN)(ImageContext.EntryPoint))(NewImageHandle, gST);
  ASSERT_EFI_ERROR (Status);

  //
  // Additional step for BootScript integrity
  // Save BootScriptExecutor image
  //
  Status = SaveLockBox (
             &mBootScriptExecutorImageGuid,
             (VOID *)(UINTN)ImageContext.ImageAddress,
             (UINTN)ImageContext.ImageSize
             );
  ASSERT_EFI_ERROR (Status);

  Status = SetLockBoxAttributes (&mBootScriptExecutorImageGuid, LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE);
  ASSERT_EFI_ERROR (Status);

  gBS->CloseEvent (Event);
}

/**
  Entrypoint of Boot script exector driver, this function will be executed in
  normal boot phase and invoked by DXE dispatch.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
BootScriptExecutorEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN                          BufferSize;
  UINTN                          Pages;
  BOOT_SCRIPT_EXECUTOR_VARIABLE  *EfiBootScriptExecutorVariable;
  EFI_PHYSICAL_ADDRESS           BootScriptExecutorBuffer;
  EFI_STATUS                     Status;
  VOID                           *DevicePath;
  EFI_EVENT                      ReadyToLockEvent;
  VOID                           *Registration;
  UINT32                         RegEax;
  UINT32                         RegEdx;

  if (!PcdGetBool (PcdAcpiS3Enable)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Make sure AddressEncMask is contained to smallest supported address field.
  //
  mAddressEncMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) & PAGING_1G_ADDRESS_MASK_64;

  //
  // Test if the gEfiCallerIdGuid of this image is already installed. if not, the entry
  // point is loaded by DXE code which is the first time loaded. or else, it is already
  // be reloaded be itself.This is a work-around
  //
  Status = gBS->LocateProtocol (&gEfiCallerIdGuid, NULL, &DevicePath);
  if (EFI_ERROR (Status)) {
    //
    // Create ReadyToLock event to reload BootScriptExecutor image
    // to RESERVED mem and save it to LockBox.
    //
    ReadyToLockEvent = EfiCreateProtocolNotifyEvent (
                         &gEfiDxeSmmReadyToLockProtocolGuid,
                         TPL_NOTIFY,
                         ReadyToLockEventNotify,
                         NULL,
                         &Registration
                         );
    ASSERT (ReadyToLockEvent != NULL);
  } else {
    //
    // the entry point is invoked after reloading. following code only run in RESERVED mem
    //
    if (PcdGetBool (PcdUse1GPageTable)) {
      AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
      if (RegEax >= 0x80000001) {
        AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
        if ((RegEdx & BIT26) != 0) {
          mPage1GSupport = TRUE;
        }
      }
    }

    BufferSize = sizeof (BOOT_SCRIPT_EXECUTOR_VARIABLE);

    BootScriptExecutorBuffer = 0xFFFFFFFF;
    Pages                    = EFI_SIZE_TO_PAGES (BufferSize);
    Status                   = gBS->AllocatePages (
                                      AllocateMaxAddress,
                                      EfiReservedMemoryType,
                                      Pages,
                                      &BootScriptExecutorBuffer
                                      );
    ASSERT_EFI_ERROR (Status);

    EfiBootScriptExecutorVariable                               = (BOOT_SCRIPT_EXECUTOR_VARIABLE *)(UINTN)BootScriptExecutorBuffer;
    EfiBootScriptExecutorVariable->BootScriptExecutorEntrypoint = (UINTN)S3BootScriptExecutorEntryFunction;

    Status = SaveLockBox (
               &gEfiBootScriptExecutorVariableGuid,
               &BootScriptExecutorBuffer,
               sizeof (BootScriptExecutorBuffer)
               );
    ASSERT_EFI_ERROR (Status);

    //
    // Additional step for BootScript integrity
    // Save BootScriptExecutor context
    //
    Status = SaveLockBox (
               &gEfiBootScriptExecutorContextGuid,
               EfiBootScriptExecutorVariable,
               sizeof (*EfiBootScriptExecutorVariable)
               );
    ASSERT_EFI_ERROR (Status);

    Status = SetLockBoxAttributes (&gEfiBootScriptExecutorContextGuid, LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
