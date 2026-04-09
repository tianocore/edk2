/** @file
The CPU specific programming for PiSmmCpuDxeSmm module when STM support
is not included.

Copyright (c) 2010 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include <Library/SmmCpuFeaturesLib.h>
#include "CpuFeaturesLib.h"

/**
  Internal worker function that is called to complete CPU initialization at the
  end of SmmCpuFeaturesInitializeProcessor().

**/
VOID
FinishSmmCpuFeaturesInitializeProcessor (
  VOID
  )
{
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
  return 0;
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
}
