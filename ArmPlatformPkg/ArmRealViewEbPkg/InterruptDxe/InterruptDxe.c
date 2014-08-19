/*++

Copyright (c) 2009, Hewlett-Packard Company. All rights reserved.<BR>
Portions copyright (c) 2010, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Gic.c

Abstract:

  Driver implementing the GIC interrupt controller protocol

--*/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>

#include <Protocol/Cpu.h>
#include <Protocol/HardwareInterrupt.h>


//
// EB board definitions
//
#define EB_GIC1_CPU_INTF_BASE   0x10040000
#define EB_GIC1_DIST_BASE       0x10041000
#define EB_GIC2_CPU_INTF_BASE   0x10050000
#define EB_GIC2_DIST_BASE       0x10051000
#define EB_GIC3_CPU_INTF_BASE   0x10060000
#define EB_GIC3_DIST_BASE       0x10061000
#define EB_GIC4_CPU_INTF_BASE   0x10070000
#define EB_GIC5_DIST_BASE       0x10071000

// number of interrupts sources supported by each GIC on the EB
#define EB_NUM_GIC_INTERRUPTS   96

// number of 32-bit registers needed to represent those interrupts as a bit
// (used for enable set, enable clear, pending set, pending clear, and active regs)
#define EB_NUM_GIC_REG_PER_INT_BITS   (EB_NUM_GIC_INTERRUPTS / 32)

// number of 32-bit registers needed to represent those interrupts as two bits
// (used for configuration reg)
#define EB_NUM_GIC_REG_PER_INT_CFG    (EB_NUM_GIC_INTERRUPTS / 16)

// number of 32-bit registers needed to represent interrupts as 8-bit priority field
// (used for priority regs)
#define EB_NUM_GIC_REG_PER_INT_BYTES  (EB_NUM_GIC_INTERRUPTS / 4)

#define GIC_DEFAULT_PRIORITY  0x80

//
// GIC definitions
//

// Distributor
#define GIC_ICDDCR          0x000 // Distributor Control Register
#define GIC_ICDICTR         0x004 // Interrupt Controller Type Register
#define GIC_ICDIIDR         0x008 // Implementer Identification Register

// each reg base below repeats for EB_NUM_GIC_REG_PER_INT_BITS (see GIC spec)
#define GIC_ICDISR          0x080 // Interrupt Security Registers
#define GIC_ICDISER         0x100 // Interrupt Set-Enable Registers
#define GIC_ICDICER         0x180 // Interrupt Clear-Enable Registers
#define GIC_ICDSPR          0x200 // Interrupt Set-Pending Registers
#define GIC_ICDCPR          0x280 // Interrupt Clear-Pending Registers
#define GIC_ICDABR          0x300 // Active Bit Registers

// each reg base below repeats for EB_NUM_GIC_REG_PER_INT_BYTES
#define GIC_ICDIPR          0x400 // Interrupt Priority Registers

// each reg base below repeats for EB_NUM_GIC_INTERRUPTS
#define GIC_ICDIPTR         0x800 // Interrupt Processor Target Registers
#define GIC_ICDICFR         0xC00 // Interrupt Configuration Registers

// just one of these
#define GIC_ICDSGIR         0xF00 // Software Generated Interrupt Register


// Cpu interface
#define GIC_ICCICR          0x00  // CPU Interface Controler Register
#define GIC_ICCPMR          0x04  // Interrupt Priority Mask Register
#define GIC_ICCBPR          0x08  // Binary Point Register
#define GIC_ICCIAR          0x0C  // Interrupt Acknowledge Register
#define GIC_ICCEIOR         0x10  // End Of Interrupt Register
#define GIC_ICCRPR          0x14  // Running Priority Register
#define GIC_ICCPIR          0x18  // Highest Pending Interrupt Register
#define GIC_ICCABPR         0x1C  // Aliased Binary Point Register
#define GIC_ICCIDR          0xFC  // Identification Register

extern EFI_HARDWARE_INTERRUPT_PROTOCOL gHardwareInterruptProtocol;

//
// Notifications
//
VOID      *CpuProtocolNotificationToken = NULL;
EFI_EVENT CpuProtocolNotificationEvent  = (EFI_EVENT)NULL;
EFI_EVENT EfiExitBootServicesEvent      = (EFI_EVENT)NULL;


HARDWARE_INTERRUPT_HANDLER  gRegisteredInterruptHandlers[EB_NUM_GIC_INTERRUPTS];

/**
  Register Handler for the specified interrupt source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt
  @param Handler  Callback for interrupt. NULL to unregister

  @retval EFI_SUCCESS Source was updated to support Handler.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
EFI_STATUS
EFIAPI
RegisterInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source,
  IN HARDWARE_INTERRUPT_HANDLER         Handler
  )
{
  if (Source > EB_NUM_GIC_INTERRUPTS) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  if ((Handler == NULL) && (gRegisteredInterruptHandlers[Source] == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Handler != NULL) && (gRegisteredInterruptHandlers[Source] != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  gRegisteredInterruptHandlers[Source] = Handler;
  return This->EnableInterruptSource(This, Source);
}


/**
  Enable interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt enabled.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
EFI_STATUS
EFIAPI
EnableInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source
  )
{
  UINT32    RegOffset;
  UINTN     RegShift;

  if (Source > EB_NUM_GIC_INTERRUPTS) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  // calculate enable register offset and bit position
  RegOffset = Source / 32;
  RegShift = Source % 32;

  // write set-enable register
  MmioWrite32 (EB_GIC1_DIST_BASE+GIC_ICDISER+(4*RegOffset), 1 << RegShift);

  return EFI_SUCCESS;
}


/**
  Disable interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt disabled.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
EFI_STATUS
EFIAPI
DisableInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source
  )
{
  UINT32    RegOffset;
  UINTN     RegShift;

  if (Source > EB_NUM_GIC_INTERRUPTS) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  // calculate enable register offset and bit position
  RegOffset = Source / 32;
  RegShift = Source % 32;

  // write set-enable register
  MmioWrite32 (EB_GIC1_DIST_BASE+GIC_ICDICER+(4*RegOffset), 1 << RegShift);

  return EFI_SUCCESS;
}



/**
  Return current state of interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt
  @param InterruptState  TRUE: source enabled, FALSE: source disabled.

  @retval EFI_SUCCESS       InterruptState is valid
  @retval EFI_DEVICE_ERROR  InterruptState is not valid

**/
EFI_STATUS
EFIAPI
GetInterruptSourceState (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source,
  IN BOOLEAN                            *InterruptState
  )
{
  UINT32    RegOffset;
  UINTN     RegShift;

  if (Source > EB_NUM_GIC_INTERRUPTS) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  // calculate enable register offset and bit position
  RegOffset = Source / 32;
  RegShift = Source % 32;

  if ((MmioRead32 (EB_GIC1_DIST_BASE+GIC_ICDISER+(4*RegOffset)) & (1<<RegShift)) == 0) {
    *InterruptState = FALSE;
  } else {
    *InterruptState = TRUE;
  }

  return EFI_SUCCESS;
}

/**
  Signal to the hardware that the End Of Intrrupt state
  has been reached.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt EOI'ed.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
EFI_STATUS
EFIAPI
EndOfInterrupt (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source
  )
{
  if (Source > EB_NUM_GIC_INTERRUPTS) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  MmioWrite32 (EB_GIC1_CPU_INTF_BASE+GIC_ICCEIOR, Source);
  return EFI_SUCCESS;
}


/**
  EFI_CPU_INTERRUPT_HANDLER that is called when a processor interrupt occurs.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.

  @return None

**/
VOID
EFIAPI
IrqInterruptHandler (
  IN EFI_EXCEPTION_TYPE           InterruptType,
  IN EFI_SYSTEM_CONTEXT           SystemContext
  )
{
  UINT32                      GicInterrupt;
  HARDWARE_INTERRUPT_HANDLER  InterruptHandler;

  GicInterrupt = MmioRead32 (EB_GIC1_CPU_INTF_BASE + GIC_ICCIAR);
  if (GicInterrupt >= EB_NUM_GIC_INTERRUPTS) {
    MmioWrite32 (EB_GIC1_CPU_INTF_BASE+GIC_ICCEIOR, GicInterrupt);
  }

  InterruptHandler = gRegisteredInterruptHandlers[GicInterrupt];
  if (InterruptHandler != NULL) {
    // Call the registered interrupt handler.
    InterruptHandler (GicInterrupt, SystemContext);
  } else {
    DEBUG ((EFI_D_ERROR, "Spurious GIC interrupt: %x\n", GicInterrupt));
  }

  EndOfInterrupt (&gHardwareInterruptProtocol, GicInterrupt);
}


//
// Making this global saves a few bytes in image size
//
EFI_HANDLE  gHardwareInterruptHandle = NULL;

//
// The protocol instance produced by this driver
//
EFI_HARDWARE_INTERRUPT_PROTOCOL gHardwareInterruptProtocol = {
  RegisterInterruptSource,
  EnableInterruptSource,
  DisableInterruptSource,
  GetInterruptSourceState,
  EndOfInterrupt
};


/**
  Shutdown our hardware

  DXE Core will disable interrupts and turn off the timer and disable interrupts
  after all the event handlers have run.

  @param[in]  Event   The Event that is being processed
  @param[in]  Context Event Context
**/
VOID
EFIAPI
ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN    i;

  for (i = 0; i < EB_NUM_GIC_INTERRUPTS; i++) {
    DisableInterruptSource (&gHardwareInterruptProtocol, i);
  }
}


//
// Notification routines
//
VOID
CpuProtocolInstalledNotification (
  IN EFI_EVENT   Event,
  IN VOID        *Context
  )
{
  EFI_STATUS              Status;
  EFI_CPU_ARCH_PROTOCOL   *Cpu;

  //
  // Get the cpu protocol that this driver requires.
  //
  Status = gBS->LocateProtocol(&gEfiCpuArchProtocolGuid, NULL, (VOID **)&Cpu);
  ASSERT_EFI_ERROR(Status);

  //
  // Unregister the default exception handler.
  //
  Status = Cpu->RegisterInterruptHandler(Cpu, EXCEPT_ARM_IRQ, NULL);
  ASSERT_EFI_ERROR(Status);

  //
  // Register to receive interrupts
  //
  Status = Cpu->RegisterInterruptHandler(Cpu, EXCEPT_ARM_IRQ, IrqInterruptHandler);
  ASSERT_EFI_ERROR(Status);
}

/**
  Initialize the state information for the CPU Architectural Protocol

  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
InterruptDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN      i;
  UINT32      RegOffset;
  UINTN       RegShift;


  // Make sure the Interrupt Controller Protocol is not already installed in the system.
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gHardwareInterruptProtocolGuid);

  for (i = 0; i < EB_NUM_GIC_INTERRUPTS; i++) {
    DisableInterruptSource (&gHardwareInterruptProtocol, i);

    // Set Priority
    RegOffset = i / 4;
    RegShift = (i % 4) * 8;
    MmioAndThenOr32 (
      EB_GIC1_DIST_BASE+GIC_ICDIPR+(4*RegOffset),
      ~(0xff << RegShift),
      GIC_DEFAULT_PRIORITY << RegShift
      );
  }

  // configure interrupts for cpu 0
  for (i = 0; i < EB_NUM_GIC_REG_PER_INT_BYTES; i++) {
    MmioWrite32 (EB_GIC1_DIST_BASE + GIC_ICDIPTR + (i*4), 0x01010101);
  }

  // set binary point reg to 0x7 (no preemption)
  MmioWrite32 (EB_GIC1_CPU_INTF_BASE + GIC_ICCBPR, 0x7);

  // set priority mask reg to 0xff to allow all priorities through
  MmioWrite32 (EB_GIC1_CPU_INTF_BASE + GIC_ICCPMR, 0xff);

  // enable gic cpu interface
  MmioWrite32 (EB_GIC1_CPU_INTF_BASE + GIC_ICCICR, 0x1);

  // enable gic distributor
  MmioWrite32 (EB_GIC1_DIST_BASE + GIC_ICCICR, 0x1);


  ZeroMem (&gRegisteredInterruptHandlers, sizeof (gRegisteredInterruptHandlers));

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gHardwareInterruptHandle,
                  &gHardwareInterruptProtocolGuid,   &gHardwareInterruptProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  // Set up to be notified when the Cpu protocol is installed.
  Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL, TPL_CALLBACK, CpuProtocolInstalledNotification, NULL, &CpuProtocolNotificationEvent);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->RegisterProtocolNotify (&gEfiCpuArchProtocolGuid, CpuProtocolNotificationEvent, (VOID *)&CpuProtocolNotificationToken);
  ASSERT_EFI_ERROR (Status);

  // Register for an ExitBootServicesEvent
  Status = gBS->CreateEvent (EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_NOTIFY, ExitBootServicesEvent, NULL, &EfiExitBootServicesEvent);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

