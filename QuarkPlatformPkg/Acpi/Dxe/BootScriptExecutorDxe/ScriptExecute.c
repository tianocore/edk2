/** @file
This is the code for Boot Script Executer module.

This driver is dispatched by Dxe core and the driver will reload itself to ACPI NVS memory
in the entry point. The functionality is to interpret and restore the S3 boot script

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ScriptExecute.h"

#pragma pack(1)
typedef union {
  struct {
    UINT32  LimitLow    : 16;
    UINT32  BaseLow     : 16;
    UINT32  BaseMid     : 8;
    UINT32  Type        : 4;
    UINT32  System      : 1;
    UINT32  Dpl         : 2;
    UINT32  Present     : 1;
    UINT32  LimitHigh   : 4;
    UINT32  Software    : 1;
    UINT32  Reserved    : 1;
    UINT32  DefaultSize : 1;
    UINT32  Granularity : 1;
    UINT32  BaseHigh    : 8;
  } Bits;
  UINT64  Uint64;
} IA32_GDT;

#pragma pack()

EFI_GUID              mBootScriptExecutorImageGuid = {
  0x9a8d3433, 0x9fe8, 0x42b6, {0x87, 0xb, 0x1e, 0x31, 0xc8, 0x4e, 0xbe, 0x3b}
};

//
// Global Descriptor Table (GDT)
//
GLOBAL_REMOVE_IF_UNREFERENCED IA32_GDT mGdtEntries[] = {
/* selector { Global Segment Descriptor                              } */
/* 0x00 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}},
/* 0x08 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}},
/* 0x10 */  {{0xFFFF, 0,  0,  0xB,  1,  0,  1,  0xF,  0,  0, 1,  1,  0}},
/* 0x18 */  {{0xFFFF, 0,  0,  0x3,  1,  0,  1,  0xF,  0,  0, 1,  1,  0}},
/* 0x20 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}},
/* 0x28 */  {{0xFFFF, 0,  0,  0xB,  1,  0,  1,  0xF,  0,  0, 0,  1,  0}},
/* 0x30 */  {{0xFFFF, 0,  0,  0x3,  1,  0,  1,  0xF,  0,  0, 0,  1,  0}},
/* 0x38 */  {{0xFFFF, 0,  0,  0xB,  1,  0,  1,  0xF,  0,  1, 0,  1,  0}},
/* 0x40 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}},
};

//
// IA32 Gdt register
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST IA32_DESCRIPTOR mGdt = {
  sizeof (mGdtEntries) - 1,
  (UINTN) mGdtEntries
  };

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
  IN ACPI_S3_CONTEXT       *AcpiS3Context,
  IN PEI_S3_RESUME_STATE   *PeiS3ResumeState
  )
{
  EFI_STATUS                                    Status;

  //
  // Disable interrupt of Debug timer, since new IDT table cannot handle it.
  //
  SaveAndSetDebugTimerInterrupt (FALSE);

  //
  // Restore IDT for debug
  //
  SetIdtEntry (AcpiS3Context);

  //
  // Initialize Debug Agent to support source level debug in S3 path.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_S3, NULL, NULL);

  //
  // Because not install BootScriptExecute PPI(used just in this module), So just pass NULL
  // for that parameter.
  //
  Status = S3BootScriptExecute ();

  AsmWbinvd ();

  //
  // We need turn back to S3Resume - install boot script done ppi and report status code on S3resume.
  //
  if (PeiS3ResumeState != 0) {
    //
    // Need report status back to S3ResumePeim.
    // If boot script execution is failed, S3ResumePeim wil report the error status code.
    //
    PeiS3ResumeState->ReturnStatus = (UINT64)(UINTN)Status;
    //
    // IA32 S3 Resume
    //
    DEBUG ((EFI_D_INFO, "Call SwitchStack() to return to S3 Resume in PEI Phase\n"));
    PeiS3ResumeState->AsmTransferControl = (EFI_PHYSICAL_ADDRESS)(UINTN)PlatformTransferControl16;

    SwitchStack (
      (SWITCH_STACK_ENTRY_POINT)(UINTN)PeiS3ResumeState->ReturnEntryPoint,
      (VOID *)(UINTN)AcpiS3Context,
      (VOID *)(UINTN)PeiS3ResumeState,
      (VOID *)(UINTN)PeiS3ResumeState->ReturnStackPointer
      );

    //
    // Never run to here
    //
    CpuDeadLoop();
    return EFI_UNSUPPORTED;
  }

  //
  // Never run to here
  //
  CpuDeadLoop();
  return EFI_UNSUPPORTED;
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
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  UINT8                                         *Buffer;
  UINTN                                         BufferSize;
  UINTN                                         Pages;
  EFI_PHYSICAL_ADDRESS                          FfsBuffer;
  PE_COFF_LOADER_IMAGE_CONTEXT                  ImageContext;
  BOOT_SCRIPT_EXECUTOR_VARIABLE                 *EfiBootScriptExecutorVariable;
  EFI_PHYSICAL_ADDRESS                          BootScriptExecutorBuffer;
  EFI_STATUS                                    Status;
  VOID                                          *DevicePath;
  EFI_HANDLE                                    NewImageHandle;

  //
  // Test if the gEfiCallerIdGuid of this image is already installed. if not, the entry
  // point is loaded by DXE code which is the first time loaded. or else, it is already
  // be reloaded be itself.This is a work-around
  //
  Status = gBS->LocateProtocol (&gEfiCallerIdGuid, NULL, &DevicePath);
  if (EFI_ERROR (Status)) {

      //
      // This is the first-time loaded by DXE core. reload itself to NVS mem
      //
      //
      // A workarouond: Here we install a dummy handle
      //
      NewImageHandle = NULL;
      Status = gBS->InstallProtocolInterface (
                  &NewImageHandle,
                  &gEfiCallerIdGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );

      Status = GetSectionFromAnyFv  (
                 &gEfiCallerIdGuid,
                 EFI_SECTION_PE32,
                 0,
                 (VOID **) &Buffer,
                 &BufferSize
                 );
      ImageContext.Handle    = Buffer;
      ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;
      //
      // Get information about the image being loaded
      //
      Status = PeCoffLoaderGetImageInfo (&ImageContext);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      Pages = EFI_SIZE_TO_PAGES(BufferSize + ImageContext.SectionAlignment);
      FfsBuffer = 0xFFFFFFFF;
      Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiACPIMemoryNVS,
                    Pages,
                    &FfsBuffer
                    );
      if (EFI_ERROR (Status)) {
        return EFI_OUT_OF_RESOURCES;
      }
      ImageContext.ImageAddress = (PHYSICAL_ADDRESS)(UINTN)FfsBuffer;
      //
      // Align buffer on section boundry
      //
      ImageContext.ImageAddress += ImageContext.SectionAlignment - 1;
      ImageContext.ImageAddress &= ~(ImageContext.SectionAlignment - 1);
      //
      // Load the image to our new buffer
      //
      Status = PeCoffLoaderLoadImage (&ImageContext);
      if (EFI_ERROR (Status)) {
        gBS->FreePages (FfsBuffer, Pages);
        return Status;
      }

      //
      // Relocate the image in our new buffer
      //
      Status = PeCoffLoaderRelocateImage (&ImageContext);

      if (EFI_ERROR (Status)) {
        PeCoffLoaderUnloadImage (&ImageContext);
        gBS->FreePages (FfsBuffer, Pages);
        return Status;
      }
      //
      // Flush the instruction cache so the image data is written before we execute it
      //
      InvalidateInstructionCacheRange ((VOID *)(UINTN)ImageContext.ImageAddress, (UINTN)ImageContext.ImageSize);
      Status = ((EFI_IMAGE_ENTRY_POINT)(UINTN)(ImageContext.EntryPoint)) (NewImageHandle, SystemTable);
      if (EFI_ERROR (Status)) {
        gBS->FreePages (FfsBuffer, Pages);
        return Status;
      }
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

    } else {
      //
      // the entry point is invoked after reloading. following code only run in  ACPI NVS
      //
      BufferSize = sizeof (BOOT_SCRIPT_EXECUTOR_VARIABLE);

      BootScriptExecutorBuffer = 0xFFFFFFFF;
      Pages = EFI_SIZE_TO_PAGES(BufferSize);
      Status = gBS->AllocatePages (
                      AllocateMaxAddress,
                      EfiACPIMemoryNVS,
                      Pages,
                      &BootScriptExecutorBuffer
                      );
      if (EFI_ERROR (Status)) {
        return EFI_OUT_OF_RESOURCES;
      }

      EfiBootScriptExecutorVariable = (BOOT_SCRIPT_EXECUTOR_VARIABLE *)(UINTN)BootScriptExecutorBuffer;
      EfiBootScriptExecutorVariable->BootScriptExecutorEntrypoint = (UINTN) S3BootScriptExecutorEntryFunction ;

      Status = SaveLockBox (
                 &gEfiBootScriptExecutorVariableGuid,
                 &BootScriptExecutorBuffer,
                 sizeof(BootScriptExecutorBuffer)
                 );
      ASSERT_EFI_ERROR (Status);

      //
      // Additional step for BootScript integrity
      // Save BootScriptExecutor context
      //
      Status = SaveLockBox (
                 &gEfiBootScriptExecutorContextGuid,
                 EfiBootScriptExecutorVariable,
                 sizeof(*EfiBootScriptExecutorVariable)
                 );
      ASSERT_EFI_ERROR (Status);

      Status = SetLockBoxAttributes (&gEfiBootScriptExecutorContextGuid, LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE);
      ASSERT_EFI_ERROR (Status);

    }

    return EFI_SUCCESS;
}

/**
  Platform specific mechanism to transfer control to 16bit OS waking vector

  @param[in] AcpiWakingVector    The 16bit OS waking vector
  @param[in] AcpiLowMemoryBase   A buffer under 1M which could be used during the transfer

**/
VOID
PlatformTransferControl16 (
  IN UINT32       AcpiWakingVector,
  IN UINT32       AcpiLowMemoryBase
  )
{
  UINT32      NewValue;
  UINT64      BaseAddress;
  UINT64      SmramLength;
  UINTN       Index;

  DEBUG (( EFI_D_INFO, "PlatformTransferControl - Entry\r\n"));

  //
  // Need to make sure the GDT is loaded with values that support long mode and real mode.
  //
  AsmWriteGdtr (&mGdt);

  //
  // Disable eSram block (this will also clear/zero eSRAM)
  // We only use eSRAM in the PEI phase. Disable now that we are resuming the OS
  //
  NewValue = QNCPortRead (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_ESRAMPGCTRL_BLOCK);
  NewValue |= BLOCK_DISABLE_PG;
  QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_ESRAMPGCTRL_BLOCK, NewValue);

  //
  // Update HMBOUND to top of DDR3 memory and LOCK
  // We disabled eSRAM so now we move HMBOUND down to top of DDR3
  //
  QNCGetTSEGMemoryRange (&BaseAddress, &SmramLength);
  NewValue = (UINT32)(BaseAddress + SmramLength);
  DEBUG ((EFI_D_INFO,"Locking HMBOUND at: = 0x%8x\n",NewValue));
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QUARK_NC_HOST_BRIDGE_HMBOUND_REG, (NewValue | HMBOUND_LOCK));

  //
  // Lock all IMR regions now that HMBOUND is locked
  //
  for (Index = (QUARK_NC_MEMORY_MANAGER_IMR0+QUARK_NC_MEMORY_MANAGER_IMRXL); Index <= (QUARK_NC_MEMORY_MANAGER_IMR7+QUARK_NC_MEMORY_MANAGER_IMRXL); Index += 4) {
    NewValue = QNCPortRead (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, Index);
    NewValue |= IMR_LOCK;
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, Index, NewValue);
  }

  //
  // Call ASM routine to switch to real mode and jump to 16bit OS waking vector
  //
  AsmTransferControl(AcpiWakingVector, 0);

  //
  // Never run to here
  //
  CpuDeadLoop();
}




