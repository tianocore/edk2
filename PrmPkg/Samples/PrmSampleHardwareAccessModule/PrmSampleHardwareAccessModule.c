/** @file

  A sample PRM Module implementation. This PRM Module provides PRM handlers that perform various types
  of hardware access. This is simply meant to demonstrate hardware access capabilities from a PRM handler.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PrmModule.h>

#include <Library/BaseLib.h>
#include <Library/MtrrLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>

#include <Register/Intel/ArchitecturalMsr.h>
#include <Register/Intel/Cpuid.h>

#include "Hpet.h"

//
// PRM Handler GUIDs
//

// {2120cd3c-848b-4d8f-abbb-4b74ce64ac89}
#define MSR_ACCESS_MICROCODE_SIGNATURE_PRM_HANDLER_GUID {0x2120cd3c, 0x848b, 0x4d8f, {0xab, 0xbb, 0x4b, 0x74, 0xce, 0x64, 0xac, 0x89}}

// {5d28b4e7-3867-4aee-aa09-51fc282c3b22}
#define MSR_PRINT_MICROCODE_SIGNATURE_PRM_HANDLER_GUID {0x5d28b4e7, 0x3867, 0x4aee, {0xaa, 0x09, 0x51, 0xfc, 0x28, 0x2c, 0x3b, 0x22}}

// {ea0935a7-506b-4159-bbbb-48deeecb6f58}
#define MSR_ACCESS_MTRR_DUMP_PRM_HANDLER_GUID {0xea0935a7, 0x506b, 0x4159, {0xbb, 0xbb, 0x48, 0xde, 0xee, 0xcb, 0x6f, 0x58}}

// {4b64b702-4d2b-4dfe-ac5a-0b4110a2ca47}
#define MSR_PRINT_MTRR_DUMP_PRM_HANDLER_GUID {0x4b64b702, 0x4d2b, 0x4dfe, {0xac, 0x5a, 0x0b, 0x41, 0x10, 0xa2, 0xca, 0x47}}

// {1bd1bda9-909a-4614-9699-25ec0c2783f7}
#define MMIO_ACCESS_HPET_PRM_HANDLER_GUID {0x1bd1bda9, 0x909a, 0x4614, {0x96, 0x99, 0x25, 0xec, 0x0c, 0x27, 0x83, 0xf7}}

// {8a0efdde-78d0-45f0-aea0-c28245c7e1db}
#define MMIO_PRINT_HPET_PRM_HANDLER_GUID {0x8a0efdde, 0x78d0, 0x45f0, {0xae, 0xa0, 0xc2, 0x82, 0x45, 0xc7, 0xe1, 0xdb}}

//
// BEGIN: MtrrLib internal library globals and function prototypes here for testing
//
extern CONST CHAR8        *mMtrrMemoryCacheTypeShortName[];

/**
  Initializes the valid bits mask and valid address mask for MTRRs.

  This function initializes the valid bits mask and valid address mask for MTRRs.

  @param[out]  MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param[out]  MtrrValidAddressMask  The valid address mask for the MTRR

**/
VOID
MtrrLibInitializeMtrrMask (
  OUT UINT64 *MtrrValidBitsMask,
  OUT UINT64 *MtrrValidAddressMask
  );

/**
  Convert variable MTRRs to a RAW MTRR_MEMORY_RANGE array.
  One MTRR_MEMORY_RANGE element is created for each MTRR setting.
  The routine doesn't remove the overlap or combine the near-by region.

  @param[in]   VariableSettings      The variable MTRR values to shadow
  @param[in]   VariableMtrrCount     The number of variable MTRRs
  @param[in]   MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param[in]   MtrrValidAddressMask  The valid address mask for MTRR
  @param[out]  VariableMtrr          The array to shadow variable MTRRs content

  @return      Number of MTRRs which has been used.

**/
UINT32
MtrrLibGetRawVariableRanges (
  IN  MTRR_VARIABLE_SETTINGS  *VariableSettings,
  IN  UINTN                   VariableMtrrCount,
  IN  UINT64                  MtrrValidBitsMask,
  IN  UINT64                  MtrrValidAddressMask,
  OUT MTRR_MEMORY_RANGE       *VariableMtrr
  );

/**
  Apply the fixed MTRR settings to memory range array.

  @param Fixed             The fixed MTRR settings.
  @param Ranges            Return the memory range array holding memory type
                           settings for all memory address.
  @param RangeCapacity     The capacity of memory range array.
  @param RangeCount        Return the count of memory range.

  @retval RETURN_SUCCESS          The memory range array is returned successfully.
  @retval RETURN_OUT_OF_RESOURCES The count of memory ranges exceeds capacity.
**/
RETURN_STATUS
MtrrLibApplyFixedMtrrs (
  IN     MTRR_FIXED_SETTINGS  *Fixed,
  IN OUT MTRR_MEMORY_RANGE    *Ranges,
  IN     UINTN                RangeCapacity,
  IN OUT UINTN                *RangeCount
  );

/**
  Apply the variable MTRR settings to memory range array.

  @param VariableMtrr      The variable MTRR array.
  @param VariableMtrrCount The count of variable MTRRs.
  @param Ranges            Return the memory range array with new MTRR settings applied.
  @param RangeCapacity     The capacity of memory range array.
  @param RangeCount        Return the count of memory range.

  @retval RETURN_SUCCESS          The memory range array is returned successfully.
  @retval RETURN_OUT_OF_RESOURCES The count of memory ranges exceeds capacity.
**/
RETURN_STATUS
MtrrLibApplyVariableMtrrs (
  IN     CONST MTRR_MEMORY_RANGE *VariableMtrr,
  IN     UINT32                  VariableMtrrCount,
  IN OUT MTRR_MEMORY_RANGE       *Ranges,
  IN     UINTN                   RangeCapacity,
  IN OUT UINTN                   *RangeCount
  );

//
// END: MtrrLib internal library function prototypes here for testing
//

/**
  Accesses MTRR values including architectural and variable MTRRs.

  If the optional OsServiceDebugPrint function pointer is provided that function is
  used to print the MTRR settings.

  @param[in]  OsServiceDebugPrint   A pointer to an OS-provided debug print function.

**/
VOID
EFIAPI
AccessAllMtrrs (
  IN PRM_OS_SERVICE_DEBUG_PRINT   OsServiceDebugPrint   OPTIONAL
  )
{
  MTRR_SETTINGS                   LocalMtrrs;
  MTRR_SETTINGS                   *Mtrrs;
  UINTN                           Index;
  UINTN                           RangeCount;
  UINT64                          MtrrValidBitsMask;
  UINT64                          MtrrValidAddressMask;
  UINT32                          VariableMtrrCount;
  BOOLEAN                         ContainVariableMtrr;
  CHAR8                           DebugMessage[256];

  MTRR_MEMORY_RANGE Ranges[
    MTRR_NUMBER_OF_FIXED_MTRR * sizeof (UINT64) + 2 * ARRAY_SIZE (Mtrrs->Variables.Mtrr) + 1
    ];
  MTRR_MEMORY_RANGE RawVariableRanges[ARRAY_SIZE (Mtrrs->Variables.Mtrr)];

  if (!IsMtrrSupported ()) {
    return;
  }

  VariableMtrrCount = GetVariableMtrrCount ();

  MtrrGetAllMtrrs (&LocalMtrrs);
  Mtrrs = &LocalMtrrs;

  //
  // Dump RAW MTRR contents
  //
  if (OsServiceDebugPrint != NULL) {
    OsServiceDebugPrint ("  MTRR Settings:\n");
    OsServiceDebugPrint ("  =============\n");

    AsciiSPrint (
      &DebugMessage[0],
      ARRAY_SIZE (DebugMessage),
      "  MTRR Default Type: %016lx\n",
      Mtrrs->MtrrDefType
      );
    OsServiceDebugPrint (&DebugMessage[0]);
  }

  if (OsServiceDebugPrint != NULL) {
    for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
        AsciiSPrint (
          &DebugMessage[0],
          ARRAY_SIZE (DebugMessage),
          "  Fixed MTRR[%02d]   : %016lx\n",
          Index,
          Mtrrs->Fixed.Mtrr[Index]
          );
        OsServiceDebugPrint (&DebugMessage[0]);
      }

    ContainVariableMtrr = FALSE;
    for (Index = 0; Index < VariableMtrrCount; Index++) {
      if ((Mtrrs->Variables.Mtrr[Index].Mask & BIT11) == 0) {
        //
        // If mask is not valid, then do not display range
        //
        continue;
      }
      ContainVariableMtrr = TRUE;
      AsciiSPrint (
        &DebugMessage[0],
        ARRAY_SIZE (DebugMessage),
        "  Variable MTRR[%02d]: Base=%016lx Mask=%016lx\n",
        Index,
        Mtrrs->Variables.Mtrr[Index].Base,
        Mtrrs->Variables.Mtrr[Index].Mask
        );
      OsServiceDebugPrint (&DebugMessage[0]);
    }
    if (!ContainVariableMtrr) {
      OsServiceDebugPrint ("  Variable MTRR    : None.\n");
    }
    OsServiceDebugPrint ("\n");
  }

  //
  // Dump MTRR setting in ranges
  //
  if (OsServiceDebugPrint != NULL) {
    OsServiceDebugPrint ("  Memory Ranges:\n");
    OsServiceDebugPrint ("  ====================================\n");
  }
  MtrrLibInitializeMtrrMask (&MtrrValidBitsMask, &MtrrValidAddressMask);
  Ranges[0].BaseAddress = 0;
  Ranges[0].Length      = MtrrValidBitsMask + 1;
  Ranges[0].Type        = MtrrGetDefaultMemoryType ();
  RangeCount = 1;

  MtrrLibGetRawVariableRanges (
    &Mtrrs->Variables, VariableMtrrCount,
    MtrrValidBitsMask, MtrrValidAddressMask, RawVariableRanges
    );
  MtrrLibApplyVariableMtrrs (
    RawVariableRanges, VariableMtrrCount,
    Ranges, ARRAY_SIZE (Ranges), &RangeCount
    );

  MtrrLibApplyFixedMtrrs (&Mtrrs->Fixed, Ranges, ARRAY_SIZE (Ranges), &RangeCount);

  if (OsServiceDebugPrint != NULL) {
    for (Index = 0; Index < RangeCount; Index++) {
      AsciiSPrint (
        &DebugMessage[0],
        ARRAY_SIZE (DebugMessage),
        "  %a:%016lx-%016lx\n",
        mMtrrMemoryCacheTypeShortName[Ranges[Index].Type],
        Ranges[Index].BaseAddress, Ranges[Index].BaseAddress + Ranges[Index].Length - 1
        );
      OsServiceDebugPrint (&DebugMessage[0]);
    }
  }
}

/**
  Reads a HPET MMIO register.

  Reads the 64-bit HPET MMIO register specified by Address.

  This function must guarantee that all MMIO read and write
  operations are serialized.

  If Address is not aligned on a 64-bit boundary, zero will be returned.

  @param  Offset                  Specifies the offset of the HPET register to read.

  @return                         The value read.

**/
UINT64
EFIAPI
HpetRead (
  IN  UINTN                       Offset
  )
{
  UINTN                           Address;
  UINT64                          Value;

  Address = HPET_BASE_ADDRESS + Offset;

  if ((Address & 7) == 0) {
    return 0;
  }

  MemoryFence ();
  Value = *(volatile UINT64*)Address;
  MemoryFence ();

  return Value;
}

/**
  Accesses HPET configuration information.

  If the optional OsServiceDebugPrint function pointer is provided that function is
  used to print HPET settings.

  @param[in]  OsServiceDebugPrint   A pointer to an OS-provided debug print function

**/
VOID
EFIAPI
AccessHpetConfiguration (
  IN PRM_OS_SERVICE_DEBUG_PRINT           OsServiceDebugPrint
  )
{
  UINTN                                   TimerIndex;
  HPET_GENERAL_CAPABILITIES_ID_REGISTER   HpetGeneralCapabilities;
  HPET_GENERAL_CONFIGURATION_REGISTER     HpetGeneralConfiguration;
  CHAR8                                   DebugMessage[256];

  HpetGeneralCapabilities.Uint64  = HpetRead (HPET_GENERAL_CAPABILITIES_ID_OFFSET);
  HpetGeneralConfiguration.Uint64 = HpetRead (HPET_GENERAL_CONFIGURATION_OFFSET);

  if (OsServiceDebugPrint != NULL) {
    AsciiSPrint (
      &DebugMessage[0],
      ARRAY_SIZE (DebugMessage),
      "  HPET Base Address = 0x%08x\n",
      HPET_BASE_ADDRESS
      );
    OsServiceDebugPrint (&DebugMessage[0]);

    AsciiSPrint (
      &DebugMessage[0],
      ARRAY_SIZE (DebugMessage),
      "  HPET_GENERAL_CAPABILITIES_ID  = 0x%016lx\n",
      HpetGeneralCapabilities
      );
    OsServiceDebugPrint (&DebugMessage[0]);

    AsciiSPrint (
      &DebugMessage[0],
      ARRAY_SIZE (DebugMessage),
      "  HPET_GENERAL_CONFIGURATION    = 0x%016lx\n",
      HpetGeneralConfiguration.Uint64
      );
    OsServiceDebugPrint (&DebugMessage[0]);

    AsciiSPrint (
      &DebugMessage[0],
      ARRAY_SIZE (DebugMessage),
      "  HPET_GENERAL_INTERRUPT_STATUS = 0x%016lx\n",
      HpetRead (HPET_GENERAL_INTERRUPT_STATUS_OFFSET)
      );
    OsServiceDebugPrint (&DebugMessage[0]);

    AsciiSPrint (
      &DebugMessage[0],
      ARRAY_SIZE (DebugMessage),
      "  HPET_MAIN_COUNTER             = 0x%016lx\n",
      HpetRead (HPET_MAIN_COUNTER_OFFSET)
      );
    OsServiceDebugPrint (&DebugMessage[0]);

    AsciiSPrint (
      &DebugMessage[0],
      ARRAY_SIZE (DebugMessage),
      "  HPET Main Counter Period      = %d (fs)\n",
      HpetGeneralCapabilities.Bits.CounterClockPeriod
      );
    OsServiceDebugPrint (&DebugMessage[0]);

    for (TimerIndex = 0; TimerIndex <= HpetGeneralCapabilities.Bits.NumberOfTimers; TimerIndex++) {
      AsciiSPrint (
        &DebugMessage[0],
        ARRAY_SIZE (DebugMessage),
        "  HPET_TIMER%d_CONFIGURATION     = 0x%016lx\n",
        TimerIndex,
        HpetRead (HPET_TIMER_CONFIGURATION_OFFSET + TimerIndex * HPET_TIMER_STRIDE)
        );
      OsServiceDebugPrint (&DebugMessage[0]);

      AsciiSPrint (
        &DebugMessage[0],
        ARRAY_SIZE (DebugMessage),
        "  HPET_TIMER%d_COMPARATOR        = 0x%016lx\n",
        TimerIndex,
        HpetRead (HPET_TIMER_COMPARATOR_OFFSET    + TimerIndex * HPET_TIMER_STRIDE)
        );
      OsServiceDebugPrint (&DebugMessage[0]);

      AsciiSPrint (
        &DebugMessage[0],
        ARRAY_SIZE (DebugMessage),
        "  HPET_TIMER%d_MSI_ROUTE         = 0x%016lx\n",
        TimerIndex,
        HpetRead (HPET_TIMER_MSI_ROUTE_OFFSET     + TimerIndex * HPET_TIMER_STRIDE)
        );
      OsServiceDebugPrint (&DebugMessage[0]);
    }
  }
}

/**
  Reads the microcode signature from architectural MSR 0x8B.

  @retval MicrocodeSignature      The microcode signature value.
**/
UINT32
GetMicrocodeSignature (
  VOID
  )
{
  MSR_IA32_BIOS_SIGN_ID_REGISTER  BiosSignIdMsr;

  AsmWriteMsr64 (MSR_IA32_BIOS_SIGN_ID, 0);
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, NULL);
  BiosSignIdMsr.Uint64 = AsmReadMsr64 (MSR_IA32_BIOS_SIGN_ID);

  return BiosSignIdMsr.Bits.MicrocodeUpdateSignature;
}

/**
  Prints the microcode update signature as read from architectural MSR 0x8B.

**/
VOID
EFIAPI
PrintMicrocodeUpdateSignature (
  IN PRM_OS_SERVICE_DEBUG_PRINT   OsServiceDebugPrint
  )
{
  UINT32                          MicrocodeSignature;
  CHAR8                           DebugMessage[256];

  if (OsServiceDebugPrint == NULL) {
    return;
  }

  MicrocodeSignature = GetMicrocodeSignature ();

  AsciiSPrint (
    &DebugMessage[0],
    ARRAY_SIZE (DebugMessage),
    "  Signature read = 0x%x.\n",
    MicrocodeSignature
    );
  OsServiceDebugPrint (&DebugMessage[0]);
}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler attempts to read the microcode update signature.

  @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer       A pointer to the PRM handler context buffer

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
PRM_HANDLER_EXPORT (MsrAccessMicrocodeSignaturePrmHandler)
{
  UINT32  MicrocodeSignature;

  MicrocodeSignature = 0;
  MicrocodeSignature = GetMicrocodeSignature ();

  if (MicrocodeSignature == 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler attempts to read the microcode update signature MSR and print the result to a debug message.

  @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer       A pointer to the PRM handler context buffer

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
PRM_HANDLER_EXPORT (MsrPrintMicrocodeSignaturePrmHandler)
{
  PRM_OS_SERVICE_DEBUG_PRINT      OsServiceDebugPrint;

  if (ParameterBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // The OS debug print service is assumed to be at the beginning of ParameterBuffer
  OsServiceDebugPrint = *((PRM_OS_SERVICE_DEBUG_PRINT *) ParameterBuffer);
  if (OsServiceDebugPrint == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OsServiceDebugPrint ("Hardware Access MsrAccessMicrocodeSignaturePrmHandler entry.\n");
  OsServiceDebugPrint ("  Attempting to read the Microcode Update Signature MSR (0x8B)...\n");
  PrintMicrocodeUpdateSignature (OsServiceDebugPrint);
  OsServiceDebugPrint ("Hardware Access MsrAccessMicrocodeSignaturePrmHandler exit.\n");

  return EFI_SUCCESS;
}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler attempts to read the current MTRR settings.

  @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer       A pointer to the PRM handler context buffer

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
PRM_HANDLER_EXPORT (MsrAccessMtrrDumpPrmHandler)
{
  AccessAllMtrrs (NULL);

  return EFI_SUCCESS;
}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler attempts to read the current MTRR settings and print the result to a debug message.

  @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer       A pointer to the PRM handler context buffer

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
PRM_HANDLER_EXPORT (MsrPrintMtrrDumpPrmHandler)
{
  PRM_OS_SERVICE_DEBUG_PRINT      OsServiceDebugPrint;

  if (ParameterBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // In the POC, the OS debug print service is assumed to be at the beginning of ParameterBuffer
  OsServiceDebugPrint = *((PRM_OS_SERVICE_DEBUG_PRINT *) ParameterBuffer);
  if (OsServiceDebugPrint == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OsServiceDebugPrint ("Hardware Access MsrAccessMtrrDumpPrmHandler entry.\n");
  OsServiceDebugPrint ("  Attempting to dump MTRR values:\n");
  AccessAllMtrrs (OsServiceDebugPrint);
  OsServiceDebugPrint ("Hardware Access MsrAccessMtrrDumpPrmHandler exit.\n");

  return EFI_SUCCESS;
}


/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler attempts to read from a HPET MMIO resource.

  @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer       A pointer to the PRM handler context buffer

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
PRM_HANDLER_EXPORT (MmioAccessHpetPrmHandler)
{
  AccessHpetConfiguration (NULL);

  return EFI_SUCCESS;
}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler attempts to read from a HPET MMIO resource and print the result to a debug message.

  @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer       A pointer to the PRM handler context buffer

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
PRM_HANDLER_EXPORT (MmioPrintHpetPrmHandler)
{
  PRM_OS_SERVICE_DEBUG_PRINT      OsServiceDebugPrint;

  if (ParameterBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // An OS debug print service is assumed to be at the beginning of ParameterBuffer
  OsServiceDebugPrint = *((PRM_OS_SERVICE_DEBUG_PRINT *) ParameterBuffer);
  if (OsServiceDebugPrint == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OsServiceDebugPrint ("Hardware Access MmioPrintHpetPrmHandler entry.\n");
  OsServiceDebugPrint ("  Attempting to read HPET configuration...\n");
  AccessHpetConfiguration (OsServiceDebugPrint);
  OsServiceDebugPrint ("Hardware Access MmioPrintHpetPrmHandler exit.\n");

  return EFI_SUCCESS;
}

//
// Register the PRM export information for this PRM Module
//
PRM_MODULE_EXPORT (
  PRM_HANDLER_EXPORT_ENTRY (MSR_ACCESS_MICROCODE_SIGNATURE_PRM_HANDLER_GUID, MsrAccessMicrocodeSignaturePrmHandler),
  PRM_HANDLER_EXPORT_ENTRY (MSR_ACCESS_MTRR_DUMP_PRM_HANDLER_GUID, MsrAccessMtrrDumpPrmHandler),
  PRM_HANDLER_EXPORT_ENTRY (MMIO_ACCESS_HPET_PRM_HANDLER_GUID, MmioAccessHpetPrmHandler),
  PRM_HANDLER_EXPORT_ENTRY (MSR_PRINT_MICROCODE_SIGNATURE_PRM_HANDLER_GUID, MsrPrintMicrocodeSignaturePrmHandler),
  PRM_HANDLER_EXPORT_ENTRY (MSR_PRINT_MTRR_DUMP_PRM_HANDLER_GUID, MsrPrintMtrrDumpPrmHandler),
  PRM_HANDLER_EXPORT_ENTRY (MMIO_PRINT_HPET_PRM_HANDLER_GUID, MmioPrintHpetPrmHandler)
  );

/**
  Module entry point.

  @param[in]   ImageHandle     The image handle.
  @param[in]   SystemTable     A pointer to the system table.

  @retval  EFI_SUCCESS         This function always returns success.

**/
EFI_STATUS
EFIAPI
PrmSampleHardwareAccessModuleInit (
  IN  EFI_HANDLE                  ImageHandle,
  IN  EFI_SYSTEM_TABLE            *SystemTable
  )
{
  return EFI_SUCCESS;
}
